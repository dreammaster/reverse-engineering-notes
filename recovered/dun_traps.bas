' ==========================================================================
'  DUN.EXE  --  traps + hazards                                       [v1]
'  reconstructed from dun.asm ; see recovered/README.md for the model + tags
'
'  SUBs: MoveHazards (trap triggered by walking onto it)
'        DoLookSearch (the Look / Search command -- reveals hidden traps)
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
'   base = (RND(1) * 50 + 10) * (dungeonLevel ^ 1.6)
'   then * 0.6 (and a +1/-1 tweak keyed on sprungCode = 10, i.e. the tile
'   was FLOOR HOLE)
    fallDmg = INT( (RND(1) * 50.0 + 10.0) * (dungeonLevel ^ 1.6) * 0.6 )  ' asm:1148-1210 '??
    '  ds:22FE 50 ; ds:2302 10 ; ds:2306 1.6 ; ds:230A 0.6 .
    '  L1: ~6..36 ; L4: heavy.  The exact role of the ds:2274 term and the
    '  20C2==10 mask still need a trace.                                    'CHECK
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


' ==========================================================================
'  SOLID
'   * trap tile codes 1..7 hidden, +8 = revealed/sprung; Search springs
'   * FLOOR HOLE (2): "YOU FALL THROUGH A HIDDEN HOLE" -> fall damage +
'     drop one dungeon level
'   * every other hidden trap -> "YOU'RE AMBUSHED BY A <monster>"
'   * fall damage scales as dungeonLevel ^ 1.6  (deep = deadly)
'
'  OPEN
'   * exact fall-damage tail (ds:2274 / ds:230A / the 20C2==10 mask)
'   * the ambush-monster selection
'   * ProcessTileFeature's chest/box loot roll, the level-clamp maths at
'     dun.asm:5106-5147, CEILING HOLE / trip-wire specifics
