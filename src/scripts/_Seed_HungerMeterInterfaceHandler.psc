scriptname _Seed_HungerMeterInterfaceHandler extends CommonMeterInterfaceHandler

import CampUtil

function RegisterForEvents()
	if !GetSKSELoaded()
		return
	endif

	RegisterForModEvent("LastSeed_ForceHungerMeterDisplay", "ForceMeterDisplay")
	RegisterForModEvent("LastSeed_RemoveHungerMeter", "RemoveMeter")
	RegisterForModEvent("LastSeed_UpdateHungerMeter", "UpdateMeterDelegate")
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
