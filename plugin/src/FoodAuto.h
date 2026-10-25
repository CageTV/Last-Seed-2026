/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#pragma once

#include "Compat.h"

// Foods Last Seed does not know, classified when the game's data has loaded instead of by asking the player.
// Last Seed asks ("It appears that you are unfamiliar with this foodstuff...") about every food that is on none of its lists and has none of the
// food-kind keywords it reads (VendorItemFood*, injected into Update.esm by Last Seed and CACO). This gives every such food those keywords, worked
// out from its name (the classifier of Gourmet - AutoPatch), so Last Seed files it on its own the first time it sees it, the same way it files a
// food with keywords from CACO or a patch. Items that only look like food (CACO's hunger-status tokens, test items: no weight, no value, no food
// words) go on Last Seed's Not Food list. A food the classifier cannot read is left to Last Seed's question, as before.
namespace FoodAuto
{
	Compat::Entry Apply();  // after data is loaded
	void OnGameLoaded();    // after a save is loaded or a new game starts: the Not Food list comes back from the save, so the tokens are put on it again
}
