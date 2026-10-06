/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
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

	// ---- spoilage (read with houseCARL on 2026-10-02) ----
	inline constexpr unsigned int SpoilageEnable = 0x204B66;           // _Seed_Setting_SpoilageEnable (2 = on)
	inline constexpr unsigned int SpoilageRemove = 0x2E8991;           // _Seed_Setting_SpoilageRemove (2 = no "perished" leftover, 3 = remove outright)
	inline constexpr unsigned int SpoilTemperatureMulti = 0x3B8333;    // _Seed_SpoilTemperatureMulti (rot speed, kept up to date by Last Seed's scripts)
	inline constexpr unsigned int ProvisionsContainer = 0x074B07;      // _Seed_ProvisionsContainerRef
	inline constexpr unsigned int PreservedList = 0x00C71C;            // _Seed_Preserved: never spoils
	inline constexpr unsigned int SpoiledFoodsList = 0x009BD3;         // _Seed_SpoiledFoods: already-spoiled foods, which later become _Seed_PerishedFood
	inline constexpr unsigned int ContainerSpoilageEnable = 0x19A5E6;  // _Seed_Setting_ContainerSpoilageEnable (2 = on)
	inline constexpr unsigned int ContainerSpoilageRate = 0x19A5E7;    // _Seed_Setting_ContainerSpoilageRate: % chance each item has spoiled
	inline constexpr unsigned int PerishedFood = 0x007B4B;             // _Seed_PerishedFood (MISC)
	inline constexpr unsigned int IceWraithTeeth = 0x44B08B;           // _Seed_IceWraithTeeth (ALCH) -> _Seed_IceWraithTeethOld
	inline constexpr unsigned int IceWraithTeethOld = 0x44B08E;
	inline constexpr unsigned int IceWraithTeethRate = 0x450191;       // _Seed_Setting_SpoilRate_IceWraithTeeth

	struct FoodCategory
	{
		const char*  name;
		unsigned int list;     // FormList of foods in the category
		unsigned int spoiled;  // what they turn into (ALCH)
		unsigned int rate;     // _Seed_Setting_SpoilRateNN_*: hours until it spoils
	};
	// Same order as Last Seed's IdentifyFood (the first matching list wins).
	inline constexpr FoodCategory FoodCategories[] = {
		{ "Bread", 0x009666, 0x009643, 0x20ED75 },
		{ "Raw meat", 0x009657, 0x009639, 0x20ED76 },
		{ "Cooked meat", 0x009658, 0x00963B, 0x20ED77 },
		{ "Raw small game", 0x009659, 0x00963D, 0x20ED78 },
		{ "Cooked small game", 0x00965A, 0x00963F, 0x20ED79 },
		{ "Raw fish", 0x00965B, 0x009645, 0x20ED7A },
		{ "Cooked fish", 0x00965C, 0x009647, 0x20ED7B },
		{ "Raw seafood", 0x00965D, 0x009649, 0x20ED7C },
		{ "Cooked seafood", 0x00965E, 0x00964B, 0x20ED7D },
		{ "Vegetables", 0x00965F, 0x00964D, 0x20ED7E },
		{ "Fruit", 0x009660, 0x009641, 0x20ED7F },
		{ "Cheese", 0x009663, 0x009661, 0x20ED80 },
		{ "Treats", 0x009664, 0x00964F, 0x20ED81 },
		{ "Pastries", 0x009665, 0x009651, 0x20ED82 },
		{ "Stews", 0x009667, 0x009653, 0x20ED83 },
		{ "Cheese bowls", 0x00C718, 0x009655, 0x20ED84 },
		{ "Milk", 0x00C71A, 0x009BD1, 0x20ED85 },
	};
}
