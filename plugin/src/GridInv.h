/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#pragma once

// Last Seed registers with Grid Inventory (when it is installed) as an extension provider and puts a colored ring on a food
// tile's socket while the cursor is over it: green when fresh, through amber, to red as it nears spoiling.
namespace GridInv
{
	// Listen for Grid Inventory announcing itself; call once from plugin load.
	void Register();
}
