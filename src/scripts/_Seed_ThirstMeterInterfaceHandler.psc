scriptname _Seed_ThirstMeterInterfaceHandler extends CommonMeterInterfaceHandler

import CampUtil

function RegisterForEvents()
	if !GetSKSELoaded()
		return
	endif

	RegisterForModEvent("LastSeed_ForceThirstMeterDisplay", "ForceMeterDisplay")
	RegisterForModEvent("LastSeed_RemoveThirstMeter", "RemoveMeter")
	RegisterForModEvent("LastSeed_UpdateThirstMeter", "UpdateMeterDelegate")
	RegisterForModEvent("LastSeed_CheckMeterRequirements", "CheckMeterRequirements")
endFunction

; Last Seed 2026: while LastSeed.dll's HUD bars are on, this SkyUI meter stays hidden whatever the meter settings say.
; @overrides CommonMeterInterfaceHandler
function UpdateMeter(bool abForceDisplayIfEnabled = false)
	if LastSeedNative.OldMetersHidden()
		RemoveMeter()
		return
	endif
	parent.UpdateMeter(abForceDisplayIfEnabled)
endFunction

; @overrides CommonMeterInterfaceHandler
function ForceMeterDisplay(bool flash = false)
	if LastSeedNative.OldMetersHidden()
		RemoveMeter()
		return
	endif
	parent.ForceMeterDisplay(flash)
endFunction
