' ==========================================================================
'  OUT.EXE  --  overworld setup / map loading                        [v1]
'  reconstructed from out.asm (enterOverworld / loadOverworldData +
'  initOverworldViewport / initOverworldState / sub_12823 / sub_116E1)
'  ; see recovered/README.md for the model + tags
'
'  enterOverworld runs once from outInit and again every time a chained
'  module (TWNDR / CASDR / MUS / DUN) hands control back to OUT.  It (re)loads
'  the map banks, recomputes the level-derived max HP, resets the per-turn
'  overworld state, and -- if you are returning onto a location tile --
'  runs the arrival handler.
'
'  THE MAP LAYER  --  S4(12)  (S4 byte offset 0x18, ds:1B96 elem 12)
'  ------------------------------------------------------------------
'  resolveMoveTarget stashes a "pending map layer" in ds:2180 after every
'  step; doMovement copies it into S4(12).  On the next enterOverworld:
'      mapLayer (ds:2192) = MIN( S4(12), 2 )
'  and that picks the OUTM file:
'      0 -> OUTM0.BSV   (the main overworld)
'      1 -> OUTM1.BSV   (a secondary area)
'      2 -> OUTM2.BSV   (+ PEGASUS.BSV -- the pegasus-flight view)
'  S4(12) > 2 is a "re-entry" sentinel: teleport to (7,5) and arrive.
'
'  DGROUP vars:
'     mapLayer      2192   (0..2, = MIN(S4(12),2); the notes' "combatPhase")
'     overworldArray 1E2A  (far ptr -- OUTM* + OUTDATA load here contiguously)
'     spriteBank     1E58  (far ptr, seg 0x140D -- OUTOBJ + PEGASUS load here)
'     videoMode 2404  videoColour 2406   (from the OUTM header, -> rt_FE29)
'     armourRating 2266   weaponRating 22D6   (derived by sub_12823)
'     locationType 1E20   turnActionFlag 212E   contextMode 1F2A
'     playerX 1B02   playerY 1B06     hitPoints 1ADA   maxHP S4(19)
'     characterLevel 1AE0
'     foodSickCtr 1AF8   terrainWearCtr 1AF4   (per-step depleting accumulators)
'  constants (OUT.EXE DGROUP base 0x8C80):
'     ds:2FE4 25000.0   ds:2970 3.5   ds:2974 2.8
'     ds:2FCC "a defghi m p s u wx"  (the overworld command-key whitelist)


' --------------------------------------------------------------------------
SUB EnterOverworld                                   ' asm: out.asm:9032 (enterOverworld)
' --------------------------------------------------------------------------
    CommandKeys$ = "a defghi m p s u wx"             ' ds:1E8A -- valid "O"verworld keys

    sub_12823                                         ' derive armourRating / weaponRating
    RollCreatureStats                                 ' pre-roll the wandering creature
    SetupLocationDisplay                              ' the 10..12 location-type indicators

    ' ---- choose the map layer from the pending-transition slot ----------
    mapLayer = S4(12)                                 ' asm:loc_1487E
    IF mapLayer > 2 THEN mapLayer = 2                 ' clamp for the filename / file set

    LoadOverworldData                                 ' BLOAD OUTM<layer> / OUTDATA / OUTOBJ (/PEGASUS)

    ' ---- reset the per-turn overworld state ---------------------------- asm:loc_148A4
    ds2184        = 0
    contextMode   = 0                                 ' ds:1F2A
    ds2148        = 0
    locationType  = &h0B                              ' ds:1E20 -- 11 = "on the open overworld"
    ds1F1A        = 0
    turnActionFlag = S4(9)                            ' ds:212E  <- persistent S4(9)

    ' ---- clamp the two per-step accumulators to a 25000 ceiling ------- asm:loc_148CE
    IF foodSickCtr   > 25000.0 THEN foodSickCtr   = 25000.0   ' ds:1AF8, ds:2FE4
    IF terrainWearCtr > 25000.0 THEN terrainWearCtr = 25000.0 ' ds:1AF4

    initOverworldState                                ' RECOMPUTE S4(19) = maxHP from level
    S4(34) = 0                                        ' asm:loc_1492D -- reset a counter

    rt_FE6B(10, 8)                                    ' viewport window params (ds:241E/2420)
    DrawStatusBar
    rt_FE49(-2)                                       ' cursor / palette (ds:2422)

    ' ---- arrival: only when returning via the >2 sentinel ------------- asm:loc_1496B
    IF S4(12) > 2 THEN
        playerX = 7 : playerY = 5
        HandleOverworldArrival
    END IF

    ' ---- death-on-return housekeeping -------------------------------- asm:loc_14991
    IF S4(12) <> 2 AND hitPoints <= 0 THEN S2(15) = 0 ' clear a held-item slot on death

    IntegrityCheckAndCrippleMaxHP                     ' asm:loc_149CE .. (the OUT.EXE checksum)
    sub_116E1                                         ' apply its verdict
    GOTO EnterLocationOrChain                         ' act on enteredLocationId (may chain out)
END SUB


' --------------------------------------------------------------------------
SUB LoadOverworldData                                ' asm: out.asm:8739 (loadOverworldData)
' --------------------------------------------------------------------------
' BLOADs the four (sometimes five) overworld banks.  rt_FE63 resolves the
' drive per DRCONFIG.DAT and opens; rt_FE07 reads the 7-byte BSAVE header
' + payload into the target BASIC array.  Every name is tried upper- then
' lower-case (DOS is case-insensitive; the pair is a portability relic).
'
    layerChar$ = CHR$(mapLayer + ASC("0"))            ' "0" / "1" / "2"       asm:14625

    '   OUTM<layer>.BSV  ->  overworldArray[0]                            asm:loc_1462F
    rt_11 overworldArray                              ' target-array descriptor
    name$ = "OUTM" + layerChar$ + ".BSV"
    rt_FE63 name$
    rt_FE07 "outm" + layerChar$ + ".bsv", overworldArrayOffset(0)

    '   OUTDATA.BSV  ->  overworldArray[0x2B22]                           asm:loc_146BC
    rt_FE63 "OUTDATA.BSV"
    rt_FE07 "outdata.bsv", overworldArrayOffset(&h2B22)

    '   video mode + palette live in the OUTM header (array words 2 and 4) asm:loc_1470F
    videoMode   = overworldArray(4)                   ' ds:2404
    videoColour = overworldArray(2)                   ' ds:2406
    rt_FE29 videoMode, videoColour                    ' the CGA 3D8/3D9 + INT 10h setter
    overworldArray(2) = videoColour                   ' (write the pair back -- FE29 side-effects)
    overworldArray(4) = videoMode

    '   OUTOBJ.BSV  ->  spriteBank[0]                                     asm:loc_1477E
    rt_11 spriteBank
    rt_FE63 "OUTOBJ.BSV"
    rt_FE07 "outobj.bsv", spriteBankOffset(0)

    '   PEGASUS.BSV  ->  spriteBank + spriteBank[8]   (layer 2 only)      asm:loc_147EB
    pegasusDest = spriteBankOffset(0) + spriteBank(8)
    IF mapLayer = 2 THEN
        rt_FE63 "PEGASUS.BSV"
        rt_FE07 "pegasus.bsv", pegasusDest
    END IF

    ' ---- draw ---------------------------------------------------------- asm:loc_1482D
    IF mapLayer = 0 THEN
        initOverworldViewport                         ' draws layer 0, centred on the player
    ELSE
        DrawOverworldViewport S4(12)                  ' layer 1/2: draw with the raw layer id
    END IF
END SUB


' --------------------------------------------------------------------------
SUB initOverworldViewport                            ' asm: out.asm:8702
' --------------------------------------------------------------------------
' Draw the main map, then -- if a non-zero layer was left pending -- snap
' it back to 0.  (So OUTM1 / OUTM2 are one-shot: the pegasus flight lands
' you and the next overworld entry is the main map again.)
    DrawOverworldViewport 0
    IF S4(12) <> 0 THEN
        S4(12) = 0
        mapLayer = 0
    END IF
END SUB


' --------------------------------------------------------------------------
SUB initOverworldState                               ' asm: out.asm:10014 (initOverworldState)
' --------------------------------------------------------------------------
' Recompute the level-derived max HP into S4(19).  Runs on every overworld
' entry, so a museum-caretaker level-up takes effect at once.  Identical
' formula to mus_caretaker.bas.
'     hp = 200
'     FOR i = 1 TO characterLevel : hp = hp + 100*(i-1) : NEXT
'     '  == 200 + 50*L*(L-1)
'     IF characterLevel > 5 THEN hp = hp - 100
'     S4(19) = hp
'   L1=200 L2=300 L3=500 L4=800 L5=1200 L6=1600 L7=2200 ... L10=4600
    hp = 200
    FOR i = 1 TO characterLevel
        hp = hp + 100 * (i - 1)
    NEXT
    IF characterLevel > 5 THEN hp = hp - 100
    S4(19) = hp
END SUB


' --------------------------------------------------------------------------
SUB sub_12823                                        ' asm: out.asm:5018
' --------------------------------------------------------------------------
' Derive the two equipment ratings the overworld combat code reads.
'   armourRating (ds:2266): 0, or if an armour slot is equipped
'       INT( S1(armourCondIdx) / 3.5 + (armourId - 9) )        ' ds:2970 = 3.5
'   weaponRating (ds:22D6): weaponId, or if a weapon slot is equipped
'       INT( weaponId + S1(weaponCondIdx) / 2.8 )              ' ds:2974 = 2.8
'   S1 = the 0..4 condition array (ds:1B68) ; armourId 9..13, weaponId 0..8.
    armourRating = 0
    IF armourCondCursor < 8 THEN _
        armourRating = INT( S1(armourCondCursor) / 3.5 + (armourId - 9) )
    weaponRating = weaponId
    IF weaponSlotCursor < 8 THEN _
        weaponRating = INT( weaponId + S1(weaponSlotCursor) / 2.8 )
END SUB


' --------------------------------------------------------------------------
SUB HandleOverworldArrival                           ' asm: out.asm:9305
' --------------------------------------------------------------------------
' Reached only from the S4(12) > 2 re-entry path.
    S4(12) = 2                                        ' pin the layer
    EnterLocationOrChain                              ' handle / chain the current tile
    ds1F1A = 0
    PegasusOrAmbush                                   ' "PEGASUS SETS YOU DOWN" / "AMBUSHED BY BANDITS!"
END SUB


' --------------------------------------------------------------------------
'  IntegrityCheckAndCrippleMaxHP + sub_116E1        ' asm: out.asm ~9195 / 2963
' --------------------------------------------------------------------------
' A 1989-era anti-tamper check.  enterOverworld re-opens OUT.EXE as a
' random-access file (rt_87 "out.exe", reclen 0x20), reads two byte ranges
' (records 0x1418.. for 0x18, then 0x6419.. for 0x64), sums them into
' ds:2236, and closes (rt_09).  Then sub_116E1:
'       IF ds:2236 <> &h9D1A THEN S4(19) = 20        ' cripple max HP
' i.e. if the executable has been patched, the party's max HP collapses to
' 20 on the next overworld entry.  Not gameplay -- a copy-protection trap.


' ==========================================================================
'  SOLID
'   * map layer = MIN(S4(12), 2) -> OUTM0 / OUTM1 / OUTM2 (+ PEGASUS on 2)
'   * file set: OUTM<n> -> overworldArray[0] ; OUTDATA -> overworldArray[0x2B22]
'     ; OUTOBJ -> spriteBank[0] ; PEGASUS -> spriteBank + spriteBank[8]
'   * OUTM header word 2 = CGA colour, word 4 = video mode -> rt_FE29
'   * S4(19) maxHP = 200 + 50*L*(L-1) - (100 if L>5), recomputed here
'   * enterOverworld resets: contextMode, locationType=0x0B, turnActionFlag
'     <- S4(9), S4(34)=0 ; clamps foodSickCtr / terrainWearCtr to 25000
'   * armourRating = INT(S1(cond)/3.5 + armourId-9) ;
'     weaponRating = INT(weaponId + S1(cond)/2.8)
'   * layer 1/2 are one-shot (initOverworldViewport snaps S4(12) back to 0)
'   * OUT.EXE self-checksum: mismatch (ds:2236 <> 0x9D1A) -> S4(19) = 20
'
'  OPEN
'   * the exact record layout the checksum loop walks (rt_87/5E/5F/09
'     semantics) -- only the verdict constant 0x9D1A is pinned
'   * rt_FE6B(10,8) / rt_FE49(-2) exact effect (viewport window / cursor)
'   * what OUTM1 actually is (OUTM2 = pegasus; OUTM0 = main; OUTM1 unclear)
'   * ds:2180 (resolveMoveTarget's pending-layer output) full value range
' ==========================================================================
