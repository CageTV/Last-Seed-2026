/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#pragma once

// Last Seed's game data as the plugin sees it: the globals behind the bars.
// Each need is a value from 0 to its max (120; vitality 160). A HIGHER need value means MORE needy (hungrier, thirstier,
// more tired), with stage breaks at 20 / 40 / 60 / 80 / 100. Vitality is the other way round (higher = healthier).
namespace Game
{
	struct Globals
	{
		RE::TESGlobal* running = nullptr;       // LastSeedRunning (2 = running)
		RE::TESGlobal* hunger = nullptr;        // _Seed_AttributeHunger
		RE::TESGlobal* hungerMax = nullptr;     // _Seed_AttributeHungerMax
		RE::TESGlobal* thirst = nullptr;        // _Seed_AttributeThirst
		RE::TESGlobal* thirstMax = nullptr;     // _Seed_AttributeThirstMax
		RE::TESGlobal* fatigue = nullptr;       // _Seed_AttributeFatigue
		RE::TESGlobal* fatigueMax = nullptr;    // _Seed_AttributeFatigueMax
		RE::TESGlobal* vitality = nullptr;      // _Seed_AttributeVitality
		RE::TESGlobal* vitalityMax = nullptr;   // _Seed_AttributeVitalityMax
		RE::TESGlobal* startupFinished = nullptr;  // LastSeedStartupFinished (2 = start-up complete)
		RE::TESGlobal* frostfallRunning = nullptr; // Frostfall.esp FrostfallRunning, null when Frostfall is not installed
		RE::TESGlobal* kwCheck = nullptr;       // LastSeedRunning_KWCheck
		RE::TESQuest*  mainQuest = nullptr;      // _Seed_MainQuest (script _Seed_Main)
		RE::TESQuest*  trackingQuest = nullptr;  // _Seed_TrackingQuest
	};

	bool           Init();  // after data is loaded
	const Globals& G();
	bool           Ready();
	bool           IsRunning();
	bool           StartupFinished();
	bool           FrostfallRunning();  // false when Frostfall is not installed
	bool           FrostfallInstalled();
	// Same steps as the MCM's start / stop option: set the running globals, then call _Seed_Main.StartLastSeed / StopLastSeed.
	void           StartLastSeed();
	void           StopLastSeed();
	// True once Last Seed has been started at least once in this save (its tracking quest reached stage 20).
	bool           EverStarted();
	// Tells LastSeedNative (Papyrus) to hide or bring back Last Seed's own SkyUI meters, depending on whether the HUD bars are on.
	void           RefreshOldMeters();
	float         Value(const RE::TESGlobal* a_global, float a_fallback = 0.0f);
}
