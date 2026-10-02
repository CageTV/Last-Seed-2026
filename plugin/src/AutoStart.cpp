#include "PCH.h"
#include "AutoStart.h"
#include "Game.h"
#include "Settings.h"

namespace AutoStart
{
	namespace
	{
		constexpr int kPollSeconds = 2;
		constexpr int kAfterFrostfallSeconds = 8;   // let Frostfall's start-up logo finish before Last Seed starts
		constexpr int kOutdoorSecondsNeeded = 30;   // without Frostfall: loaded save that has not been indoors this session

		bool        seenInterior = false;
		int         outdoorSeconds = 0;
		int         frostfallSeconds = 0;
		bool        requested = false;
		const char* lastReason = "";

		void Reason(const char* a_reason)
		{
			if (a_reason != lastReason) {
				lastReason = a_reason;
				SKSE::log::info("Auto-start: {}", a_reason);
			}
		}

		// Runs on the game thread.
		void Check()
		{
			if (requested || !Game::Ready() || Game::IsRunning() || Game::EverStarted()) {
				return;
			}
			if (!Settings::Get().autoStart) {
				Reason("off in settings");
				return;
			}
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* ui = RE::UI::GetSingleton();
			auto* controls = RE::ControlMap::GetSingleton();
			if (!player || !player->Is3DLoaded() || !ui || !controls || !player->GetParentCell()) {
				Reason("waiting for the player to load");
				return;
			}

			// Follow Frostfall's pattern: with Frostfall installed, wait until it is running, then start Last Seed a few seconds later.
			if (Settings::Get().waitForFrostfall && Game::FrostfallInstalled()) {
				if (!Game::FrostfallRunning()) {
					frostfallSeconds = 0;
					Reason("waiting for Frostfall to start");
					return;
				}
				if (ui->GameIsPaused() || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME)) {
					Reason("Frostfall is running; waiting for the menu to close");
					return;
				}
				frostfallSeconds += kPollSeconds;
				if (frostfallSeconds < kAfterFrostfallSeconds) {
					Reason("Frostfall is running; giving its start-up logo time to finish");
					return;
				}
				requested = true;
				SKSE::log::info("Auto-start: starting Last Seed after Frostfall");
				Game::StartLastSeed();
				return;
			}

			// No Frostfall: start the first time the player is outside, in control and out of any menu or fight.
			if (player->GetParentCell()->IsInteriorCell()) {
				seenInterior = true;
				outdoorSeconds = 0;
				Reason("indoors");
				return;
			}
			if (ui->GameIsPaused() || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) || !controls->IsFightingControlsEnabled() ||
				!controls->IsMovementControlsEnabled() || player->IsInCombat()) {
				outdoorSeconds = 0;
				Reason("waiting for control (menu, cutscene or fight)");
				return;
			}
			outdoorSeconds += kPollSeconds;
			if (!seenInterior && outdoorSeconds < kOutdoorSecondsNeeded) {
				Reason("outdoors; waiting to be sure the opening is over");
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
		SKSE::log::info("Auto-start watcher running (Frostfall {})", Game::FrostfallInstalled() ? "installed: waiting for it" : "not found");
	}
}
