#include "PCH.h"
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
			return dh ? dh->LookupForm<RE::TESGlobal>(a_localId, "LastSeed.esp") : nullptr;
		}

		// Calls a method of _Seed_ConfigurationHandler on the game thread.
		void CallHandler(const char* a_method)
		{
			SKSE::GetTaskInterface()->AddTask([a_method]() {
				auto* dh = RE::TESDataHandler::GetSingleton();
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				auto* quest = dh ? dh->LookupForm<RE::TESQuest>(kConfigHandlerQuest, "LastSeed.esp") : nullptr;
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
				auto* quest = dh ? dh->LookupForm<RE::TESQuest>(a_quest, "LastSeed.esp") : nullptr;
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
								if (auto* spell = dh->LookupForm<RE::SpellItem>(a_e.aux, "LastSeed.esp"); spell && RE::PlayerCharacter::GetSingleton()) {
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

	void DrawPage(int a_page)
	{
		if (a_page < 0 || a_page >= static_cast<int>(std::size(mcm::kPages))) {
			return;
		}
		if (!Game::Ready() || !Game::IsRunning()) {
			TextDisabled("Last Seed is not running. Start it from the Overview page first.");
			return;
		}
		sinceDraw = 0.0f;
		if (!session) {
			session = true;
			changed = false;
			CallHandler("onStart");  // Last Seed notes what the settings were, so it can start or stop systems when they change
		}
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
