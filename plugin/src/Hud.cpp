/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#include "PCH.h"
#include "Hud.h"
#include "Freshness.h"
#include "NativeMcm.h"
#include "Game.h"
#include "Settings.h"

#include "SKSEMenuFramework.h"
#include "BarKit.h"

namespace Hud
{
	namespace
	{
		using namespace ImGuiMCP;
		using namespace BarKit;

		int   previewFrames = 0;
		float saveTimer = -1.0f;
		float groupAlpha = 1.0f;
		int   editTarget = -1;  // see Hud::EditTarget
		int   dragBar = -1;

		FrostfallLayout frostfall;
		constexpr const char* kLogoPath = "Data\\Interface\\lastseed\\lastseed_logo.png";
		constexpr float       kLogoAspect = 417.0f / 1499.0f;
		constexpr float       kLogoFadeIn = 0.8f;
		constexpr float       kLogoHold = 2.6f;
		constexpr float       kLogoFadeOut = 1.2f;
		std::atomic<bool>     logoRequested{ false };
		float                 logoTime = -1.0f;  // seconds since the logo started, -1 = not showing
		int                   lastStartupFinished = -1;  // -1 = not seen yet (a save that is already running shows no logo)
		float                 frostfallTimer = 1e6f;  // seconds since Frostfall's INI was last read

		struct Tracked
		{
			float shown = -1.0f;      // animated fill (0..1)
			float last = -1.0f;       // last raw value, to detect change
			float sinceChange = 1e6f; // seconds since the raw value last changed
		};
		Tracked hungerT, thirstT, fatigueT, vitalityT;

		float Frac(const RE::TESGlobal* a_value, const RE::TESGlobal* a_max, float a_defaultMax)
		{
			const float max = std::max(Game::Value(a_max, a_defaultMax), 1.0f);
			return std::clamp(Game::Value(a_value) / max, 0.0f, 1.0f);
		}

		void Track(Tracked& a_t, float a_frac, float a_dt)
		{
			if (a_t.shown < 0.0f) {
				a_t.shown = a_frac;
				a_t.last = a_frac;
			}
			if (std::abs(a_frac - a_t.last) > 0.001f) {
				a_t.sinceChange = 0.0f;
				a_t.last = a_frac;
			} else {
				a_t.sinceChange += a_dt;
			}
			a_t.shown += (a_frac - a_t.shown) * std::min(1.0f, a_dt * 6.0f);  // smooth fill movement
		}

		// White glyphs on transparent PNGs (Interface/lastseed/icons, drawn by tools/make_icons.py), tinted here.
		enum Icon { kHunger, kThirst, kFatigue, kVitality, kIconCount };

		ImTextureID IconTexture(Icon a_icon)
		{
			static constexpr const char* paths[kIconCount] = {
				"Data\\Interface\\lastseed\\icons\\hunger.png",
				"Data\\Interface\\lastseed\\icons\\thirst.png",
				"Data\\Interface\\lastseed\\icons\\fatigue.png",
				"Data\\Interface\\lastseed\\icons\\vitality.png",
			};
			static ImTextureID textures[kIconCount] = {};
			static bool        loaded = false;
			if (!loaded) {
				loaded = true;
				for (int i = 0; i < kIconCount; ++i) {
					textures[i] = SKSEMenuFramework::LoadTexture(paths[i]);
					if (!textures[i]) {
						SKSE::log::warn("HUD icon not found: {}", paths[i]);
					}
				}
			}
			return textures[a_icon];
		}

		void IconFor(ImDrawList* a_dl, Icon a_icon, const Bar& a_b, float a_k, float a_alpha) { DrawIconFor(a_dl, IconTexture(a_icon), a_b, a_k, a_alpha); }

		bool HudHiddenByGame()
		{
			auto* ui = RE::UI::GetSingleton();
			if (!ui) {
				return true;
			}
			if (ui->GameIsPaused() || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) || ui->IsMenuOpen(RE::MainMenu::MENU_NAME)) {
				return true;
			}
			return !ui->IsShowingMenus();  // the HUD was toggled off (tm)
		}

		// Frostfall's group, as its own INI describes it (the same shape as this one's).
		Group FrostfallGroup(const FrostfallLayout& a_ff)
		{
			Group g;
			g.layout = std::min(a_ff.layout, 1);
			g.orientation = a_ff.orientation;
			g.flip = a_ff.flipFill;
			g.icons = a_ff.showIcons;
			g.thickness = a_ff.barThickness;
			g.length = a_ff.barLength;
			g.spacing = a_ff.spacing;
			g.posX = a_ff.posX;
			g.posY = a_ff.posY;
			return g;
		}

		void DrawBars(ImDrawList* a_dl, float a_dt, bool a_preview, bool a_demo)
		{
			auto&       s = Settings::Get();
			const auto& g = Game::G();
			const auto* io = GetIO();
			const float W = io->DisplaySize.x;
			const float H = io->DisplaySize.y;

			// Need values: higher = needier. Vitality: higher = healthier.
			// While the settings page is open before Last Seed is running, sample values stand in so the bars can still be placed.
			const float nHunger = a_demo ? 0.25f : Frac(g.hunger, g.hungerMax, 120.0f);
			const float nThirst = a_demo ? 0.40f : Frac(g.thirst, g.thirstMax, 120.0f);
			const float nFatigue = a_demo ? 0.30f : Frac(g.fatigue, g.fatigueMax, 120.0f);
			const float fVitality = a_demo ? 0.80f : Frac(g.vitality, g.vitalityMax, 160.0f);
			const bool  satisfaction = s.fillMode == 0;
			Track(hungerT, satisfaction ? 1.0f - nHunger : nHunger, a_dt);
			Track(thirstT, satisfaction ? 1.0f - nThirst : nThirst, a_dt);
			Track(fatigueT, satisfaction ? 1.0f - nFatigue : nFatigue, a_dt);
			Track(vitalityT, fVitality, a_dt);

			// Contextual: stay visible while something changed recently or a need is at "Hungry" / "Thirsty" / "Tired" or worse
			// (value >= 40 of 120), or vitality is low
			float target = 1.0f;
			if (s.displayMode == 1 && !a_preview) {
				const float recent = std::min({ hungerT.sinceChange, thirstT.sinceChange, fatigueT.sinceChange, vitalityT.sinceChange });
				const bool  uncomfortable = nHunger >= 0.33f || nThirst >= 0.33f || nFatigue >= 0.33f || fVitality < 0.35f;
				target = (recent < s.contextualSeconds || uncomfortable) ? 1.0f : 0.0f;
			}
			groupAlpha += (target - groupAlpha) * std::min(1.0f, a_dt * 3.0f);
			const float alpha = groupAlpha * s.opacity;
			if (alpha < 0.01f) {
				return;
			}

			// Size and layout: this group's own settings, or Frostfall's while stacked and matching it. Stacking needs Frostfall's grouped layout
			// (a Frostfall with its bars placed one by one has no group to sit under).
			const bool  stacked = s.stackUnderFrostfall && frostfall.loaded && frostfall.hudEnabled && frostfall.layout != 2;
			const bool  matching = stacked && s.matchFrostfall;
			const float k = ((matching ? frostfall.scale : s.scale) * H) / 1080.0f;
			const bool  icons = matching ? frostfall.showIcons : s.showIcons;

			Bar slots[4];
			if (s.layout == 2 && !stacked) {
				Placed placed[4];
				for (int i = 0; i < 4; ++i) {
					placed[i] = { s.freeBar[i].x, s.freeBar[i].y, s.freeBar[i].orient, s.freeBar[i].flip, s.freeBar[i].length, s.freeBar[i].thickness };
				}
				FreeSlots(placed, s.barThickness, s.barLength, W, H, k, slots);
			} else {
				Group grp;
				grp.icons = icons;
				grp.thickness = matching ? frostfall.barThickness : s.barThickness;
				grp.length = matching ? frostfall.barLength : s.barLength;
				grp.spacing = matching ? frostfall.spacing : s.spacing;
				grp.layout = matching ? std::min(frostfall.layout, 1) : std::min(s.layout, 1);
				grp.orientation = matching ? frostfall.orientation : s.orientation;
				grp.flip = matching ? frostfall.flipFill : s.flipFill;
				if (stacked) {
					// hang from Frostfall's right edge, with the top a little below where its group ends
					float ffTop = 0.0f, ffBottom = 0.0f;
					Bar   scratch[4];
					GroupedSlots(FrostfallGroup(frostfall), W, H, (H / 1080.0f) * frostfall.scale, scratch, &ffTop, &ffBottom);
					grp.posX = frostfall.posX;
					grp.posY = 0.5f;
					float myTop = 0.0f;
					GroupedSlots(grp, W, H, k, slots, &myTop, nullptr);
					const float shift = ffBottom + s.stackGap * (H / 1080.0f) - myTop;
					for (auto& b : slots) {
						b.y += shift;
					}
				} else {
					grp.posX = s.posX;
					grp.posY = s.posY;
					GroupedSlots(grp, W, H, k, slots);
				}
			}

			// Last Seed's own meter colours, a little brighter so they read on the dark track
			const Rgb   hungerCol{ 221, 70, 130 }, thirstCol{ 156, 204, 101 }, fatigueCol{ 80, 160, 220 }, vitalityCol{ 253, 216, 53 };
			const float pulseT = static_cast<float>(ImGuiMCP::GetTime()) * 5.0f;
			auto        pulse = [&](float a_need) { return a_need >= 0.83f ? 0.75f + 0.25f * std::sin(pulseT) : 1.0f; };  // value >= 100: starving etc.

			auto iconAt = [&](int a_slot, Icon a_icon) {
				if (icons) {
					IconFor(a_dl, a_icon, slots[a_slot], k, alpha);
				}
			};

			if (s.showHunger) {
				DrawBar(a_dl, slots[0], hungerT.shown, hungerCol, alpha * pulse(nHunger));
				iconAt(0, kHunger);
			}
			if (s.showThirst) {
				DrawBar(a_dl, slots[1], thirstT.shown, thirstCol, alpha * pulse(nThirst));
				iconAt(1, kThirst);
			}
			if (s.showFatigue) {
				DrawBar(a_dl, slots[2], fatigueT.shown, fatigueCol, alpha * pulse(nFatigue));
				iconAt(2, kFatigue);
			}
			if (s.showVitality) {
				DrawBar(a_dl, slots[3], vitalityT.shown, vitalityCol, alpha);
				iconAt(3, kVitality);
			}

			if (a_preview && editTarget >= 0) {
				float dx = 0, dy = 0;
				if (s.layout == 2 && !stacked && editTarget < 4) {  // one unlocked bar
					const int moved = HandleDrag(a_dl, slots, 4, dragBar, W, H, dx, dy, editTarget);
					if (moved >= 0) {
						auto& fb = s.freeBar[moved];
						fb.x = std::clamp(fb.x + dx, 0.0f, 1.0f);
						fb.y = std::clamp(fb.y + dy, 0.0f, 1.0f);
						MarkSettingsDirty();
					}
				} else if (s.layout != 2 && !stacked && editTarget == 4) {  // the whole group
					const Bar bounds = Bounds(slots, 4, 14.0f * k);
					const int moved = HandleDrag(a_dl, &bounds, 1, dragBar, W, H, dx, dy);
					if (moved >= 0) {
						s.posX = std::clamp(s.posX + dx, 0.0f, 1.0f);
						s.posY = std::clamp(s.posY + dy, 0.0f, 1.0f);
						MarkSettingsDirty();
					}
				} else {
					dragBar = -1;
				}
			} else {
				dragBar = -1;
			}
		}

		void DrawLogo(ImDrawList* a_dl, float a_dt)
		{
			if (logoRequested.exchange(false)) {
				logoTime = 0.0f;
			}
			if (logoTime < 0.0f) {
				return;
			}
			logoTime += a_dt;
			const float total = kLogoFadeIn + kLogoHold + kLogoFadeOut;
			if (logoTime >= total) {
				logoTime = -1.0f;
				return;
			}
			float a = 1.0f;
			if (logoTime < kLogoFadeIn) {
				a = logoTime / kLogoFadeIn;
			} else if (logoTime > kLogoFadeIn + kLogoHold) {
				a = 1.0f - (logoTime - kLogoFadeIn - kLogoHold) / kLogoFadeOut;
			}
			a = a * a * (3.0f - 2.0f * a);  // smoothstep

			static ImTextureID tex = SKSEMenuFramework::LoadTexture(kLogoPath);
			if (!tex) {
				return;
			}
			const auto* io = GetIO();
			const float w = std::min(io->DisplaySize.x * 0.34f, 900.0f);
			const float h = w * kLogoAspect;
			const float x = (io->DisplaySize.x - w) * 0.5f;
			const float y = io->DisplaySize.y * 0.24f;
			const float drift = (1.0f - a) * 6.0f;  // settles upward slightly as it fades in
			const ImU32 shadow = IM_COL32(0, 0, 0, static_cast<int>(165 * a));
			const ImU32 white = IM_COL32(255, 255, 255, static_cast<int>(255 * a));
			ImDrawListManager::AddImage(a_dl, tex, ImVec2(x + 2, y + 3 + drift), ImVec2(x + w + 2, y + h + 3 + drift), ImVec2(0, 0), ImVec2(1, 1), shadow);
			ImDrawListManager::AddImage(a_dl, tex, ImVec2(x, y + drift), ImVec2(x + w, y + h + drift), ImVec2(0, 0), ImVec2(1, 1), white);
		}
	}

	void ShowStartupLogo()
	{
		logoRequested = true;
		SKSE::log::info("Start-up logo requested");
	}

	void MarkPreviewFrame() { previewFrames = 2; }

	int& EditTarget() { return editTarget; }

	void MarkSettingsDirty() { saveTimer = 0.6f; }

	void __stdcall Render()
	{
		const float dt = std::clamp(GetIO()->DeltaTime, 0.0f, 0.1f);
		NativeMcm::Tick(dt);

		if (saveTimer >= 0.0f) {
			saveTimer -= dt;
			if (saveTimer < 0.0f) {
				Settings::Get().Save();
			}
		}

		// Frostfall's layout changes only from its own settings page, so a re-read once a second is plenty
		frostfallTimer += dt;
		if (frostfallTimer >= 1.0f) {
			frostfallTimer = 0.0f;
			frostfall = FrostfallLayout::Read();
		}

		const bool preview = previewFrames > 0;
		if (previewFrames > 0) {
			--previewFrames;
		}

		auto* dl = GetForegroundDrawList();
		Freshness::Draw(dl, GetIO()->DisplaySize.x, GetIO()->DisplaySize.y);  // over the inventory item card, whatever the HUD is doing
		if (!preview && (SKSEMenuFramework::IsAnyBlockingWindowOpened() || HudHiddenByGame())) {
			return;
		}

		// The logo plays when Last Seed's start-up finishes, however it was started (auto-start, MCM, menu button).
		const int finished = Game::StartupFinished() ? 2 : 1;
		if (lastStartupFinished == -1) {
			lastStartupFinished = finished;
			if (finished == 2) {
				Game::RefreshOldMeters();  // a save that is already running: hide the old SkyUI meters
			}
		} else if (finished != lastStartupFinished) {
			lastStartupFinished = finished;
			if (finished == 2) {
				ShowStartupLogo();
				Game::RefreshOldMeters();
			}
		}
		// Switching the bars on or off in the settings page: hide or bring back the old meters right away.
		static int lastHudEnabled = -1;
		const int hudEnabled = Settings::Get().hudEnabled ? 1 : 0;
		if (lastHudEnabled != -1 && hudEnabled != lastHudEnabled && finished == 2) {
			Game::RefreshOldMeters();
		}
		lastHudEnabled = hudEnabled;
		DrawLogo(dl, dt);

		// Bars appear with the logo, once Last Seed's start-up has finished (not while it is still checking mods). The settings page shows
		// them with sample values before that, so they can be placed.
		const bool running = Game::IsRunning() && Game::StartupFinished();
		if (!running && !preview) {
			return;
		}
		if (Settings::Get().hudEnabled) {
			DrawBars(dl, dt, preview, !running);
		}
	}
}
