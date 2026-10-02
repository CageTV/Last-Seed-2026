#include "PCH.h"
#include "Game.h"
#include "GameIds.h"

namespace Game
{
	namespace
	{
		Globals g;
		bool    ready = false;

		RE::TESGlobal* Lookup(RE::FormID a_localID, const char* a_name)
		{
			auto* dh = RE::TESDataHandler::GetSingleton();
			auto* form = dh ? dh->LookupForm<RE::TESGlobal>(a_localID, "LastSeed.esp") : nullptr;
			if (!form) {
				SKSE::log::error("LastSeed.esp global {} ({:06X}) not found", a_name, a_localID);
			}
			return form;
		}
	}

	bool Init()
	{
		g.running = Lookup(ids::LastSeedRunning, "LastSeedRunning");
		g.hunger = Lookup(ids::Seed_AttributeHunger, "_Seed_AttributeHunger");
		g.hungerMax = Lookup(ids::Seed_AttributeHungerMax, "_Seed_AttributeHungerMax");
		g.thirst = Lookup(ids::Seed_AttributeThirst, "_Seed_AttributeThirst");
		g.thirstMax = Lookup(ids::Seed_AttributeThirstMax, "_Seed_AttributeThirstMax");
		g.fatigue = Lookup(ids::Seed_AttributeFatigue, "_Seed_AttributeFatigue");
		g.fatigueMax = Lookup(ids::Seed_AttributeFatigueMax, "_Seed_AttributeFatigueMax");
		g.vitality = Lookup(ids::Seed_AttributeVitality, "_Seed_AttributeVitality");
		g.vitalityMax = Lookup(ids::Seed_AttributeVitalityMax, "_Seed_AttributeVitalityMax");
		ready = g.running && g.hunger && g.thirst && g.fatigue && g.vitality;
		SKSE::log::info("Last Seed globals {}", ready ? "found" : "MISSING - is LastSeed.esp enabled?");
		return ready;
	}

	const Globals& G() { return g; }
	bool           Ready() { return ready; }

	float Value(const RE::TESGlobal* a_global, float a_fallback)
	{
		return a_global ? a_global->value : a_fallback;
	}

	bool IsRunning()
	{
		return ready && static_cast<int>(Value(g.running)) == 2;
	}
}
