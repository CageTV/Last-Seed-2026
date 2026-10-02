scriptname _Seed_VitalityMeterInterfaceHandler extends CommonMeterInterfaceHandler

import CampUtil

function RegisterForEvents()
	if !GetSKSELoaded()
		return
	endif

	RegisterForModEvent("LastSeed_ForceVitalityMeterDisplay", "ForceMeterDisplay")
	RegisterForModEvent("LastSeed_RemoveVitalityMeter", "RemoveMeter")
	RegisterForModEvent("LastSeed_UpdateVitalityMeter", "UpdateMeterDelegate")
	RegisterForModEvent("LastSeed_CheckMeterRequirements", "CheckMeterRequirements")

	; Special Last Seed indicator element
	RegisterForModEvent("LastSeed_UpdateVitalityMeterIndicator", "UpdateMeterIndicator")
endFunction

Event UpdateMeterIndicator(float percent)
	(Meter as _Seed_Meter).SetIndicatorPercent(percent, false)
endEvent

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
