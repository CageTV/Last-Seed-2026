/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#pragma once

// Starts Last Seed on its own, the way Frostfall 2026 does: the first time the player is outside, in control and not in a
// menu (so not during the opening cart ride or its cutscenes), Last Seed is started exactly as the MCM's start option does.
namespace AutoStart
{
	void Begin();  // after data is loaded: starts the watcher
}
