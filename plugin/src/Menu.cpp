#include "PCH.h"
#include "Menu.h"
#include "Game.h"
#include "Hud.h"
#include "Settings.h"

#include "SKSEMenuFramework.h"

namespace Menu
{
	namespace
	{
		using namespace ImGuiMCP;

		bool Changed(bool a_changed)
		{
			if (a_changed) {
				Hud::MarkSettingsDirty();
			}
			return a_changed;
		}

		void __stdcall RenderOverview()
		{
			if (!Game::Ready()) {
				TextColored(ImVec4(1.0f, 0.45f, 0.4f, 1.0f), "LastSeed.esp is not loaded.");
				return;
			}

			if (Game::IsRunning()) {
				TextColored(ImVec4(0.65f, 0.9f, 0.5f, 1.0f), "Last Seed is running.");
			} else {
				TextColored(ImVec4(0.75f, 0.75f, 0.75f, 1.0f), "Last Seed is not running.");
			}

			Spacing();
			if (Game::IsRunning()) {
				if (Button("Stop Last Seed")) {
					Game::StopLastSeed();
				}
			} else if (Button("Start Last Seed")) {
				Game::StartLastSeed();
			}
			auto& settings = Settings::Get();
			if (Checkbox("Start Last Seed automatically on a new game", &settings.autoStart)) {
				settings.Save();
			}
			TextWrapped("Auto-start waits until you are outside, in control and out of any menu or fight, so it never interrupts the opening.");

			Spacing();
			Separator();
			Text("Right now (0 = fully satisfied, higher = needier; vitality: higher = healthier)");
			const auto& g = Game::G();
			Text("Hunger    %.0f / %.0f", Game::Value(g.hunger), Game::Value(g.hungerMax, 120.0f));
			Text("Thirst    %.0f / %.0f", Game::Value(g.thirst), Game::Value(g.thirstMax, 120.0f));
			Text("Fatigue   %.0f / %.0f", Game::Value(g.fatigue), Game::Value(g.fatigueMax, 120.0f));
			Text("Vitality  %.0f / %.0f", Game::Value(g.vitality), Game::Value(g.vitalityMax, 160.0f));

			Spacing();
			Separator();
			TextWrapped("Last Seed's other settings (gameplay, food, diseases, profiles) are still in the SkyUI Mod Configuration Menu.");
		}

		void __stdcall RenderHud()
		{
			auto& s = Settings::Get();
			Hud::MarkPreviewFrame();  // keep the bars visible while this page is open

			Changed(Checkbox("Show Last Seed bars", &s.hudEnabled));
			Spacing();

			BeginDisabled(!s.hudEnabled);
			static const char* displayModes[] = { "Always", "Contextual (fade out while comfortable)" };
			Changed(Combo("Display", &s.displayMode, displayModes, 2));
			if (s.displayMode == 1) {
				Changed(SliderFloat("Stay visible after a change (s)", &s.contextualSeconds, 1.0f, 30.0f, "%.0f"));
			}
			static const char* fillModes[] = { "How well off you are (bars drain as a need grows)", "The need itself (bars fill as a need grows)" };
			Changed(Combo("Hunger / thirst / fatigue bars show", &s.fillMode, fillModes, 2));

			Spacing();
			Separator();
			Text("Position and size");
			Changed(Checkbox("Stack directly below Frostfall's bars", &s.stackUnderFrostfall));
			if (s.stackUnderFrostfall) {
				Changed(Checkbox("Match Frostfall's bar size and layout", &s.matchFrostfall));
				Changed(SliderFloat("Gap below Frostfall", &s.stackGap, 0.0f, 100.0f, "%.0f"));
				TextWrapped("Stacking needs Frostfall's HUD bars to be on; otherwise the position below is used.");
			}
			const bool stacked = s.stackUnderFrostfall;
			const bool matching = stacked && s.matchFrostfall;
			BeginDisabled(stacked);
			Changed(SliderFloat("Horizontal (right edge)", &s.posX, 0.0f, 1.0f, "%.3f"));
			Changed(SliderFloat("Vertical (centre)", &s.posY, 0.0f, 1.0f, "%.3f"));
			EndDisabled();
			BeginDisabled(matching);
			static const char* layouts[] = { "2 x 2 grid", "Single row" };
			Changed(Combo("Layout", &s.layout, layouts, 2));
			Changed(SliderFloat("Scale", &s.scale, 0.5f, 3.0f, "%.2f"));
			Changed(SliderFloat("Bar length", &s.barLength, 30.0f, 400.0f, "%.0f"));
			Changed(SliderFloat("Bar thickness", &s.barThickness, 3.0f, 30.0f, "%.0f"));
			Changed(SliderFloat("Spacing", &s.spacing, 2.0f, 40.0f, "%.0f"));
			EndDisabled();
			Changed(SliderFloat("Opacity", &s.opacity, 0.1f, 1.0f, "%.2f"));

			Spacing();
			Separator();
			Text("Bars");
			Changed(Checkbox("Hunger", &s.showHunger));
			Changed(Checkbox("Thirst", &s.showThirst));
			Changed(Checkbox("Fatigue", &s.showFatigue));
			Changed(Checkbox("Vitality", &s.showVitality));
			BeginDisabled(matching);
			Changed(Checkbox("Icons under the bars", &s.showIcons));
			EndDisabled();
			EndDisabled();

			Spacing();
			if (Button("Reset position and size")) {
				s.ResetHudLayout();
				Hud::MarkSettingsDirty();
			}
		}

		std::atomic<bool> registered{ false };
	}

	bool Registered() { return registered; }

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			SKSE::log::warn("SKSE Menu Framework not found: no HUD bars or settings page (Last Seed keeps its SkyUI meters)");
			return;
		}
		SKSEMenuFramework::SetSection("Last Seed");
		SKSEMenuFramework::AddSectionItem("Overview", RenderOverview);
		SKSEMenuFramework::AddSectionItem("HUD", RenderHud);
		SKSEMenuFramework::AddHudElement(Hud::Render);
		registered = true;
		SKSE::log::info("Registered with SKSE Menu Framework {}", SKSEMenuFramework::GetMenuFrameworkVersion());
	}
}
