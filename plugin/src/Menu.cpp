#include "PCH.h"
#include "Menu.h"
#include "Game.h"
#include "Hud.h"
#include "NativeMcm.h"
#include "Settings.h"
#include "Spoilage.h"

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

		constexpr const char* kLogoPath = "Data\\Interface\\lastseed\\lastseed_logo.png";

		void Logo()
		{
			static ImTextureID tex = SKSEMenuFramework::LoadTexture(kLogoPath);
			if (!tex) {
				return;
			}
			const float avail = GetContentRegionAvail().x;
			const float w = std::min(avail, 460.0f);
			SetCursorPosX(GetCursorPosX() + (avail - w) * 0.5f);
			Image(tex, ImVec2(w, w * 417.0f / 1499.0f));
			Spacing();
		}

		void __stdcall RenderOverview()
		{
			Logo();
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
			if (Game::FrostfallInstalled()) {
				if (Checkbox("Wait for Frostfall to start first", &settings.waitForFrostfall)) {
					settings.Save();
				}
			}
			TextWrapped("Auto-start waits for Frostfall to start (when it is installed), otherwise until you are outside, in control and out of any menu or fight.");

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

		void __stdcall RenderSpoilage()
		{
			auto& s = Settings::Get();
			if (Checkbox("Track food spoilage in LastSeed.dll", &s.nativeSpoilage)) {
				s.Save();
			}
			TextWrapped(
				"Replaces Last Seed's Papyrus spoilage (an invisible tracker object per food stack, each waking up every game hour) with "
				"records kept by this plugin. The food categories, rot times and spoiled items are still Last Seed's own, and they are still "
				"set in its Mod Configuration Menu. Takes effect on the next game start, or when you start a new game.");
			if (Checkbox("Freshness bar over the item card when hovering food", &s.freshnessBar)) {
				s.Save();
			}
			BeginDisabled(!s.freshnessBar);
			TextWrapped("The defaults fit the vanilla / SkyUI inventory. Other inventory skins put the item card elsewhere: open the inventory, hover a food, and use these to place the bar.");
			if (SliderFloat("Bar left edge", &s.freshLeft, 0.0f, 1.0f, "%.3f")) {
				s.Save();
			}
			if (SliderFloat("Bar width", &s.freshWidth, 0.02f, 1.0f, "%.3f")) {
				s.Save();
			}
			if (SliderFloat("Bar height offset", &s.freshOffsetY, -200.0f, 200.0f, "%.0f")) {
				s.Save();
			}
			if (Button("Reset bar position")) {
				const Settings d;
				s.freshLeft = d.freshLeft;
				s.freshWidth = d.freshWidth;
				s.freshOffsetY = d.freshOffsetY;
				s.Save();
			}
			EndDisabled();
			Spacing();
			BeginDisabled(!s.nativeSpoilage);
			if (Checkbox("Foods Last Seed does not list spoil too (by Vendor Item Food keywords)", &s.keywordFoods)) {
				s.Save();
			}
			TextWrapped(
				"Food added by other mods that Last Seed knows nothing about: anything eaten (not drunk) that carries Skyrim's VendorItemFood "
				"or VendorItemFoodRaw keyword rots into Perished Food after the hours below. No patches needed for new foods.");
			BeginDisabled(!s.keywordFoods);
			if (SliderFloat("Raw food rots after (game hours)", &s.rawFoodHours, 1.0f, 500.0f, "%.0f")) {
				s.Save();
			}
			if (SliderFloat("Other food rots after (game hours)", &s.otherFoodHours, 1.0f, 500.0f, "%.0f")) {
				s.Save();
			}
			EndDisabled();
			Spacing();
			if (Checkbox("Food in world containers spoils (barrels, chests, sacks)", &s.worldSpoilage)) {
				s.Save();
			}
			TextWrapped(
				"When you come near a container for the first time, each food in it may have spoiled, using the chance below (this replaces "
				"Last Seed's Container Spoilage Rate), but a share of its food is always left fresh. After the reset period, spoiled food "
				"you left in it turns fresh again and is rolled again, so containers do not stay rotten forever. Chests you have put food "
				"into, and containers you own, are never touched.");
			BeginDisabled(!s.worldSpoilage);
			if (SliderFloat("Chance each food has spoiled (%)", &s.worldSpoilChance, 0.0f, 100.0f, "%.0f")) {
				s.Save();
			}
			if (SliderFloat("Always left fresh in each container (%)", &s.worldFreshShare, 0.0f, 100.0f, "%.0f")) {
				s.Save();
			}
			if (SliderFloat("Reset period (game days)", &s.containerResetDays, 1.0f, 120.0f, "%.0f")) {
				s.Save();
			}
			EndDisabled();
			EndDisabled();
			Spacing();
			Separator();

			const auto stats = Spoilage::Snapshot(40);
			if (!Game::IsRunning()) {
				TextDisabled("Last Seed is not running.");
				return;
			}
			if (!s.nativeSpoilage) {
				TextDisabled("Native tracking is off: Last Seed's own scripts handle spoilage.");
				return;
			}
			if (!stats.active) {
				TextDisabled("Spoilage is switched off in Last Seed's settings (Gameplay > Spoilage), so nothing is being tracked.");
				return;
			}
			Text("%d stacks of food, %d items in total. Rot speed now: x%.2f", stats.batches, stats.items, stats.speed);
				if (s.worldSpoilage) {
					Text("%d world containers currently hold food that spoiled before you got to them.", stats.worldContainers);
				}
			Spacing();
			if (stats.rows.empty()) {
				TextDisabled("You are not carrying anything that spoils.");
				return;
			}
			Text("Closest to spoiling first (you, your Provisions container, and your followers):");
			for (const auto& r : stats.rows) {
				ImVec4 col(0.65f, 0.9f, 0.5f, 1.0f);  // fresh
				if (r.rottedFraction >= 0.75f) {
					col = ImVec4(0.95f, 0.45f, 0.35f, 1.0f);
				} else if (r.rottedFraction >= 0.4f) {
					col = ImVec4(0.95f, 0.8f, 0.4f, 1.0f);
				}
				TextColored(col, "%s x%d", r.food.c_str(), r.count);
				SameLine();
				TextDisabled("(%s)", r.container.c_str());
				if (r.hoursLeft >= 48.0f) {
					ProgressBar(r.rottedFraction, ImVec2(-1.0f, 0.0f), std::format("about {:.0f} days left", r.hoursLeft / 24.0f).c_str());
				} else {
					ProgressBar(r.rottedFraction, ImVec2(-1.0f, 0.0f), std::format("about {:.0f} hours left", r.hoursLeft).c_str());
				}
			}
		}

		void __stdcall RenderFoodSpoilageSettings() { NativeMcm::DrawPage(4); }

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
		SKSEMenuFramework::AddSectionItem("Spoilage", RenderSpoilage);
		SKSEMenuFramework::AddSectionItem("Food Spoilage Settings", RenderFoodSpoilageSettings);
		SKSEMenuFramework::AddHudElement(Hud::Render);
		registered = true;
		SKSE::log::info("Registered with SKSE Menu Framework {}", SKSEMenuFramework::GetMenuFrameworkVersion());
	}
}
