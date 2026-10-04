/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#pragma once

// Last Seed registers with Grid Inventory (when it is installed) as an extension provider and puts a colored ring on a food
// tile's socket while the cursor is over it: green when fresh, through amber, to red as it nears spoiling.
namespace GridInv
{
	// Listen for Grid Inventory announcing itself; call once from plugin load.
	void Register();
}
