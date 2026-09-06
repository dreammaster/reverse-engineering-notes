' ==========================================================================
'  OUT.EXE  --  overworld encounter generation                       [v2]
'  reconstructed from out.asm ; see recovered/README.md for the model + tags
'
'  SUBs: CreatureApproach   (per-step: does a creature appear? how many?)
'        BeginEncounterView  (picks the creature TYPE + sets the combat view)
'  RollEncounterMod / rollCreatureStats live in out_combat.bas.
'
'  *** ds:2092 / ds:2096 are NOT runtime-loaded and NOT a formula. ***
'  They are assigned every step from a 5-row table of hardcoded constants,
'  selected by the map tile the player is standing on.  Full table below.
'  (Earlier notes asked for a DOSBox dump "per map" -- not needed; the
'   OUTM<n>.BSV banks do not carry these.)
' ==========================================================================
'
'  DGROUP vars:
'     contextMode 1F2A     facing 1E1E        characterLevel 1AE0
'     stepCounter 1AF8     rawTileType 214A (== 2182 == enteredLocationId
'                          before classifyLocationTile rewrites it)
'     encFreq   208E  (single) -- per-tile encounter weight  (the ON GOSUB
'                                 also parks it in the generic step scratch)
'     encGate1  2092  (single) -- tier-A probability   ("regionWeakGate")
'     encGate2  2096  (single) -- tier-B probability   ("regionTierGate")
'     groupSize 2192       creatureTypeBase 1F04       creatureIndex 2254
'     creaturesToFight 21FE
'  constants:
'     ds:2790 0.06   ds:2794 3.2   ds:2798 0.83   ds:279C 4.0
'     ds:2482 0.5    ds:24E6 1.0   ds:2552 500    ds:290E 256
'
'  --- the per-tile encounter presets (regionPreset_A..E, out.asm:0x1009C..) ---
'  ON GOSUB, 1-based, selector = rawTileType + 1, 8 entries -> 5 presets:
'
'     rawTile  preset   encFreq  encGate1  encGate2   (encGate2 '-' = untouched)
'        0       B        0.67     0.25      0.50
'        1       A        0.51     0.22      0.40
'        2       B        0.67     0.25      0.50
'        3       C        0.90     0.35      0.55
'        4       D        1.25     0.40      0.60
'        5       C        0.90     0.35      0.55
'        6       D        1.25     0.40      0.60
'        7       E        0.40    -1.00       -
'      8..n    (index out of range -> ON GOSUB is a no-op; presets unchanged)
'
'  Field samples (Paul, DOSBox) confirm the table exactly:
'     museum-adjacent : 2092=0.00 2096=0.00  (never left tiles >= 8 yet)
'     far west        : 2092=0.22 2096=0.40  = preset A  (rawTile 1)
'     north-west      : 2092=0.35 2096=0.55  = preset C  (rawTile 3 or 5)


' --------------------------------------------------------------------------
SUB CreatureApproach                                 ' asm: out.asm:3101 (creatureApproach)
' --------------------------------------------------------------------------
' Called once per step from outInit's main loop.

    IF contextMode > 0 THEN EXIT SUB                  ' already busy       ' asm:3102
    IF facing = 0 OR workIntHi <> -9 THEN EXIT SUB    ' not a real move    ' asm:3107-3125

    ' ---- load this tile's encounter preset ---------------------- asm:3133-3143
    '   ON (rawTileType + 1) GOSUB regionPreset_B,A,B,C,D,C,D,E
    '   -> sets encFreq (208E), encGate1 (2092), encGate2 (2096)
    SELECT CASE rawTileType
        CASE 0, 2 : encFreq = .67 : encGate1 = .25 : encGate2 = .50   ' preset B
        CASE 1    : encFreq = .51 : encGate1 = .22 : encGate2 = .40   ' preset A
        CASE 3, 5 : encFreq = .90 : encGate1 = .35 : encGate2 = .55   ' preset C
        CASE 4, 6 : encFreq = 1.25: encGate1 = .40 : encGate2 = .60   ' preset D
        CASE 7    : encFreq = .40 : encGate1 = -1.0                   ' preset E (encGate2 kept)
        CASE ELSE : ' rawTileType >= 8 : ON GOSUB out of range, no change
    END SELECT

    ' ---- the encounter trigger --------------------------------- asm:3160-3175
    '   asm:  FF4B encFreq ; RND ; FF4B ; ax = level+9 ; FF20 ; FF4C (mul) ;
    '         FF27 (INT/FIX!) ; FF1F ; ja -> return (no encounter)
    '   FF1F is CONFIRMED reversed (leglib loc_21BC0 `xchg si,di`), so the
    '   `ja` tests  TOS > TOS1  =  INT(RND*(level+9)) > encFreq :
    '        NO encounter  <=>  INT( RND(1) * (characterLevel + 9) ) > encFreq
    '        encounter     <=>  INT( RND(1) * (characterLevel + 9) ) <= encFreq
    '   INT(...) is a NON-NEGATIVE INTEGER and encFreq is 0.40..1.25, so:
    '     encFreq < 1  (0.40 / 0.51 / 0.67 / 0.90)  ->  encounter iff INT == 0
    '                                                   -> per-step ~ 1/(level+9)
    '     encFreq 1.25 (rough terrain / preset D)    ->  encounter iff INT in {0,1}
    '                                                   -> per-step ~ 2/(level+9)
    '   So encFreq is really just "1 or 2 buckets count", and higher level
    '   (bigger level+9) means FEWER encounters.  L1 road ~10%/step, L1
    '   rough ~20%/step, tapering as you level.
    IF INT( RND(1) * (characterLevel + 9) ) > encFreq THEN EXIT SUB       ' asm:3160-3175

    ' ---- rare scripted / tougher encounter --------------------- asm:3167-3227
    '   holding S2(15) AND stepCounter > 500 AND 2 <= level <= 7
    '   AND RND(1) < 0.06  ->  loc_14AA1 (banditAmbushEvent path)
    IF haveCompendium AND stepCounter > 500 AND characterLevel >= 2 _
       AND characterLevel <= 7 AND RND(1) < 0.06 THEN                     ' asm:3196-3227
        BanditAmbushEvent : EXIT SUB
    END IF

    contextMode = 10                                                     ' asm:3232

    ' ---- group size (capped at 7) ---------------------------- asm:3233-3252
    '   rt_14 = 32-bit \ ;  (stepCounter \ 2500) + 1  clamped to 7.
    groupSize = (stepCounter \ 2500) + 1                                 ' asm:3233-3252
    IF groupSize > 7 THEN groupSize = 7

    ' ---- how many of the group actually engage -------------- asm:3255-3279
    r = RND(1)
    creaturesToFight = INT( (r ^ (3.2 * r + 0.83)) * groupSize + 1 )     ' asm:3255-3279
    '   ds:2794 3.2 ; ds:2798 0.83 .  exponent 0.83..4 skews r low, so
    '   usually only 1..3 engage even for a big group.

    ' ---- approach direction (50% of the time) --------------- asm:3280-3306
    IF RND(1) <= 0.5 THEN                             ' ds:2482            ' asm:3280-3289
        approachDir = INT( RND(1) * 4.0 + 1.0 )       ' ds:279C 4.0
    END IF

    PRINT "UNKNOWN CREATURE"; PLURAL$(creaturesToFight)                  ' asm:3309-3336
    PRINT "APPROACHING FROM THE "; Direction$(approachDir)               ' asm:3339+
    BeginEncounterView
END SUB


' --------------------------------------------------------------------------
SUB BeginEncounterView                               ' asm: out.asm:4386 (beginEncounterView)
' --------------------------------------------------------------------------
' Also entered from quitOrTalk (re-open combat after a failed parley).
'
' encGate1 / encGate2 form a 3-way cascade that picks the creature BAND
' (typeBase + groupSize) that creatureIndex is then rolled inside.

    IF contextMode < 5 THEN backupContextMode = contextMode              ' asm:4388-4395
    contextMode = 11                                  ' "encounter view"  ' asm:4398

    IF RND(1) < encGate1 THEN                         ' ds:2092  tier A   ' asm:4399-4417
        ' ON (rawTileType + 1) GOSUB combatBeat_1..7  -- per-tile band:
        SELECT CASE rawTileType                                          ' asm:4420-4434
            CASE 0 : groupSize = 4 : creatureTypeBase = 3    ' combatBeat_1
            CASE 1 : groupSize = 6 : creatureTypeBase = 11   ' combatBeat_2
            CASE 2 : groupSize = 4 : creatureTypeBase = 17   ' combatBeat_3
            CASE 3 : groupSize = 4 : creatureTypeBase = 21   ' combatBeat_4
            CASE 4 : groupSize = 3 : creatureTypeBase = 25   ' combatBeat_5
            CASE 5, 6 : groupSize = 4 : creatureTypeBase = 28 ' combatBeat_6
            CASE 7 : groupSize = 4 : creatureTypeBase = 7    ' combatBeat_7
        END SELECT
    ELSE
        groupSize = 4 : creatureTypeBase = 3             ' default mid     ' asm:4438-4440
        IF RND(1) < encGate2 THEN                     ' ds:2096  tier B   ' asm:4441-4459
            IF RND(1) < 0.5 THEN                                          ' asm:4462-4474
                contextMode = 12
                groupSize = 3 : creatureTypeBase = 0     ' weak band 0..2 (pixie/strider/farmer)
            END IF
        END IF
    END IF

    ' ---- the creature type ------------------------------------ asm:4481-4513
    creatureIndex = INT( creatureTypeBase + RND(1) * groupSize )
    '  OUTDAT.DAT arrays indexed by creatureIndex (ds:209A word[], ds:20AC word[]):
    '    creatureDefense = A1(creatureIndex) \ 256   (ds:21FC ; ds:290E = 256)
    '    creatureAtk     = A1(creatureIndex) AND 255 (ds:2264 ; via rollCreatureStats)
    '    creatureWeak    = A2(creatureIndex) \ 256   (ds:22A6 ; 99 = no weakness)

    RollEncounterMod                                  ' out_combat.bas / rollCreatureStats

    ' ---- per-creature HP into viewObjectArray ----------------- asm: creatureDefeated area
    FOR slot = 1 TO creaturesToFight
        viewObjectArray(slot) = INT( creatureAtk * (RND(1) / 4.0 + 0.35) _
                                     * (S4(12) + 2) )
    NEXT slot
    ' ... draw the encounter view ...
END SUB


' ==========================================================================
'  SOLID
'   * encGate1 (2092) / encGate2 (2096) / encFreq (208E): a 5-row constant
'     table (regionPreset_A..E), selected by rawTileType via ON GOSUB.
'     FULLY RECOVERED -- table above.  No per-map / DOSBox data needed.
'   * creature band selection cascade (encGate1 -> per-tile combatBeat_*,
'     else mid, else encGate2 -> weak) -- structure verified from the asm.
'   * combatBeat_* (groupSize, creatureTypeBase) table -- verified.
'   * creaturesToFight  = INT( r^(3.2r + 0.83) * groupSize + 1 )
'   * approach direction = INT( RND(1)*4 + 1 )   (50% of the time)
'   * rare special = S2(15) + stepCounter>500 + level 2..7 + RND<0.06
'
'  OPEN
'   (encounter-trigger polarity is now RESOLVED -- FF1F confirmed reversed;
'    the FF27 INT truncation makes encFreq a 1-or-2-bucket threshold)
'   * groupSize's exact rt_14 divisor (2500 assumed from ds:09C4)
'   * whether creatureTypeBase gets a story-phase (S4(12)) bump anywhere
