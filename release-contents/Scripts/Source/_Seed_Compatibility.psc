ScriptName _Seed_Compatibility Extends ReferenceAlias

;-- Variables ---------------------------------------
String CONFIG_PATH = "../LastSeedData/"
Bool datastore_update_required = False

;-- Properties --------------------------------------
Int Property CAMPFIRE_MIN_VERSION = 11100 AutoReadOnly
LeveledItem Property LItemApothecaryPotionCureHMS75 Auto
LeveledItem Property LItemPotionCureHMS Auto
GlobalVariable Property LastSeedRunning Auto
GlobalVariable Property LastSeedRunning_KWCheck Auto
GlobalVariable Property LastSeedStartupFinished Auto
ReferenceAlias Property PlayerAlias Auto
Actor Property PlayerRef Auto
Int Property SKSE_MIN_VERSION = 10703 AutoReadOnly
GlobalVariable Property _Camp_IsSpecialEdition Auto
Perk Property _Seed_Activators Auto
Spell Property _Seed_AutoDrink Auto
Spell Property _Seed_AutoEat Auto
Message Property _Seed_CACOWarning Auto
GlobalVariable Property _Seed_CACO_Loaded Auto
GlobalVariable Property _Seed_CarriageRode Auto
Spell Property _Seed_CheckNeedsSpell Auto
Spell Property _Seed_ConfigMenu Auto
Message Property _Seed_CriticalError_Campfire Auto
Message Property _Seed_CriticalError_SKSE Auto
Spell Property _Seed_DrinkFromStreamSpell Auto
Spell Property _Seed_ExamineFood Auto
GlobalVariable Property _Seed_FocusOnMenu Auto
GlobalVariable Property _Seed_FoodListsSet Auto
Message Property _Seed_HunterbornSoupsWarning Auto
Spell Property _Seed_Intensity Auto
Spell Property _Seed_IntensityPlayerSpell Auto
GlobalVariable Property _Seed_LastSeedVersion Auto
GlobalVariable Property _Seed_LoadingChecksum Auto
FormList Property _Seed_OblivionAreas Auto
FormList Property _Seed_OblivionCells Auto
FormList Property _Seed_OblivionLocations Auto
Spell Property _Seed_OpenProvisionsSpell Auto
Activator Property _Seed_PerkNodeController_Provisioning Auto
FormList Property _Seed_PortionsToProvisionsList Auto
FormList Property _Seed_Potion_CureList Auto
GlobalVariable Property _Seed_PreviousVersion Auto
GlobalVariable Property _Seed_ProgressiveDiseasesEnabled Auto
GlobalVariable Property _Seed_ProvisionsAddPortions Auto
ObjectReference Property _Seed_ProvisionsContainerRef Auto
GlobalVariable Property _Seed_SettingVampireMonitoring Auto
GlobalVariable Property _Seed_Setting_AlternateDeathSystem Auto
GlobalVariable Property _Seed_Setting_DiseasePotionsCure Auto
GlobalVariable Property _Seed_Setting_FollowersConsumeFood Auto
GlobalVariable Property _Seed_Setting_HungerMeterDisplayMode Auto
GlobalVariable Property _Seed_Setting_ManualMeterConfig Auto
GlobalVariable Property _Seed_Setting_ProgressiveDiseases Auto
GlobalVariable Property _Seed_Setting_RefreshSystemsOnLoad Auto
GlobalVariable Property _Seed_Setting_SpoilRate_IceWraithTeeth Auto
GlobalVariable Property _Seed_Setting_SpoilageEnable Auto
GlobalVariable Property _Seed_Setting_SpoilageTemperature Auto
GlobalVariable Property _Seed_Setting_SystemEnabled_FollowerNeeds Auto
GlobalVariable Property _Seed_Setting_VitalityMeterDisplayMode Auto
GlobalVariable Property _Seed_Setting_isBetterVampiresLoaded Auto
Potion Property _Seed_TestFood Auto
Quest Property _seed_MainQuest Auto
Bool Property isApothecaryLoaded Auto hidden
Bool Property isBathingInSkyrimLoaded Auto hidden
Bool Property isBetterVampiresLoaded Auto hidden
Bool Property isBrumaLoaded Auto hidden
Bool Property isCACOLoaded Auto hidden
Bool Property isCCFishingLoaded Auto hidden
Bool Property isCCTheCauseLoaded Auto hidden
Bool Property isCRFLoaded Auto hidden
Bool Property isCookingExpandedLoaded Auto hidden
Bool Property isDirtAndBloodLoaded Auto hidden
Bool Property isDiseasedLoaded Auto hidden
Bool Property isFalskaarLoaded Auto hidden
Int Property isFirstSessionLoad = -1 Auto hidden
Bool Property isFoodExpandedLoaded Auto hidden
Bool Property isFoodExpandedLoadedFr Auto hidden
Bool Property isFrostfallLoaded Auto hidden
Bool Property isFrozenNorthLoaded Auto hidden
Bool Property isHunterbornLoaded Auto hidden
Bool Property isHunterbornSoupsLoaded Auto hidden
Bool Property isImmersiveEncountersLoaded Auto hidden
Bool Property isInterestingNPCsLoaded Auto hidden
Bool Property isKeepItCleanLoaded Auto hidden
Bool Property isMealtimeLoaded Auto hidden
Bool Property isNordicCookingLoaded Auto hidden
Bool Property isPO3Loaded Auto hidden
Bool Property isProjectAhoLoaded Auto hidden
Bool Property isRequiemLoaded Auto hidden
Bool Property isSAFOLoaded Auto hidden
Bool Property isSKSELoaded Auto hidden
Bool Property isSKYUILoaded Auto hidden
Bool Property isShadowOfSkyrimLoaded Auto hidden
Bool Property isSkyrimVR Auto hidden
Bool Property isUSSEPLoaded Auto hidden
Bool Property isWarmDrinksLoaded Auto hidden
Bool Property isWintersunLoaded Auto hidden
Bool Property isWyrmstoothLoaded Auto hidden

;-- Functions ---------------------------------------

; Skipped compiler generated GetState

; Skipped compiler generated GotoState

Function RegisterForControlsOnLoad()
  ; Empty function
EndFunction

Function VanillaGameLoadUp()
  ; Empty function
EndFunction

Event OnPlayerLoadGame()
  _Seed_LoadingChecksum.setValue(Utility.RandomFloat(0.0, 999.0)) ; #DEBUG_LINE_NO:147
  If LastSeedRunning.getValue() as Int == 2 ; #DEBUG_LINE_NO:148
    isFirstSessionLoad = Self.isfirstSessionLoad_Int() ; #DEBUG_LINE_NO:149
    If isFirstSessionLoad != 1 ; #DEBUG_LINE_NO:150
      If _Seed_Setting_RefreshSystemsOnLoad.getValue() == 2 as Float ; #DEBUG_LINE_NO:151
        Self.refreshSystems() ; #DEBUG_LINE_NO:152
      EndIf
      If LastSeedStartupFinished.getValue() != 2 as Float ; #DEBUG_LINE_NO:155
        LastSeedStartupFinished.setValue(2 as Float) ; #DEBUG_LINE_NO:156
      EndIf
      Self.RunUpdate() ; #DEBUG_LINE_NO:159
      Self.RunCompatibility() ; #DEBUG_LINE_NO:160
      Self.checkMods(False) ; #DEBUG_LINE_NO:161
      _Seed_CarriageRode.setValue(1 as Float) ; #DEBUG_LINE_NO:162
    EndIf
    Self.RegisterForEventsOnLoad() ; #DEBUG_LINE_NO:166
    Self.SendEvent_LastSeedLoaded() ; #DEBUG_LINE_NO:169
  EndIf
EndEvent

Function RunUpdate()
  Float currentVersion = _Seed_LastSeedVersion.getValue() ; #DEBUG_LINE_NO:174
  Float latestVersion = 4.599999905 ; #DEBUG_LINE_NO:175
  If currentVersion == 100.0 ; #DEBUG_LINE_NO:177
    _Seed_LastSeedVersion.setValue(1.0) ; #DEBUG_LINE_NO:178
  EndIf
  If currentVersion <= 1.0 ; #DEBUG_LINE_NO:182
    seedutil.GetSpoilageSystem().StopSystem() ; #DEBUG_LINE_NO:183
    Utility.wait(1 as Float) ; #DEBUG_LINE_NO:184
    seedutil.GetSpoilageSystem().StartSystem() ; #DEBUG_LINE_NO:185
  EndIf
  If currentVersion < 1.299999952 ; #DEBUG_LINE_NO:189
    If _Seed_Setting_HungerMeterDisplayMode.getValueInt() == 5 ; #DEBUG_LINE_NO:190
      _Seed_Setting_HungerMeterDisplayMode.setValueInt(0) ; #DEBUG_LINE_NO:191
    EndIf
    If _Seed_Setting_SpoilageEnable.getValueInt() != 2 ; #DEBUG_LINE_NO:193
      seedutil.GetSpoilageSystem().StopSystem() ; #DEBUG_LINE_NO:194
    EndIf
  EndIf
  If currentVersion < 2.0 ; #DEBUG_LINE_NO:199
    _Seed_Setting_SpoilageTemperature.setValue(1 as Float) ; #DEBUG_LINE_NO:201
    If _Seed_Setting_SpoilageEnable.getValueInt() == 2 ; #DEBUG_LINE_NO:204
      seedutil.GetSpoilageSystem().OnUpdateGameTime() ; #DEBUG_LINE_NO:205
    EndIf
  EndIf
  If currentVersion < 2.400000095 ; #DEBUG_LINE_NO:228
    _Seed_FoodListsSet.setValue(2 as Float) ; #DEBUG_LINE_NO:229
  EndIf
  If currentVersion < 3.0 ; #DEBUG_LINE_NO:233
    seedutil.getSkillTreeHandler().StartSystem() ; #DEBUG_LINE_NO:235
    PlayerRef.removeSpell(_Seed_Intensity) ; #DEBUG_LINE_NO:236
    seedutil.getFoodDatastoreHandler().addCampfireWaterskinsAll() ; #DEBUG_LINE_NO:239
    seedutil.getFoodDatastoreHandler().addCACOInjectedRecords() ; #DEBUG_LINE_NO:240
    Self.setCACOGlobals() ; #DEBUG_LINE_NO:243
    seedutil.getHungerSystem().updateJailTime() ; #DEBUG_LINE_NO:246
    seedutil.getThirstSystem().updateJailTime() ; #DEBUG_LINE_NO:247
    seedutil.getFatigueSystem().updateJailTime() ; #DEBUG_LINE_NO:248
    seedutil.getVitalitySystem().updateJailTime() ; #DEBUG_LINE_NO:249
    seedutil.getPartyHungerSystem().updateJailTime() ; #DEBUG_LINE_NO:250
    seedutil.getPartyThirstSystem().updateJailTime() ; #DEBUG_LINE_NO:251
    seedutil.GetFollowerSystem().getHungerSystem(1).updateJailTime() ; #DEBUG_LINE_NO:252
    seedutil.GetFollowerSystem().getHungerSystem(2).updateJailTime() ; #DEBUG_LINE_NO:253
    seedutil.GetFollowerSystem().getHungerSystem(3).updateJailTime() ; #DEBUG_LINE_NO:254
    seedutil.GetFollowerSystem().getThirstSystem(1).updateJailTime() ; #DEBUG_LINE_NO:255
    seedutil.GetFollowerSystem().getThirstSystem(2).updateJailTime() ; #DEBUG_LINE_NO:256
    seedutil.GetFollowerSystem().getThirstSystem(3).updateJailTime() ; #DEBUG_LINE_NO:257
    seedutil.GetSpoilageSystem().setAllInventoryEventFilters() ; #DEBUG_LINE_NO:260
    If _Seed_Setting_SystemEnabled_FollowerNeeds.getValue() as Int == 2 ; #DEBUG_LINE_NO:263
      seedutil.GetFollowerSystem().StopSystem() ; #DEBUG_LINE_NO:264
      seedutil.GetFollowerSystem().StartPartySystem() ; #DEBUG_LINE_NO:265
    ElseIf _Seed_Setting_SystemEnabled_FollowerNeeds.getValue() as Int == 3 ; #DEBUG_LINE_NO:266
      _Seed_Setting_SystemEnabled_FollowerNeeds.setValue(2 as Float) ; #DEBUG_LINE_NO:267
      seedutil.getPartyHungerSystem().RemoveAllAttributeSpells() ; #DEBUG_LINE_NO:268
      seedutil.getPartyThirstSystem().RemoveAllAttributeSpells() ; #DEBUG_LINE_NO:269
    EndIf
    LastSeedRunning_KWCheck.setValue(2 as Float) ; #DEBUG_LINE_NO:274
  EndIf
  If currentVersion < 3.200000048 ; #DEBUG_LINE_NO:278
    _Seed_Setting_SpoilRate_IceWraithTeeth.setValue(120.0) ; #DEBUG_LINE_NO:279
  EndIf
  If currentVersion < 3.400000095 ; #DEBUG_LINE_NO:283
    If isBetterVampiresLoaded ; #DEBUG_LINE_NO:284
      _Seed_SettingVampireMonitoring.setValue(2 as Float) ; #DEBUG_LINE_NO:285
    EndIf
  EndIf
  If currentVersion < 3.5 ; #DEBUG_LINE_NO:290
    seedutil.getFoodDatastoreHandler().resetFormlistBloodPotions() ; #DEBUG_LINE_NO:291
    If isBetterVampiresLoaded ; #DEBUG_LINE_NO:292
      seedutil.getFoodDatastoreHandler().addBetterVampiresBloodPotions(False) ; #DEBUG_LINE_NO:293
    EndIf
    If isCACOLoaded ; #DEBUG_LINE_NO:295
      seedutil.getFoodDatastoreHandler().addCACOBloodPotions() ; #DEBUG_LINE_NO:296
    EndIf
    isCCFishingLoaded = False ; #DEBUG_LINE_NO:299
    isRequiemLoaded = False ; #DEBUG_LINE_NO:300
  EndIf
  If currentVersion < 3.700000048 ; #DEBUG_LINE_NO:304
    seedutil.getFoodDatastoreHandler().addRiverWater() ; #DEBUG_LINE_NO:305
    PlayerRef.AddSpell(_Seed_DrinkFromStreamSpell, False) ; #DEBUG_LINE_NO:306
  EndIf
  If currentVersion < 4.0 ; #DEBUG_LINE_NO:310
    LastSeedStartupFinished.setValue(2 as Float) ; #DEBUG_LINE_NO:312
    seedutil.GetVendorStockSystem().StartSystem() ; #DEBUG_LINE_NO:315
    seedutil.GetRescueSystem().StartSystem() ; #DEBUG_LINE_NO:316
    seedutil.getVitalitySystem().update_4_0() ; #DEBUG_LINE_NO:318
    seedutil.getFoodDatastoreHandler().update_4_0() ; #DEBUG_LINE_NO:319
    If isCACOLoaded ; #DEBUG_LINE_NO:320
      seedutil.getFoodDatastoreHandler().initialiseWaterBottles_CACO() ; #DEBUG_LINE_NO:321
      seedutil.getFoodDatastoreHandler().update_4_0_CACO() ; #DEBUG_LINE_NO:322
    EndIf
    If isBrumaLoaded ; #DEBUG_LINE_NO:324
      seedutil.getFoodDatastoreHandler().initialiseWaterBottles_Bruma() ; #DEBUG_LINE_NO:325
    EndIf
    If isApothecaryLoaded ; #DEBUG_LINE_NO:327
      seedutil.getFoodDatastoreHandler().initialiseWaterBottles_Apothecary() ; #DEBUG_LINE_NO:328
    EndIf
    If isRequiemLoaded ; #DEBUG_LINE_NO:330
      seedutil.getFoodDatastoreHandler().initialiseWaterBottles_Requiem() ; #DEBUG_LINE_NO:331
    EndIf
    If isCRFLoaded ; #DEBUG_LINE_NO:333
      seedutil.getFoodDatastoreHandler().initialiseWaterBottles_CRF() ; #DEBUG_LINE_NO:334
    EndIf
    If isCCFishingLoaded ; #DEBUG_LINE_NO:336
      seedutil.getFoodDatastoreHandler().initialiseWaterBottles_CCFishing() ; #DEBUG_LINE_NO:337
    EndIf
    If isHunterbornLoaded ; #DEBUG_LINE_NO:339
      seedutil.getFoodDatastoreHandler().update_4_0_Hunterborn() ; #DEBUG_LINE_NO:340
      seedutil.getFoodDatastoreHandler().initialiseWaterBottles_Hunterborn() ; #DEBUG_LINE_NO:341
    EndIf
  EndIf
  If currentVersion < 4.000000954 ; #DEBUG_LINE_NO:346
    seedutil.GetRescueSystem().update_4_0() ; #DEBUG_LINE_NO:347
    seedutil.getDiseaseHitMonitor().start() ; #DEBUG_LINE_NO:348
    If _Seed_Setting_ProgressiveDiseases.getValue() == 2 as Float ; #DEBUG_LINE_NO:349
      _Seed_ProgressiveDiseasesEnabled.setValue(2 as Float) ; #DEBUG_LINE_NO:350
    EndIf
    seedutil.getFoodDatastoreHandler().update_4_0_micro() ; #DEBUG_LINE_NO:352
    If isCACOLoaded ; #DEBUG_LINE_NO:353
      seedutil.getFoodDatastoreHandler().initialiseMultiPartFood_CACO() ; #DEBUG_LINE_NO:354
    EndIf
  EndIf
  If currentVersion < 4.199999809 ; #DEBUG_LINE_NO:358
    If isCookingExpandedLoaded ; #DEBUG_LINE_NO:359
      seedutil.getFoodDatastoreHandler().AddCookingExpanded(False) ; #DEBUG_LINE_NO:360
    EndIf
  EndIf
  If currentVersion < 4.300000191 ; #DEBUG_LINE_NO:364
    seedutil.getFoodDatastoreHandler().update_4_3() ; #DEBUG_LINE_NO:365
    seedutil.GetFollowerSystem().suspendNeeds(1, None) ; #DEBUG_LINE_NO:366
    seedutil.GetFollowerSystem().suspendNeeds(2, None) ; #DEBUG_LINE_NO:367
    seedutil.GetFollowerSystem().suspendNeeds(3, None) ; #DEBUG_LINE_NO:368
    PlayerRef.AddPerk(_Seed_Activators) ; #DEBUG_LINE_NO:370
  EndIf
  If currentVersion < 4.400000095 ; #DEBUG_LINE_NO:373
    seedutil.getPartyThirstSystem().update_4_4() ; #DEBUG_LINE_NO:374
  EndIf
  If currentVersion < 4.599999905 ; #DEBUG_LINE_NO:377
    Self.StartInnMeals(False) ; #DEBUG_LINE_NO:378
    seedutil.getFoodDatastoreHandler().update_4_6() ; #DEBUG_LINE_NO:379
  EndIf
  If currentVersion < latestVersion ; #DEBUG_LINE_NO:383
    _Seed_LastSeedVersion.setValue(latestVersion) ; #DEBUG_LINE_NO:384
  EndIf
EndFunction

Function ErrorSKSE(Int version)
  Debug.trace((("[LastSeed][Warning] Detected SKSE version " + (version as Float / 10000 as Float) as String) + ". Expected " + (Self.SKSE_MIN_VERSION as Float / 10000 as Float) as String) + " or newer. Using Fallback Mode.", 0) ; #DEBUG_LINE_NO:389
  _Seed_CriticalError_SKSE.Show(version as Float / 10000 as Float, Self.SKSE_MIN_VERSION as Float / 10000 as Float, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0) ; #DEBUG_LINE_NO:390
EndFunction

Function FatalErrorCampfire(Float version)
  Float version_formatted = version / 10000 as Float ; #DEBUG_LINE_NO:394
  Float min_version_formatted = Self.CAMPFIRE_MIN_VERSION as Float / 10000 as Float ; #DEBUG_LINE_NO:395
  Debug.trace((("[LastSeed][ERROR] Detected Campfire version " + version_formatted as String) + ", out of date! Expected " + min_version_formatted as String) + " or newer.", 0) ; #DEBUG_LINE_NO:396
  While true
    _Seed_CriticalError_Campfire.Show(version_formatted, min_version_formatted, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0) ; #DEBUG_LINE_NO:398
    Utility.wait(3.0) ; #DEBUG_LINE_NO:399
  EndWhile
EndFunction

Function RunCompatibility()
  Debug.trace("[LastSeed]======================================================================================================", 0) ; #DEBUG_LINE_NO:404
  Debug.trace("[LastSeed]                    Last Seed is now performing start-up and compatibility checks.                    ", 0) ; #DEBUG_LINE_NO:405
  Debug.trace("[LastSeed]     Papyrus warnings related to missing files may follow. These are NORMAL and should be ignored.    ", 0) ; #DEBUG_LINE_NO:406
  Debug.trace("[LastSeed]======================================================================================================", 0) ; #DEBUG_LINE_NO:407
  If seedutil.isSpecialEdition() == False ; #DEBUG_LINE_NO:410
    Bool skse_loaded = skse.GetVersion() as Bool ; #DEBUG_LINE_NO:411
    If skse_loaded ; #DEBUG_LINE_NO:412
      Int skse_version = skse.GetVersion() * 10000 + skse.GetVersionMinor() * 100 + skse.GetVersionBeta() ; #DEBUG_LINE_NO:413
      If skse_version < Self.SKSE_MIN_VERSION ; #DEBUG_LINE_NO:414
        isSKSELoaded = False ; #DEBUG_LINE_NO:415
        Self.ErrorSKSE(skse_version) ; #DEBUG_LINE_NO:416
      Else
        isSKSELoaded = True ; #DEBUG_LINE_NO:418
        Debug.trace((("[LastSeed] Detected SKSE version " + (skse_version as Float / 10000 as Float) as String) + " (expected " + (Self.SKSE_MIN_VERSION as Float / 10000 as Float) as String) + " or newer, success!)", 0) ; #DEBUG_LINE_NO:419
      EndIf
    Else
      isSKSELoaded = False ; #DEBUG_LINE_NO:422
      Self.ErrorSKSE(0) ; #DEBUG_LINE_NO:423
    EndIf
  EndIf
  Self.VanillaGameLoadUp() ; #DEBUG_LINE_NO:427
  Self.CheckDatastore() ; #DEBUG_LINE_NO:430
  Float campfire_version = camputil.GetCampfireVersion() ; #DEBUG_LINE_NO:432
  If campfire_version < Self.CAMPFIRE_MIN_VERSION as Float ; #DEBUG_LINE_NO:433
    Self.FatalErrorCampfire(campfire_version) ; #DEBUG_LINE_NO:434
  Else
    Debug.trace((("[LastSeed] Detected Campfire version " + campfire_version as String) + " (expected " + Self.CAMPFIRE_MIN_VERSION as String) + " or newer, success!)", 0) ; #DEBUG_LINE_NO:436
  EndIf
  seedutil.GetTranslationHandler().stop() ; #DEBUG_LINE_NO:441
  Utility.wait(0.100000001) ; #DEBUG_LINE_NO:442
  seedutil.GetTranslationHandler().start() ; #DEBUG_LINE_NO:443
  If seedutil.isSpecialEdition() ; #DEBUG_LINE_NO:446
    isSKYUILoaded = Self.IsPluginLoaded(16779284, "SkyUI_SE.esp") ; #DEBUG_LINE_NO:447
  Else
    isSKYUILoaded = Self.IsPluginLoaded(16779284, "SkyUI.esp") ; #DEBUG_LINE_NO:449
  EndIf
  Debug.trace("[LastSeed]======================================================================================================", 0) ; #DEBUG_LINE_NO:452
  Debug.trace("[LastSeed]                      Last Seed start-up and compatibility checks complete.   \t\t                ", 0) ; #DEBUG_LINE_NO:453
  Debug.trace("[LastSeed]======================================================================================================", 0) ; #DEBUG_LINE_NO:454
  Self.RegisterForEventsOnLoad() ; #DEBUG_LINE_NO:457
  Self.RegisterForMenusOnLoad() ; #DEBUG_LINE_NO:458
  If isSKYUILoaded ; #DEBUG_LINE_NO:460
    PlayerRef.removeSpell(_Seed_ConfigMenu) ; #DEBUG_LINE_NO:461
  Else
    PlayerRef.AddSpell(_Seed_ConfigMenu, False) ; #DEBUG_LINE_NO:463
  EndIf
  If _Seed_Setting_ManualMeterConfig.getValueInt() == 2 ; #DEBUG_LINE_NO:468
    Self.SendEvent_SKSE_ApplyMeterPreset(2) ; #DEBUG_LINE_NO:469
  EndIf
  If isSKYUILoaded ; #DEBUG_LINE_NO:472
    Self.SendEvent_SKSE_LoadProfileOnStartup() ; #DEBUG_LINE_NO:473
  EndIf
EndFunction

Function checkMods(bool forceSetFoodProperties = false)
  Bool lastSkyrimVRCheck = isSkyrimVR ; #DEBUG_LINE_NO:479
  isSkyrimVR = Self.IsPluginLoaded(3031, "SkyrimVR.esm") ; #DEBUG_LINE_NO:480
  If isSkyrimVR && lastSkyrimVRCheck == False ; #DEBUG_LINE_NO:481
    _Seed_SettingVampireMonitoring.setValue(2 as Float) ; #DEBUG_LINE_NO:482
    seedutil.getMonsterHandler().startMonitoring() ; #DEBUG_LINE_NO:483
  EndIf
  Bool lastFrostfallCheck = isFrostfallLoaded ; #DEBUG_LINE_NO:487
  isFrostfallLoaded = Self.IsPluginLoaded(412408, "Frostfall.esp") ; #DEBUG_LINE_NO:488
  If isFrostfallLoaded && lastFrostfallCheck == False ; #DEBUG_LINE_NO:489
    seedutil.getFoodDatastoreHandler().AddFrostfall(False) ; #DEBUG_LINE_NO:490
  EndIf
  isBathingInSkyrimLoaded = Self.IsPluginLoaded(162285, "Bathing in Skyrim - Main.esp") ; #DEBUG_LINE_NO:494
  isKeepItCleanLoaded = Self.IsPluginLoaded(1588732, "Keep It Clean.esp") ; #DEBUG_LINE_NO:497
  isDirtAndBloodLoaded = Self.IsPluginLoaded(2084, "Dirt and Blood - Dynamic Visuals.esp") ; #DEBUG_LINE_NO:500
  isDiseasedLoaded = Self.IsPluginLoaded(10378, "Diseased.esp") ; #DEBUG_LINE_NO:503
  Bool lastCACOCheck = isCACOLoaded ; #DEBUG_LINE_NO:509
  Bool thisCACOCheck = Self.IsPluginLoaded(666175, "Complete Alchemy & Cooking Overhaul.esp") ; #DEBUG_LINE_NO:510
  If thisCACOCheck ; #DEBUG_LINE_NO:511
    _Seed_CACO_Loaded.setValue(2 as Float) ; #DEBUG_LINE_NO:512
  Else
    _Seed_CACO_Loaded.setValue(1 as Float) ; #DEBUG_LINE_NO:514
  EndIf
  ; Last Seed 2026: CACO's food is imported without asking, like every other food mod Last Seed knows (its progress shows as notifications).
  If thisCACOCheck == True && lastCACOCheck == False ; #DEBUG_LINE_NO:516
    isCACOLoaded = True ; #DEBUG_LINE_NO:518
    seedutil.getFoodDatastoreHandler().addCACO(False) ; #DEBUG_LINE_NO:519
    Self.setCACOGlobals() ; #DEBUG_LINE_NO:520
  EndIf
  Bool lastBrumaCheck = isBrumaLoaded ; #DEBUG_LINE_NO:525
  isBrumaLoaded = Self.IsPluginLoaded(6497, "BSHeartland.esm") ; #DEBUG_LINE_NO:526
  If isBrumaLoaded && lastBrumaCheck == False ; #DEBUG_LINE_NO:527
    seedutil.getFoodDatastoreHandler().AddBruma(False) ; #DEBUG_LINE_NO:528
  EndIf
  Bool lastRequiemCheck = isRequiemLoaded ; #DEBUG_LINE_NO:532
  isRequiemLoaded = Self.IsPluginLoaded(3094686, "Requiem.esp") ; #DEBUG_LINE_NO:533
  If isRequiemLoaded && lastRequiemCheck == False ; #DEBUG_LINE_NO:534
    seedutil.getFoodDatastoreHandler().AddRequiem(False) ; #DEBUG_LINE_NO:535
  EndIf
  Bool lastHunterbornCheck = isHunterbornLoaded ; #DEBUG_LINE_NO:539
  isHunterbornLoaded = Self.IsPluginLoaded(13159, "Hunterborn.esp") ; #DEBUG_LINE_NO:540
  If isHunterbornLoaded && lastHunterbornCheck == False ; #DEBUG_LINE_NO:541
    seedutil.getFoodDatastoreHandler().AddHunterborn(False) ; #DEBUG_LINE_NO:542
  EndIf
  Bool lastApothecaryCheck = isApothecaryLoaded ; #DEBUG_LINE_NO:546
  isApothecaryLoaded = Self.IsPluginLoaded(479981, "ApothecaryFood.esp") ; #DEBUG_LINE_NO:547
  If isApothecaryLoaded && lastApothecaryCheck == False ; #DEBUG_LINE_NO:548
    seedutil.getFoodDatastoreHandler().addApothecary(False) ; #DEBUG_LINE_NO:549
  EndIf
  Bool lastCRFCheck = isCRFLoaded ; #DEBUG_LINE_NO:553
  isCRFLoaded = Self.IsPluginLoaded(223485, "Cutting Room Floor.esp") ; #DEBUG_LINE_NO:554
  If isCRFLoaded && lastCRFCheck == False ; #DEBUG_LINE_NO:555
    seedutil.getFoodDatastoreHandler().addCRF(False) ; #DEBUG_LINE_NO:556
  EndIf
  Bool LastCCFishingCheck = isCCFishingLoaded ; #DEBUG_LINE_NO:560
  isCCFishingLoaded = Self.IsPluginLoaded(2239, "ccBGSSSE001-Fish.esm") ; #DEBUG_LINE_NO:561
  If isCCFishingLoaded && LastCCFishingCheck == False ; #DEBUG_LINE_NO:562
    seedutil.getFoodDatastoreHandler().addCCFishing(False) ; #DEBUG_LINE_NO:563
  EndIf
  Bool LastBetterVampiresCheck = isBetterVampiresLoaded ; #DEBUG_LINE_NO:567
  isBetterVampiresLoaded = Self.IsPluginLoaded(255955, "Better Vampires.esp") ; #DEBUG_LINE_NO:568
  If isBetterVampiresLoaded && !LastBetterVampiresCheck ; #DEBUG_LINE_NO:569
    _Seed_Setting_isBetterVampiresLoaded.setValue(2 as Float) ; #DEBUG_LINE_NO:570
    _Seed_SettingVampireMonitoring.setValue(2 as Float) ; #DEBUG_LINE_NO:571
    seedutil.getFoodDatastoreHandler().addBetterVampiresBloodPotions(False) ; #DEBUG_LINE_NO:572
  ElseIf !isBetterVampiresLoaded && LastBetterVampiresCheck ; #DEBUG_LINE_NO:573
    _Seed_Setting_isBetterVampiresLoaded.setValue(1 as Float) ; #DEBUG_LINE_NO:574
  EndIf
  String USSEP_ESP = "Unofficial Skyrim Special Edition Patch.esp" ; #DEBUG_LINE_NO:578
  If !seedutil.isSpecialEdition() ; #DEBUG_LINE_NO:579
    USSEP_ESP = "Unofficial Skyrim Legendary Edition Patch.esp" ; #DEBUG_LINE_NO:580
  EndIf
  Bool LastUSSEPCheck = isUSSEPLoaded ; #DEBUG_LINE_NO:582
  isUSSEPLoaded = Self.IsPluginLoaded(6289, USSEP_ESP) ; #DEBUG_LINE_NO:583
  If isUSSEPLoaded && LastUSSEPCheck == False ; #DEBUG_LINE_NO:584
    seedutil.getFoodDatastoreHandler().addUSSEP(False) ; #DEBUG_LINE_NO:585
  EndIf
  Bool lastWintersunCheck = isWintersunLoaded ; #DEBUG_LINE_NO:589
  isWintersunLoaded = Self.IsPluginLoaded(22785, "Wintersun - Faiths of Skyrim.esp") ; #DEBUG_LINE_NO:590
  If isWintersunLoaded && lastWintersunCheck == False ; #DEBUG_LINE_NO:591
    seedutil.getFoodDatastoreHandler().addWintersun(False) ; #DEBUG_LINE_NO:592
  EndIf
  Bool lastFoodExpandedCheck = isFoodExpandedLoaded ; #DEBUG_LINE_NO:596
  isFoodExpandedLoaded = Self.IsPluginLoaded(3425, "Ana_Skyrim Food Expanded - EN.esp") ; #DEBUG_LINE_NO:597
  If isFoodExpandedLoaded && lastFoodExpandedCheck == False ; #DEBUG_LINE_NO:598
    seedutil.getFoodDatastoreHandler().AddFoodExpanded(False) ; #DEBUG_LINE_NO:599
  EndIf
  If !isFoodExpandedLoaded ; #DEBUG_LINE_NO:601
    Bool lastFoodExpandedCheckFr = isFoodExpandedLoadedFr ; #DEBUG_LINE_NO:602
    isFoodExpandedLoadedFr = Self.IsPluginLoaded(3425, "Ana_Skyrim Food Expanded.esp") ; #DEBUG_LINE_NO:603
    If isFoodExpandedLoaded && lastFoodExpandedCheck == False ; #DEBUG_LINE_NO:604
      seedutil.getFoodDatastoreHandler().AddFoodExpanded(False) ; #DEBUG_LINE_NO:605
    EndIf
  EndIf
  Bool lastCookingExpandedCheck = isCookingExpandedLoaded ; #DEBUG_LINE_NO:610
  isCookingExpandedLoaded = Self.IsPluginLoaded(26281, "CookingExpanded.esp") ; #DEBUG_LINE_NO:611
  If isCookingExpandedLoaded && lastCookingExpandedCheck == False ; #DEBUG_LINE_NO:612
    seedutil.getFoodDatastoreHandler().AddCookingExpanded(False) ; #DEBUG_LINE_NO:613
  EndIf
  Bool lastShadowOfSkyrimCheck = isShadowOfSkyrimLoaded ; #DEBUG_LINE_NO:617
  isShadowOfSkyrimLoaded = Self.IsPluginLoaded(2058, "Shadow of Skyrim.esp") ; #DEBUG_LINE_NO:618
  If isShadowOfSkyrimLoaded && lastShadowOfSkyrimCheck == False ; #DEBUG_LINE_NO:619
    _Seed_Setting_AlternateDeathSystem.setValue(2 as Float) ; #DEBUG_LINE_NO:620
  EndIf
  Bool lastCCTheCauseCheck = isCCTheCauseLoaded ; #DEBUG_LINE_NO:624
  isCCTheCauseLoaded = Self.IsPluginLoaded(442305, "ccbgssse067-daedinv.esm") ; #DEBUG_LINE_NO:625
  If isCCTheCauseLoaded && lastCCTheCauseCheck == False ; #DEBUG_LINE_NO:626
    Location DeadlandsLocation = Game.GetFormFromFile(212793, "ccbgssse067-daedinv.esm") as Location ; #DEBUG_LINE_NO:627
    If DeadlandsLocation ; #DEBUG_LINE_NO:628
      _Seed_OblivionLocations.AddForm(DeadlandsLocation as Form) ; #DEBUG_LINE_NO:629
    EndIf
  EndIf
  If camputil.GetSKSELoaded() ; #DEBUG_LINE_NO:634
    Int rand = 0 ; #DEBUG_LINE_NO:635
    rand = po3_sksefunctions.GenerateRandomInt(1, 2) ; #DEBUG_LINE_NO:636
    isPO3Loaded = rand > 0 ; #DEBUG_LINE_NO:637
  EndIf
  Bool lastWarmDrinksCheck = isWarmDrinksLoaded ; #DEBUG_LINE_NO:641
  isWarmDrinksLoaded = Self.IsPluginLoaded(64266, "Warm Drinks.esp") ; #DEBUG_LINE_NO:642
  If isWarmDrinksLoaded == True && lastWarmDrinksCheck == False ; #DEBUG_LINE_NO:643
    seedutil.getFoodDatastoreHandler().AddWarmDrinks(False) ; #DEBUG_LINE_NO:644
  EndIf
  Bool lastProjectAhoCheck = isProjectAhoLoaded ; #DEBUG_LINE_NO:648
  isProjectAhoLoaded = Self.IsPluginLoaded(7999717, "Dwarfsphere.esp") ; #DEBUG_LINE_NO:649
  If isProjectAhoLoaded == True && lastProjectAhoCheck == False ; #DEBUG_LINE_NO:650
    seedutil.getFoodDatastoreHandler().AddProjectAho(False) ; #DEBUG_LINE_NO:651
  EndIf
  Bool lastInterstingNPCsCheck = isInterestingNPCsLoaded ; #DEBUG_LINE_NO:655
  isInterestingNPCsLoaded = Self.IsPluginLoaded(4858, "3DNPC.esp") ; #DEBUG_LINE_NO:656
  If isInterestingNPCsLoaded == True && lastInterstingNPCsCheck == False ; #DEBUG_LINE_NO:657
    seedutil.getFoodDatastoreHandler().AddInterestingNPCs(False) ; #DEBUG_LINE_NO:658
  EndIf
  Bool lastImmersiveEncountersCheck = isImmersiveEncountersLoaded ; #DEBUG_LINE_NO:662
  isImmersiveEncountersLoaded = Self.IsPluginLoaded(64266, "Immersive Encounters.esp") ; #DEBUG_LINE_NO:663
  If isImmersiveEncountersLoaded == True && lastImmersiveEncountersCheck == False ; #DEBUG_LINE_NO:664
    seedutil.getFoodDatastoreHandler().AddImmersiveEncounters(False) ; #DEBUG_LINE_NO:665
  EndIf
  Bool lastWyrmstoothCheck = isWyrmstoothLoaded ; #DEBUG_LINE_NO:669
  isWyrmstoothLoaded = Self.IsPluginLoaded(140584, "Wyrmstooth.esp") ; #DEBUG_LINE_NO:670
  If isWyrmstoothLoaded == True && lastWyrmstoothCheck == False ; #DEBUG_LINE_NO:671
    seedutil.getFoodDatastoreHandler().AddWyrmstooth(False) ; #DEBUG_LINE_NO:672
  EndIf
  Bool lastFalskaarCheck = isFalskaarLoaded ; #DEBUG_LINE_NO:676
  isFalskaarLoaded = Self.IsPluginLoaded(140084, "Falskaar.esm") ; #DEBUG_LINE_NO:677
  If isFalskaarLoaded == True && lastFalskaarCheck == False ; #DEBUG_LINE_NO:678
    seedutil.getFoodDatastoreHandler().AddFalskaar(False) ; #DEBUG_LINE_NO:679
  EndIf
  Bool lastNordicCookingCheck = isNordicCookingLoaded ; #DEBUG_LINE_NO:683
  isNordicCookingLoaded = Self.IsPluginLoaded(3426, "NordicCooking.esp") ; #DEBUG_LINE_NO:684
  If isNordicCookingLoaded == True && lastNordicCookingCheck == False ; #DEBUG_LINE_NO:685
    seedutil.getFoodDatastoreHandler().AddNordicCooking(False) ; #DEBUG_LINE_NO:686
  EndIf
  Bool lastMealtimeCheck = isMealtimeLoaded ; #DEBUG_LINE_NO:690
  isMealtimeLoaded = Self.IsPluginLoaded(458419, "mealtimeyum.esp") ; #DEBUG_LINE_NO:691
  If isMealtimeLoaded == True && lastMealtimeCheck == False ; #DEBUG_LINE_NO:692
    seedutil.getFoodDatastoreHandler().AddMealtime(False) ; #DEBUG_LINE_NO:693
  EndIf
  Bool lastSAFOCheck = isSAFOLoaded ; #DEBUG_LINE_NO:697
  isSAFOLoaded = Self.IsPluginLoaded(2054, "SAFO.esp") ; #DEBUG_LINE_NO:698
  If isSAFOLoaded == True && lastSAFOCheck == False ; #DEBUG_LINE_NO:699
    seedutil.getFoodDatastoreHandler().AddSAFO(False) ; #DEBUG_LINE_NO:700
  EndIf
  isFrozenNorthLoaded = Self.IsPluginLoaded(2088, "TheFrozenNorth.esp") ; #DEBUG_LINE_NO:704
  _Seed_PortionsToProvisionsList.Revert() ; #DEBUG_LINE_NO:708
  If isFirstSessionLoad == 2 || forceSetFoodProperties ; #DEBUG_LINE_NO:711
    seedutil.getFoodMaintenanceHandler().setFoodProperties() ; #DEBUG_LINE_NO:712
  EndIf
EndFunction

Int Function isfirstSessionLoad_Int()
  If camputil.GetSKSELoaded() ; #DEBUG_LINE_NO:718
    If _Seed_TestFood.GetGoldValue() == 1 ; #DEBUG_LINE_NO:719
      _Seed_TestFood.SetGoldValue(2) ; #DEBUG_LINE_NO:720
      Return 2 ; #DEBUG_LINE_NO:721
    Else
      Return 1 ; #DEBUG_LINE_NO:723
    EndIf
  EndIf
  Return -1 ; #DEBUG_LINE_NO:726
EndFunction

Function refreshSystems()
  _seedinternal.SeedDebug(1, "[Compatability]: Refreshing Systems.") ; #DEBUG_LINE_NO:730
  If !PlayerRef ; #DEBUG_LINE_NO:731
    PlayerRef = Game.GetPlayer() ; #DEBUG_LINE_NO:732
  EndIf
  seedutil.GetMainSystem().PlayerAlias.ForceRefTo(PlayerRef as ObjectReference) ; #DEBUG_LINE_NO:734
  seedutil.getHungerSystem().refreshSystem() ; #DEBUG_LINE_NO:735
  seedutil.getThirstSystem().refreshSystem() ; #DEBUG_LINE_NO:736
  seedutil.getPartyHungerSystem().refreshSystem() ; #DEBUG_LINE_NO:737
  seedutil.getPartyThirstSystem().refreshSystem() ; #DEBUG_LINE_NO:738
  seedutil.getFatigueSystem().refreshSystem() ; #DEBUG_LINE_NO:739
  seedutil.getVitalitySystem().refreshSystem() ; #DEBUG_LINE_NO:740
  seedutil.GetAlcoholSystem().refreshSystem() ; #DEBUG_LINE_NO:741
  seedutil.GetSkoomaSystem().refreshSystem() ; #DEBUG_LINE_NO:742
  seedutil.GetSpoilageSystem().refreshSystem() ; #DEBUG_LINE_NO:743
  seedutil.GetVendorStockSystem().refreshSystem() ; #DEBUG_LINE_NO:744
  If _Seed_SettingVampireMonitoring.getValue() == 2 as Float ; #DEBUG_LINE_NO:745
    seedutil.getMonsterHandler().startMonitoring() ; #DEBUG_LINE_NO:746
  EndIf
EndFunction

Function StartInnMeals(Bool startup)
  If Self.IsPluginLoaded(2055, "Last Seed - Inn Meals.esp") ; #DEBUG_LINE_NO:751
    Quest _MM_DialogHandlerQuest = Game.GetFormFromFile(2055, "Last Seed - Inn Meals.esp") as Quest ; #DEBUG_LINE_NO:752
    If startup ; #DEBUG_LINE_NO:753
      If !_MM_DialogHandlerQuest.IsRunning() ; #DEBUG_LINE_NO:754
        _MM_DialogHandlerQuest.start() ; #DEBUG_LINE_NO:755
      EndIf
    ElseIf _MM_DialogHandlerQuest.IsRunning() ; #DEBUG_LINE_NO:758
      _MM_DialogHandlerQuest.stop() ; #DEBUG_LINE_NO:759
    EndIf
  EndIf
EndFunction

Function RegisterForMenusOnLoad()
  If camputil.GetSKSELoaded() ; #DEBUG_LINE_NO:774
    Self.RegisterForMenu("Crafting Menu") ; #DEBUG_LINE_NO:775
    Self.RegisterForMenu("Dialogue Menu") ; #DEBUG_LINE_NO:776
    _seedinternal.SeedDebug(0, "[Compatibility]Registered for menus") ; #DEBUG_LINE_NO:777
  EndIf
EndFunction

Function RegisterForEventsOnLoad()
  seedutil.getHungerSystem().RegisterForEvents() ; #DEBUG_LINE_NO:782
  seedutil.getThirstSystem().RegisterForEvents() ; #DEBUG_LINE_NO:783
  seedutil.getFatigueSystem().RegisterForEvents() ; #DEBUG_LINE_NO:784
  seedutil.GetVitalityMeterHandler().RegisterForEvents() ; #DEBUG_LINE_NO:785
  seedutil.GetHungerMeterHandler().RegisterForEvents() ; #DEBUG_LINE_NO:786
  seedutil.GetThirstMeterHandler().RegisterForEvents() ; #DEBUG_LINE_NO:787
  seedutil.GetFatigueMeterHandler().RegisterForEvents() ; #DEBUG_LINE_NO:788
EndFunction

Function AddStartupSpells()
  PlayerRef.AddSpell(_Seed_CheckNeedsSpell, False) ; #DEBUG_LINE_NO:793
  PlayerRef.AddSpell(_Seed_OpenProvisionsSpell, False) ; #DEBUG_LINE_NO:794
  PlayerRef.AddSpell(_Seed_AutoEat, False) ; #DEBUG_LINE_NO:795
  PlayerRef.AddSpell(_Seed_AutoDrink, False) ; #DEBUG_LINE_NO:796
  PlayerRef.AddSpell(_Seed_ExamineFood, False) ; #DEBUG_LINE_NO:797
  PlayerRef.AddSpell(_Seed_DrinkFromStreamSpell, False) ; #DEBUG_LINE_NO:798
EndFunction

Function SendEvent_LastSeedLoaded()
  fallbackeventemitter emitter = seedutil.GetEventEmitter_LastSeedLoaded() ; #DEBUG_LINE_NO:802
  Int handle = emitter.Create("LastSeed_Loaded") ; #DEBUG_LINE_NO:803
  If handle ; #DEBUG_LINE_NO:804
    emitter.Send(handle) ; #DEBUG_LINE_NO:805
  EndIf
EndFunction

Function RegisterCampfireSkill()
  GlobalVariable CampfireAPIVersion = Game.GetFormFromFile(258494, "Campfire.esm") as GlobalVariable ; #DEBUG_LINE_NO:810
  If CampfireAPIVersion as Bool && CampfireAPIVersion.getValueInt() >= 4 ; #DEBUG_LINE_NO:811
    Bool b = camputil.RegisterPerkTree(_Seed_PerkNodeController_Provisioning, "LastSeed.esp") ; #DEBUG_LINE_NO:812
  Else
    Debug.trace("[Campfire] ERROR: Unable to register Campfire Skill System for LastSeed.esp. Campfire was not found or the version loaded is not compatible. Expected CampUtil API 4 or higher, got " + CampfireAPIVersion.getValueInt() as String, 0) ; #DEBUG_LINE_NO:814
  EndIf
EndFunction

Function CheckDatastore()
  _seedinternal.SeedDebug(3, "Implement datastore sanity checking not yet implemented.") ; #DEBUG_LINE_NO:819
EndFunction

Bool Function IsPluginLoaded(Int iFormID, String sPluginName)
  If isSKSELoaded ; #DEBUG_LINE_NO:824
    Int i = Game.GetModByName(sPluginName) ; #DEBUG_LINE_NO:825
    If i != 255 ; #DEBUG_LINE_NO:826
      Debug.trace("[LastSeed] Loaded: " + sPluginName, 0) ; #DEBUG_LINE_NO:827
      Return True ; #DEBUG_LINE_NO:828
    Else
      Return False ; #DEBUG_LINE_NO:830
    EndIf
  Else
    Bool b = Game.GetFormFromFile(iFormID, sPluginName) as Bool ; #DEBUG_LINE_NO:833
    If b ; #DEBUG_LINE_NO:834
      Debug.trace("[LastSeed] Loaded: " + sPluginName, 0) ; #DEBUG_LINE_NO:835
      Return True ; #DEBUG_LINE_NO:836
    Else
      Return False ; #DEBUG_LINE_NO:838
    EndIf
  EndIf
EndFunction

Function SendEvent_SKSE_LoadProfileOnStartup()
  If isSKSELoaded ; #DEBUG_LINE_NO:845
    Int handle = modevent.Create("LastSeed_LoadProfileOnStartup") ; #DEBUG_LINE_NO:846
    If handle ; #DEBUG_LINE_NO:847
      modevent.Send(handle) ; #DEBUG_LINE_NO:848
    EndIf
  EndIf
EndFunction

Function SendEvent_SKSE_ApplyMeterPreset(Int aiValue)
  If isSKSELoaded ; #DEBUG_LINE_NO:855
    Int handle = modevent.Create("LastSeed_ApplyMeterPreset") ; #DEBUG_LINE_NO:856
    If handle ; #DEBUG_LINE_NO:857
      modevent.PushInt(handle, aiValue) ; #DEBUG_LINE_NO:858
      modevent.Send(handle) ; #DEBUG_LINE_NO:859
    EndIf
  EndIf
EndFunction

Function OnMenuOpen(String MenuName)
  _seedinternal.SeedDebug(0, "[_Seed_Main] Menu Opened. MenuName = " + MenuName) ; #DEBUG_LINE_NO:865
  If MenuName == "Crafting Menu" || MenuName == "Dialogue Menu" ; #DEBUG_LINE_NO:868
    _Seed_FocusOnMenu.setValue(2 as Float) ; #DEBUG_LINE_NO:869
  EndIf
EndFunction

Function OnMenuClose(String MenuName)
  If MenuName == "Crafting Menu" || MenuName == "Dialogue Menu" ; #DEBUG_LINE_NO:875
    _Seed_FocusOnMenu.setValue(1 as Float) ; #DEBUG_LINE_NO:876
  EndIf
EndFunction

Bool Function isFirstSessionLoad()
  If camputil.GetSKSELoaded() && _Seed_TestFood.GetGoldValue() == 1 ; #DEBUG_LINE_NO:885
    _Seed_TestFood.SetGoldValue(2) ; #DEBUG_LINE_NO:886
    Return True ; #DEBUG_LINE_NO:887
  EndIf
  Return False ; #DEBUG_LINE_NO:889
EndFunction

Function SetPotionLeveledLists(Bool addPotions = True)
  seedutil.GetVendorStockSystem().SetPotionLeveledLists(addPotions) ; #DEBUG_LINE_NO:895
EndFunction

Function setCACOGlobals()
  Self.setExternalGlobal(5127441, "Complete Alchemy & Cooking Overhaul.esp", 0 as Float) ; #DEBUG_LINE_NO:899
  Self.setExternalGlobal(2928116, "Complete Alchemy & Cooking Overhaul.esp", 0 as Float) ; #DEBUG_LINE_NO:900
  Self.setExternalGlobal(2928115, "Complete Alchemy & Cooking Overhaul.esp", 0 as Float) ; #DEBUG_LINE_NO:901
  Self.setExternalGlobal(6103422, "Complete Alchemy & Cooking Overhaul.esp", 0 as Float) ; #DEBUG_LINE_NO:902
  Self.setExternalGlobal(5106669, "Complete Alchemy & Cooking Overhaul.esp", 0 as Float) ; #DEBUG_LINE_NO:903
EndFunction

Function setExternalGlobal(Int aiFormID, String espName, Float value)
  GlobalVariable theGlobal = Game.GetFormFromFile(aiFormID, espName) as GlobalVariable ; #DEBUG_LINE_NO:908
  If theGlobal ; #DEBUG_LINE_NO:909
    theGlobal.setValue(value) ; #DEBUG_LINE_NO:910
  EndIf
EndFunction

Int Function GetPlayerExposureLevelFrozenNorth()
  Spell warm = Game.GetFormFromFile(2098, "TheFrozenNorth.esp") as Spell ; #DEBUG_LINE_NO:915
  If warm as Bool && PlayerRef.HasSpell(warm as Form) ; #DEBUG_LINE_NO:916
    Return 1 ; #DEBUG_LINE_NO:917
  EndIf
  Spell chilly = Game.GetFormFromFile(2086, "TheFrozenNorth.esp") as Spell ; #DEBUG_LINE_NO:920
  If chilly as Bool && PlayerRef.HasSpell(chilly as Form) ; #DEBUG_LINE_NO:921
    Return 3 ; #DEBUG_LINE_NO:922
  EndIf
  Spell cold = Game.GetFormFromFile(2050, "TheFrozenNorth.esp") as Spell ; #DEBUG_LINE_NO:925
  If cold as Bool && PlayerRef.HasSpell(cold as Form) ; #DEBUG_LINE_NO:926
    Return 4 ; #DEBUG_LINE_NO:927
  EndIf
  Spell freezing = Game.GetFormFromFile(2051, "TheFrozenNorth.esp") as Spell ; #DEBUG_LINE_NO:930
  If freezing as Bool && PlayerRef.HasSpell(freezing as Form) ; #DEBUG_LINE_NO:931
    Return 5 ; #DEBUG_LINE_NO:932
  EndIf
  Return 2 ; #DEBUG_LINE_NO:935
EndFunction

Event OnVampireFeed(Actor akTarget)
  If seedutil.isSpecialEdition() && _Seed_SettingVampireMonitoring.getValueInt() != 2 ; #DEBUG_LINE_NO:943
    seedutil.VampireFeed() ; #DEBUG_LINE_NO:944
  EndIf
EndEvent
