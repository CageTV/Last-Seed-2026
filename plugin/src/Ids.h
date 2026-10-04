/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#pragma once

// Last Seed 2026 comes in two builds of the same plugin: LastSeed.esp as a normal plugin, and an ESL build with every FormID renumbered into the
// light range. LastSeed.dll reads forms by their local id in LastSeed.esp (the ids in McmTable.h and GameIds.h are the normal build's), so every
// lookup goes through here: for the normal build the id passes straight through, for the ESL build it is translated with the table that
// tools/esl_build.py writes (EslMap.h). The same goes for the one Frostfall form the plugin reads, when Frostfall is the ESL build.
namespace ids
{
	RE::FormID Ls(RE::FormID a_local);         // a local FormID in LastSeed.esp, whichever build is loaded
	RE::FormID Frostfall(RE::FormID a_local);  // a local FormID in Frostfall.esp, whichever build is loaded
}
