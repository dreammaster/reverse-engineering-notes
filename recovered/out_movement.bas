' ==========================================================================
'  OUT.EXE  --  overworld movement + per-step upkeep                  [v2]
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
'     facing  1E1E   hitPoints 1ADA   food 1ACE
'     enteredTileType 2182 / 214A     enteredLocationId 1F02
'     tileObjectType  1F04  (0 = plain terrain -> run the tick;
'                            nonzero = a location/object -> chain out)
'     terrainWear  1AF4  (accumulates enteredLocationId/20 per step)
'     stepCounter  1AF8  (+1 per step; sickness when it passes 500)
'     rollWindow   2184  (set to 10 after a kill, 0 after sickness/entry;
'                         must be >= 1 for the per-step roll to fire)
'     ds:2584 = 3.0   ds:2552 = 500.0   ds:24EA = 20.0   ds:24E6 = 1.0


' --------------------------------------------------------------------------
SUB DoMovement                                        ' asm: out.asm:1427 (doMovement)
' --------------------------------------------------------------------------

    PRINT "MOVE "; Direction$(facing)                                     ' asm:1428-1454
    IF contextMode >= 10 THEN GOTO NonWorldMove        ' ds:1F2A          ' asm:1457-1459

    trialX = playerX : trialY = playerY                                   ' asm:1464-1467
    RandomizeStep facing                              ' rtm_FC           ' asm:1468-1469
    enteredTileType = MapWindow(trialY, trialX)                           ' asm:1470-1478
    ResolveMoveTarget trialX, trialY, turnActionFlag, remarkIndex, _
                      enteredTileType, facing                             ' asm:1479-1491
    S4(24) = enteredTileType                          ' checkpoint copy   ' asm:1492-1497
    enteredLocationId = ds1F02                                            ' asm:1498-1499

    IF tileObjectType <> 0 THEN GOTO TileHasObject     ' ds:1F04         ' asm:1500-1502

    ' ===================== the plain-terrain per-step tick =============
PlainStep:                                                                ' loc_10BB7
    enteredTileType = enteredLocationId                                   ' asm:1507-1508
    playerX = trialX : playerY = trialY               ' commit the move   ' asm:1509-1512
    ClassifyLocationTile                                                  ' asm:1513

    stepCost = enteredLocationId / 20.0                ' ds:24EA         ' asm:1514-1519
    food     = food - stepCost                         ' food drain      ' asm:1520-1525 '?ord (FF53)
    terrainWear = terrainWear + stepCost                                  ' asm:1526-1531
    stepCounter = stepCounter + 1                                         ' asm:1532-1537

    workIntHi = -9                                     ' ds:1F06         ' asm:1538
    IF rollWindow < 1 THEN EXIT SUB                                       ' asm:1539-1545
    rollWindow = rollWindow - 1                                           ' asm:1549

    ' ---- sickness roll (food poisoning from "flesh for food") ------
    '   sick unless ( RND(1) < ds:2186 )  OR  ( stepCounter < 500 )
    IF RND(1) >= ds2186 AND stepCounter >= 500.0 THEN     ' ds:2552 = 500 ' asm:1550-1578 '??
        PRINT "YOU GROW SICK FROM SOMETHING YOU ATE!"                     ' asm:1586-1607
        sickDmg  = INT( hitPoints / (3.0 * (RND(1) + 1.0)) )  ' ds:2584   ' asm:1608-1622
        hitPoints = hitPoints - sickDmg                                   ' asm:1623
        rollWindow = 0                                                    ' asm:1624
        PRINT "HIT POINTS: -"; sickDmg                                    ' asm:1625-1640
        Delay 3 : Tone 600 : StageSfx_Bump                                ' asm:1641-1649
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
' enteredLocationId (ds:1F02) -- which IS the terrain cost: DoMovement's
' tick does  food -= enteredLocationId / 20  and the encounter trigger is
' stepScratch (= enteredLocationId/20) <= RND(1)*(level+9).  So a higher
' code means BOTH more food per step AND fewer encounters.

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
