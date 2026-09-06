' ==========================================================================
'  DUN.EXE  --  traps + hazards + stairs                              [v2]
'  reconstructed from dun.asm ; see recovered/README.md for the model + tags
'
'  SUBs: MoveHazards (trap triggered by walking onto it)
'        DoLookSearch (the Look / Search command -- reveals hidden traps)
'        Climb (the "CLIMB" command -- climbUp -> climbDownOrExit) +
'        DungeonExit (climbing up off level 0)
'  ProcessTileFeature (dun.asm:5065) = level-entry setup + feature dispatch,
'  partly reconstructed.
' ==========================================================================
'
'  Feature codes on a dungeon tile (ds:20C4 = tileAhead):
'     0        floor
'     1..7     HIDDEN trap  (POISON GAS VENT / FLOOR HOLE / SLIME SPLOTCH /
'              TRIP WIRE / CEILING HOLE / TREASURE CHEST / BOX)
'     9..15    the same trap, REVEALED / already sprung  (= code + 8)
'  Search adds 8 to a 1..7 tile (springs / reveals it); walking onto a 1..7
'  tile triggers it; walking onto a 9..15 tile is safe ("YOU AVOID THE ...").
'
'  DUN DGROUP:0 = DUN.EXE file offset 0x5F00.
'  ds:22FE = 50   ds:2302 = 10   ds:2306 = 1.6   ds:230A = 0.6


' --------------------------------------------------------------------------
SUB MoveHazards                                       ' asm: dun.asm:1040 (moveHazards)
' --------------------------------------------------------------------------
' Called when the player steps onto the tile ahead.

    IF tileAhead > 7 THEN                             ' already sprung     ' asm:1044-1046
        PRINT "YOU AVOID THE "; Feature$(tileAhead)                        ' asm:1049-1069
        EXIT SUB
    END IF

    sprungCode = tileAhead + 8                        ' ds:20C2           ' asm:1073-1075

    IF tileAhead = 2 THEN                             ' FLOOR HOLE         ' asm:1076-1077
        PRINT "YOU FALL THROUGH A HIDDEN HOLE."                            ' asm:1082
        FallThroughDamage                            ' loc_106B3          ' asm:1093
        DropOneLevel                                 ' sub_10336          ' asm:1095
        Tone 1000 : EXIT SUB                                              ' asm:1100-1104
    END IF

    ' every other hidden trap springs a monster ambush
    PRINT "YOU'RE AMBUSHED BY A "; Creature$(ambushMonster)               ' asm:1108-1113
    SpawnAmbush ambushMonster
END SUB


' --------------------------------------------------------------------------
SUB FallThroughDamage                                 ' asm: dun.asm:1147 (loc_106B3)
' --------------------------------------------------------------------------
'   base = (RND(1) * 50 + 10) * (dungeonNumber ^ 1.6)      ' ds:1ACA, NOT the level
'   then * 0.6 when featureUnderfoot (ds:20C2) == &h0A  (you dropped via
'   the down-stairs area rather than a bare FLOOR HOLE)
    fallDmg = INT( (RND(1) * 50.0 + 10.0) * (dungeonNumber ^ 1.6) _
                   * (0.6 IF featureUnderfoot = &h0A ELSE 1.0) )        ' asm:1148-1210
    '  ds:22FE 50 ; ds:2302 10 ; ds:2306 1.6 ; ds:230A 0.6 ; DUN base 0x5F00.
    '  dungeon 1: ~10..60 ; dungeon 3: ~50..180.
    hitPoints = hitPoints - fallDmg
END SUB


' --------------------------------------------------------------------------
SUB DoLookSearch                                      ' asm: dun.asm:1916 (doLookSearch)
' --------------------------------------------------------------------------
' The Look / Search command.

    turnActionFlag = 1                                                    ' asm:1917
    ' ... rebuild the forward view ...
    IF tileAhead > 0 AND tileAhead < 8 THEN           ' a hidden trap      ' asm:1969-1979
        featureRevealed = -1                          ' ds:1F06           ' asm:1974-1976
        tileAhead = tileAhead + 8                     ' reveal it          ' asm:1983
        WriteMapTile playerAhead, tileAhead          ' rtm_FE41           ' asm:1984-1988
    END IF
    IF tileAhead = 8 OR (tileAhead > 8 AND featureRevealed) THEN          ' asm:1993-2013
        containerAhead = tileAhead                   ' 8 = visible chest / box
    END IF
END SUB


' --------------------------------------------------------------------------
SUB Climb                                             ' asm: dun.asm:1369 climbUp -> :1422 climbDownOrExit
' --------------------------------------------------------------------------
' The "CLIMB" command.  featureUnderfoot = ds:20C2 ; dungeonPos = ds:1AE2
' (packed  level<<8 | cell) ; dungeonNumber = ds:1ACA (1..3).
'
'   IF featureUnderfoot <> &h0A AND featureUnderfoot <> &h0D THEN
'       PRINT "NOTHING TO CLIMB" : EXIT SUB
'   END IF
'
'   ' -- direction from which staircase you are on --
'   IF featureUnderfoot = &h0A THEN               ' stairs DOWN
'       PRINT "DOWN" : step = +&h100               ' one level deeper
'   ELSE                                          ' &h0D = stairs UP
'       PRINT "UP"   : step = -&h100               ' one level shallower
'   END IF
'
'   dungeonPos = dungeonPos + step                ' == the FLOOR HOLE trap's path too
'   IF dungeonPos < 0 THEN DungeonExit : EXIT SUB ' climbed up off level 0
'
'   ' -- ordinary level change --
'   PRINT "YOU ARE NOW AT LEVEL "; (dungeonPos \ &h100 + 1)
'   LoadDungeonMonsters                           ' reload the sprite band for the new level
'   featureUnderfoot = &h0A + &h0D - featureUnderfoot   ' toggle &h0A <-> &h0D
'                                                 ' (you arrive on the reciprocal staircase)
'   LoadDungeonData                               ' sub_12E7D -- the new level's map
END SUB


' --------------------------------------------------------------------------
SUB DungeonExit                                       ' asm: dun.asm:1527 (climbDownOrExit loc_108F7)
' --------------------------------------------------------------------------
' Reached only by climbing UP off dungeon level 0.
'
'   PRINT "YOU CLIMB OUT OF THE DUNGEON."
'
'   ' --- award this dungeon's quest-flag bit (ON dungeonNumber GOSUB) ---
'   flagBits = 0
'   SELECT CASE dungeonNumber
'   CASE 1 : IF S2(16) > 0 AND S2(20) > 0 THEN flagBits = &h0010   ' bit 4
'   CASE 2 :                                    flagBits = &h0100   ' bit 8 (unconditional)
'   CASE 3 : IF S2(14) > 3               THEN flagBits = &h0800   ' bit 11 (all 4 guard jewels)
'   END SELECT
'   questFlagWord(S4(11)) = questFlagWord OR flagBits              ' byte 0x16 ; also -> ds:20D2
'
'   ' --- the STRENGTH FLOOR (only when this dungeon awarded a bit) ---
'   IF flagBits <> 0 THEN
'       strFloor = 10*dungeonNumber + IIF(dungeonNumber > 1, 20, 15)   ' 25 / 40 / 50
'       IF strFloor > Strength(ds:1B08) THEN
'           PRINT "STRENGTH:  +"; (strFloor - Strength)
'           PlayFanfare 2000                                          ' rt_FE54 ; Delay &h1B
'           Strength = strFloor
'       END IF
'   END IF
'
'   ' --- chain out (ON dungeonNumber GOSUB) ---
'   '   dungeon 1 -> OUT.EXE  ;  dungeons 2 & 3 -> MUS.EXE
END SUB


' ==========================================================================
'  SOLID
'   * trap tile codes 1..7 hidden, +8 = revealed/sprung; Search springs
'   * FLOOR HOLE (2): "YOU FALL THROUGH A HIDDEN HOLE" -> fall damage +
'     drop one dungeon level  (shares climbDownOrExit's level-change path)
'   * every other hidden trap -> "YOU'RE AMBUSHED BY A <monster>"
'   * fall damage = INT((RND*50+10) * dungeonNumber^1.6 * {0.6 on the
'     down-stairs tile / 1.0 on a bare hole})  -- ds:22FE/2302/2306/230A
'   * CLIMB: only on a staircase tile (&h0A down / &h0D up) ; dungeonPos
'     (ds:1AE2) += &h100 ; the two stair tiles toggle so you land on the
'     reciprocal staircase ; LoadDungeonMonsters + LoadDungeonData per level
'   * DUNGEON EXIT (climb up off level 0): awards a quest-flag bit
'     (D1 bit 4 if S2(16)&S2(20) held, D2 bit 8 always, D3 bit 11 if
'     S2(14) > 3) ; if a bit was awarded, raises Strength to a FLOOR of
'     25 / 40 / 50 (= 10*dn + 15/20/20) ; chains D1->OUT, D2/D3->MUS
'
'  RESOLVED 2026-09-07
'   * fall damage = INT((RND*50+10)*dungeonNumber^1.6*mask), mask = 0.6
'     when featureUnderfoot (ds:20C2) == 0x0A else 1.0.  DUN base 0x5F00.
'   * chest/box loot roll -> dun_chest.bas (booby-trap 3%, loot 84%)
'
'  OPEN (minor)
'   * the exact per-level ambush-monster index (pulled from the level's
'     monster set -- a table lookup, no formula)
'   * CEILING HOLE / trip-wire distinct effects (vs the generic ambush)
