/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#pragma once

// Last Seed's "Food & Drink Lists" editor, drawn through SKSE Menu Framework: browse the lists of foods and drinks Last Seed keeps, see which
// lists a food is on, and change its category, how much hunger it restores, whether it is preserved or salted, and what kind of alcohol it is.
// The edits are made by the same SeedUtil functions the SkyUI menu called, so Last Seed's own data stays the source of truth.
namespace NativeFoodLists
{
	void Draw();
}
