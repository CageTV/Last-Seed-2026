ScriptName _Seed_ConvertWineBottles Extends ObjectReference
{Last Seed 2026: LastSeed.dll can switch off the "clean this bottle for water?" question (SKSE Menu Framework page, Overview).}

;-- Properties --------------------------------------
MiscObject Property bottle Auto

;-- Functions ---------------------------------------

Event OnContainerChanged(ObjectReference akNewContainer, ObjectReference akOldContainer)
	if LastSeedNative.IsInstalled() && !LastSeedNative.AskWineBottle()
		return
	endif
	seedutil.getActivatorHandler().convertWineBottle(akNewContainer, bottle as Form)
EndEvent
