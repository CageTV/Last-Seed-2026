/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#pragma once

// Hunterborn 2026 offers an ESL edition of Hunterborn.esp whose records were renumbered into the light range. Last Seed's food lists name Hunterborn's foods by their
// record number; Fix turns a number of the regular Hunterborn.esp into the one the loaded plugin really uses (unchanged for the regular plugin and for every other plugin).
namespace HunterbornIds
{
	unsigned Fix(const char* a_plugin, unsigned a_local);
}
