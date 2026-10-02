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
	};

	bool           Init();  // after data is loaded
	const Globals& G();
	bool           Ready();
	bool           IsRunning();
	float          Value(const RE::TESGlobal* a_global, float a_fallback = 0.0f);
}
