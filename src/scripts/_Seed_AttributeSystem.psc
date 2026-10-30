ScriptName _Seed_AttributeSystem Extends _Seed_BaseSystem

;-- Variables ---------------------------------------

;-- Properties --------------------------------------
Float Property ATTR_LEVEL_1 = 20.0 Auto hidden
Float Property ATTR_LEVEL_2 = 40.0 Auto hidden
Float Property ATTR_LEVEL_3 = 60.0 Auto hidden
Float Property ATTR_LEVEL_4 = 80.0 Auto hidden
Float Property ATTR_LEVEL_5 = 100.0 Auto hidden
Float Property ATTR_MAX = 120.0 Auto hidden
Float Property ATTR_MIN = 0.0 Auto hidden
Actor Property PlayerRef Auto
GlobalVariable Property TimeScale Auto
GlobalVariable Property _Seed_CarriageRode Auto
GlobalVariable Property _Seed_Setting_NeedsForceFeedback Auto
GlobalVariable Property _Seed_Setting_NeedsMeterDisplayMode Auto
GlobalVariable Property _Seed_Setting_NeedsSFX Auto
GlobalVariable Property _Seed_Setting_NeedsVFX Auto
GlobalVariable Property _Seed_Setting_Notifications Auto
GlobalVariable Property _Seed_Setting_VampireBehavior Auto
GlobalVariable Property attributeEnabled Auto
ImageSpaceModifier[] Property attributeISMs Auto hidden
GlobalVariable Property attributeLevelGlobal Auto
Message[] Property attributeMessages Auto hidden
GlobalVariable Property attributeRateGlobal Auto
GlobalVariable Property attributeRateMultiGlobal Auto
Float Property attributeRegeneratingDelayed = 0.0 Auto hidden
Sound[] Property attributeSoundsF Auto hidden
Sound[] Property attributeSoundsM Auto hidden
Spell[] Property attributeSpells Auto hidden
GlobalVariable Property attributeValueGlobal Auto
Actor Property currentActor Auto hidden
String Property debugSystemName Auto hidden
Int Property delayedAttributeIncreaseIntervals = 0 Auto hidden
Int Property followerIndex = 0 Auto hidden
Float Property lastAttributeValue = 0.0 Auto hidden
Int Property lastKnownDaysJailed = 0 Auto hidden
Float Property lastUpdateTime = 0.0 Auto hidden
String Property meterForceEvent Auto hidden
String Property meterUpdateEvent Auto hidden
Actor Property oldActor Auto hidden
Bool Property undeadImmunity = False Auto
Bool Property wasSleeping = False Auto hidden

;-- Functions ---------------------------------------

; Skipped compiler generated GetState

; Skipped compiler generated GotoState

Function RegisterForEvents()
  ; Empty function
EndFunction

Function enqueueSound(Sound thisSound)
  ; Empty function
EndFunction

Actor Function getActor()
  If !currentActor ; #DEBUG_LINE_NO:58
    currentActor = PlayerRef ; #DEBUG_LINE_NO:59
  EndIf
  Return PlayerRef ; #DEBUG_LINE_NO:61
EndFunction

Bool Function hasActor()
  Return Self.getActor() != None ; #DEBUG_LINE_NO:65
EndFunction

Bool Function isPlayer()
  Return True ; #DEBUG_LINE_NO:69
EndFunction

Function StartSystem()
  oldActor = Self.getActor() ; #DEBUG_LINE_NO:74
  Parent.StartSystem() ; #DEBUG_LINE_NO:76
  If attributeEnabled.GetValueInt() != 2 ; #DEBUG_LINE_NO:79
    attributeEnabled.SetValueInt(2) ; #DEBUG_LINE_NO:80
  EndIf
  Self.RegisterForSleep() ; #DEBUG_LINE_NO:83
  Self.updateJailTime() ; #DEBUG_LINE_NO:85
  _seedinternal.SeedDebug(1, "[" + debugSystemName + "]: Started.") ; #DEBUG_LINE_NO:87
EndFunction

Function StopSystem()
  Self.RemoveAllAttributeSpells() ; #DEBUG_LINE_NO:91
  Self.RemoveAllISMs() ; #DEBUG_LINE_NO:92
  lastUpdateTime = 0.0 ; #DEBUG_LINE_NO:93
  Parent.StopSystem() ; #DEBUG_LINE_NO:94
  _seedinternal.SeedDebug(1, "[" + debugSystemName + "]: Stopped.") ; #DEBUG_LINE_NO:95
EndFunction

Function Update()
  If attributeEnabled.GetValueInt() == 2 && (Self.isPlayer() || Self.hasActor()) ; #DEBUG_LINE_NO:99
    Self.ChangeAttributeOverTime(False, False) ; #DEBUG_LINE_NO:100
  EndIf
EndFunction

Event OnSleepStart(Float afSleepStartTime, Float afDesiredSleepEndTime)
  wasSleeping = True ; #DEBUG_LINE_NO:105
EndEvent

Float Function getCarriageRideMulti()
  If _Seed_CarriageRode.getValue() as Int == 2 ; #DEBUG_LINE_NO:133
    Return 0.5 ; #DEBUG_LINE_NO:134
  EndIf
  Return 1 as Float ; #DEBUG_LINE_NO:136
EndFunction

Bool Function wasInJail()
  Int daysJailed = Game.QueryStat("Days Jailed") ; #DEBUG_LINE_NO:140
  If daysJailed > lastKnownDaysJailed ; #DEBUG_LINE_NO:141
    lastKnownDaysJailed = daysJailed ; #DEBUG_LINE_NO:142
    Return True ; #DEBUG_LINE_NO:143
  EndIf
  Return False ; #DEBUG_LINE_NO:145
EndFunction

Bool Function updateJailTime()
  lastKnownDaysJailed = Game.QueryStat("Days Jailed") ; #DEBUG_LINE_NO:149
EndFunction

Function IncreaseAttribute(float amount, float target = -1.0)
  If target == -1.0 || target > ATTR_MAX ; #DEBUG_LINE_NO:153
    target = ATTR_MAX ; #DEBUG_LINE_NO:154
  EndIf
  Float currentAttributeValue = attributeValueGlobal.getValue() ; #DEBUG_LINE_NO:157
  If currentAttributeValue >= target ; #DEBUG_LINE_NO:160
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Attribute already higher than target, not increasing further.") ; #DEBUG_LINE_NO:161
  ElseIf currentAttributeValue + amount >= target ; #DEBUG_LINE_NO:163
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Attribute + amount is higher than target, increasing to target " + target as String) ; #DEBUG_LINE_NO:164
    attributeValueGlobal.SetValue(target) ; #DEBUG_LINE_NO:165
  Else
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Increasing Attribute :" + amount as String) ; #DEBUG_LINE_NO:168
    attributeValueGlobal.SetValue(currentAttributeValue + amount) ; #DEBUG_LINE_NO:169
  EndIf
  Self.SendEvent_UpdateAttributeMeter() ; #DEBUG_LINE_NO:171
  Self.ApplyAttributeEffects() ; #DEBUG_LINE_NO:172
EndFunction

Function DecreaseAttribute(float amount, float target = -1.0)
  If target == -1.0 || target < ATTR_MIN ; #DEBUG_LINE_NO:176
    target = ATTR_MIN ; #DEBUG_LINE_NO:177
  EndIf
  Float currentAttributeValue = attributeValueGlobal.getValue() ; #DEBUG_LINE_NO:180
  If currentAttributeValue <= target ; #DEBUG_LINE_NO:183
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Attribute already lower than target, not decreasing further.") ; #DEBUG_LINE_NO:184
  ElseIf currentAttributeValue - amount <= target ; #DEBUG_LINE_NO:185
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Attribute + amount is lower than target, decreasing to target " + target as String) ; #DEBUG_LINE_NO:186
    attributeValueGlobal.SetValue(target) ; #DEBUG_LINE_NO:187
  Else
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Decreasing Attribute :" + amount as String) ; #DEBUG_LINE_NO:190
    attributeValueGlobal.SetValue(currentAttributeValue - amount) ; #DEBUG_LINE_NO:191
  EndIf
  Self.SendEvent_UpdateAttributeMeter() ; #DEBUG_LINE_NO:193
  Self.ApplyAttributeEffects() ; #DEBUG_LINE_NO:194
EndFunction

Function ModAttribute(Float amount)
  Float newAttributeValue = attributeValueGlobal.getValue() + amount ; #DEBUG_LINE_NO:198
  If newAttributeValue >= ATTR_MAX ; #DEBUG_LINE_NO:199
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Modding attribute: " + ATTR_MAX as String) ; #DEBUG_LINE_NO:200
    attributeValueGlobal.SetValue(ATTR_MAX) ; #DEBUG_LINE_NO:201
  ElseIf newAttributeValue <= ATTR_MIN ; #DEBUG_LINE_NO:202
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Modding attribute: " + ATTR_MIN as String) ; #DEBUG_LINE_NO:203
    attributeValueGlobal.SetValue(ATTR_MIN) ; #DEBUG_LINE_NO:204
  Else
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Modding attribute: " + newAttributeValue as String) ; #DEBUG_LINE_NO:206
    attributeValueGlobal.SetValue(newAttributeValue) ; #DEBUG_LINE_NO:207
  EndIf
  Self.SendEvent_UpdateAttributeMeter() ; #DEBUG_LINE_NO:209
  Self.ApplyAttributeEffects() ; #DEBUG_LINE_NO:210
EndFunction

Function SetAttribute(Float value)
  If value >= ATTR_MAX ; #DEBUG_LINE_NO:214
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Setting attribute: " + ATTR_MAX as String) ; #DEBUG_LINE_NO:215
    attributeValueGlobal.SetValue(ATTR_MAX) ; #DEBUG_LINE_NO:216
  ElseIf value <= ATTR_MIN ; #DEBUG_LINE_NO:217
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Setting attribute: " + ATTR_MIN as String) ; #DEBUG_LINE_NO:218
    attributeValueGlobal.SetValue(ATTR_MIN) ; #DEBUG_LINE_NO:219
  Else
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Setting attribute: " + value as String) ; #DEBUG_LINE_NO:221
    attributeValueGlobal.SetValue(value) ; #DEBUG_LINE_NO:222
  EndIf
  Self.SendEvent_UpdateAttributeMeter() ; #DEBUG_LINE_NO:224
  Self.ApplyAttributeEffects() ; #DEBUG_LINE_NO:225
EndFunction

Float Function GetAttributeMulti()
  Return attributeRateMultiGlobal.getValue() ; #DEBUG_LINE_NO:229
EndFunction

Function ChangeAttributeOverTimeIfFocussed(bool suspendWhileSleeping = false)
  If seedutil.isPlayerFocused() && delayedAttributeIncreaseIntervals > 1 ; #DEBUG_LINE_NO:233
    Self.ChangeAttributeOverTime(suspendWhileSleeping, True) ; #DEBUG_LINE_NO:234
  EndIf
EndFunction

Function ChangeAttributeOverTime(bool suspendWhileSleeping = false, bool forceUpdate = false)
  Float attributeIncrease
  _seedinternal.SeedDebug(1, "[" + debugSystemName + "]: ChangeAttributeOverTime()") ; #DEBUG_LINE_NO:239
  Float thisTime = Utility.GetCurrentGameTime() * 24.0 ; #DEBUG_LINE_NO:241
  If lastUpdateTime == 0.0 ; #DEBUG_LINE_NO:244
    lastUpdateTime = thisTime ; #DEBUG_LINE_NO:245
    Return  ; #DEBUG_LINE_NO:246
  EndIf
  If suspendWhileSleeping && wasSleeping ; #DEBUG_LINE_NO:249
    wasSleeping = False ; #DEBUG_LINE_NO:250
    lastUpdateTime = thisTime ; #DEBUG_LINE_NO:251
    Return  ; #DEBUG_LINE_NO:252
  EndIf
  Float thisRate = attributeRateGlobal.getValue() * Self.GetAttributeMulti() ; #DEBUG_LINE_NO:255
  Int cycles = Math.Floor((thisTime - lastUpdateTime) * 2 as Float) ; #DEBUG_LINE_NO:257
  If cycles < 1 ; #DEBUG_LINE_NO:258
    cycles = 1 ; #DEBUG_LINE_NO:259
  EndIf
  If seedutil.isPlayerFocused() && !forceUpdate ; #DEBUG_LINE_NO:262
    delayedAttributeIncreaseIntervals += cycles ; #DEBUG_LINE_NO:263
    _seedinternal.SeedDebug(1, ("[" + debugSystemName + "]: Player is focused. Delaying update. (" + delayedAttributeIncreaseIntervals as String) + " updates delayed)") ; #DEBUG_LINE_NO:264
    Return  ; #DEBUG_LINE_NO:265
  EndIf
  Int totalCycles = cycles + delayedAttributeIncreaseIntervals ; #DEBUG_LINE_NO:268
  delayedAttributeIncreaseIntervals = 0 ; #DEBUG_LINE_NO:269
  _seedinternal.SeedDebug(1, ("[" + debugSystemName + "]: Updating, " + totalCycles as String) + " cycles.") ; #DEBUG_LINE_NO:270
  If !wasSleeping ; #DEBUG_LINE_NO:273
    attributeIncrease = thisRate * totalCycles as Float ; #DEBUG_LINE_NO:274
  Else
    wasSleeping = False ; #DEBUG_LINE_NO:276
    attributeIncrease = (thisRate * totalCycles as Float) / 4 as Float ; #DEBUG_LINE_NO:277
  EndIf
  Float target = -1 as Float ; #DEBUG_LINE_NO:281
  If Self.wasInJail() ; #DEBUG_LINE_NO:282
    ; Last Seed 2026: the prison penalty (needs jump to level 4 on release) can be switched off in LastSeed.dll's settings, in which case the time served costs nothing.
    If LastSeedNative.IsInstalled() && !LastSeedNative.JailPenalty()
      _seedinternal.SeedDebug(1, "[" + debugSystemName + "]: Released from jail, prison penalty is off: no increase.")
      lastUpdateTime = thisTime
      Return
    EndIf
    target = ATTR_LEVEL_4 ; #DEBUG_LINE_NO:283
  EndIf
  _seedinternal.SeedDebug(1, "[" + debugSystemName + "]: Total attribute increase: " + attributeIncrease as String) ; #DEBUG_LINE_NO:285
  Self.IncreaseAttribute(attributeIncrease, target) ; #DEBUG_LINE_NO:286
  lastUpdateTime = thisTime ; #DEBUG_LINE_NO:287
EndFunction

Function RunDelayedCycles()
  _seedinternal.SeedDebug(0, "[" + debugSystemName + "] Running delayed cycles.") ; #DEBUG_LINE_NO:293
  Float thisRate = attributeRateGlobal.getValue() ; #DEBUG_LINE_NO:294
  Int totalCycles = delayedAttributeIncreaseIntervals ; #DEBUG_LINE_NO:295
  _seedinternal.SeedDebug(1, (("[" + debugSystemName + "]: Updating, " + totalCycles as String) + " cycles (running delayed updates). Also updating " + attributeRegeneratingDelayed as String) + " pts from regeneration") ; #DEBUG_LINE_NO:296
  Float attributeIncrease = (thisRate * totalCycles as Float) + attributeRegeneratingDelayed ; #DEBUG_LINE_NO:297
  _seedinternal.SeedDebug(1, "[" + debugSystemName + "]: Total attribute increase: " + attributeIncrease as String) ; #DEBUG_LINE_NO:298
  Self.IncreaseAttribute(attributeIncrease, -1.0) ; #DEBUG_LINE_NO:299
  attributeRegeneratingDelayed = 0 as Float ; #DEBUG_LINE_NO:300
  delayedAttributeIncreaseIntervals = 0 ; #DEBUG_LINE_NO:301
EndFunction

Function updateAttributeRegenerating(Float rate, GlobalVariable meterDisplayMode)
  Float amount = rate * Self.GetAttributeMulti() * (TimeScale.getValue() / 20 as Float) ; #DEBUG_LINE_NO:307
  If Self.getActor().IsInCombat() ; #DEBUG_LINE_NO:308
    amount *= 0.5 ; #DEBUG_LINE_NO:309
  EndIf
  If seedutil.isPlayerFocused() ; #DEBUG_LINE_NO:312
    attributeRegeneratingDelayed += amount ; #DEBUG_LINE_NO:313
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Delaying attribute update from regeneration. New delayed amount: " + attributeRegeneratingDelayed as String) ; #DEBUG_LINE_NO:314
  Else
    Self.IncreaseAttribute(amount, -1.0) ; #DEBUG_LINE_NO:317
    Self.SendEvent_ForceAttributeMeterDisplay(False) ; #DEBUG_LINE_NO:319
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Updating attribute update from regeneration: " + amount as String) ; #DEBUG_LINE_NO:321
  EndIf
EndFunction

Function ApplyAttributeEffects()
  If Self.hasActor() ; #DEBUG_LINE_NO:326
    Float currentAttributeValue = attributeValueGlobal.getValue() ; #DEBUG_LINE_NO:327
    Bool increasing = False ; #DEBUG_LINE_NO:329
    If currentAttributeValue > lastAttributeValue ; #DEBUG_LINE_NO:330
      increasing = True ; #DEBUG_LINE_NO:331
    EndIf
    If !_seedinternal.IsUpToAndBetween(lastAttributeValue, ATTR_LEVEL_1, ATTR_MIN) && _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_LEVEL_1, ATTR_MIN) ; #DEBUG_LINE_NO:334
      Self.ApplyAttributeLevel(0, increasing, False, False, 0, False, False) ; #DEBUG_LINE_NO:335
    ElseIf !_seedinternal.IsUpToAndBetween(lastAttributeValue, ATTR_LEVEL_2, ATTR_LEVEL_1) && _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_LEVEL_2, ATTR_LEVEL_1) ; #DEBUG_LINE_NO:337
      Self.ApplyAttributeLevel(1, increasing, False, False, 0, False, False) ; #DEBUG_LINE_NO:338
    ElseIf !_seedinternal.IsUpToAndBetween(lastAttributeValue, ATTR_LEVEL_3, ATTR_LEVEL_2) && _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_LEVEL_3, ATTR_LEVEL_2) ; #DEBUG_LINE_NO:340
      Self.ApplyAttributeLevel(2, increasing, False, False, 0, False, False) ; #DEBUG_LINE_NO:341
    ElseIf !_seedinternal.IsUpToAndBetween(lastAttributeValue, ATTR_LEVEL_4, ATTR_LEVEL_3) && _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_LEVEL_4, ATTR_LEVEL_3) ; #DEBUG_LINE_NO:343
      Self.ApplyAttributeLevel(3, increasing, False, False, 0, False, False) ; #DEBUG_LINE_NO:344
    ElseIf !_seedinternal.IsUpToAndBetween(lastAttributeValue, ATTR_LEVEL_5, ATTR_LEVEL_4) && _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_LEVEL_5, ATTR_LEVEL_4) ; #DEBUG_LINE_NO:346
      Self.ApplyAttributeLevel(4, increasing, True, True, 1, False, False) ; #DEBUG_LINE_NO:347
    ElseIf !_seedinternal.IsUpToAndBetween(lastAttributeValue, ATTR_MAX, ATTR_LEVEL_5) && _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_MAX, ATTR_LEVEL_5) ; #DEBUG_LINE_NO:349
      Self.ApplyAttributeLevel(5, increasing, True, True, 1, False, False) ; #DEBUG_LINE_NO:350
    EndIf
    lastAttributeValue = currentAttributeValue ; #DEBUG_LINE_NO:353
  EndIf
EndFunction

Function RemoveAllAttributeSpells()
  Int i = 0 ; #DEBUG_LINE_NO:358
  While i < attributeSpells.Length ; #DEBUG_LINE_NO:359
    If Self.hasActor() ; #DEBUG_LINE_NO:360
      Self.getActor().RemoveSpell(attributeSpells[i]) ; #DEBUG_LINE_NO:361
    EndIf
    If !Self.isPlayer() ; #DEBUG_LINE_NO:364
      If oldActor ; #DEBUG_LINE_NO:365
        oldActor.RemoveSpell(attributeSpells[i]) ; #DEBUG_LINE_NO:366
      EndIf
      oldActor = Self.getActor() ; #DEBUG_LINE_NO:368
    EndIf
    i += 1 ; #DEBUG_LINE_NO:370
  EndWhile
EndFunction

Function RemoveAllISMs()
  If Self.isPlayer() ; #DEBUG_LINE_NO:375
    Int i = 0 ; #DEBUG_LINE_NO:376
    While i < attributeISMs.Length ; #DEBUG_LINE_NO:377
      If attributeISMs[i] ; #DEBUG_LINE_NO:378
        attributeISMs[i].Remove() ; #DEBUG_LINE_NO:379
      EndIf
      i += 1 ; #DEBUG_LINE_NO:381
    EndWhile
  EndIf
EndFunction

Function ApplyAttributeLevel(int level, bool isIncreasing, bool forceMeter = false, bool flashMeter = false, int rumbleLevel = 0, bool lowerIsWorse = false, bool bypassVitalityTargetUpdate = false)
  _seedinternal.SeedDebug(0, ((("[" + debugSystemName + "] level " + level as String) + ", isIncreasing " + isIncreasing as String) + ", forceMeter " + forceMeter as String) + ", rumbleLevel " + rumbleLevel as String) ; #DEBUG_LINE_NO:391
  Self.RemoveAllAttributeSpells() ; #DEBUG_LINE_NO:393
  attributeLevelGlobal.SetValueInt(level) ; #DEBUG_LINE_NO:395
  Self.showAttributeMessage(level) ; #DEBUG_LINE_NO:396
  Spell thisSpell = attributeSpells[level] ; #DEBUG_LINE_NO:398
  Self.getActor().AddSpell(thisSpell, False) ; #DEBUG_LINE_NO:399
  If Self.isPlayer() ; #DEBUG_LINE_NO:401
    Sound thisSound
    If PlayerRef.GetActorBase().GetSex() == 1 ; #DEBUG_LINE_NO:403
      thisSound = attributeSoundsF[level] ; #DEBUG_LINE_NO:404
    Else
      thisSound = attributeSoundsM[level] ; #DEBUG_LINE_NO:406
    EndIf
    ImageSpaceModifier thisISM = attributeISMs[level] ; #DEBUG_LINE_NO:409
    If _Seed_Setting_NeedsSFX.GetValueInt() == 2 ; #DEBUG_LINE_NO:411
      Self.enqueueSound(thisSound) ; #DEBUG_LINE_NO:412
    EndIf
    If _Seed_Setting_NeedsVFX.GetValueInt() == 2 ; #DEBUG_LINE_NO:415
      If thisISM ; #DEBUG_LINE_NO:416
        _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Applying crossfade ISM.") ; #DEBUG_LINE_NO:417
        thisISM.ApplyCrossFade(4.0) ; #DEBUG_LINE_NO:418
      Else
        Self.RemoveAllISMs() ; #DEBUG_LINE_NO:420
      EndIf
    EndIf
    If forceMeter ; #DEBUG_LINE_NO:424
      _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Forcing meter display.") ; #DEBUG_LINE_NO:425
      Self.SendEvent_ForceAttributeMeterDisplay(flashMeter) ; #DEBUG_LINE_NO:426
    EndIf
    If rumbleLevel > 0 && (!lowerIsWorse && isIncreasing || lowerIsWorse && !isIncreasing) ; #DEBUG_LINE_NO:429
      If rumbleLevel == 1 ; #DEBUG_LINE_NO:430
        _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Playing rumble level 1.") ; #DEBUG_LINE_NO:431
        Game.ShakeController(0.600000024, 0.200000003, 1.0) ; #DEBUG_LINE_NO:432
      ElseIf rumbleLevel == 2 ; #DEBUG_LINE_NO:433
        _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Playing rumble level 2.") ; #DEBUG_LINE_NO:434
        Game.ShakeController(0.200000003, 0.400000006, 2.0) ; #DEBUG_LINE_NO:435
      EndIf
    EndIf
    If !bypassVitalityTargetUpdate ; #DEBUG_LINE_NO:439
      seedutil.GetVitalitySystem().UpdateVitalityTarget() ; #DEBUG_LINE_NO:440
    EndIf
  EndIf
EndFunction

Function showAttributeMessage(Int level)
  If Self.isPlayer() ; #DEBUG_LINE_NO:446
    If _Seed_Setting_Notifications.GetValueInt() == 2 ; #DEBUG_LINE_NO:447
      _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Showing message.") ; #DEBUG_LINE_NO:448
      attributeMessages[level].Show(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0) ; #DEBUG_LINE_NO:449
    EndIf
  ElseIf Self.getFollowerMessage(level) ; #DEBUG_LINE_NO:451
    String name = "One of Your followers" ; #DEBUG_LINE_NO:452
    If camputil.GetSKSELoaded() ; #DEBUG_LINE_NO:453
      name = Self.getActor().GetBaseObject().GetName() ; #DEBUG_LINE_NO:454
    ElseIf followerIndex == 3 ; #DEBUG_LINE_NO:456
      name = seedutil.GetTranslationHandler().FollowerFirst ; #DEBUG_LINE_NO:457
    ElseIf followerIndex == 2 ; #DEBUG_LINE_NO:458
      name = seedutil.GetTranslationHandler().FollowerSecond ; #DEBUG_LINE_NO:459
    ElseIf followerIndex == 1 ; #DEBUG_LINE_NO:460
      name = seedutil.GetTranslationHandler().FollowerThird ; #DEBUG_LINE_NO:461
    EndIf
    Debug.Notification(name + Self.getFollowerMessage(level)) ; #DEBUG_LINE_NO:464
  EndIf
EndFunction

String Function getFollowerMessage(Int i)
  Return "" ; #DEBUG_LINE_NO:469
EndFunction

Function DisplayCurrentStatus()
  _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Displaying Current Status") ; #DEBUG_LINE_NO:473
  Float currentAttributeValue = attributeValueGlobal.getValue() ; #DEBUG_LINE_NO:474
  If _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_LEVEL_1, ATTR_MIN) ; #DEBUG_LINE_NO:476
    Self.showAttributeMessage(0) ; #DEBUG_LINE_NO:477
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Level 0") ; #DEBUG_LINE_NO:478
  ElseIf _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_LEVEL_2, ATTR_LEVEL_1) ; #DEBUG_LINE_NO:479
    Self.showAttributeMessage(1) ; #DEBUG_LINE_NO:480
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Level 1") ; #DEBUG_LINE_NO:481
  ElseIf _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_LEVEL_3, ATTR_LEVEL_2) ; #DEBUG_LINE_NO:482
    Self.showAttributeMessage(2) ; #DEBUG_LINE_NO:483
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Level 2") ; #DEBUG_LINE_NO:484
  ElseIf _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_LEVEL_4, ATTR_LEVEL_3) ; #DEBUG_LINE_NO:485
    Self.showAttributeMessage(3) ; #DEBUG_LINE_NO:486
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Level 3") ; #DEBUG_LINE_NO:487
  ElseIf _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_LEVEL_5, ATTR_LEVEL_4) ; #DEBUG_LINE_NO:488
    Self.showAttributeMessage(4) ; #DEBUG_LINE_NO:489
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Level 4") ; #DEBUG_LINE_NO:490
  ElseIf _seedinternal.IsUpToAndBetween(currentAttributeValue, ATTR_MAX, ATTR_LEVEL_5) ; #DEBUG_LINE_NO:491
    Self.showAttributeMessage(5) ; #DEBUG_LINE_NO:492
    _seedinternal.SeedDebug(0, "[" + debugSystemName + "]: Level 5") ; #DEBUG_LINE_NO:493
  EndIf
  Self.SendEvent_ForceAttributeMeterDisplay(False) ; #DEBUG_LINE_NO:496
EndFunction

Function SendEvent_UpdateAttributeMeter()
  If camputil.GetSKSELoaded() && Self.isPlayer() && _Seed_Setting_NeedsMeterDisplayMode.GetValueInt() != 0 ; #DEBUG_LINE_NO:505
    Int handle = modevent.Create(meterUpdateEvent) ; #DEBUG_LINE_NO:506
    If handle ; #DEBUG_LINE_NO:507
      modevent.Send(handle) ; #DEBUG_LINE_NO:508
    EndIf
  EndIf
EndFunction

Function SendEvent_ForceAttributeMeterDisplay(bool flash = false)
  If camputil.GetSKSELoaded() && Self.isPlayer() && _Seed_Setting_NeedsMeterDisplayMode.GetValueInt() != 0 ; #DEBUG_LINE_NO:515
    Int handle = modevent.Create(meterForceEvent) ; #DEBUG_LINE_NO:516
    If handle ; #DEBUG_LINE_NO:517
      modevent.PushBool(handle, flash) ; #DEBUG_LINE_NO:518
      modevent.Send(handle) ; #DEBUG_LINE_NO:519
    EndIf
  EndIf
EndFunction

Function RefreshSystem()
  If attributeEnabled.getValue() == 2 as Float ; #DEBUG_LINE_NO:525
    _seedinternal.SeedDebug(1, "[" + debugSystemName + "]: Refreshing.") ; #DEBUG_LINE_NO:526
    Parent.RefreshSystem() ; #DEBUG_LINE_NO:527
  EndIf
EndFunction
