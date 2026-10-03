#include "PCH.h"
#include "NativeMcm.h"
#include "Game.h"
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
