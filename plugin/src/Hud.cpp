#include "PCH.h"
#include "Hud.h"
#include "Game.h"
#include "Settings.h"

#include "SKSEMenuFramework.h"

namespace Hud
{
	namespace
	{
		using namespace ImGuiMCP;

		int   previewFrames = 0;
		float saveTimer = -1.0f;
		float groupAlpha = 1.0f;

		FrostfallLayout frostfall;
		constexpr const char* kLogoPath = "Data\\Interface\\lastseed\\lastseed_logo.png";
		constexpr float       kLogoAspect = 150.0f / 760.0f;
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

		ImU32 Col(float r, float g, float b, float a)
		{
			return IM_COL32(static_cast<int>(std::clamp(r, 0.0f, 255.0f)), static_cast<int>(std::clamp(g, 0.0f, 255.0f)),
				static_cast<int>(std::clamp(b, 0.0f, 255.0f)), static_cast<int>(std::clamp(a, 0.0f, 1.0f) * 255.0f));
		}

		struct Rgb { float r, g, b; };

		// One flat vertical bar: dark track, fill from the bottom, slightly lighter at the top of the fill. No border.
		// (Same drawing as Frostfall's bars so the two groups look like one HUD.)
		void DrawBar(ImDrawList* a_dl, float a_x, float a_y, float a_w, float a_h, float a_fill, Rgb a_color, float a_alpha)
		{
			const float rounding = std::min(a_w * 0.35f, 4.0f);
			ImDrawListManager::AddRectFilled(a_dl, ImVec2(a_x, a_y), ImVec2(a_x + a_w, a_y + a_h), Col(10, 12, 16, 0.55f * a_alpha), rounding, 0);
			const float fh = a_h * std::clamp(a_fill, 0.0f, 1.0f);
			if (fh < 0.5f) {
				return;
			}
			const float top = a_y + a_h - fh;
			const ImU32 cTop = Col(a_color.r + 35, a_color.g + 35, a_color.b + 35, 0.95f * a_alpha);
			const ImU32 cBot = Col(a_color.r * 0.8f, a_color.g * 0.8f, a_color.b * 0.8f, 0.95f * a_alpha);
			ImDrawListManager::AddRectFilledMultiColor(a_dl, ImVec2(a_x, top), ImVec2(a_x + a_w, a_y + a_h), cTop, cTop, cBot, cBot);
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

		void DrawIcon(ImDrawList* a_dl, Icon a_icon, float a_cx, float a_top, float a_size, float a_alpha)
		{
			auto* tex = IconTexture(a_icon);
			if (!tex) {
				return;
			}
			const float x = a_cx - a_size * 0.5f;
			ImDrawListManager::AddImage(a_dl, tex, ImVec2(x + 1.0f, a_top + 1.0f), ImVec2(x + a_size + 1.0f, a_top + a_size + 1.0f),
				ImVec2(0, 0), ImVec2(1, 1), Col(0, 0, 0, 0.55f * a_alpha));
			ImDrawListManager::AddImage(a_dl, tex, ImVec2(x, a_top), ImVec2(x + a_size, a_top + a_size),
				ImVec2(0, 0), ImVec2(1, 1), Col(225, 232, 240, 0.9f * a_alpha));
		}

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

		// Where the bottom edge of Frostfall's bar group (including the icons under its last row) sits, using Frostfall's own
		// layout maths (Frostfall.dll Hud.cpp DrawBars), so this group can start just below it.
		float FrostfallBottom(const FrostfallLayout& a_ff, float a_w, float a_h)
		{
			(void)a_w;
			const float k = (a_h / 1080.0f) * a_ff.scale;
			const float L = a_ff.barLength * k;
			const float gap = a_ff.spacing * k;
			const float icon = a_ff.showIcons ? 16.0f * k : 0.0f;
			const float rowGap = gap + icon + (a_ff.showIcons ? 6.0f * k : 0.0f);
			const float centerY = a_ff.posY * a_h;
			float       barsBottom;
			if (a_ff.layout == 0) {
				const float groupH = 2 * L + rowGap;
				barsBottom = centerY - groupH * 0.5f + groupH;
			} else {
				barsBottom = centerY - (L + icon) * 0.5f + L;
			}
			return barsBottom + (a_ff.showIcons ? 19.0f * k : 0.0f);
		}

		void DrawBars(ImDrawList* a_dl, float a_dt, bool a_preview)
		{
			auto&       s = Settings::Get();
			const auto& g = Game::G();
			const auto* io = GetIO();
			const float W = io->DisplaySize.x;
			const float H = io->DisplaySize.y;

			// Need values: higher = needier. Vitality: higher = healthier.
			const float nHunger = Frac(g.hunger, g.hungerMax, 120.0f);
			const float nThirst = Frac(g.thirst, g.thirstMax, 120.0f);
			const float nFatigue = Frac(g.fatigue, g.fatigueMax, 120.0f);
			const float fVitality = Frac(g.vitality, g.vitalityMax, 160.0f);
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

			// Size and layout: this group's own settings, or Frostfall's while stacked and matching it
			const bool stacked = s.stackUnderFrostfall && frostfall.loaded && frostfall.hudEnabled;
			const bool matching = stacked && s.matchFrostfall;
			const float k = ((matching ? frostfall.scale : s.scale) * H) / 1080.0f;
			const float t = (matching ? frostfall.barThickness : s.barThickness) * k;
			const float L = (matching ? frostfall.barLength : s.barLength) * k;
			const float gap = (matching ? frostfall.spacing : s.spacing) * k;
			const bool  icons = matching ? frostfall.showIcons : s.showIcons;
			const int   layout = matching ? frostfall.layout : s.layout;
			const float icon = icons ? 16.0f * k : 0.0f;
			const float rowGap = gap + icon + (icons ? 6.0f * k : 0.0f);
			const float stepX = t + gap * 1.6f;

			const float groupH = layout == 0 ? 2 * L + rowGap : L + icon;
			float       right = s.posX * W;
			float       y0;
			if (stacked) {
				right = frostfall.posX * W;
				y0 = FrostfallBottom(frostfall, W, H) + s.stackGap * (H / 1080.0f);
			} else {
				y0 = s.posY * H - groupH * 0.5f;
			}

			// Slot positions: grid = 2 columns x 2 rows; row = 4 across
			struct Slot { float x, y; };
			Slot slots[4];
			if (layout == 0) {
				const float groupW = 2 * t + gap * 1.6f;
				const float x0 = right - groupW;
				slots[0] = { x0, y0 };
				slots[1] = { x0 + stepX, y0 };
				slots[2] = { x0, y0 + L + rowGap };
				slots[3] = { x0 + stepX, y0 + L + rowGap };
			} else {
				const float x0 = right - (4 * t + 3 * gap * 1.6f);
				for (int i = 0; i < 4; ++i) {
					slots[i] = { x0 + i * stepX, y0 };
				}
			}

			// Last Seed's own meter colours, a little brighter so they read on the dark track
			const Rgb hungerCol{ 221, 70, 130 }, thirstCol{ 156, 204, 101 }, fatigueCol{ 80, 160, 220 }, vitalityCol{ 253, 216, 53 };
			const float pulseT = static_cast<float>(ImGuiMCP::GetTime()) * 5.0f;
			auto        pulse = [&](float a_need) { return a_need >= 0.83f ? 0.75f + 0.25f * std::sin(pulseT) : 1.0f; };  // value >= 100: starving etc.

			auto iconAt = [&](int a_slot, Icon a_icon) {
				if (icons) {
					DrawIcon(a_dl, a_icon, slots[a_slot].x + t * 0.5f, slots[a_slot].y + L + 5.0f * k, 14.0f * k, alpha);
				}
			};

			if (s.showHunger) {
				DrawBar(a_dl, slots[0].x, slots[0].y, t, L, hungerT.shown, hungerCol, alpha * pulse(nHunger));
				iconAt(0, kHunger);
			}
			if (s.showThirst) {
				DrawBar(a_dl, slots[1].x, slots[1].y, t, L, thirstT.shown, thirstCol, alpha * pulse(nThirst));
				iconAt(1, kThirst);
			}
			if (s.showFatigue) {
				DrawBar(a_dl, slots[2].x, slots[2].y, t, L, fatigueT.shown, fatigueCol, alpha * pulse(nFatigue));
				iconAt(2, kFatigue);
			}
			if (s.showVitality) {
				DrawBar(a_dl, slots[3].x, slots[3].y, t, L, vitalityT.shown, vitalityCol, alpha);
				iconAt(3, kVitality);
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

	void MarkSettingsDirty() { saveTimer = 0.6f; }

	void __stdcall Render()
	{
		const float dt = std::clamp(GetIO()->DeltaTime, 0.0f, 0.1f);

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
		if (!preview && (SKSEMenuFramework::IsAnyBlockingWindowOpened() || HudHiddenByGame())) {
			return;
		}

		// The logo plays when Last Seed's start-up finishes, however it was started (auto-start, MCM, menu button).
		const int finished = Game::StartupFinished() ? 2 : 1;
		if (lastStartupFinished == -1) {
			lastStartupFinished = finished;
		} else if (finished != lastStartupFinished) {
			lastStartupFinished = finished;
			if (finished == 2) {
				ShowStartupLogo();
			}
		}
		DrawLogo(dl, dt);

		if (!Game::IsRunning()) {
			return;
		}
		if (Settings::Get().hudEnabled) {
			DrawBars(dl, dt, preview);
		}
	}
}
