#include "PCH.h"
#include "Ids.h"
#include "Hotkeys.h"
#include "Game.h"
#include "McmTable.h"

#include "SKSEMenuFramework.h"

namespace Hotkeys
{
	namespace
	{
		constexpr RE::FormID kMcmQuest = 0x00E774;             // _Seed_MCMQuest, carries the old _Seed_SkyUIConfigPanelScript
		constexpr RE::FormID kUnboundIntensity = 0x47DAAE;     // Provisioning_PerkRank_UnboundIntensity: the intensity hotkey needs the perk
		constexpr RE::FormID kSkillLockedMessage = 0x4DDE3A;   // _Seed_SkillLockedMessage
		constexpr std::uint32_t kEscape = 1, kBackspace = 14, kDelete = 211;

		struct Slot
		{
			RE::FormID  global;
			RE::FormID  spell;
			bool        needsIntensityPerk;
		};
		std::vector<Slot> slots;

		bool previousDown[256]{};  // key state when capture began, so the click that started it is not taken as the key

		// While SKSE Menu Framework's window is open it consumes the game's own input events, so the key is read from the keyboard state
		// instead. Returns the DirectInput scan code of a newly pressed key, -1 for Delete/Backspace, -3 for Escape, or -2 for none.
		int PollKeyboard()
		{
			DWORD pid = 0;
			GetWindowThreadProcessId(GetForegroundWindow(), &pid);
			if (pid != GetCurrentProcessId()) {
				return -2;  // the game is not the active window
			}
			for (int vk = 8; vk < 255; ++vk) {
				if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 || vk == VK_XBUTTON2 || vk == VK_RETURN) {
					continue;  // mouse buttons; Enter is how menus are activated
				}
				const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
				if (down && !previousDown[vk]) {
					previousDown[vk] = true;
					if (vk == VK_ESCAPE) {
						return -3;
					}
					if (vk == VK_BACK || vk == VK_DELETE) {
						return -1;
					}
					const UINT sc = MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC_EX);
					if (sc == 0) {
						continue;
					}
					return ((sc & 0xFF00) == 0xE000) ? static_cast<int>((sc & 0xFF) | 0x80) : static_cast<int>(sc & 0xFF);
				}
				previousDown[vk] = down;
			}
			return -2;
		}

		std::atomic<int> capturing{ -1 };
		std::atomic<int> captured{ -2 };  // -2 = nothing yet
		int              captureSlot = -1;

		RE::TESForm* Look(RE::FormID a_id)
		{
			auto* dh = RE::TESDataHandler::GetSingleton();
			return dh ? dh->LookupForm(ids::Ls(a_id), "LastSeed.esp") : nullptr;
		}

		// The menus in which the MCM script's OnKeyDown ignored the keys, plus our own settings window.
		bool Blocked()
		{
			auto* ui = RE::UI::GetSingleton();
			if (!ui) {
				return true;
			}
			for (const char* menu : { "Console", "Book Menu", "BarterMenu", "ContainerMenu", "Crafting Menu", "Dialogue Menu", "FavoritesMenu", "InventoryMenu",
					 "Journal Menu", "Lockpicking Menu", "MagicMenu", "MapMenu", "MessageBoxMenu", "Sleep/Wait Menu", "StatsMenu", "LoadingMenu", "MainMenu" }) {
				if (ui->IsMenuOpen(menu)) {
					return true;
				}
			}
			return SKSEMenuFramework::IsAnyBlockingWindowOpened();
		}

		void Fire(const Slot& a_slot)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* spell = Look(a_slot.spell) ? Look(a_slot.spell)->As<RE::SpellItem>() : nullptr;
			if (!player || !spell) {
				return;
			}
			if (a_slot.needsIntensityPerk) {
				auto* perk = Look(kUnboundIntensity) ? Look(kUnboundIntensity)->As<RE::TESGlobal>() : nullptr;
				if (!perk || perk->value <= 0.0f) {
					RE::DebugNotification("You have not unlocked Unbound Intensity.");
					return;
				}
			}
			if (auto* caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
				caster->CastSpellImmediate(spell, false, nullptr, 1.0f, false, 0.0f, nullptr);
			}
		}

		class Sink : public RE::BSTEventSink<RE::InputEvent*>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override
			{
				if (!a_event) {
					return RE::BSEventNotifyControl::kContinue;
				}
				for (auto* e = *a_event; e; e = e->next) {
					auto* button = e->AsButtonEvent();
					if (!button || button->GetDevice() != RE::INPUT_DEVICE::kKeyboard || !button->IsDown()) {
						continue;
					}
					const int code = static_cast<int>(button->GetIDCode());
					if (capturing.load() >= 0) {
						captured = (code == kEscape) ? -3 : ((code == kBackspace || code == kDelete) ? -1 : code);  // -3 = cancelled
						continue;
					}
					if (!Game::IsRunning()) {
						continue;
					}
					for (const auto& slot : slots) {
						auto* g = Look(slot.global) ? Look(slot.global)->As<RE::TESGlobal>() : nullptr;
						if (g && static_cast<int>(g->value) == code && code > 0) {
							SKSE::GetTaskInterface()->AddTask([slot]() {
								if (!Blocked()) {
									Fire(slot);
								}
							});
							break;
						}
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
		Sink sink;
	}

	void Init()
	{
		slots.clear();
		for (const auto& page : mcm::kPages) {
			for (int i = 0; i < page.count; ++i) {
				const auto& e = page.entries[i];
				if (e.kind == mcm::Kind::Key) {
					slots.push_back({ e.formId, e.aux, std::string_view(e.label) == "Unbound Intensity" });
				}
			}
		}
		if (auto* input = RE::BSInputDeviceManager::GetSingleton()) {
			input->AddEventSink(&sink);
		}
		SKSE::log::info("Hotkeys: {} keys handled natively", slots.size());
	}

	void OnGameLoaded()
	{
		// The old MCM script also listens for these keys (it registered them with RegisterForKey): stop it, or each press would act twice.
		SKSE::GetTaskInterface()->AddTask([]() {
			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* quest = Look(kMcmQuest) ? Look(kMcmQuest)->As<RE::TESQuest>() : nullptr;
			if (!vm || !quest) {
				return;
			}
			const auto handle = vm->GetObjectHandlePolicy()->GetHandleForObject(RE::TESQuest::FORMTYPE, quest);
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
			vm->DispatchMethodCall2(handle, "_Seed_SkyUIConfigPanelScript", "UnregisterForAllKeys", RE::MakeFunctionArguments(), callback);
			SKSE::log::info("Hotkeys: the old MCM script no longer listens for keys");
		});
	}

	void BeginCapture(int a_slot)
	{
		for (int vk = 0; vk < 256; ++vk) {
			previousDown[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
		}
		captured = -2;
		capturing = a_slot;
	}

	void CancelCapture()
	{
		capturing = -1;
		captured = -2;
	}

	int CapturingSlot() { return capturing.load(); }

	bool TakeCaptured(int a_slot, int& a_keyCode)
	{
		if (capturing.load() != a_slot) {
			return false;
		}
		int c = captured.load();
		if (c == -2) {
			c = PollKeyboard();
		}
		if (c == -2) {
			return false;
		}
		capturing = -1;
		captured = -2;
		if (c == -3) {
			return false;  // cancelled
		}
		a_keyCode = c;
		return true;
	}

	const char* KeyName(int a_keyCode)
	{
		static thread_local char buffer[64];
		if (a_keyCode <= 0) {
			return "None";
		}
		// DirectInput scan codes: the high bit marks the extended keys
		const LONG ext = (a_keyCode & 0x80) ? 1 : 0;
		const LONG lparam = ((a_keyCode & 0x7F) << 16) | (ext << 24);
		wchar_t    wide[48]{};
		if (GetKeyNameTextW(lparam, wide, 48) > 0) {
			WideCharToMultiByte(CP_UTF8, 0, wide, -1, buffer, sizeof(buffer), nullptr, nullptr);
			return buffer;
		}
		std::snprintf(buffer, sizeof(buffer), "Key %d", a_keyCode);
		return buffer;
	}
}
