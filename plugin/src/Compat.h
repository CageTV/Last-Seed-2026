/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#pragma once

// Compatibility that used to need patch plugins, done when the game's data has loaded and only when the other mod is there:
//   * fishing bait: Perished Food and the spoiled meats and fish go into the bait list of any fishing mod (found by what is on the list)
//   * Growl - Werebeasts of Skyrim: the werewolf feeding spell gets Growl's effects
//   * Travellers of Skyrim: the Apothecary's cure disease ability cures with Last Seed's disease cure
// Nothing here adds a master to LastSeed.esp.
namespace Compat
{
	struct Entry
	{
		std::string name;     // what it is
		bool        present;  // the other mod is in the load order
		bool        applied;  // and the change was made
		std::string detail;
	};

	void Apply();                       // call after data is loaded
	std::vector<Entry> Status();        // for the settings page
}
