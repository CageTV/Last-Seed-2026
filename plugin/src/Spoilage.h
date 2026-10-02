#pragma once

// Native food spoilage. Replaces Last Seed's Papyrus spoilage (one world "tracker" object per food stack, each waking up every
// game hour) with plain records kept by LastSeed.dll: which container holds how many of which food, and how long it has been
// rotting. It uses Last Seed's own food categories, rot-time settings and spoiled-food items, so nothing in the plugin changes.
//
// Tracked containers match Last Seed's: the player, the Provisions container and the player's followers and animals.
namespace Spoilage
{
	// One row of the settings page: a stack of food of one kind in one container, all aged the same.
	struct Row
	{
		std::string food;
		std::string container;
		int         count = 0;
		float       rottedFraction = 0.0f;  // 0 = fresh, 1 = about to spoil
		float       hoursLeft = 0.0f;       // at the current rot speed
	};

	struct Stats
	{
		bool             active = false;   // native tracking is on and Last Seed is running
		int              batches = 0;
		int              items = 0;
		int              worldContainers = 0;  // world containers whose food has been rolled and is waiting for its reset
		float            speed = 1.0f;     // rot speed multiplier right now (Last Seed's _Seed_SpoilTemperatureMulti)
		std::vector<Row> rows;             // freshest-to-oldest sorted by how close they are to spoiling, capped
	};

	void  Init();         // after data is loaded
	void  Begin();        // starts the update thread
	void  Register();     // save-game (SKSE co-save) callbacks, call from plugin load
	bool  Active();       // native spoilage is switched on
	bool  WorldActive();  // world-container spoilage is on too (Last Seed's own one-shot container script steps aside)
	Stats Snapshot(std::size_t a_maxRows);
}
