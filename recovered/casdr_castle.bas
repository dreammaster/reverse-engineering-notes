' ==========================================================================
'  CASDR.EXE  --  castle / fortress interiors                         [v1]
'  reconstructed from casdr.asm ; see recovered/README.md for the model + tags
'
'  SUBs: AttackHit (incoming melee), GasDamage, WarlordAttack,
'        DescribeRoom + DescribeChest / DescribeGasRoom / DescribePotionShop /
'        DescribeLockedDoor, FortressSelfDestruct, WarlordConfrontation
'
'  CASDR DGROUP:0 = CASDR.EXE file offset 0x84C0.
'  RND(1) = `push ds:25B2 : push ds:25B0 : call B$RND`  (ds:25B0 = 1.0)
' ==========================================================================
'
'  DGROUP vars (CASDR):
'     hitPoints 1ADA   Endurance 1ACC   armorId 1AEC   enemyHitPoints 2222
'     playerX/Y (castle grid)   destructTimer (0x14 / 8)
'     ds:28B2 1.8   ds:28B6 600   ds:28BA 300   ds:28BE 0.9   ds:2744 2
'     ds:28DA 50    ds:28A0 99    ds:2B94 80    ds:2084 enemyAtk (runtime)
'     ds:226E  difficulty = 3.5 (castle, ds:31A8) / 1.0 (fort, ds:25B0)


' --------------------------------------------------------------------------
SUB AttackHit                                         ' asm: casdr.asm:4166 (attackHit)
' --------------------------------------------------------------------------
' A castle enemy's melee blow lands.  Endurance and armour both mitigate,
' in the DENOMINATOR (so they scale defensively, not linearly).

    raw = (enemyAtk ^ 1.8) * (RND(1) * 600 + 300) * difficulty            ' asm:4167-4212
    '  ds:28B2 1.8 ; ds:28B6 600 ; ds:28BA 300
    '  difficulty (ds:226E, set by loadCastleLevel casdr.asm:10033):
    '     3.5 inside the castle (ds:31A8) ; 1.0 inside the fort (ds:25B0)

    IF armorId > 0 THEN armorVal = armorId - 6 ELSE armorVal = armorId + 2 ' asm:4221-4231
    '  (mask*8 + armorId + 2 : with armour  armorId-6, without  2)

    dmg = INT( raw / (armorVal * Endurance ^ 0.9) + 2 )                   ' asm:4235-4267
    '  ds:28BE 0.9 ; ds:2744 2 .  Higher Endurance / armour -> smaller dmg.
    hitPoints = hitPoints - dmg                                           ' asm:4270-4271
    PRINT "HIT POINTS: -"; dmg                                            ' asm:4272-...
END SUB


' --------------------------------------------------------------------------
SUB GasDamage                                         ' asm: casdr.asm:4376 (gasDamage)
' --------------------------------------------------------------------------
' The cloudy-air trap rooms tick this each turn spent inside.
    dmg = INT( RND(1) * 50.0 + base )      ' ds:28DA 50 ; base from stack   ' asm:4377-4397 '??
    hitPoints = hitPoints - dmg                                           ' asm:4414 -> loc_11EB7
    PRINT "GAS DAMAGE: -"; dmg
END SUB


' --------------------------------------------------------------------------
SUB WarlordAttack                                     ' asm: casdr.asm:6034 (warlordAttack)
' --------------------------------------------------------------------------
' The Warlord's blow -- announced ("WARLORD ATTACK - BLOW n"); the
' subtraction from hitPoints is applied by the caller (doWalk /
' castleTurnUpdate) with this value.
    warlordBlow = INT( RND(1) * 99.0 + 80.0 )     ' ds:28A0 99 ; ds:2B94 80 ' asm:6036-6066
    '  80 .. 178 damage per blow.
    PRINT "WARLORD ATTACK - BLOW "; warlordBlow
END SUB


' --------------------------------------------------------------------------
SUB DescribeRoom                                      ' asm: casdr.asm:6187 (describeRoom)
' --------------------------------------------------------------------------
' SELECT CASE on the room the player faces:
'   TREASURE CHEST   -> DescribeChest   ("YOU SEE A TREASURE CHEST.")
'   LOCKED DOOR      -> DescribeLockedDoor ("A MASSIVE DOOR LOOMS ... LOCKED")
'   GAS ROOM         -> DescribeGasRoom ("BARREN ROOM ... AIR LOOKS CLOUDY")
'                       -> temporarily forces game-speed to 5, ticks GasDamage
'   POTION SHOP      -> DescribePotionShop ("A LOVELY YOUNG WOMAN ...")
'                       -> the potionWizard's room (+5 END / +36 DEX quest)
' The rooms themselves are pure text; the effects live in the trap /
' shop / chest handlers.


' ==========================================================================
'  SOLID
'   * castle incoming melee: dmg = INT( enemyAtk^1.8 * (RND*600 + 300)
'       * difficulty / (armorVal * Endurance^0.9) + 2 )
'     -- Endurance and armour mitigate as a DENOMINATOR term
'   * Warlord blow: INT( RND(1)*99 + 80 )   (80..178)
'   * gas room: ~INT( RND(1)*50 + base )    per turn inside
'   * FLOOR-plan rooms are a SELECT CASE; effects are separate handlers
'
'  OPEN
'   * gas damage `base` (stack leftover), chest-loot roll, locked-door key
'   * WarlordConfrontation script + FortressSelfDestruct countdown (0x14 / 8)
'   * doFight / enemyAttack (the player's castle attack + non-warlord enemies)
