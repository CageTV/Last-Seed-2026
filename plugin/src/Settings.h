#pragma once

// LastSeed.dll settings, stored in Data/SKSE/Plugins/LastSeed.ini (edited from the SKSE Menu Framework page).
struct Settings
{
	// [HUD]
	bool  hudEnabled = true;
	int   displayMode = 1;           // 0 = always, 1 = contextual (fade out while comfortable and unchanged)
	int   fillMode = 0;              // 0 = bars show how well off you are (drain as a need grows), 1 = bars show the need itself
	int   layout = 0;                // 0 = 2 x 2 grid, 1 = single row (ignored while matching Frostfall's layout)
	bool  stackUnderFrostfall = true;  // put the group directly below Frostfall's bars (needs Frostfall.dll's HUD on)
	bool  matchFrostfall = true;     // while stacked: use Frostfall's bar size, spacing, layout and icon setting
	float stackGap = 18.0f;          // pixels at 1080p between Frostfall's group and this one
	float posX = 0.975f;             // right edge of the bar group, fraction of screen width (when not stacked)
	float posY = 0.78f;              // vertical centre of the bar group, fraction of screen height (when not stacked)
	float scale = 1.0f;
	float opacity = 0.90f;
	float barLength = 96.0f;         // pixels at 1080p, before scale (when not matching Frostfall)
	float barThickness = 8.0f;
	float spacing = 10.0f;
	float contextualSeconds = 6.0f;
	bool  showHunger = true;
	bool  showThirst = true;
	bool  showFatigue = true;
	bool  showVitality = true;
	bool  showIcons = true;

	static constexpr int kVersion = 1;

	static Settings& Get();
	void Load();
	void Save() const;
	void ResetHudLayout();
};

// Frostfall.dll's HUD layout, read (never written) from its own INI so Last Seed's group can sit directly below it.
struct FrostfallLayout
{
	bool  loaded = false;        // Frostfall.dll is loaded in this game
	bool  hudEnabled = true;
	int   layout = 0;
	float posX = 0.975f;
	float posY = 0.50f;
	float scale = 1.0f;
	float barLength = 96.0f;
	float barThickness = 8.0f;
	float spacing = 10.0f;
	bool  showIcons = true;

	static FrostfallLayout Read();
};
