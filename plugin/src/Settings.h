#pragma once

// LastSeed.dll settings, stored in Data/SKSE/Plugins/LastSeed.ini (edited from the SKSE Menu Framework page).
struct Settings
{
	// [General]
	bool  waitForFrostfall = true;   // with Frostfall installed, start Last Seed only after Frostfall has started (like Frostfall's own pattern)
	float freshLeft = 0.532f;         // freshness bar: left edge, fraction of the screen width
	float freshWidth = 0.390f;        // freshness bar: width, fraction of the screen width
	float freshOffsetY = 0.0f;        // freshness bar: moves it down (+) or up (-), in 720p stage units
	bool  keywordFoods = true;        // foods Last Seed does not list spoil too, found by Skyrim's Vendor Item Food keywords
	float rawFoodHours = 48.0f;       // ... raw ones (VendorItemFoodRaw)
	float otherFoodHours = 120.0f;    // ... everything else (VendorItemFood)
	bool  baitCompat = true;          // spoiled food goes on the bait list of any fishing mod
	bool  freshnessBar = true;        // thin freshness bar over the item card while hovering a food that spoils
	bool  worldSpoilage = true;       // food in world containers spoils (LastSeed.dll), and is restocked fresh after the reset period
	float worldSpoilChance = 40.0f;   // % chance each food in a world container has spoiled when first seen (replaces Last Seed's Container Spoilage Rate)
	float worldFreshShare = 25.0f;    // % of the food in each world container that is always left fresh
	float containerResetDays = 30.0f; // game days before a world container's spoiled food is made fresh again and re-rolled
	bool  nativeSpoilage = true;     // food spoilage runs in LastSeed.dll instead of Last Seed's Papyrus trackers
	bool  autoStart = true;          // start Last Seed on its own the first time the player is outside and in control

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
