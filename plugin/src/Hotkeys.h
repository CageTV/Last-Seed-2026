/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#pragma once

// Last Seed's hotkeys (check needs, eat, drink, provisions, intensity, examine food, drink from stream), handled here instead of by the
// MCM script's OnKeyDown. The seven keys are the settings in the "Other" page of McmTable.h; each one casts its spell on the player.
namespace Hotkeys
{
	void Init();          // after data is loaded: start listening
	void OnGameLoaded();  // after a save is loaded or a new game starts: stop the old MCM script from also reacting to the keys

	// Key capture for the settings page: the next key pressed is reported by TakeCaptured. Escape cancels, Delete or Backspace clear (-1).
	void BeginCapture(int a_slot);
	void CancelCapture();
	int  CapturingSlot();                          // -1 when not capturing
	bool TakeCaptured(int a_slot, int& a_keyCode); // true once, when the key for that slot has been chosen (a_keyCode -1 = cleared)

	const char* KeyName(int a_keyCode);  // "F", "Left Shift", ... or "None"
}
