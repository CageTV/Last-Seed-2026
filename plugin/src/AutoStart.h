/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#pragma once

// Starts Last Seed on its own, the way Frostfall 2026 does: the first time the player is outside, in control and not in a
// menu (so not during the opening cart ride or its cutscenes), Last Seed is started exactly as the MCM's start option does.
namespace AutoStart
{
	void Begin();  // after data is loaded: starts the watcher
}
