Scriptname QF__Seed_TrackingQuest_0400A69D Extends Quest Hidden
{Last Seed 2026: quest fragments of _Seed_TrackingQuest (the "Rest and plan for the journey ahead" start-up objective).
With LastSeed.dll's auto-start on, Last Seed starts by itself, so the objective is never shown and no "COMPLETED: LAST SEED"
banner appears (the start-up logo replaces it); the quest is stopped quietly, as Frostfall 2026 does. Without the plugin, or with
auto-start off, everything behaves as before.}

; Stage 20: Last Seed has started
Function Fragment_0()
	bool quiet = LastSeedNative.QuietStart()
	debug.trace("[LastSeed 2026] tracking quest stage 20, quiet start = " + quiet)
	if quiet
		SetObjectiveDisplayed(10, false)
		Stop()
	else
		SetObjectiveCompleted(10, true)
	endif
EndFunction

; Stage 10: start-up objective, SkyUI installed
Function Fragment_2()
	if !LastSeedNative.QuietStart()
		SetObjectiveDisplayed(10, true, false)
	endif
EndFunction

; Stage 15: start-up objective, no SkyUI
Function Fragment_4()
	if !LastSeedNative.QuietStart()
		SetObjectiveDisplayed(10, true, false)
	endif
EndFunction
