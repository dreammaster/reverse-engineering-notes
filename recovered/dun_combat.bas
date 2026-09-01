' ==========================================================================
'  DUN.EXE  --  dungeon combat                                        [v1]
'  reconstructed from dun.asm ; see recovered/README.md for the model + tags
'
'  SUBs: DoAttack (player)   MonsterAttack + MonsterSpecialAttack (monsters)
'        CastSpell (dungeon spells)
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

    FOR slot = 1 TO 7                                                     ' asm:3169-3177
        IF NOT monsterPresent(slot) THEN GOTO NextSlot
        IF monsterIsSpecial(slot) THEN                 ' ds:1F02          ' asm:2831-2837
            MonsterSpecialAttack
        ELSE
            ' ---- normal physical hit --------------------------------
            ' NOTE: the normal-hit to-hit + damage rolls sit in a region
            ' IDA did not fully coerce (dun.asm ~3138-3168, raw db bytes).
            ' Needs an IDA re-pass or a DOSBox trace.                     'CHECK
            NormalMonsterHit slot
        END IF
NextSlot:
    NEXT slot
END SUB


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
'   * player to hit : RND(1) * 70 < Dexterity + 30
'   * player damage : INT( (RND(1) + 1/2) * (Strength + 30) * (weaponPower
'                          + 40) / 450 )   [+50% while spell-buffed]
'   * DANGLER drain : INT( RND(1) * 3 + 1 )  from Endurance
'   * KNUCKLES / armour-eater : destroy the equipped weapon / armour
'   * specials fire at ~3% (ds:24B4 = 0.97)
'
'  OPEN
'   * the normal monster-hit to-hit + damage (un-coerced in dun.asm)
'   * weaponPower (ds:21CE) derivation -- DUN's ComputeEquippedPower analogue
'   * CastSpell (dun.asm:4696)
'   * the /450 and the (21CE + 40) -- confirm 21CE is weaponPower with a trace
