' ==========================================================================
'  CASDR.EXE  --  castle / fortress interiors                         [v1]
'  reconstructed from casdr.asm ; see recovered/README.md for the model + tags
'
'  SUBs: DoFight (player attack), EnemyAttack, AttackHit (incoming melee),
'        GasDamage, WarlordAttack, DescribeRoom + DescribeChest /
'        DescribeGasRoom / DescribePotionShop / DescribeLockedDoor,
'        FortressSelfDestruct, WarlordConfrontation
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
'     DoFight consts: ds:2724 7500  ds:2742 7  ds:2744 2  ds:270E 0.53
'                     ds:2712 6  ds:27C4 -7.5  ds:27C8 28  ds:31A4 26
'     ds:2214  = Dexterity/26 (castle) / 1.0 (fort)   [loadCastleLevel]
'     ds:1F02 tileHit   ds:1F04 targetCode   ds:1F06 rangeToTarget
'     ds:1AEE attackMode (0 weapon / 1 spell)   ds:1E24 selectedSpell (0..5)


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
SUB DoFight                                           ' asm: casdr.asm:2808 (doFight)
' --------------------------------------------------------------------------
' The player's castle attack ("F"ight).  Prompts "FIGHT WITH <weapon>" then
' "ENTER DIRECTION:"; a sub-menu can switch to a spell.  traceCombatLine
' projects along the chosen direction and sets tileHit (ds:1F02) /
' targetCode (ds:1F04) / range.  Then geometry checks (arrow drops, you
' surprise a guard, a bow shot MISSED at range, you HIT A DOOR ...), then
' the to-hit + damage below.
'
'   attackMode  ds:1AEE   0 = weapon, 1 = spell   (set by the spell sub-menu)
'   weaponId    ds:1AFE   (0..8 ; ids 6 & 8 are the two bows)
'   spell slot  = menuChoice + 15  ->  S2(24..29) charge, like DUN.
'                 selectedSpell (ds:1E24) = menuChoice - 9  (0..5)
'   K           ds:2214   loaded by loadCastleLevel (casdr.asm:10040):
'                 castle -> Dexterity / 26      fort -> 1.0
'                 [FF49 imm = TOS / imm, per the beginEncounterView
'                  precedent -- but that makes higher Dex slightly LOWER
'                  the castle hit-rate, which is odd; flagged for a trace]

    IF attackMode = 0 THEN
        ' ---- WEAPON to-hit ---------------------------------- asm:loc_1176C
        toHit = (11 * weaponId + 99) * (Dexterity + 13) / (7500.0 * K)
        IF RND(1) < toHit THEN GOTO WeaponHit
        PRINT "ATTACK ON "; Enemy$; " MISSED." : EXIT SUB
    ELSE
        ' ---- SPELL to-hit ---------------------------------- asm:loc_116D5
        S2(menuChoice + 15) = S2(menuChoice + 15) - 1          ' consume charge
        IF RND(1) * 6.0 >= Intelligence ^ 0.53 THEN            ' ds:270E 0.53, ds:2712 6
            PRINT Spell$(selectedSpell); " FIZZLES." : EXIT SUB
        END IF
        GOTO SpellHit
    END IF

WeaponHit:
    base = (weaponId \ 2 + 1) * Strength \ 7                    ' ds:2742 = 7
    dmg  = INT( base * (1.0 + 2.0 * RND(1)) )                   ' ds:2744 = 2
    '   Knife (id 1) / Str 15  -> base 2   -> 2..6
    '   Compound bow (id 8) / Str 20 -> base 14 -> 14..42
    GOTO ApplyBlow

SpellHit:
    ' spell base damage, computed at menu time (asm:loc_11B06):
    dmg = INT( (selectedSpell + 24 - 22.5) * 28.0 * (RND(1) + 1.0) )
    '   == OUT's SpellAttack shape, INT((selectedSpell - 22.5)*K*(RND+1)),
    '   with K = 28 here (vs 15 in OUT).  selectedSpell here is 0..5, so
    '   use (selectedSpell + 24) for the OUT-numbered value.
    IF inCastle THEN dmg = dmg \ 5                              ' castle: /5
    dmg = dmg \ rangeToTarget                                  ' ds:1F06 falloff
    GOTO ApplyBlow

ApplyBlow:
    viewObjectArray(tileHit) = viewObjectArray(tileHit) - dmg  ' asm:loc_118B6
    PRINT "ATTACK ON "; Enemy$; " STRUCK "; dmg; " H.P. BLOW"
    IF viewObjectArray(tileHit) <= 0 THEN PRINT Enemy$; " KILLED"
END SUB


' --------------------------------------------------------------------------
SUB EnemyAttack                                       ' asm: casdr.asm:5683 (enemyAttack)
' --------------------------------------------------------------------------
' A non-Warlord castle enemy's blow.  "<enemy> ATTACK - BLOW <n>"; the
' caller subtracts <n> from hitPoints (same shape as WarlordAttack).
' Computed with 32-bit math (rt_14 / rt_FF21 / rt_FF28) from a per-enemy
' attack stat (ds:20B8) and a running accumulator -- exact formula not
' fully pinned (the function is entered mid-expression).  *partial*


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
'   * PLAYER attack (DoFight):
'       weapon to-hit  RND(1) < (11*weaponId + 99)*(Dex + 13) / (7500*K)
'                      K = Dex/26 (castle) or 1.0 (fort)   [ds:2214]
'       weapon damage  INT( base * (1 + 2*RND(1)) ),
'                      base = (weaponId\2 + 1) * Strength \ 7
'       spell  cast succeeds when RND(1)*6 < Intelligence^0.53
'       spell damage   INT( (selectedSpell - 22.5) * 28 * (RND(1)+1) ),
'                      then \5 in the castle, then \range
'       damage -> viewObjectArray(tileHit) ; "<n> H.P. BLOW" / "KILLED"
'
'  OPEN
'   * gas damage `base` (stack leftover), chest-loot roll, locked-door key
'   * WarlordConfrontation script + FortressSelfDestruct countdown (0x14 / 8)
'   * enemyAttack (non-Warlord blow) -- 32-bit math, entered mid-expression
'   * ds:2214 = Dex/26 in the castle makes higher Dex slightly WORSE at
'     hitting -- verify the FF49 operand order / whether this is an
'     original bug (one DOSBox trace)
'   * FF1F compare polarity (same reversed reading as DUN gives sane rates)
