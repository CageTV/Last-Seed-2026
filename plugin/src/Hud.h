/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#pragma once

// Last Seed's HUD, drawn through SKSE Menu Framework's HUD API: four flat vertical bars (hunger, thirst, fatigue,
// vitality) in the same style as Frostfall's, stacked directly below Frostfall's group when Frostfall.dll's HUD is on.
namespace Hud
{
	void __stdcall Render();

	// Fade the Last Seed logo in and out (when Last Seed finishes starting).
	void ShowStartupLogo();

	// The settings page calls this every frame it is visible, so the bars stay drawn (as a live preview) while
	// SKSE Menu Framework's window is open.
	void MarkPreviewFrame();

	// Mark settings as changed; they are written to the INI shortly after the last change.
	void MarkSettingsDirty();
}
