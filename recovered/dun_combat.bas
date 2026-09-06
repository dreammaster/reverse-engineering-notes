' ==========================================================================
'  DUN.EXE  --  dungeon combat                                        [v2]
'  reconstructed from dun.asm ; see recovered/README.md for the model + tags
'
'  SUBs: DoAttack (player)   MonsterAttack + MonsterSpecialAttack (monsters)
'        (dungeon spells are fully reconstructed in recovered/dun_spells.bas)
'
'  DUN uses a DIFFERENT combat model from OUT -- simpler linear formulas,
'  no weapon-weakness system, no per-encounter "creatureHP" scalar.
'  DUN DGROUP:0 = DUN.EXE file offset 0x5F00.
' ==========================================================================
'
'  DGROUP vars (DUN):
'     Dexterity 1AC0   Strength 1B08   Endurance 1ACC
'     weaponId 1AFE    weaponSlot 1AFC   armorSlot 1AEA   armorId 1AEC
'     weaponPower 21CE   damageOut 21CC   drain 20EA
'     spellBuffTurns 1AE8   monsterHp cell in the S4()/viewObject arrays
'     RND(1) = `push ds:2276 : push ds:2274 : call B$RND`  (ds:2274 = 1.0)
'     ds:2564 = 70   ds:2512 = 0.5   ds:25D0 = 3.0   ds:24B4 = 0.97   ds:2274 = 1.0


' --------------------------------------------------------------------------
SUB DoAttack                                          ' asm: dun.asm:3973 (doAttack)
' --------------------------------------------------------------------------
' The player's melee swing at the monster ahead.

    PRINT " WITH "; Weapon$(weaponId); "."                                ' asm:3974-4014

    ' ---- to hit ---------------------------------------------------- asm:4017-4046
    '   HIT when  RND(1) * 70  <  Dexterity + 30
    '   -> hit chance = min(1, (Dexterity + 30) / 70).  Dex 15 -> 64%,
    '      Dex 40 -> always.
    IF RND(1) * 70.0 >= Dexterity + 30 THEN            ' ds:2564          ' asm:4017-4046 '?ord
        IF attackMode < 2 THEN missTargetX = 0         ' ds:2140, ds:208C ' asm:4050-4056
        PRINT "YOUR ATTACK MISSES."                                       ' asm:4059
        Delay 4 : GOTO EndTurn                                            ' asm:4073-4078
    END IF

    ' ---- damage ------------------------------------------------- asm:4081-4131
    damageOut = INT( (RND(1) + 0.5) * (Strength + 30) _
                     * (weaponPower + 40) / 450 )      ' ds:2512, ds:21CE ' asm:4081-4120
    '   (RND + 0.5) in [0.5, 1.5) ; the /450 keeps it modest.
    '   Str 15, wp 5:  ~0.5..1.5 * 45 * 45 / 450  =  2.25 .. 6.75  ->  2..6.

    IF spellBuffTurns > 0 THEN                         ' ds:1AE8          ' asm:4121-4131
        damageOut = damageOut + damageOut \ 2          ' +50% while buffed
        spellBuffTurns = spellBuffTurns - 1
    END IF

    PRINT "ENEMY HIT BY BLOW OF "; damageOut; "!"                         ' asm:4135-...
    MonsterHp = MonsterHp - damageOut                                     ' (loc past 4152)
    IF MonsterHp <= 0 THEN MonsterDefeated
END SUB


' --------------------------------------------------------------------------
SUB MonsterAttack                                     ' asm: dun.asm:2812 (monsterAttack)
' --------------------------------------------------------------------------
' Each of up to 7 monster slots gets a turn (loop on ds:20EC, 1..7).
' If the slot's ds:1F02 flag is set -> MonsterSpecialAttack, else a normal
' physical hit.

    FOR slot = 1 TO 7                                 ' ds:20EC           ' asm:2982-3177
        IF NOT monsterPresent(slot) THEN GOTO NextSlot
        IF monsterIsSpecial(slot) THEN                 ' ds:1F02          ' asm:2831-2837
            MonsterSpecialAttack

        ELSE
            ' ---- normal physical hit ------------------------------ asm:2867-2962
            '   MISS when  RND(1) * 70 <= Dexterity
            '   -> monster hit chance = 1 - Dexterity/70 .  Dex 20 -> 71%,
            '      Dex 40 -> 43%, Dex 70 -> never.  (Mirror of the player's
            '      own DUN to-hit, RND*70 < Dex+30, minus the +30.)
            IF RND(1) * 70.0 <= Dexterity THEN         ' ds:2564         ' asm:2869-2885 '?ord
                PRINT "ATTACK MISSED."                                    ' asm:2890
            ELSE
                dmg = INT( (RND(1) + 0.5) * monsterAtk(slot) )  ' ds:2512, ds:2192 ' asm:2912-2931
                '   NO armour / Endurance mitigation here -- unlike OUT and
                '   CASDR, DUN monster damage is just (RND + 0.5) * atk.
                hitPoints = hitPoints - dmg                               ' asm:2932
                PRINT "HIT BY BLOW OF "; dmg; "!"                         ' asm:2933-2957
            END IF
        END IF
NextSlot:
    NEXT slot
END SUB


' --------------------------------------------------------------------------
SUB MoveMonsters                                      ' asm: dun.asm:7006 (moveMonsters, far SUB)
' --------------------------------------------------------------------------
' The per-turn monster phase.  dunMain calls it AFTER the player's action,
' but SKIPS the whole phase (move + attack) while confuseTimer > 0.
'
' viewObjectArray = ds:1C7C, 8 monster slots.  Per slot s (element e = s*2):
'     [e+0]    occupied (> 0 = a monster is here)
'     [e+0x10] monsterX          [e+0x18] monsterY
'     [e+0x20] packedCell        (index into the map array ds:1E2A)
'     [e+0x28] attackDir         (0 = not adjacent, else 1..4 -- see below)
'
'   monstersReached (ds:2188) = 0
'   FOR s = 0 TO 7
'     IF viewObjectArray(s).occupied <= 0 THEN attackDir = 0 : GOTO nextSlot
'     dx = monsterX - playerX(ds:20CE)                ' step is toward the player
'     dy = monsterY - playerY(ds:20D0)
'     xStep = SGN(-dx)  (0 if dx = 0)     ' packed cell delta -/+1
'     yStep = SGN(-dy)  (0 if dy = 0)     ' packed cell delta -/+0x10
'     IF ABS(dx) < ABS(dy) THEN
'         IF StepMonster(yStep) <> 0 THEN StepMonster(xStep)   ' Y first, X fallback
'     ELSE
'         IF StepMonster(xStep) <> 0 THEN StepMonster(yStep)   ' X first, Y fallback
'     END IF
'     ' write monsterX / monsterY / packedCell back to the slot
'     attackDir = CheckMonsterAdjacent(s)             ' 0 / 1 / 2 / 3 / 4
'     viewObjectArray(s).attackDir = attackDir
'     IF attackDir > 0 THEN monstersReached = monstersReached + 1
'   NEXT
'   ' dunMain then calls MonsterAttack, which resolves the monstersReached slots.
'
' *** There is NO aggro range and NO random element -- every monster does a
'     greedy Manhattan chase every turn (one orthogonal step, dominant axis
'     first, the other axis as a single fallback ; no diagonal, no real
'     path-finding around obstacles). ***
END SUB


' --------------------------------------------------------------------------
SUB StepMonster                                       ' asm: dun.asm:8136 (sub_139FC, far SUB)
' --------------------------------------------------------------------------
' StepMonster(delta) -- move the current monster one cell.  Returns 0 if it
' moved, 1 if blocked.  (delta = +-1 for X, +-0x10 for Y in packed units.)
'
'   oldTile   = mapByte(packedCell)                  ' map array ds:1E2A
'   classBits = oldTile AND &h70                     ' bits 4-6 = monster class
'   mapByte(packedCell) = oldTile AND &h8F           ' lift the monster off the cell
'   target    = packedCell + delta
'   IF mapByte(target) >= &h10 THEN RETURN 1         ' wall / object / revealed feature
'   IF target = playerCell(ds:1AE2) THEN RETURN 1    ' can't step onto the player
'   packedCell = target
'   <the paired X or Y coordinate> += <step sign>
'   result = 0
' finally (moved or not):
'   mapByte(packedCell) = mapByte(packedCell) OR classBits   ' re-stamp the monster
'   RETURN result
'
' Map byte layout:  bit7 flag | bits4-6 class<<4 | bits0-3 wall/floor
'   -- floor = &h00 ; anything >= &h10 blocks a moving monster.


' --------------------------------------------------------------------------
SUB CheckMonsterAdjacent                              ' asm: dun.asm:6319 (stepMonsterToward)
' --------------------------------------------------------------------------
' Returns the direction the monster now sits relative to the player, or 0:
'     monster at (playerX,   playerY+1) -> 1
'     monster at (playerX+1, playerY)   -> 2
'     monster at (playerX,   playerY-1) -> 3
'     monster at (playerX-1, playerY)   -> 4
'     not orthogonally adjacent          -> 0
' MonsterAttack uses this both to gate the attack and to pick the "HIT BY
' BLOW OF <monster>" side / sprite.


' --------------------------------------------------------------------------
'  BEFUDDLE  --  the confuseTimer (ds:1AE6) gate, in dunMain     [confirmed]
' --------------------------------------------------------------------------
' One shared SIGNED counter, stepped one toward 0 every turn:
'     confuseTimer < 0  ->  "YOU ARE BEFUDDLED."  + Delay &h12 ; your action
'                           is skipped this turn (confuseTimer++)
'     confuseTimer > 0  ->  the ENTIRE monster phase (MoveMonsters +
'                           MonsterAttack) is skipped this turn (confuseTimer--)
' (Cast by the Befuddle spell -- see dun_spells.bas.)


' --------------------------------------------------------------------------
SUB MonsterSpecialAttack                              ' asm: dun.asm (monsterSpecialAttack)
' --------------------------------------------------------------------------
' The named nasties.  Each fires ~3% of the time (RND(1) >= 0.97, ds:24B4).

    SELECT CASE specialKind
    CASE knuckles                                     ' asm:3238-3282
        ' "KNUCKLES BROKE YOUR <weapon>"
        S0(weaponSlot) = 0 : weaponSlot = 99 : weaponId = 0              ' asm:3272-3280
        RecomputeEquip

    CASE armorEater                                   ' asm:3331-3409
        ' "THE <monster> ATE YOUR <armour>"
        S0(armorSlot) = 0 : armorSlot = 99 : armorId = 0                 ' asm:3401-3409
        RecomputeEquip

    CASE dangler                                      ' asm:3459-3518
        ' "ENDURANCE DRAIN: <n>"
        drain = INT( RND(1) * 3.0 + 1.0 )             ' ds:25D0, ds:2274 ' asm:3459-3478
        Endurance = Endurance - drain                 ' ds:1ACC          ' asm:3501-3502
        S4(20)    = S4(20) - drain                    ' checkpoint copy  ' asm:3503-3512
    END SELECT
END SUB


' ==========================================================================
'  SOLID
'   * player to hit  : RND(1) * 70 < Dexterity + 30
'   * player damage  : INT( (RND(1) + 1/2) * (Strength + 30) * (weaponPower
'                           + 40) / 450 )   [+50% while spell-buffed]
'   * monster to hit : MISS when RND(1) * 70 <= Dexterity
'   * monster damage : INT( (RND(1) + 0.5) * monsterAtk )  -- NO mitigation
'   * DANGLER drain  : INT( RND(1) * 3 + 1 )  from Endurance
'   * KNUCKLES / armour-eater : destroy the equipped weapon / armour
'   * specials fire at ~3% (ds:24B4 = 0.97)
'   * MONSTER MOVEMENT (moveMonsters + sub_139FC): greedy Manhattan chase,
'     every monster every turn -- one orthogonal step toward the player,
'     dominant axis first, other axis as a single fallback.  No aggro
'     range, no randomness, no path-finding.  Blocked by any map byte
'     >= &h10 or the player's cell.
'   * BEFUDDLE gate: confuseTimer (ds:1AE6) < 0 skips YOUR turn, > 0 skips
'     the whole MONSTER phase ; steps toward 0 each turn
'
'  DERIVED / CONFIRMED (updateLevelState, dun.asm:8041 -- static consts;
'  DUN DGROUP base = 0x5F00) :
'   * weaponPower (ds:21CE) = weaponId*10 + 10 + (S1(weaponSlot) * 100 \ 28)
'     (weaponSlot 99 -> no condition bonus)
'   * monsterAtk (ds:2192) = INT( (dungeonLevel + 7) * adjDungeonNum * 100
'                                 \ (armorDefenseTerm + 30) )
'     adjDungeonNum   = (dungeonNumber == 3) ? 4 : dungeonNumber   ' NOT "- k"
'     armorDefenseTerm = (S1(armorSlot) * 100 \ 35) + armorId*10 - 70
'                        (armorSlot 99 -> stays at the default 10)
'     -- a well-armoured party shrinks the monsters' rolls at level load
'   * the player-damage /450 is ds:24DA = 450.0 (confirmed)
'
'  OPEN
'   (dungeon spells: SOLVED -- recovered/dun_spells.bas)
