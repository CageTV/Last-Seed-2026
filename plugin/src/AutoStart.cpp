#include "PCH.h"
#include "AutoStart.h"
#include "Game.h"
#include "Settings.h"

namespace AutoStart
{
	namespace
	{
		constexpr auto kPollSeconds = 5;
		constexpr int  kOutdoorSecondsNeeded = 30;  // loaded save that has not been indoors this session: wait this long, outside and in control

		bool seenInterior = false;
		int  outdoorSeconds = 0;
		bool requested = false;

		// Runs on the game thread.
		void Check()
		{
			if (requested || !Settings::Get().autoStart || !Game::Ready() || Game::IsRunning() || Game::EverStarted()) {
				return;
			}
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* ui = RE::UI::GetSingleton();
			auto* controls = RE::ControlMap::GetSingleton();
			if (!player || !player->Is3DLoaded() || !ui || !controls) {
				return;
			}
			const auto* cell = player->GetParentCell();
			if (!cell) {
				return;
			}
			if (cell->IsInteriorCell()) {
				seenInterior = true;
				outdoorSeconds = 0;
				return;
			}
			// Not at an inopportune time: a menu or cutscene, controls locked (cart ride, intro), or a fight.
			if (ui->GameIsPaused() || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) || !controls->IsFightingControlsEnabled() ||
				!controls->IsMovementControlsEnabled() || player->IsInCombat()) {
				outdoorSeconds = 0;
				return;
			}
			outdoorSeconds += kPollSeconds;
			if (!seenInterior && outdoorSeconds < kOutdoorSecondsNeeded) {
				return;
			}
			requested = true;
			SKSE::log::info("Auto-start: starting Last Seed (seen interior: {}, outdoors for {}s)", seenInterior, outdoorSeconds);
			Game::StartLastSeed();
		}
	}

	void Begin()
	{
		static std::atomic<bool> started{ false };
		if (started.exchange(true)) {
			return;
		}
		std::thread([]() {
			for (;;) {
				std::this_thread::sleep_for(std::chrono::seconds(kPollSeconds));
				SKSE::GetTaskInterface()->AddTask(Check);
			}
		}).detach();
		SKSE::log::info("Auto-start watcher running");
	}
}
