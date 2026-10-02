ScriptName _Seed_AliasFoodMonitor Extends ReferenceAlias
; Last Seed 2026: when LastSeed.dll tracks spoilage (LastSeedNative.NativeSpoilage), this alias stops listening to inventory events.
{ Tracks the food that goes into, and out of, a given alias for the purposes of tracking. }

;-- Variables ---------------------------------------

;-- Properties --------------------------------------
lastseedapi Property LastSeed Auto
FormList Property TrackingList Auto
{ The formlist that tracks this container/actor alias. }
FormList Property _Seed_Bread Auto
FormList Property _Seed_Cheese Auto
FormList Property _Seed_CheeseBowls Auto
FormList Property _Seed_DrinkMilk Auto
FormList Property _Seed_FishCooked Auto
FormList Property _Seed_FishRaw Auto
FormList Property _Seed_Fruit Auto
Potion Property _Seed_IceWraithTeeth Auto
MiscObject Property _Seed_IceWraithTeethOld Auto
FormList Property _Seed_MeatCooked Auto
FormList Property _Seed_MeatRaw Auto
FormList Property _Seed_Pastries Auto
Activator Property _Seed_PerishableFoodTracker Auto
MiscObject Property _Seed_PerishedFood Auto
FormList Property _Seed_Preserved Auto
FormList Property _Seed_SeafoodCooked Auto
FormList Property _Seed_SeafoodRaw Auto
GlobalVariable Property _Seed_Setting_SpoilRate01_Bread Auto
GlobalVariable Property _Seed_Setting_SpoilRate02_RawMeat Auto
GlobalVariable Property _Seed_Setting_SpoilRate03_CookedMeat Auto
GlobalVariable Property _Seed_Setting_SpoilRate04_RawSmallGame Auto
GlobalVariable Property _Seed_Setting_SpoilRate05_CookedSmallGame Auto
GlobalVariable Property _Seed_Setting_SpoilRate06_RawFish Auto
GlobalVariable Property _Seed_Setting_SpoilRate07_CookedFish Auto
GlobalVariable Property _Seed_Setting_SpoilRate08_RawSeafood Auto
GlobalVariable Property _Seed_Setting_SpoilRate09_CookedSeafood Auto
GlobalVariable Property _Seed_Setting_SpoilRate10_Vegitables Auto
GlobalVariable Property _Seed_Setting_SpoilRate11_Fruit Auto
GlobalVariable Property _Seed_Setting_SpoilRate12_Cheese Auto
GlobalVariable Property _Seed_Setting_SpoilRate13_Treats Auto
GlobalVariable Property _Seed_Setting_SpoilRate14_Pastry Auto
GlobalVariable Property _Seed_Setting_SpoilRate15_Stew Auto
GlobalVariable Property _Seed_Setting_SpoilRate16_CheeseBowls Auto
GlobalVariable Property _Seed_Setting_SpoilRate17_Milk Auto
GlobalVariable Property _Seed_Setting_SpoilRate_IceWraithTeeth Auto
GlobalVariable Property _Seed_Setting_SpoilageEnable Auto
FormList Property _Seed_SmallGameCooked Auto
FormList Property _Seed_SmallGameRaw Auto
ObjectReference Property _Seed_SpoiledFoodSystemContainerRef Auto
FormList Property _Seed_SpoiledFoods Auto
Potion Property _Seed_Spoiled_Bread Auto
Potion Property _Seed_Spoiled_Cheese Auto
Potion Property _Seed_Spoiled_CheeseBowl Auto
Potion Property _Seed_Spoiled_FishCooked Auto
Potion Property _Seed_Spoiled_FishRaw Auto
Potion Property _Seed_Spoiled_Fruit Auto
Potion Property _Seed_Spoiled_MeatCooked Auto
Potion Property _Seed_Spoiled_MeatRaw Auto
Potion Property _Seed_Spoiled_Milk Auto
Potion Property _Seed_Spoiled_Pastry Auto
Potion Property _Seed_Spoiled_SeafoodCooked Auto
Potion Property _Seed_Spoiled_SeafoodRaw Auto
Potion Property _Seed_Spoiled_SmallGameCooked Auto
Potion Property _Seed_Spoiled_SmallGameRaw Auto
Potion Property _Seed_Spoiled_Stew Auto
Potion Property _Seed_Spoiled_Treat Auto
Potion Property _Seed_Spoiled_Vegetable Auto
FormList Property _Seed_Stews Auto
ObjectReference Property _Seed_TrackerAnchorRef Auto
FormList Property _Seed_Treats Auto
FormList Property _Seed_Vegetables Auto
Int Property aliasId Auto
Bool Property isActor = True Auto
{ Default: true }

;-- Functions ---------------------------------------

; Skipped compiler generated GetState

; Skipped compiler generated GotoState

Event OnInit()
  Self.addInventoryEventFilters() ; #DEBUG_LINE_NO:86
EndEvent

Function addInventoryEventFilters()
  If LastSeedNative.NativeSpoilage() ; Last Seed 2026: LastSeed.dll tracks spoilage
    Return 
  EndIf
  Self.AddInventoryEventFilter(_Seed_Bread as Form) ; #DEBUG_LINE_NO:90
  Self.AddInventoryEventFilter(_Seed_MeatRaw as Form) ; #DEBUG_LINE_NO:91
  Self.AddInventoryEventFilter(_Seed_MeatCooked as Form) ; #DEBUG_LINE_NO:92
  Self.AddInventoryEventFilter(_Seed_SmallGameRaw as Form) ; #DEBUG_LINE_NO:93
  Self.AddInventoryEventFilter(_Seed_SmallGameCooked as Form) ; #DEBUG_LINE_NO:94
  Self.AddInventoryEventFilter(_Seed_FishRaw as Form) ; #DEBUG_LINE_NO:95
  Self.AddInventoryEventFilter(_Seed_FishCooked as Form) ; #DEBUG_LINE_NO:96
  Self.AddInventoryEventFilter(_Seed_SeafoodRaw as Form) ; #DEBUG_LINE_NO:97
  Self.AddInventoryEventFilter(_Seed_SeafoodCooked as Form) ; #DEBUG_LINE_NO:98
  Self.AddInventoryEventFilter(_Seed_Vegetables as Form) ; #DEBUG_LINE_NO:99
  Self.AddInventoryEventFilter(_Seed_Fruit as Form) ; #DEBUG_LINE_NO:100
  Self.AddInventoryEventFilter(_Seed_Cheese as Form) ; #DEBUG_LINE_NO:101
  Self.AddInventoryEventFilter(_Seed_Treats as Form) ; #DEBUG_LINE_NO:102
  Self.AddInventoryEventFilter(_Seed_Pastries as Form) ; #DEBUG_LINE_NO:103
  Self.AddInventoryEventFilter(_Seed_Stews as Form) ; #DEBUG_LINE_NO:104
  Self.AddInventoryEventFilter(_Seed_CheeseBowls as Form) ; #DEBUG_LINE_NO:105
  Self.AddInventoryEventFilter(_Seed_DrinkMilk as Form) ; #DEBUG_LINE_NO:106
  Self.AddInventoryEventFilter(_Seed_IceWraithTeeth as Form) ; #DEBUG_LINE_NO:107
EndFunction

Function stopSpoilage()
  Int i = TrackingList.GetSize() ; #DEBUG_LINE_NO:111
  While i
    i -= 1 ; #DEBUG_LINE_NO:113
    ObjectReference ref = TrackingList.GetAt(i) as ObjectReference ; #DEBUG_LINE_NO:114
    If ref ; #DEBUG_LINE_NO:115
      _seed_perishablefoodtrackerscript tracker = ref as _seed_perishablefoodtrackerscript ; #DEBUG_LINE_NO:116
      tracker.deleteTracker() ; #DEBUG_LINE_NO:117
    EndIf
  EndWhile
  TrackingList.revert() ; #DEBUG_LINE_NO:120
EndFunction

Event OnItemAdded(Form akBaseItem, Int aiItemCount, ObjectReference akItemReference, ObjectReference akSourceContainer)
  If LastSeedNative.NativeSpoilage() ; Last Seed 2026: LastSeed.dll tracks spoilage
    Return 
  EndIf
  If _Seed_Setting_SpoilageEnable.getValueInt() == 2 && (akBaseItem as Potion) as Bool ; #DEBUG_LINE_NO:124
    Potion food = akBaseItem as Potion ; #DEBUG_LINE_NO:125
    Int foodType = Self.IdentifyFood(food) ; #DEBUG_LINE_NO:127
    Bool isPreserved = Self.IsFoodPreserved(food) ; #DEBUG_LINE_NO:128
    Bool isIceWraithTeeth = food == _Seed_IceWraithTeeth ; #DEBUG_LINE_NO:129
    If foodType > 0 && !isPreserved || isIceWraithTeeth ; #DEBUG_LINE_NO:130
      Form spoiled = None ; #DEBUG_LINE_NO:131
      Int spoilDuration = -1 ; #DEBUG_LINE_NO:132
      If isIceWraithTeeth ; #DEBUG_LINE_NO:133
        spoiled = _Seed_IceWraithTeethOld as Form ; #DEBUG_LINE_NO:134
        spoilDuration = _Seed_Setting_SpoilRate_IceWraithTeeth.getValue() as Int ; #DEBUG_LINE_NO:135
      Else
        spoiled = Self.GetSpoiledVersion(food, foodType) ; #DEBUG_LINE_NO:137
        spoilDuration = Self.GetFoodMaxPerishDurationByType(foodType) ; #DEBUG_LINE_NO:138
      EndIf
      If !spoiled || spoilDuration == -1 ; #DEBUG_LINE_NO:141
        Return  ; #DEBUG_LINE_NO:142
      EndIf
      ObjectReference ref = _Seed_TrackerAnchorRef.PlaceAtMe(_Seed_PerishableFoodTracker as Form, 1, False, False) ; #DEBUG_LINE_NO:145
      _seed_perishablefoodtrackerscript tracker = ref as _seed_perishablefoodtrackerscript ; #DEBUG_LINE_NO:146
      tracker.food = akBaseItem ; #DEBUG_LINE_NO:148
      tracker.SpoiledFood = spoiled ; #DEBUG_LINE_NO:149
      tracker.Quantity = aiItemCount ; #DEBUG_LINE_NO:150
      tracker.MaxPerishHours = spoilDuration as Float ; #DEBUG_LINE_NO:151
      tracker.MyTrackingList = TrackingList ; #DEBUG_LINE_NO:152
      If isActor ; #DEBUG_LINE_NO:153
        tracker.TrackedContainer = Self.GetActorRef() as ObjectReference ; #DEBUG_LINE_NO:154
      Else
        tracker.TrackedContainer = Self.GetRef() ; #DEBUG_LINE_NO:156
      EndIf
      tracker.startupNonSKSE() ; #DEBUG_LINE_NO:172
      _seedinternal.SeedDebug(0, (((("New tracker data >>>> Food: " + akBaseItem as String) + ", Spoiled Food: " + spoiled as String) + ", Count: " + aiItemCount as String) + ", Max Perish Hours: " + spoilDuration as String) + ", TrackedContainer: " + tracker.TrackedContainer as String) ; #DEBUG_LINE_NO:174
      TrackingList.AddForm(tracker as Form) ; #DEBUG_LINE_NO:176
      _seedinternal.SeedDebug(0, ("Added " + tracker as String) + " to the list " + TrackingList as String) ; #DEBUG_LINE_NO:177
      _seedinternal.SeedDebug(0, "State of the Food Tracking FormList is:") ; #DEBUG_LINE_NO:178
      Int j = 0 ; #DEBUG_LINE_NO:179
      While j < TrackingList.GetSize() ; #DEBUG_LINE_NO:180
        _seedinternal.SeedDebug(0, "    " + TrackingList.GetAt(j) as String) ; #DEBUG_LINE_NO:181
        j += 1 ; #DEBUG_LINE_NO:182
      EndWhile
    EndIf
  EndIf
EndEvent

Event OnItemRemoved(Form akBaseItem, Int aiItemCount, ObjectReference akItemReference, ObjectReference akDestContainer)
  If LastSeedNative.NativeSpoilage() ; Last Seed 2026: LastSeed.dll tracks spoilage
    Return 
  EndIf
  Self.GotoState("Processing") ; #DEBUG_LINE_NO:189
  _seedinternal.SeedDebug(0, "_Seed_AliasFoodMonitor OnItemRemoved") ; #DEBUG_LINE_NO:190
  If _Seed_Setting_SpoilageEnable.getValueInt() == 2 && (akBaseItem as Potion) as Bool ; #DEBUG_LINE_NO:191
    _seedinternal.SeedDebug(0, "_Seed_AliasFoodMonitor item was food.") ; #DEBUG_LINE_NO:192
    If akDestContainer == _Seed_SpoiledFoodSystemContainerRef ; #DEBUG_LINE_NO:193
      Return  ; #DEBUG_LINE_NO:195
    EndIf
    Int foodType = Self.IdentifyFood(akBaseItem as Potion) ; #DEBUG_LINE_NO:198
    Bool isPreserved = Self.IsFoodPreserved(akBaseItem as Potion) ; #DEBUG_LINE_NO:199
    If foodType > 0 && !isPreserved ; #DEBUG_LINE_NO:200
      Int remaining = aiItemCount ; #DEBUG_LINE_NO:201
      Int i = TrackingList.GetSize() - 1 ; #DEBUG_LINE_NO:202
      While remaining > 0 && i >= 0 ; #DEBUG_LINE_NO:203
        ObjectReference ref = TrackingList.GetAt(i) as ObjectReference ; #DEBUG_LINE_NO:204
        If ref ; #DEBUG_LINE_NO:205
          _seed_perishablefoodtrackerscript tracker = ref as _seed_perishablefoodtrackerscript ; #DEBUG_LINE_NO:206
          If tracker.food == akBaseItem && tracker.Quantity > 0 ; #DEBUG_LINE_NO:207
            Int amountRemoved = tracker.ReduceQuantity(remaining) ; #DEBUG_LINE_NO:208
            remaining -= amountRemoved ; #DEBUG_LINE_NO:209
            If remaining > 0 ; #DEBUG_LINE_NO:211
              _seedinternal.SeedDebug(0, "Starting the tracker search over.") ; #DEBUG_LINE_NO:212
              i = TrackingList.GetSize() - 1 ; #DEBUG_LINE_NO:213
            EndIf
          Else
            _seedinternal.SeedDebug(0, ("Checking next value. Reason: Food = " + tracker.food as String) + ", Quantity = " + tracker.Quantity as String) ; #DEBUG_LINE_NO:216
            i -= 1 ; #DEBUG_LINE_NO:217
          EndIf
        Else
          _seedinternal.SeedDebug(0, "Checking next value. Reason: ref was None") ; #DEBUG_LINE_NO:220
          i -= 1 ; #DEBUG_LINE_NO:221
        EndIf
      EndWhile
      _seedinternal.SeedDebug(0, "Finished search. State of the Food Tracking FormList is:") ; #DEBUG_LINE_NO:225
      Int j = 0 ; #DEBUG_LINE_NO:226
      While j < TrackingList.GetSize() ; #DEBUG_LINE_NO:227
        _seedinternal.SeedDebug(0, "    " + TrackingList.GetAt(j) as String) ; #DEBUG_LINE_NO:228
        j += 1 ; #DEBUG_LINE_NO:229
      EndWhile
    EndIf
  EndIf
  Self.GotoState("") ; #DEBUG_LINE_NO:233
EndEvent

Int Function IdentifyFood(Potion food)
  If _Seed_Bread.HasForm(food as Form) ; #DEBUG_LINE_NO:239
    Return 1 ; #DEBUG_LINE_NO:240
  ElseIf _Seed_MeatRaw.HasForm(food as Form) ; #DEBUG_LINE_NO:241
    Return 2 ; #DEBUG_LINE_NO:242
  ElseIf _Seed_MeatCooked.HasForm(food as Form) ; #DEBUG_LINE_NO:243
    Return 3 ; #DEBUG_LINE_NO:244
  ElseIf _Seed_SmallGameRaw.HasForm(food as Form) ; #DEBUG_LINE_NO:245
    Return 4 ; #DEBUG_LINE_NO:246
  ElseIf _Seed_SmallGameCooked.HasForm(food as Form) ; #DEBUG_LINE_NO:247
    Return 5 ; #DEBUG_LINE_NO:248
  ElseIf _Seed_FishRaw.HasForm(food as Form) ; #DEBUG_LINE_NO:249
    Return 6 ; #DEBUG_LINE_NO:250
  ElseIf _Seed_FishCooked.HasForm(food as Form) ; #DEBUG_LINE_NO:251
    Return 7 ; #DEBUG_LINE_NO:252
  ElseIf _Seed_SeafoodRaw.HasForm(food as Form) ; #DEBUG_LINE_NO:253
    Return 8 ; #DEBUG_LINE_NO:254
  ElseIf _Seed_SeafoodCooked.HasForm(food as Form) ; #DEBUG_LINE_NO:255
    Return 9 ; #DEBUG_LINE_NO:256
  ElseIf _Seed_Vegetables.HasForm(food as Form) ; #DEBUG_LINE_NO:257
    Return 10 ; #DEBUG_LINE_NO:258
  ElseIf _Seed_Fruit.HasForm(food as Form) ; #DEBUG_LINE_NO:259
    Return 11 ; #DEBUG_LINE_NO:260
  ElseIf _Seed_Cheese.HasForm(food as Form) ; #DEBUG_LINE_NO:261
    Return 12 ; #DEBUG_LINE_NO:262
  ElseIf _Seed_Treats.HasForm(food as Form) ; #DEBUG_LINE_NO:263
    Return 13 ; #DEBUG_LINE_NO:264
  ElseIf _Seed_Pastries.HasForm(food as Form) ; #DEBUG_LINE_NO:265
    Return 14 ; #DEBUG_LINE_NO:266
  ElseIf _Seed_Stews.HasForm(food as Form) ; #DEBUG_LINE_NO:267
    Return 15 ; #DEBUG_LINE_NO:268
  ElseIf _Seed_CheeseBowls.HasForm(food as Form) ; #DEBUG_LINE_NO:269
    Return 16 ; #DEBUG_LINE_NO:270
  ElseIf _Seed_DrinkMilk.HasForm(food as Form) ; #DEBUG_LINE_NO:271
    Return 17 ; #DEBUG_LINE_NO:272
  ElseIf seedutil.GetCompatibilitySystem().isHunterbornSoupsLoaded ; #DEBUG_LINE_NO:282
    Keyword Soup = Game.GetFormFromFile(2673970, "Hunterborn - Soups and Stews.esp") as Keyword ; #DEBUG_LINE_NO:283
    If food.HasKeyword(Soup) ; #DEBUG_LINE_NO:284
      Return 15 ; #DEBUG_LINE_NO:285
    EndIf
  EndIf
  Return 0 ; #DEBUG_LINE_NO:290
EndFunction

Int Function GetFoodMaxPerishDurationByType(Int aiFoodType)
  If aiFoodType < 1 || aiFoodType > 17 ; #DEBUG_LINE_NO:296
    Return -1 ; #DEBUG_LINE_NO:297
  EndIf
  If aiFoodType == 1 ; #DEBUG_LINE_NO:301
    Return _Seed_Setting_SpoilRate01_Bread.getValueInt() ; #DEBUG_LINE_NO:302
  ElseIf aiFoodType == 2 ; #DEBUG_LINE_NO:304
    Return _Seed_Setting_SpoilRate02_RawMeat.getValueInt() ; #DEBUG_LINE_NO:305
  ElseIf aiFoodType == 3 ; #DEBUG_LINE_NO:307
    Return _Seed_Setting_SpoilRate03_CookedMeat.getValueInt() ; #DEBUG_LINE_NO:308
  ElseIf aiFoodType == 4 ; #DEBUG_LINE_NO:310
    Return _Seed_Setting_SpoilRate04_RawSmallGame.getValueInt() ; #DEBUG_LINE_NO:311
  ElseIf aiFoodType == 5 ; #DEBUG_LINE_NO:313
    Return _Seed_Setting_SpoilRate05_CookedSmallGame.getValueInt() ; #DEBUG_LINE_NO:314
  ElseIf aiFoodType == 6 ; #DEBUG_LINE_NO:316
    Return _Seed_Setting_SpoilRate06_RawFish.getValueInt() ; #DEBUG_LINE_NO:317
  ElseIf aiFoodType == 7 ; #DEBUG_LINE_NO:319
    Return _Seed_Setting_SpoilRate07_CookedFish.getValueInt() ; #DEBUG_LINE_NO:320
  ElseIf aiFoodType == 8 ; #DEBUG_LINE_NO:322
    Return _Seed_Setting_SpoilRate08_RawSeafood.getValueInt() ; #DEBUG_LINE_NO:323
  ElseIf aiFoodType == 9 ; #DEBUG_LINE_NO:325
    Return _Seed_Setting_SpoilRate09_CookedSeafood.getValueInt() ; #DEBUG_LINE_NO:326
  ElseIf aiFoodType == 10 ; #DEBUG_LINE_NO:328
    Return _Seed_Setting_SpoilRate10_Vegitables.getValueInt() ; #DEBUG_LINE_NO:329
  ElseIf aiFoodType == 11 ; #DEBUG_LINE_NO:331
    Return _Seed_Setting_SpoilRate11_Fruit.getValueInt() ; #DEBUG_LINE_NO:332
  ElseIf aiFoodType == 12 ; #DEBUG_LINE_NO:334
    Return _Seed_Setting_SpoilRate12_Cheese.getValueInt() ; #DEBUG_LINE_NO:335
  ElseIf aiFoodType == 13 ; #DEBUG_LINE_NO:337
    Return _Seed_Setting_SpoilRate13_Treats.getValueInt() ; #DEBUG_LINE_NO:338
  ElseIf aiFoodType == 14 ; #DEBUG_LINE_NO:340
    Return _Seed_Setting_SpoilRate14_Pastry.getValueInt() ; #DEBUG_LINE_NO:341
  ElseIf aiFoodType == 15 ; #DEBUG_LINE_NO:343
    Return _Seed_Setting_SpoilRate15_Stew.getValueInt() ; #DEBUG_LINE_NO:344
  ElseIf aiFoodType == 16 ; #DEBUG_LINE_NO:346
    Return _Seed_Setting_SpoilRate16_CheeseBowls.getValueInt() ; #DEBUG_LINE_NO:347
  ElseIf aiFoodType == 17 ; #DEBUG_LINE_NO:349
    Return _Seed_Setting_SpoilRate17_Milk.getValueInt() ; #DEBUG_LINE_NO:350
  EndIf
  Return -1 ; #DEBUG_LINE_NO:353
EndFunction

Bool Function IsFoodPreserved(Potion food)
  If _Seed_Preserved.HasForm(food as Form) ; #DEBUG_LINE_NO:359
    Return True ; #DEBUG_LINE_NO:360
  Else
    Return False ; #DEBUG_LINE_NO:362
  EndIf
EndFunction

Form Function GetSpoiledVersion(Potion food, Int foodType)
  If _Seed_SpoiledFoods.HasForm(food as Form) ; #DEBUG_LINE_NO:368
    Return _Seed_PerishedFood as Form ; #DEBUG_LINE_NO:369
  EndIf
  If foodType == 1 ; #DEBUG_LINE_NO:372
    Return _Seed_Spoiled_Bread as Form ; #DEBUG_LINE_NO:373
  ElseIf foodType == 2 ; #DEBUG_LINE_NO:374
    Return _Seed_Spoiled_MeatRaw as Form ; #DEBUG_LINE_NO:375
  ElseIf foodType == 3 ; #DEBUG_LINE_NO:376
    Return _Seed_Spoiled_MeatCooked as Form ; #DEBUG_LINE_NO:377
  ElseIf foodType == 4 ; #DEBUG_LINE_NO:378
    Return _Seed_Spoiled_SmallGameRaw as Form ; #DEBUG_LINE_NO:379
  ElseIf foodType == 5 ; #DEBUG_LINE_NO:380
    Return _Seed_Spoiled_SmallGameCooked as Form ; #DEBUG_LINE_NO:381
  ElseIf foodType == 6 ; #DEBUG_LINE_NO:382
    Return _Seed_Spoiled_FishRaw as Form ; #DEBUG_LINE_NO:383
  ElseIf foodType == 7 ; #DEBUG_LINE_NO:384
    Return _Seed_Spoiled_FishCooked as Form ; #DEBUG_LINE_NO:385
  ElseIf foodType == 8 ; #DEBUG_LINE_NO:386
    Return _Seed_Spoiled_SeafoodRaw as Form ; #DEBUG_LINE_NO:387
  ElseIf foodType == 9 ; #DEBUG_LINE_NO:388
    Return _Seed_Spoiled_SeafoodCooked as Form ; #DEBUG_LINE_NO:389
  ElseIf foodType == 10 ; #DEBUG_LINE_NO:390
    Return _Seed_Spoiled_Vegetable as Form ; #DEBUG_LINE_NO:391
  ElseIf foodType == 11 ; #DEBUG_LINE_NO:392
    Return _Seed_Spoiled_Fruit as Form ; #DEBUG_LINE_NO:393
  ElseIf foodType == 12 ; #DEBUG_LINE_NO:394
    Return _Seed_Spoiled_Cheese as Form ; #DEBUG_LINE_NO:395
  ElseIf foodType == 13 ; #DEBUG_LINE_NO:396
    Return _Seed_Spoiled_Treat as Form ; #DEBUG_LINE_NO:397
  ElseIf foodType == 14 ; #DEBUG_LINE_NO:398
    Return _Seed_Spoiled_Pastry as Form ; #DEBUG_LINE_NO:399
  ElseIf foodType == 15 ; #DEBUG_LINE_NO:400
    Return _Seed_Spoiled_Stew as Form ; #DEBUG_LINE_NO:401
  ElseIf foodType == 16 ; #DEBUG_LINE_NO:402
    Return _Seed_Spoiled_CheeseBowl as Form ; #DEBUG_LINE_NO:403
  ElseIf foodType == 17 ; #DEBUG_LINE_NO:404
    Return _Seed_Spoiled_Milk as Form ; #DEBUG_LINE_NO:405
  EndIf
EndFunction

;-- State -------------------------------------------
State Processing

  Event OnItemRemoved(Form akBaseItem, Int aiItemCount, ObjectReference akItemReference, ObjectReference akDestContainer)
  If LastSeedNative.NativeSpoilage() ; Last Seed 2026: LastSeed.dll tracks spoilage
    Return 
  EndIf
    Self.GotoState("") ; #DEBUG_LINE_NO:411
    _seedinternal.SeedDebug(0, "_Seed_AliasFoodMonitor suppressing duplicate call.") ; #DEBUG_LINE_NO:412
  EndEvent
EndState
