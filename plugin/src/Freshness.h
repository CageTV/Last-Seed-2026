/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
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

	// While the settings page is open: a sample bar where the real one would be, and (once unlocked) mouse dragging to place it.
	void DrawPreview(ImGuiMCP::ImDrawList* a_dl, float a_displayW, float a_displayH, bool a_preview);

	// True while the settings page lets the mouse move the bar (it locks itself again when the page closes).
	bool& Unlocked();
}
