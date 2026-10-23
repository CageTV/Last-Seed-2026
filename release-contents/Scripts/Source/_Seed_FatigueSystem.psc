scriptname _Seed_FatigueSystem extends _Seed_AttributeSystem
import seedUtil
import campUtil
import _SeedInternal
import utility


;/
REFERENCED IN: 
_Seed_FatigueSystemQuest "Fatigue Quest" [QUST:070029D5] \ Scripts
/;

GlobalVariable property _Seed_Setting_SleepAffectedByNeeds auto
GlobalVariable property _Seed_Setting_SleepAffectedByLocation auto
GlobalVariable property _Seed_IsAddicted auto
GlobalVariable property _Seed_Setting_NeedsAffectedByRegeneration auto
GlobalVariable property _Seed_Setting_Notifications_Followers auto
GlobalVariable property Provisioning_PerkRank_Outdoorsman auto
GlobalVariable property _Seed_DiseaseNeedsMulti_BrownRot auto

Spell property FatigueSpell1 auto
Spell property FatigueSpell2 auto
Spell property FatigueSpell3 auto
Spell property FatigueSpell4 auto
Spell property FatigueSpell5 auto
Spell property FatigueSpell6 auto

Spell property _Seed_FatigueSpell_AdoptionFemale auto
Spell property _Seed_FatigueSpell_AdoptionMale auto
Spell property _Seed_FatigueSpell_Rested auto
Spell property _Seed_FatigueSpell_WellRested auto
Spell property _Seed_FatigueSpell_Marriage auto
ReferenceAlias Property LoveInterest Auto
LocationAlias Property CurrentHomeLocation Auto
Keyword Property LocTypeInn Auto
Keyword Property LocTypePlayerHouse Auto
Quest Property RelationshipMarriageFIN Auto
Quest Property BYOHRelationshipAdoption Auto
Spell Property pDoomLoverAbility Auto
CompanionsHousekeepingScript Property CHScript Auto


Message property FatigueMessage1 auto
Message property FatigueMessage2 auto
Message property FatigueMessage3 auto
Message property FatigueMessage4 auto
Message property FatigueMessage5 auto
Message property FatigueMessage6 auto

Message property _Seed_Fatigue_WellRestedMessage auto
Message property _Seed_Fatigue_MarriageRestedMessage auto
Message property _Seed_Fatigue_BeastBloodMessage auto
Message property _Seed_Fatigue_BYOHAdoptionRestedMessageMale auto
Message property _Seed_Fatigue_BYOHAdoptionRestedMessageFemale auto
Message property _Seed_FatiguePoorSleepHungerThirstMsg auto
Message property _Seed_FatiguePoorSleepHungerMsg auto
Message property _Seed_FatiguePoorSleepThirstMsg auto
Message property _Seed_FatiguePoorSleepLocationMsg auto

Sound property FatigueSound3_M auto
Sound property FatigueSound4_M auto
Sound property FatigueSound5_M auto
Sound property FatigueSound6_M auto
Sound property FatigueSound3_F auto
Sound property FatigueSound4_F auto
Sound property FatigueSound5_F auto
Sound property FatigueSound6_F auto

ImageSpaceModifier property FatigueISM5 auto
ImageSpaceModifier property FatigueISM6 auto

Quest property _Seed_FatigueMeterQuest auto

float property lastSleepDuration = 0.0 auto hidden

;TODO: Make these global variables.
float SLEEP_RESTORE_RATE = 12.0
float REGEN_FATIGUE_RATE = 0.083
float lastMagicka = 0.0

float sleepStartTime = 0.0

GlobalVariable property _Seed_Setting_HungerMeterDisplayMode auto

; TUTORIAL
globalVariable property _Seed_Setting_DisplayTutorials auto
globalVariable property _Seed_HelpDone_PoorSleep auto
Message property _Seed_Help_PoorSleep auto

function StartUp()
    debugSystemName = "Fatigue"
    meterUpdateEvent = "LastSeed_UpdateFatigueMeter"
    meterForceEvent = "LastSeed_ForceFatigueMeterDisplay"

    ; Initialize arrays
    attributeSpells = new Spell[6]
    attributeMessages = new Message[6]
    attributeSoundsM = new Sound[6]
    attributeSoundsF = new Sound[6]
    attributeISMs = new ImageSpaceModifier[6]

    ; Populate arrays
    attributeSpells[0] = FatigueSpell1
    attributeSpells[1] = FatigueSpell2
    attributeSpells[2] = FatigueSpell3
    attributeSpells[3] = FatigueSpell4
    attributeSpells[4] = FatigueSpell5
    attributeSpells[5] = FatigueSpell6

    attributeMessages[0] = FatigueMessage1
    attributeMessages[1] = FatigueMessage2
    attributeMessages[2] = FatigueMessage3
    attributeMessages[3] = FatigueMessage4
    attributeMessages[4] = FatigueMessage5
    attributeMessages[5] = FatigueMessage6

    attributeSoundsM[2] = FatigueSound3_M
    attributeSoundsM[3] = FatigueSound4_M
    attributeSoundsM[4] = FatigueSound5_M
    attributeSoundsM[5] = FatigueSound6_M
	
	attributeSoundsF[2] = FatigueSound3_F
    attributeSoundsF[3] = FatigueSound4_F
    attributeSoundsF[4] = FatigueSound5_F
    attributeSoundsF[5] = FatigueSound6_F

    RegisterForEvents()

    ; Apply initial condition.
    IncreaseAttribute(0.01)
endFunction


function RegisterForEvents()
    if !self.IsRunning()
        return
    endif
endFunction

function ChangeAttributeOverTime(bool suspendWhileSleeping = false, bool forceUpdate = false)
    parent.ChangeAttributeOverTime(true, forceUpdate)
endFunction

; Impact the player's fatigue if the player is regenerating magicka.
Event OnUpdate()
    if self.IsRunning() && _Seed_Setting_NeedsAffectedByRegeneration.getValueInt() == 2
		bool regenerating = false
		float thisMagicka = getActor().GetActorValue("Magicka")
		if thisMagicka > lastMagicka
			regenerating = true
		endif
	
		if regenerating
			updateAttributeRegenerating(REGEN_FATIGUE_RATE, _Seed_Setting_HungerMeterDisplayMode)

			RegisterForSingleUpdate(2)
		else
			RegisterForSingleUpdate(5)
		endif
		lastMagicka = thisMagicka
	endif
EndEvent


Event OnSleepStart(float afSleepStartTime, float afDesiredSleepEndTime)
	; Apply Attribute Changes before Continuing
	If isPlayerFocused() && delayedAttributeIncreaseIntervals > 1
		wasSleeping = false
		ChangeAttributeOverTime(true, true)
	endif
	wasSleeping = true
	
	sleepStartTime = afSleepStartTime
	;lastSleepDuration = (afDesiredSleepEndTime - afSleepStartTime) * 24.0
EndEvent

Event OnSleepStop(bool abInterrupted)
    ; if not interrupted...
	;float fatigue_decrease = lastSleepDuration * SLEEP_RESTORE_RATE
	float sleepTime = (GetCurrentGameTime() - sleepStartTime) * 24.0
	float fatigue_decrease = sleepTime * SLEEP_RESTORE_RATE
	
	bool isHungry = false
	bool isThirsty = false
	bool isBadLocation = false
	float target = 0
	float hungerMod = 0
	float thirstMod = 0
	float locationMod = 0
	
	;Get hunger modifier
	if(_Seed_Setting_SleepAffectedByNeeds.getValue() == 2 && GetPlayerHungerLevel() >= 2)
		hungerMod = 1
		isHungry = true
	endif
	
	;Get thirst modifier
	if(_Seed_Setting_SleepAffectedByNeeds.getValue() == 2 && GetPlayerThirstLevel() >= 2)
		thirstMod = 1
		isThirsty = true
	endif
	
	; ;Get location modifier
	if _Seed_Setting_SleepAffectedByLocation.getValue() == 2 && (GetDiseaseSystem().getLocationHazardLevel() > 3 || (!IsRefInInterior(PlayerRef) && !PlayerIsCamping() && Provisioning_PerkRank_Outdoorsman.getValue() == 0))
		locationMod = 1
		isBadLocation = true
	endif
	
	; Set target and decrease attribute
	target = (hungerMod + thirstMod + locationMod) * 25
	SeedDebug(1, "[" + debugSystemName + "]: Sleep Modifier " + target)
	SeedDebug(1, "[" + debugSystemName + "]: Is Hungry " + isHungry)
	SeedDebug(1, "[" + debugSystemName + "]: Is Thirsty " + isThirsty)
	SeedDebug(1, "[" + debugSystemName + "]: Is Bad Location " + isBadLocation)
	DecreaseAttributeSleep(fatigue_decrease, target, isHungry, isThirsty, isBadLocation)
EndEvent

; Show message if sleep quality effected
; Last Seed 2026: a Campfire tent that shelters the player, or a bedroll by the campfire, is a safe place to sleep.
; Without this, sleeping outdoors in a tent counted as a dangerous location and the sleep could not fully restore energy.
bool function PlayerIsCamping()
	ObjectReference tent = GetCurrentTent()
	if !tent
		return false
	endif
	if TentHasShelter(tent)
		return true
	endif
	ObjectReference fire = GetLastUsedCampfire()
	if fire && !fire.IsDisabled() && fire.Is3DLoaded() && PlayerRef.GetDistance(fire) < 1200.0
		return true
	endif
	return false
endFunction

function showPoorSleepMessage(float target, bool isHungry = false, bool isThirsty = false, bool isBadLocation = false)
	if target != 0
		if(isHungry && isThirsty)
			_Seed_FatiguePoorSleepHungerThirstMsg.Show()
			ShowTutorial_PoorSleep()
		elseif(isHungry)
			_Seed_FatiguePoorSleepHungerMsg.Show()
			ShowTutorial_PoorSleep()
		elseIf(isThirsty)
			_Seed_FatiguePoorSleepThirstMsg.Show()
			ShowTutorial_PoorSleep()
		elseIf(isBadLocation)
			_Seed_FatiguePoorSleepLocationMsg.Show()
			ShowTutorial_PoorSleep()
		endIf
	endif
endFunction





; Overrides AttributeSystem method
function DecreaseAttributeSleep(float amount, float target, bool isHungry = false, bool isThirsty = false, bool isBadLocation = false)
    float currentAttributeValue = attributeValueGlobal.GetValue()
    ; If attribute is currently below target, do nothing
	if currentAttributeValue <= target
		return
	; If decreasing below target, only decrease amount to match target
	elseif currentAttributeValue - amount <= target
        attributeValueGlobal.SetValue(target)
		;TODO: Finish Follower Fatigue
		;/if isPlayer()
			showPoorSleepMessage(target, isHungry, isThirsty, isBadLocation)
		endif/;
		showPoorSleepMessage(target, isHungry, isThirsty, isBadLocation)
    ; Decrease amount normally
	else
        attributeValueGlobal.SetValue(currentAttributeValue - amount)
    endif
    SendEvent_UpdateAttributeMeter()
    ApplyAttributeEffects()
	
	;ADD RESTED, WELL RESTED, LOVER'S COMFORT AND PARENT'S TOUCH
	;TODO: Finish follower fatigue
	;if ;/isPlayer() &&/; attributeValueGlobal.GetValue() <= ATTR_LEVEL_1
	if attributeValueGlobal.GetValue() <= ATTR_LEVEL_1
		If CHScript.PlayerHasBeastBlood == 1
			RemoveRested()
			_Seed_Fatigue_BeastBloodMessage.Show()
		ElseIf PlayerRef.HasSpell(pDoomLoverAbility) == 0
			RemoveRested()
			;don't run this if player has the Lover sign
			If RelationshipMarriageFIN.IsRunning() == True && RelationshipMarriageFIN.GetStage() >= 10 && PlayerRef.GetCurrentLocation() == LoveInterest.GetActorRef().GetCurrentLocation()
				_Seed_Fatigue_MarriageRestedMessage.Show()
				PlayerRef.AddSpell(_Seed_FatigueSpell_Marriage, abVerbose = false)
			ElseIf PlayerRef.GetCurrentLocation().HasKeyword(LocTypeInn) == True || PlayerRef.GetCurrentLocation().HasKeyword(LocTypePlayerHouse) == True
				_Seed_Fatigue_WellRestedMessage.Show()
				PlayerRef.AddSpell(_Seed_FatigueSpell_WellRested, abVerbose = false)
			Else
				PlayerRef.AddSpell(_Seed_FatigueSpell_Rested, abVerbose = false)
			EndIf
		EndIf
		if (CHScript.PlayerHasBeastBlood != 1)
			;Additionally, for Adoption...
			If (BYOHRelationshipAdoption.IsRunning() && PlayerRef.GetCurrentLocation() == CurrentHomeLocation.GetLocation())
				RemoveAdoptionRested()
				If (PlayerRef.GetActorBase().GetSex() == 0)
					;Player is a father.
					_Seed_Fatigue_BYOHAdoptionRestedMessageMale.Show()
					PlayerRef.AddSpell(_Seed_FatigueSpell_AdoptionMale, False)
				Else
					;Player is a mother.
					_Seed_Fatigue_BYOHAdoptionRestedMessageFemale.Show()
					PlayerRef.AddSpell(_Seed_FatigueSpell_AdoptionFemale, False)
				EndIf
			EndIf
		EndIf		
	endIf	
endFunction

;remove all previous rested states
Function RemoveRested()
	PlayerRef.RemoveSpell(_Seed_FatigueSpell_Rested)
	PlayerRef.RemoveSpell(_Seed_FatigueSpell_WellRested)
	PlayerRef.RemoveSpell(_Seed_FatigueSpell_AdoptionMale)

EndFunction

;remove all previous adoption rested states
Function RemoveAdoptionRested()
	PlayerRef.RemoveSpell(_Seed_FatigueSpell_AdoptionMale)
	PlayerRef.RemoveSpell(_Seed_FatigueSpell_AdoptionFemale)
EndFunction

;Overrides method in _Seed_BaseSystem
function StopSystem()
	RemoveRested()
	RemoveAdoptionRested()
	parent.StopSystem()
endFunction


;@Override Function
function enqueueSound(sound thisSound)
	GetConsumeManager().EnqueuePlayerFatigueSound(thisSound)
endFunction

;@Override Function
float function GetAttributeMulti()
	float result = 1
	
	;Brown Rot Multiplier
	result = result + _Seed_DiseaseNeedsMulti_BrownRot.getValue()
	
	;Drunk
	int alcoholLevel = GetPlayerAlcoholLevel() as int
	if AlcoholLevel == 2 ; Tipsy
		result = result + 0.1
		; SeedDebug(1, "[" + debugSystemName + "]: Player is tipsy")
	ElseIf AlcoholLevel == 3 ; Drunk
		result = result + 0.25
		; SeedDebug(1, "[" + debugSystemName + "]: Player is drunk")
	ElseIf AlcoholLevel == 4 ; Very Drunk
		result = result + 0.5
		; SeedDebug(1, "[" + debugSystemName + "]: Player is very drunk")
	Endif
	;Skooma Addiction
	if _Seed_IsAddicted.getValue() == 2
		result = result + 0.5
	endif
		
	; Player-set multiplier
	result = result * attributeRateMultiGlobal.getValue()
	result = result * getCarriageRideMulti()
	
	SeedDebug(1, "[" + debugSystemName + "]: Attribute Multiplier: " + result)
	return result
EndFunction

function ShowTutorial_PoorSleep()
    if _Seed_Setting_DisplayTutorials.GetValueInt() == 2 && _Seed_HelpDone_PoorSleep.GetValueInt() == 1
        _Seed_Help_PoorSleep.Show()
        _Seed_HelpDone_PoorSleep.SetValue(2)
    endif
endFunction

; @override
function ApplyAttributeLevel(int level, bool isIncreasing, bool forceMeter = false, bool flashMeter = false, int rumbleLevel = 0, bool lowerIsWorse = false, bool bypassVitalityTargetUpdate = false)
	; Apply Level
	parent.ApplyAttributeLevel(level, isIncreasing, forceMeter, flashMeter, rumbleLevel, lowerIsWorse, bypassVitalityTargetUpdate)
	; Increase Experience
	if level == 0
		GetSkillTreeHandler().progressExperience()
	endif
endFunction

;TODO: Finish Follower Fatigue
;/
function showAttributeMessage(int level)
	if isPlayer()
		if _Seed_Setting_Notifications.GetValueInt() == 2
			SeedDebug(0, "[" + debugSystemName + "]: Showing message.")
			attributeMessages[level].Show()
		endif
	elseif getFollowerMessage(level)
		if _Seed_Setting_Notifications_Followers.getValueInt() == 2
			string name = "One of Your followers"
			if GetSKSELoaded()
				name = getActor().GetBaseObject().GetName()
			else
				if followerIndex == 3
					name = "Your third follower"
				elseif followerIndex == 2
					name = "Your second follower"
				elseif followerIndex == 1	
					name = "Your first follower"
				endif
			endif
			Debug.Notification(name + getFollowerMessage(level))
		endIf	
	endIf
endFunction

; @override
String Function getFollowerMessage(int i)
	return GetTranslationHandler().GetFollowerFatigueMessage(i)
endFunction
/;

