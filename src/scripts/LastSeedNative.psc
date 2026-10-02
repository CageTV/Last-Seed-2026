scriptname LastSeedNative hidden
{Last Seed 2026: bridge to LastSeed.dll, the SKSE plugin behind the HUD bars, the start-up logo and the SKSE Menu Framework settings
page. Every native call is guarded by IsInstalled(), so Last Seed runs exactly as before (SkyUI meters, MCM) when the plugin is not installed.}

bool function IsInstalled() global
	return SKSE.GetPluginVersion("LastSeed") > 0
endFunction

; Implemented by LastSeed.dll
bool function HudBarsActive() global native
bool function AutoStartEnabled() global native
bool function NativeSpoilage() global native

; True when LastSeed.dll starts Last Seed on its own: the start-up objective (journal prompt) and its "COMPLETED" banner are skipped.
bool function QuietStart() global
	return IsInstalled() && AutoStartEnabled()
endFunction

; True while LastSeed.dll's HUD bars replace Last Seed's SkyUI meters (the meters then stay hidden).
bool function OldMetersHidden() global
	return IsInstalled() && HudBarsActive()
endFunction

; Called by LastSeed.dll when its HUD bars are switched on or off: hide the SkyUI meters, or bring them back.
function RefreshOldMeters() global
	if OldMetersHidden()
		SendMeterEvent("LastSeed_RemoveHungerMeter")
		SendMeterEvent("LastSeed_RemoveThirstMeter")
		SendMeterEvent("LastSeed_RemoveFatigueMeter")
		SendMeterEvent("LastSeed_RemoveVitalityMeter")
	else
		SendMeterEvent("LastSeed_ForceHungerMeterDisplay", true)
		SendMeterEvent("LastSeed_ForceThirstMeterDisplay", true)
		SendMeterEvent("LastSeed_ForceFatigueMeterDisplay", true)
		SendMeterEvent("LastSeed_ForceVitalityMeterDisplay", true)
	endif
endFunction

function SendMeterEvent(string asEvent, bool abWithFlag = false) global
	int handle = ModEvent.Create(asEvent)
	if handle
		if abWithFlag
			ModEvent.PushBool(handle, false)
		endif
		ModEvent.Send(handle)
	endif
endFunction
