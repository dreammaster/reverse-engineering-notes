' ==========================================================================
'  OUT.EXE  --  overworld movement + per-step upkeep                  [v3]
'  reconstructed from out.asm ; see recovered/README.md for the model + tags
'
'  SUBs: DoMovement (per-step tick solid; sickness/encounter gating partial)
'        ClassifyLocationTile (out.asm:11858 -- the terrain-cost classifier)
'
'  ResolveMoveTarget (out.asm:9973) and ReadTileObject (out.asm:10468) are
'  viewport clipping + tile-record reads (rtm_FE1B/FE14 cell blits, 13x13
'  window copy from ds:1E2A), not game logic -- not reconstructed here.
' ==========================================================================
'
'  DGROUP vars:
'     playerX 1B02   playerY 1B06     trialX 208C   trialY 208A
'     facing  1E1E   hitPoints 1ADA   fatigue 1ACE
'     enteredTileType 2182 / 214A     enteredLocationId 1F02
'     tileObjectType  1F04  (0 = plain terrain -> run the tick;
'                            nonzero = a location/object -> chain out)
'     terrainWear     1AF4  (+= enteredLocationId/20 per step)
'     rationStaleness 1AF8  (*** the food clock *** ; +1 per step ;
'                            food-poisoning risk once it passes 500 ;
'                            AddFoodDays knocks it back toward 0)
'     sickWindow  2184  (armed to 10 by CreatureDefeated, else 0 ; the
'                        poisoning roll fires only the step it counts out)
'     poisonP     2186  (BSS ; CreatureDefeated sets 0.03 if reward>63 else
'                        0.005 ; ds:2F00/2F04/2F08 = 0.005 / 0.03 / 0.01)
'     ds:2584 = 3.0   ds:2552 = 500.0   ds:24EA = 20.0   ds:24E6 = 1.0
'
'  FOOD ECONOMY:  buy days of food at a TWNDR provisioner
'  (pricePerDay = INT(13 - Charm/7)*0.1 ; maxDays = MIN(1000,
'  partyGold/pricePerDay)) -> AddFoodDays reduces rationStaleness ;
'  a defeated creature can also drop "N DAYS OF FOOD".  spendFoodDays
'  (out.asm:6413) is DEAD CODE -- never called.  Food is NOT saved to
'  CHAR.DAT; rationStaleness (ds:1AF8) is a resident working var.


' --------------------------------------------------------------------------
SUB DoMovement                                        ' asm: out.asm:1427 (doMovement)
' --------------------------------------------------------------------------

    PRINT "MOVE "; Direction$(facing)                                     ' asm:1428-1454
    IF contextMode >= 10 THEN GOTO NonWorldMove        ' ds:1F2A          ' asm:1457-1459

    trialX = playerX : trialY = playerY                                   ' asm:1464-1467
    ' ON facing GOSUB move_north, move_east, move_south, move_west  (rt_FC)
    '   move_north: trialY = trialY - 1     move_south: trialY = trialY + 1
    '   move_east : trialX = trialX + 1     move_west : trialX = trialX - 1
    SELECT CASE facing                               ' rtm_FC            ' asm:1468-1469
        CASE 1 : trialY = trialY - 1
        CASE 2 : trialX = trialX + 1
        CASE 3 : trialY = trialY + 1
        CASE 4 : trialX = trialX - 1
        CASE ELSE : PRINT "bad command" : EXIT SUB   ' out-of-range arm
    END SELECT
    enteredTileType = MapWindow(trialY, trialX)                           ' asm:1470-1478
    ResolveMoveTarget trialX, trialY, turnActionFlag, remarkIndex, _
                      enteredTileType, facing                             ' asm:1479-1491
    S4(24) = enteredTileType                          ' checkpoint copy   ' asm:1492-1497
    enteredLocationId = ds1F02                                            ' asm:1498-1499
    rawTileType = enteredLocationId       ' -> ds:214A ; the ONLY input to
                                          ' CreatureApproach's per-tile
                                          ' encounter-preset ON GOSUB.      asm:1552-1553

    IF tileObjectType <> 0 THEN GOTO TileHasObject     ' ds:1F04         ' asm:1500-1502

    ' ===================== the plain-terrain per-step tick =============
PlainStep:                                                                ' loc_10BB7
    enteredTileType = enteredLocationId                                   ' asm:1507-1508
    playerX = trialX : playerY = trialY               ' commit the move   ' asm:1509-1512
    ClassifyLocationTile                                                  ' asm:1513

    stepCost   = enteredLocationId / 20.0             ' ds:24EA = 20     ' asm:1514-1519
    fatigue    = fatigue  <FF53> stepCost   ' ds:1ACE ; FF53 op          ' asm:1520-1525
    terrainWear = terrainWear + stepCost              ' ds:1AF4          ' asm:1526-1531
    rationStaleness = rationStaleness + 1.0           ' ds:1AF8 += 1     ' asm:1532-1537
    '   ^ THE FOOD CLOCK.  +1 EVERY step (NOT scaled by terrain, NOT a
    '     decrement).  Rises from ~18 (new game) toward 500.  buyFood /
    '     "you gain N days of food" call AddFoodDays which knocks it back
    '     down toward 0 (fresh rations).

    workIntHi = -9                                    ' ds:1F06          ' asm:1538
    IF sickWindow < 1 THEN EXIT SUB                   ' ds:2184          ' asm:1539-1545
    sickWindow = sickWindow - 1                                          ' asm:1549
    '   ^ sickWindow: only checked when >= 1 ; armed to 10 by
    '     CreatureDefeated ("YOU GAIN N DAYS OF FOOD"), 0 elsewhere ->
    '     a food-poisoning check fires ~10 steps after each fight.

    ' ---- food-poisoning roll -----------------------------------------
    '   SICK when  ( rationStaleness >= 500 )  AND  ( RND(1) <= poisonP )
    IF rationStaleness >= 500.0 AND RND(1) <= poisonP THEN   ' ds:2552=500 ; ds:2186 ' asm:1550-1636
        '  poisonP (ds:2186) = 0.03 if the last kill's reward code > 63,
        '     else 0.005  (set by CreatureDefeated ; ds:2F00/2F04/2F08 =
        '     0.005 / 0.03 / 0.01)
        PRINT "YOU GROW SICK FROM SOMETHING YOU ATE!"                    ' asm:1643-1670
        sickDmg  = INT( hitPoints / (3.0 * (RND(1) + 1.0)) ) ' ds:2584=3 ' asm:1672-1691
        '   -> loses 1/6 .. 1/3 of CURRENT hit points
        hitPoints = hitPoints - sickDmg                                  ' asm:1694
        sickWindow = 0                                                   ' asm:1695
        PRINT "HIT POINTS: -"; sickDmg                                   ' asm:1696-1704
    END IF
    EXIT SUB

    ' ===================== destination tile has an object =============
TileHasObject:                                                            ' loc_10D6A
    IF tileObjectType = 6 THEN GOTO PlainStep    ' type 6 = walkable decor ' asm:1653-1655
    workIntHi = 0                                                         ' asm:1659
    ChainToLocation tileObjectType               ' rtm_FD dispatch        ' asm:1662-1663
END SUB


' --------------------------------------------------------------------------
SUB ClassifyLocationTile                              ' asm: out.asm:11858 (classifyLocationTile)
' --------------------------------------------------------------------------
' Maps the raw tile-object type under the player (ds:2182, 0..13) to
' enteredLocationId (ds:1F02) -- which IS the terrain FOOD cost: DoMovement's
' tick does  food -= enteredLocationId / 20  (code 5 = 0.25/step, 10 = 0.50,
' 15 = 0.75).  This is SEPARATE from the encounter rate: the encounter
' weight comes from CreatureApproach's per-tile ON GOSUB keyed on the RAW
' tile type (see out_encounter.bas), not from this classified value.

    SELECT CASE tileObjectType                        ' ds:2182           ' asm:11863-11935
    CASE 0            : enteredLocationId = 10   ' open ground             ' asm:11871
    CASE 1, 2         : enteredLocationId = 5    ' road / easy             ' asm:11888
    CASE 3, 4, 5      : enteredLocationId = 10                            ' asm:11906
    CASE 6           : enteredLocationId = 15   ' rough (forest / swamp)   ' asm:11918
    CASE 7 TO 13     : enteredLocationId = 5                              ' asm:11935
    CASE ELSE        : ' unchanged
    END SELECT
    '  -> food per step: code 5 = 0.25, code 10 = 0.50, code 15 = 0.75.
END SUB
