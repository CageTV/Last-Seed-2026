/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#pragma once

// A thin freshness bar drawn over the item card of the inventory, container and barter lists while the cursor is on a food that
// spoils. The bar shrinks as the food rots (its length is how fresh the stack still is) and goes from green to amber to red.
// Drawn with SKSE Menu Framework's draw list from Hud::Render; the hovered item is read on the game thread.
#include "SKSEMenuFramework.h"

namespace Freshness
{
	// Render thread, once per frame, inside SKSE Menu Framework's HUD callback. a_dl is the foreground draw list.
	void Draw(ImGuiMCP::ImDrawList* a_dl, float a_displayW, float a_displayH);
}
