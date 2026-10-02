#pragma once
// Local FormIDs (in LastSeed.esp) of the globals LastSeed.dll reads. Read from LastSeed.esp 5.3 (Nexus 56393) with houseCARL on 2026-10-02.

namespace ids
{
	inline constexpr unsigned int FrostfallRunning = 0x06DCFB;          // in Frostfall.esp (2 = running)
	inline constexpr unsigned int MainQuest = 0x000D72;                // _Seed_MainQuest, carries the _Seed_Main script
	inline constexpr unsigned int TrackingQuest = 0x00A69D;            // _Seed_TrackingQuest (stage 20 = has been started)
	inline constexpr unsigned int LastSeedStartupFinished = 0x5525A8;
	inline constexpr unsigned int LastSeedRunningKWCheck = 0xD9FF01;    // declared in Update.esm, overridden by LastSeed.esp
	inline constexpr unsigned int LastSeedRunning = 0x00B162;
	inline constexpr unsigned int Seed_AttributeHunger = 0x0029CD;
	inline constexpr unsigned int Seed_AttributeThirst = 0x0029CE;
	inline constexpr unsigned int Seed_AttributeFatigue = 0x0029CF;
	inline constexpr unsigned int Seed_AttributeVitality = 0x004FC4;
	inline constexpr unsigned int Seed_AttributeHungerMax = 0x0102A9;
	inline constexpr unsigned int Seed_AttributeThirstMax = 0x0102AA;
	inline constexpr unsigned int Seed_AttributeFatigueMax = 0x0102A8;
	inline constexpr unsigned int Seed_AttributeVitalityMax = 0x0102AB;
}
