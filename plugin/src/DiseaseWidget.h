/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#pragma once

#include "Compat.h"

// Compatibility with "Disease Widget - SKSE" (DiseaseWidget.dll). The widget shows an icon for every disease on the player, with the icon chosen by
// the definitions in Data/SKSE/Plugins/DiseaseWidget/Diseases.json (spell names and forms). Last Seed's diseases come in stages ("Mild Ataxia" to
// "Deadly Ataxia") that the shipped definitions do not know, so they only get the generic icon. When DiseaseWidget.dll is loaded, Last Seed adds
// definitions for all of its diseases and stages to that file (once; the entries are the ones whose id starts with "ls_").
// Nothing happens when the widget is not installed.
namespace DiseaseWidget
{
	// Call at kPostLoad: the widget reads its definitions at kDataLoaded, so they are in place for the same launch.
	void MergeDefinitions();

	// Call when a game has loaded: a little later, logs which Disease-type effects the player has, so a missing icon can be traced.
	void LogActiveDiseases();

	Compat::Entry Status();  // for the settings page
}
