/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#include "PCH.h"
#include "Ids.h"
#include <fstream>
#include <map>
#include <regex>
#include "NativeMcm.h"
#include "Game.h"
#include "Hotkeys.h"
#include "McmTable.h"

#include "SKSEMenuFramework.h"

namespace NativeMcm
{
	namespace
	{
		using namespace ImGuiMCP;

		constexpr RE::FormID kConfigHandlerQuest = 0x250AD0;  // _Seed_ConfigurationHandlerQuest, carries _Seed_ConfigurationHandler
		constexpr RE::FormID kAutoSaveLoad = 0x00E776;        // _Seed_Setting_AutoSaveLoad (2 = changes are written to the current profile)
		constexpr RE::FormID kCurrentProfile = 0x00E775;      // _Seed_Setting_CurrentProfile
		constexpr RE::FormID kSkillTreeQuest = 0x4A11E1;      // _Seed_SkillTreeHandlerQuest, carries _Seed_SkillTreeHandler
		constexpr RE::FormID kPerkPointsTotal = 0x47DAA3;     // ProvisioningPerkPointsTotal
		constexpr RE::FormID kMcmQuest = 0x00E774;            // _Seed_MCMQuest, carries _Seed_SkyUIConfigPanelScript (profile functions live there)
		constexpr RE::FormID kDiseaseQuest = 0x0108E8;        // _Seed_DiseaseManagerQuest, carries _Seed_DiseaseManager
		constexpr RE::FormID kSafeLocations = 0x36731A;       // _Seed_SafeLocations: locations the player marked as safe
		constexpr const char* kConfigPath = "../LastSeedData/";  // where the MCM keeps its profiles (JsonUtil path)
		constexpr float       kSessionGraceSeconds = 0.4f;     // the pages count as closed when not drawn for this long
		constexpr float       kProfileWriteDelay = 0.6f;       // profile writes wait for the last change, like dragging a slider

		bool                         session = false;      // the pages are open
		bool                         changed = false;      // something was changed in this session
		float                        sinceDraw = 0.0f;
		std::map<std::string, int>   pendingProfile;       // profile key -> value, written after a short delay
		float                        profileTimer = -1.0f;

		RE::TESGlobal* Global(unsigned int a_localId)
		{
			auto* dh = RE::TESDataHandler::GetSingleton();
			return dh ? dh->LookupForm<RE::TESGlobal>(ids::Ls(a_localId), "LastSeed.esp") : nullptr;
		}

		// Calls a method of _Seed_ConfigurationHandler on the game thread.
		void CallHandler(const char* a_method)
		{
			SKSE::GetTaskInterface()->AddTask([a_method]() {
				auto* dh = RE::TESDataHandler::GetSingleton();
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				auto* quest = dh ? dh->LookupForm<RE::TESQuest>(ids::Ls(kConfigHandlerQuest), "LastSeed.esp") : nullptr;
				if (!vm || !quest) {
					SKSE::log::warn("Last Seed settings: cannot call _Seed_ConfigurationHandler.{} (quest or VM missing)", a_method);
					return;
				}
				const auto                                       handle = vm->GetObjectHandlePolicy()->GetHandleForObject(RE::TESQuest::FORMTYPE, quest);
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
				const bool ok = vm->DispatchMethodCall2(handle, "_Seed_ConfigurationHandler", a_method, RE::MakeFunctionArguments(), callback);
				SKSE::log::info("Last Seed settings: _Seed_ConfigurationHandler.{}() -> {}", a_method, ok ? "dispatched" : "FAILED");
			});
		}

		// Writes the pending keys to the current profile with PapyrusUtil's JsonUtil, as the MCM's SaveSettingToCurrentProfile does.
		void FlushProfile()
		{
			if (pendingProfile.empty()) {
				return;
			}
			auto pending = std::move(pendingProfile);
			pendingProfile.clear();
			SKSE::GetTaskInterface()->AddTask([pending = std::move(pending)]() {
				auto* autoSave = Global(kAutoSaveLoad);
				auto* profile = Global(kCurrentProfile);
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				if (!vm || !autoSave || !profile || static_cast<int>(autoSave->value) != 2) {
					return;  // automatic profile saving is off: nothing to write
				}
				const std::string path = std::string(kConfigPath) + "profile" + std::to_string(static_cast<int>(profile->value));
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
				for (const auto& [key, value] : pending) {
					vm->DispatchStaticCall("JsonUtil", "SetIntValue", RE::MakeFunctionArguments(std::string(path), std::string(key), static_cast<std::int32_t>(value)), callback);
				}
				vm->DispatchStaticCall("JsonUtil", "Save", RE::MakeFunctionArguments(std::string(path), false), callback);
			});
		}

		// Calls a method of a Last Seed quest script on the game thread (arguments are optional ints).
		template <class... Args>
		void CallQuest(RE::FormID a_quest, const char* a_script, const char* a_method, Args... a_args)
		{
			SKSE::GetTaskInterface()->AddTask([=]() mutable {
				auto* dh = RE::TESDataHandler::GetSingleton();
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				auto* quest = dh ? dh->LookupForm<RE::TESQuest>(ids::Ls(a_quest), "LastSeed.esp") : nullptr;
				if (!vm || !quest) {
					SKSE::log::warn("Last Seed settings: cannot call {}.{} (quest or VM missing)", a_script, a_method);
					return;
				}
				const auto handle = vm->GetObjectHandlePolicy()->GetHandleForObject(RE::TESQuest::FORMTYPE, quest);
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
				const bool ok = vm->DispatchMethodCall2(handle, a_script, a_method, RE::MakeFunctionArguments(std::move(a_args)...), callback);
				SKSE::log::info("Last Seed settings: {}.{}() -> {}", a_script, a_method, ok ? "dispatched" : "FAILED");
			});
		}

		// PapyrusUtil JsonUtil calls on the game thread, as the MCM makes them.
		void JsonSetInt(std::string a_path, std::string a_key, int a_value)
		{
			SKSE::GetTaskInterface()->AddTask([a_path, a_key, a_value]() {
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
				if (vm) {
					vm->DispatchStaticCall("JsonUtil", "SetIntValue", RE::MakeFunctionArguments(std::string(a_path), std::string(a_key), static_cast<std::int32_t>(a_value)), callback);
				}
			});
		}

		void JsonSetString(std::string a_path, std::string a_key, std::string a_value)
		{
			SKSE::GetTaskInterface()->AddTask([a_path, a_key, a_value]() {
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
				if (vm) {
					vm->DispatchStaticCall("JsonUtil", "SetStringValue", RE::MakeFunctionArguments(std::string(a_path), std::string(a_key), std::string(a_value)), callback);
				}
			});
		}

		void JsonSave(std::string a_path)
		{
			SKSE::GetTaskInterface()->AddTask([a_path]() {
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
				if (vm) {
					vm->DispatchStaticCall("JsonUtil", "Save", RE::MakeFunctionArguments(std::string(a_path), false), callback);
				}
			});
		}

		// The name stored in a profile's file (Data/SKSE/Plugins/LastSeedData/profileN.json), or "Profile N".
		std::string ProfileName(int a_index)
		{
			std::ifstream in("Data/SKSE/Plugins/LastSeedData/profile" + std::to_string(a_index) + ".json");
			if (in) {
				const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
				static const std::regex re("\"profile_name\"\\s*:\\s*\"([^\"]*)\"");
				std::smatch m;
				if (std::regex_search(text, m, re) && !m[1].str().empty()) {
					return m[1].str();
				}
			}
			return "Profile " + std::to_string(a_index);
		}

		void BeginSession()
		{
			sinceDraw = 0.0f;
			if (!session) {
				session = true;
				changed = false;
				CallHandler("onStart");  // Last Seed notes what the settings were, so it can start or stop systems when they change
			}
		}

		// Result of a Papyrus function that returns an int, kept for the page to read.
		class IntResult : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			explicit IntResult(std::atomic<int>& a_out) :
				out(a_out) {}
			void operator()(RE::BSScript::Variable a_result) override
			{
				if (a_result.IsInt()) {
					out = a_result.GetSInt();
				}
			}
			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

		private:
			std::atomic<int>& out;
		};
		std::atomic<int> locationHazard{ 0 };  // _Seed_DiseaseManager.getLocationHazardLevel(false): 1 (very safe) to 6 (very unsafe)
		float            hazardTimer = 0.0f;

		void AskLocationHazard()
		{
			SKSE::GetTaskInterface()->AddTask([]() {
				auto* dh = RE::TESDataHandler::GetSingleton();
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				auto* quest = dh ? dh->LookupForm<RE::TESQuest>(ids::Ls(kDiseaseQuest), "LastSeed.esp") : nullptr;
				if (!vm || !quest) {
					return;
				}
				const auto                                               handle = vm->GetObjectHandlePolicy()->GetHandleForObject(RE::TESQuest::FORMTYPE, quest);
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback(new IntResult(locationHazard));
				vm->DispatchMethodCall2(handle, "_Seed_DiseaseManager", "getLocationHazardLevel", RE::MakeFunctionArguments(false), callback);
			});
		}

		// FormList.AddForm / RemoveAddedForm through Papyrus, so the list is saved the way Last Seed's own menu saved it.
		void SetSafeLocation(RE::BGSLocation* a_location, bool a_safe)
		{
			SKSE::GetTaskInterface()->AddTask([a_location, a_safe]() {
				auto* dh = RE::TESDataHandler::GetSingleton();
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				auto* list = dh ? dh->LookupForm<RE::BGSListForm>(ids::Ls(kSafeLocations), "LastSeed.esp") : nullptr;
				if (!vm || !list || !a_location) {
					return;
				}
				const auto                                               handle = vm->GetObjectHandlePolicy()->GetHandleForObject(RE::BGSListForm::FORMTYPE, list);
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
				vm->DispatchMethodCall2(handle, "FormList", a_safe ? "AddForm" : "RemoveAddedForm", RE::MakeFunctionArguments(static_cast<RE::TESForm*>(a_location)), callback);
			});
		}

		void Changed(const mcm::Entry& a_e, RE::TESGlobal* a_g, float a_value, int a_profileValue)
		{
			a_g->value = a_value;
			changed = true;
			if (a_e.profileKey && a_e.profileKey[0]) {
				pendingProfile[a_e.profileKey] = a_profileValue;
				profileTimer = kProfileWriteDelay;
			}
		}

		const char* Format(const char* a_fmt)
		{
			// the MCM's display formats: {0} = whole number, {1} = one decimal
			return (a_fmt && a_fmt[0] == '{' && a_fmt[1] == '1') ? "%.1f" : "%.0f";
		}

		void DrawEntry(const mcm::Entry& a_e, int a_id)
		{
			PushID(a_id);
			switch (a_e.kind) {
			case mcm::Kind::Header:
				Spacing();
				SeparatorText(a_e.label);
				break;
			case mcm::Kind::Column:
				Spacing();
				Separator();
				break;
			case mcm::Kind::Toggle:
				if (auto* g = Global(a_e.formId)) {
					bool on = static_cast<int>(g->value) == 2;
					if (Checkbox(a_e.label, &on)) {
						Changed(a_e, g, on ? 2.0f : 1.0f, on ? 2 : 1);
					}
				}
				break;
			case mcm::Kind::Slider:
				if (auto* g = Global(a_e.formId)) {
					float v = g->value;
					if (SliderFloat(a_e.label, &v, a_e.min, a_e.max, Format(a_e.format))) {
						if (a_e.step > 0.0f) {
							v = a_e.min + std::round((v - a_e.min) / a_e.step) * a_e.step;
						}
						v = std::clamp(v, a_e.min, a_e.max);
						Changed(a_e, g, v, static_cast<int>(v));  // the MCM saves sliders to profiles as whole numbers
					}
				}
				break;
			case mcm::Kind::Key:
				if (auto* g = Global(a_e.formId)) {
					const int slot = static_cast<int>(a_e.formId);
					int       picked = 0;
					if (Hotkeys::CapturingSlot() == slot) {
						if (Hotkeys::TakeCaptured(slot, picked)) {
							Changed(a_e, g, static_cast<float>(picked), picked);
							if (auto* dh = RE::TESDataHandler::GetSingleton()) {
								if (auto* spell = dh->LookupForm<RE::SpellItem>(ids::Ls(a_e.aux), "LastSeed.esp"); spell && RE::PlayerCharacter::GetSingleton()) {
									RE::PlayerCharacter::GetSingleton()->RemoveSpell(spell);  // as the MCM does when a hotkey changes
								}
							}
						}
					}
					if (Hotkeys::CapturingSlot() == slot) {
						if (Button("Press a key...  (Esc cancels, Delete clears)")) {
							Hotkeys::CancelCapture();
						}
					} else {
						const std::string text = std::string(Hotkeys::KeyName(static_cast<int>(g->value))) + "##key";
						if (Button(text.c_str(), ImVec2(180.0f, 0.0f))) {
							Hotkeys::BeginCapture(slot);
						}
					}
					SameLine();
					Text("%s", a_e.label);
				}
				break;
			case mcm::Kind::Menu:
				if (auto* g = Global(a_e.formId)) {
					int index = std::clamp(static_cast<int>(g->value) - a_e.menuBase, 0, std::max(0, a_e.optionCount - 1));
					if (Combo(a_e.label, &index, a_e.options, a_e.optionCount)) {
						const int stored = index + a_e.menuBase;
						Changed(a_e, g, static_cast<float>(stored), stored);
					}
				}
				break;
			}
			PopID();
		}
	}

	namespace
	{
		void DrawProvisioning()
		{
			static int   confirm = 0;        // 1 = respec asked, 2 = restore asked
			static float restore = 0.0f;
			static float confirmTimer = 0.0f;
			confirmTimer -= GetIO()->DeltaTime;
			if (confirmTimer <= 0.0f) {
				confirm = 0;
			}
			Spacing();
			SeparatorText("Provisioning Skill");
			if (confirm == 1) {
				TextWrapped("Are you sure you want to refund all earned Provisioning skill points so you can reallocate them?");
				if (Button("Yes, respec my perks")) {
					CallQuest(kSkillTreeQuest, "_Seed_SkillTreeHandler", "RefundSkillPoints");
					confirm = 0;
				}
				SameLine();
				if (Button("Cancel")) {
					confirm = 0;
				}
			} else if (Button("Respec Perks")) {
				confirm = 1;
				confirmTimer = 10.0f;
			}
			float total = 0.0f;
			if (auto* g = Global(kPerkPointsTotal)) {
				total = g->value;
			}
			TextWrapped("Restore Skill Progress: reclaim Provisioning skill progress lost to a clean save or a mod uninstall. This replaces your current progress.");
			restore = std::clamp(restore, 0.0f, std::max(total, 0.0f));
			SliderFloat("Skill points to restore", &restore, 0.0f, std::max(total, 1.0f), "%.0f");
			if (confirm == 2) {
				if (Button("Yes, restore these skill points")) {
					CallQuest(kSkillTreeQuest, "_Seed_SkillTreeHandler", "restorePerkPoints", static_cast<std::int32_t>(restore));
					confirm = 0;
				}
				SameLine();
				if (Button("Cancel##restore")) {
					confirm = 0;
				}
			} else if (Button("Restore skill progress")) {
				confirm = 2;
				confirmTimer = 10.0f;
			}
		}
	}

	void DrawProfiles()
	{
		if (!Game::Ready() || !Game::IsRunning()) {
			TextDisabled("Last Seed is not running. Start it from the Overview page first.");
			return;
		}
		BeginSession();
		auto* current = Global(kCurrentProfile);
		auto* autoSave = Global(kAutoSaveLoad);
		if (!current || !autoSave) {
			return;
		}
		static int         pendingSwitch = 0;   // profile asked for, waiting for the confirmation
		static bool        askDefault = false;  // "reset the current profile" waiting for the confirmation
		static float       confirmTimer = 0.0f;
		static float       namesTimer = 0.0f;
		static std::string names[10];
		static char        renameBuffer[64] = "";
		confirmTimer -= GetIO()->DeltaTime;
		if (confirmTimer <= 0.0f) {
			pendingSwitch = 0;
			askDefault = false;
		}
		namesTimer -= GetIO()->DeltaTime;
		if (namesTimer <= 0.0f) {
			namesTimer = 1.0f;
			for (int i = 0; i < 10; ++i) {
				names[i] = ProfileName(i + 1);
			}
		}
		const char* items[10];
		for (int i = 0; i < 10; ++i) {
			items[i] = names[i].c_str();
		}

		SeparatorText("Settings Profiles");
		const int activeIndex = std::clamp(static_cast<int>(current->value), 1, 10) - 1;
		int       choice = activeIndex;
		if (Combo("Current profile", &choice, items, 10) && choice != activeIndex) {
			pendingSwitch = choice + 1;
			confirmTimer = 10.0f;
		}
		if (pendingSwitch > 0) {
			TextWrapped("Load the selected profile? Your current settings are replaced by that profile's.");
			if (Button("Yes, load it")) {
				CallQuest(kMcmQuest, "_Seed_SkyUIConfigPanelScript", "SwitchToProfile", static_cast<std::int32_t>(pendingSwitch));
				changed = true;
				pendingSwitch = 0;
			}
			SameLine();
			if (Button("Cancel##switch")) {
				pendingSwitch = 0;
			}
		}

		Spacing();
		bool automatic = static_cast<int>(autoSave->value) == 2;
		if (Checkbox("Automatic profile save / load", &automatic)) {
			autoSave->value = automatic ? 2.0f : 1.0f;
			JsonSetInt(std::string(kConfigPath) + "common", "auto_load", automatic ? 2 : 1);
			JsonSave(std::string(kConfigPath) + "common");
			if (automatic) {
				CallQuest(kMcmQuest, "_Seed_SkyUIConfigPanelScript", "SaveAllSettings", static_cast<std::int32_t>(current->value));  // write every setting to the profile now
			}
		}
		TextWrapped(
			"A profile stores all of Last Seed's settings in a file. With automatic save / load on, each change is saved to the current profile, and "
			"loading a game, switching characters or starting a new game picks the profile up again. There are 10 profile slots. The files are in "
			"Data/SKSE/Plugins/LastSeedData/ (common.json and profile*.json); with Mod Organizer 2 they end up in your Overwrite folder.");

		if (automatic) {
			Spacing();
			SeparatorText("This profile");
			InputText("##rename", renameBuffer, sizeof(renameBuffer));
			SameLine();
			if (Button("Rename profile")) {
				if (renameBuffer[0] != '\0') {
					const std::string path = std::string(kConfigPath) + "profile" + std::to_string(static_cast<int>(current->value));
					JsonSetString(path, "profile_name", renameBuffer);
					JsonSave(path);
					renameBuffer[0] = '\0';
					namesTimer = 0.5f;  // re-read the names once the file has been written
				}
			}
			if (askDefault) {
				TextWrapped("Are you sure you want to restore all settings on your current profile to their default values?");
				if (Button("Yes, restore the defaults")) {
					const auto profile = static_cast<std::int32_t>(current->value);
					CallQuest(kMcmQuest, "_Seed_SkyUIConfigPanelScript", "GenerateDefaultProfile", profile);
					CallQuest(kMcmQuest, "_Seed_SkyUIConfigPanelScript", "SwitchToProfile", profile);
					changed = true;
					askDefault = false;
				}
				SameLine();
				if (Button("Cancel##default")) {
					askDefault = false;
				}
			} else if (Button("Default current profile")) {
				askDefault = true;
				confirmTimer = 10.0f;
			}
		}
	}

	void DrawOverviewExtras()
	{
		if (!Game::Ready() || !Game::IsRunning() || !Game::StartupFinished()) {
			return;
		}
		BeginSession();
		auto* dh = RE::TESDataHandler::GetSingleton();
		auto* preset = dh ? dh->LookupForm<RE::TESGlobal>(ids::Ls(mcm::kOverview_PresetGlobal), "LastSeed.esp") : nullptr;

		Spacing();
		Separator();
		Text("Presets");
		if (preset) {
			int index = std::clamp(static_cast<int>(preset->value) - 1, 0, static_cast<int>(std::size(mcm::kOverview_PresetsGameplay)) - 1);
			if (Combo("Gameplay preset", &index, mcm::kOverview_PresetsGameplay, static_cast<int>(std::size(mcm::kOverview_PresetsGameplay)))) {
				preset->value = static_cast<float>(index + 1);
				changed = true;
				pendingProfile["gameplayPreset"] = index + 1;
				profileTimer = kProfileWriteDelay;
				CallQuest(kConfigHandlerQuest, "_Seed_ConfigurationHandler", "setPresets", static_cast<std::int32_t>(index + 1));  // applies the preset's settings
			}
		}

		Spacing();
		Separator();
		Text("Location");
		hazardTimer -= GetIO()->DeltaTime;
		if (hazardTimer <= 0.0f) {
			hazardTimer = 2.0f;
			AskLocationHazard();
		}
		const int hazard = locationHazard.load();
		if (hazard >= 1 && hazard <= static_cast<int>(std::size(mcm::kOverview_LocationText))) {
			TextWrapped("%s", mcm::kOverview_LocationText[hazard - 1]);
		}
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* location = player ? player->GetCurrentLocation() : nullptr;
		auto* safe = dh ? dh->LookupForm<RE::BGSListForm>(ids::Ls(kSafeLocations), "LastSeed.esp") : nullptr;
		if (location && safe && hazard - 1 > 1) {
			bool marked = safe->HasForm(location);
			if (Checkbox("Mark this location as safe", &marked)) {
				SetSafeLocation(location, marked);
			}
		} else {
			TextDisabled("This location cannot be marked as safe.");
		}
	}

	void DrawPage(int a_page)
	{
		if (a_page < 0 || a_page >= static_cast<int>(std::size(mcm::kPages))) {
			return;
		}
		if (!Game::Ready() || !Game::IsRunning()) {
			TextDisabled("Last Seed is not running. Start it from the Overview page first.");
			return;
		}
		BeginSession();
		const auto& page = mcm::kPages[a_page];
		for (int i = 0; i < page.count; ++i) {
			DrawEntry(page.entries[i], i);
		}
		if (std::string_view(page.title) == "Other") {
			DrawProvisioning();
			Spacing();
			TextWrapped("Click a hotkey button, then press the key. Escape cancels, Delete clears it.");
		}
	}

	void Tick(float a_dt)
	{
		if (profileTimer >= 0.0f) {
			profileTimer -= a_dt;
			if (profileTimer < 0.0f) {
				FlushProfile();
			}
		}
		if (!session) {
			return;
		}
		sinceDraw += a_dt;
		if (sinceDraw > kSessionGraceSeconds) {
			session = false;
			FlushProfile();
			if (changed) {
				CallHandler("onFinish");  // applies what changed (starts or stops the hunger, thirst, fatigue systems and so on)
			}
			changed = false;
		}
	}
}
