' ==========================================================================
'  OUT.EXE  --  overworld combat                                     [v7]
'  reconstructed from out.asm ; see recovered/README.md for the model + tags
'
'  SUBs: RollEncounterMod  ComputeEquippedPower  SpellAttack
'        ResolvePlayerAttack  CreatureDefeated  CreatureAttack
'
'  v7: CreatureAttack (the monster's turn) reconstructed -- `creatureAttack`
'  was un-folded in out.idb (ida_scripts/expand_folded_out.py) and is now
'  fully in out.asm (0x11CB3..0x12143).
'  v6: to-hit confirmed by a 2nd DOSBox trace -- L is the CONSTANT 0.8
'  (ds:2E3A), not Dex/(wp+18).  Formula: Dex^0.8 * (wp+18) / (creatureHP*11).
' ==========================================================================
'
'  v4: the to-hit is SOLVED (exact float match against Paul's DOSBox trace).
'  Two things unlocked it:
'   - the leglib op table, read from LEGLIB.EXE (ds:0F7C): + - -rev * / /rev
'     cmp, plus `FF2B` = `^` reversed  (NOT in the table -- own thunk).
'   - the value-stack node layout: a node's value lives at [node.ptrField],
'     which for the top slot == [ds:111C].  (The 8 bytes at the node start
'     are unused -- an earlier trace misread them as 0.)
'
'  Verified against Paul's DOSBox trace (knife vs neural cloud):
'     Dex 16, Str 19, weaponPower 2, creatureHP 50, creatureWeak 3,
'     weaponId 1  ->  hitScratch = 0x3EAB17EA = 0.33416682  (exact),
'     "ENEMY HIT BY BLOW OF 6", and a separate "MISSES".
'
'  leglib op table (ds:0F7C, index = 0/4/8/C/10/14/18/1C):
'     0x00  +            FF44 (imm) / FF42 (stack)
'     0x04  -            (a - b)
'     0x08  - reversed   FF53                 (b - a)
'     0x0C  *            FF4E (imm) / FF4C (stack)
'     0x10  /            FF47                 (TOS1 / TOS)
'     0x14  / reversed   FF49                 (TOS / TOS1; as imm: TOS / imm)
'     0x18  compare      FF1F
'  FF2B  =  `^` reversed  (TOS ^ TOS1)  -- own thunk, leglib seg004:0x3954
'  RND(1) = `push ds:24E8 : push ds:24E6 : call B$RND`  (ds:24E6 = SINGLE 1.0)
'
'  SINGLE constant pool (decoders/dgroup_consts.py OUT.EXE; ds:2C26/279C/
'  2C32/2E6C spot-checked in DOSBox 2026-09-01 -- all correct):
'     ds:2476 0.40  ds:2482 0.50  ds:24E6 1.0   ds:24EA 20.0  ds:279C 4.0
'     ds:280A 2.0   ds:2906 18.0  ds:290A 12.6  ds:2C26 11.0  ds:2C32 1.3
'     ds:2E6C 6.0
'
'  DGROUP vars:
'     Dexterity 1AC0   Strength 1B08   weaponPower 22D6   weaponId 1AFE
'     creatureHP   21FC = (creatureStatWord MOD 256)     [out.asm:3924-3943]
'     creatureWeak 22A6 = (creatureWeakWord MOD 256), 99 = "no weakness"
'                                                       [out.asm:3944-3963]
'     enemyHpCell  21FE  (index into viewObjectArray, ds:1C7C)
'     hitScratch   208E (single)     workDamage 2192 (int; also reused as
'                                    the combatPhase enum elsewhere)


' --------------------------------------------------------------------------
SUB RollEncounterMod                                  ' asm: out.asm:3723 (rollCreatureStats)
' --------------------------------------------------------------------------
' Rolls encMod: the sentinel -1000 most of the time, a 12..30 value the
' rest of the time.  Also called from enterOverworld.

    encMod = -1000                                                        ' asm:3725

    IF RND(1) < 0.40 THEN                    ' ds:2476              '?ord ' asm:3727 (FF1F, jb)
        encMod = INT( RND(1) * 18.0 + 12.6 )      ' ds:2906 *, ds:290A + ' asm:3753-3757
        '  RND(1) in [0,1) -> encMod in [12, 30].  (This is why op 0x0C
        '  must be `*`: `/` would collapse it to a constant 12.)
    END IF
END SUB


' --------------------------------------------------------------------------
SUB ComputeEquippedPower                              ' asm: out.asm:4337 (sub_12823)
' --------------------------------------------------------------------------
' Turns the equipped weapon + armour (and their wear) into the two combat
' numbers.  Runs once on encounter start (also from enterOverworld).
'   armorSlot  = ds:1AEA   armorId  = ds:1AEC   (armour ids 9..13)
'   weaponSlot = ds:1AFC   weaponId = ds:1AFE   (weapon ids 0..8)
'   S1() = ds:1B68 = equipment condition, 0..4 (Shoddy..Superb)

    playerDefense = 0                                     ' ds:2266     ' asm:4339
    IF armorSlot < 8 THEN                                              ' asm:4340
        playerDefense = INT( S1(armorSlot) / 3.5 + (armorId - 9) )     ' asm:4354-4361
        '  ds:2970 = 3.5 ; (armorId - 9) = armour tier 0..4.
        '  -> defense 0..5.  Consumed by CreatureAttack (still collapsed).
    END IF

    weaponPower = weaponId                                ' ds:22D6    ' asm:4364-4365
    IF weaponSlot < 8 THEN                                            ' asm:4366
        weaponPower = INT( weaponId + S1(weaponSlot) / 2.8 )          ' asm:4372-4385
        '  ds:2974 = 2.8 .  Knife (id 1), Fair (cond 1) -> INT(1 + 0.36) = 1.
        '  Paul's DOSBox trace had weaponPower = 2 (a better-condition knife).
    END IF
END SUB


' --------------------------------------------------------------------------
SUB SpellAttack                                       ' asm: out.asm:6365 (doAttackOrCast)
' --------------------------------------------------------------------------
' The "cast" branch of the attack command.  selectedSpell (ds:1E24) is the
' spell's index; 29 = Seek (handled elsewhere), 23..28 = the attack spells.
'   spellPower = ds:231C = selectedSpell

    SpellCharge(selectedSpell) = SpellCharge(selectedSpell) - 1        ' asm:6416-6428
    PRINT "ATTACK WITH "; Spell$(selectedSpell); "."                   ' asm:6429-6451
    IF SpellCharge(selectedSpell) < 1 THEN selectedSpell = 0           ' asm:6452-6464

    ' ---- fizzle check -------------------------------------------- asm:6466-6480
    IF RND(1) * 45.0 > (Intelligence + 20) THEN            ' ds:2DBA 45 ' asm:6467-6479
        PRINT "ATTACK FIZZLES"                                          ' asm:6492
        StageSfx_Event : EXIT SUB
        '  success chance = min(1, (Intelligence + 20) / 45).  INT 15 -> 78%,
        '  INT >= 25 -> never fizzles.
    END IF

    ' ---- spell damage ------------------------------------------- asm:6507-6523
    workDamage = INT( (selectedSpell * 15.0 - 337.5) * (RND(1) + 1.0) ) ' asm:6507-6523
    '  ds:2502 = 15.0 ; ds:2DD0 = -337.5 ( = -22.5*15 ) ; ds:24E6 = 1.0
    '  = INT( (selectedSpell - 22.5) * 15 * (RND(1) + 1) )
    '  Magic flame (23): 7.5..15 ; Kill flash (28): 82.5..165.            '?? (selectedSpell range)
    GOTO ApplyHitAndMaybeKill   ' shares resolvePlayerAttack's tail (loc_13FBA)
END SUB


' --------------------------------------------------------------------------
SUB ResolvePlayerAttack                               ' asm: out.asm:6686 (resolvePlayerAttack)
' --------------------------------------------------------------------------

    ' ---- 1. to-hit score --------------------------------------------- asm:6687-6714
    '   push CSNG(Dexterity)                                    ' asm:6687
    '   FF2B   ->  Dexterity ^ 0.8   (0.8 = ds:2E3A, pushed by the caller)  asm:6691
    '   push (weaponPower + 18) ; FF4C (*)                      ' asm:6694-6698
    '   push creatureHP ; FF4E ds:2C26 (*) -> creatureHP * 11.0 ' asm:6702-6706
    '   FF47 (/) ; pop -> hitScratch (ds:208E)                  ' asm:6710-6713
    hitScratch = Dexterity ^ 0.8 * (weaponPower + 18) / (creatureHP * 11.0) ' asm:6687-6714
    '  VERIFIED against two DOSBox traces (bit-exact bar 1-2 ULP of the
    '  software pow):
    '     Dex 16, wp 2, cHP 50  -> 0.334167  (0x3EAB17EA)
    '     Dex 20, wp 8, cHP 35  -> 0.741885  (0x3F3DEC2F)
    '  Dexterity has diminishing returns (^0.8); the +18 makes weaponPower a
    '  modest linear bonus; tougher creatures (higher HP) are harder to hit.

    IF creatureWeak = weaponId THEN                                       ' asm:6717
        hitScratch = 1.0                       ' ds:24E6  (plain assign)  ' asm:6723-6727
    END IF

    PRINT "ATTACK "; Creature$(creatureIndex)                             ' asm:6730
    PRINT "WITH "; Weapon$(weaponId); "."                                 ' asm:6756

    ' ---- 2. to hit: HIT when RND(1) < hitScratch ------------------- asm:6789-6808
    IF RND(1) >= hitScratch THEN                                          '?ord ' asm:6803 (FF1F, jnb -> MISS)
        PRINT "YOUR ATTACK MISSES."                                       ' asm:6812
        Delay 4 : StageSfx_Attack : EXIT SUB                              ' asm:6823-6827
    END IF

    ' ---- 3. base damage ------------------------------------------- asm:6830-6874
    '   push Strength                                           ' asm:6831
    '   push weaponPower                                        ' asm:6835
    '   FF49 ds:2E6C (/rev, imm) -> weaponPower / 6.0           ' asm:6839
    '   FF44 ds:2482 (+)         -> weaponPower/6 + 0.5          ' asm:6843
    '   FF4C (*)                 -> Strength * (weaponPower/6 + 0.5)   asm:6847
    '   push RND(1) ; FF4E ds:280A (*) -> RND(1) * 2.0          ' asm:6850-6861
    '   FF44 ds:24E6 (+)              -> RND(1)*2 + 1.0          ' asm:6865
    '   FF47 (/)  -> Strength*(wp/6+0.5) / (RND(1)*2 + 1)       ' asm:6867
    workDamage = INT( Strength * (weaponPower / 6.0 + 0.5) _
                      / (RND(1) * 2.0 + 1.0) )                            ' asm:6831-6874
    '  Str 19, wp 2 -> 19 * 0.8333 / (1..3) = 5..15 dmg.

    ' ---- 4. weapon-vs-weakness branches -------------------------- asm:6875-6934
    IF creatureWeak < 99 AND creatureWeak <> weaponId THEN                ' asm:6875-6892
        ' wrong weapon against a creature that HAS a weak weapon: chip hit
        workDamage = INT( RND(1) * 4.0 + weaponPower / 1.3 + 1.0 )        ' asm:6895-6912
        '  ds:279C 4.0 ; ds:2C32 1.3 ; ds:24E6 1.0.  Str 19 case is moot
        '  here -- Paul's "BLOW OF 6" (knife vs cloud, weak 3 <> 1) came
        '  from THIS line: 4*RND + 2/1.3 + 1 = 6 needs RND ~ 0.87 (or a
        '  slightly bigger ds:279C -- worth a DOSBox check).              'CHECK
    END IF
    IF weaponId = creatureWeak THEN                                       ' asm:6915
        ' matching the creature's weak weapon: the big hit
        workDamage = INT( Strength + RND(1) * 20.0 )          ' ds:24EA 20.0 ' asm:6921-6934
        '  Strength .. Strength+19 -- one-shots most early creatures.
    END IF

    PRINT "ENEMY HIT BY BLOW OF "; workDamage                            ' asm:6938
    Delay 2 : StageSfx_Attack                                             ' asm:6970-6976

    ' ---- 5. apply to the enemy HP cell --------------------------- asm:6979-7005
    ViewObject(enemyHpCell) = ViewObject(enemyHpCell) - workDamage         ' asm:6980-6991
    IF ViewObject(enemyHpCell) > 0 THEN EXIT SUB                          ' asm:6996
    PRINT "THE "; Creature$(creatureIndex); " DIES."                      ' asm:7005
    CreatureDefeated
END SUB


' --------------------------------------------------------------------------
SUB CreatureDefeated                                  ' asm: out.asm:7023 (creatureDefeated)
' --------------------------------------------------------------------------
' Death resolution (~1.1 KB, mostly text).  rewardTier drives every payout.
'   rewardTier   = A3(creatureIndex) \ 256          ' ds:1F04  (A3 = ds:20BE)
'   creatureCount = ds:22B0   (how many of this type were in the encounter)
'   food = ds:1ACE   partyGold = ds:1AD2 (dword)
'   ds:2496 = 0.6   ds:29FA = 0.7   ds:2E9E = 200

    PRINT "THE "; Creature$(creatureIndex); " DIES."                      ' asm:7025-7038
    encounterActive = encounterActive - 1                                 ' asm:7052 (ds:21FE)
    IF encounterActive > 0 THEN StageSfx_Event : EXIT SUB    ' more to fight  asm:7053-7055

    contextMode = 0 : RedrawAfterAction                                   ' asm:7062-7063
    rewardTier = A3(creatureIndex) \ 256                                  ' asm:7072-7081

    ' ---- "flesh for food?" option ------------------------------- asm:7088-7234
    '   offered ~60% of the time (RND < 0.4 gate), and not when food >= 200
    IF food < 200 AND RND(1) < 0.4 THEN                    ' ds:2E9E, ds:2476 '?ord
        PRINT "DO YOU WANT TO USE THE "; Creature$(creatureIndex); _
              " FLESH FOR FOOD?"                                          ' asm:7144-7207
        IF YesNo() THEN                                    ' ds:1E22 = 1  ' asm:7221
            foodGain = 0
            FOR k = 1 TO creatureCount                                    ' asm:7234-7263
                foodGain = foodGain + (RND(1) * 0.6 + 0.7) * (rewardTier AND 63)
            NEXT k                                         ' ds:2496, ds:29FA
            foodGain = INT(foodGain) + 1                                  ' asm:7264
            food = food + foodGain                                       ' asm:7266-7271
            PRINT "YOU GAIN"; foodGain; " DAYS OF FOOD."                  ' asm:7272-7291
        END IF
    END IF

    ' ---- dropped item ------------------------------------------ asm:7311-7343
    IF RND(1) < DropChance() AND creatureDropId <> 0 THEN   ' ds:1AEE     ' asm:7333-7342
        AwardFoundItem                    ' "YOU FIND A <item>"           ' asm:7343
        EXIT SUB
    END IF

    ' ---- gold ------------------------------------------------- asm:7346-7462
    IF S0(4) = 0 AND RND(1) < GoldChance() THEN            ' asm:7348-7385 '??
        goldFound = INT( (RND(1) * 0.6 + 0.7) * creatureCount _
                         * (rewardTier AND 63) )                          ' asm:7413-7430
        PRINT "YOU FIND"; goldFound; " GOLD."                             ' asm:7431-7450
        partyGold = partyGold + goldFound                                 ' asm:7451-7456
    ELSE
        AddFoodDays 4                     ' ds:231C = 4                    ' asm:7460-7462
    END IF
    '  NOTE: no experience award anywhere in this function -- overworld
    '  kills give food + gold + items only.  XP is museum-only (per guide). '??
END SUB


' --------------------------------------------------------------------------
SUB CreatureAttack                                   ' asm: out.asm:3474 (creatureAttack, 0x11CB3)
' --------------------------------------------------------------------------
' The monsters' turn.  Every engaged creature rolls to hit; hits accumulate
' one combined damage number; death here is NOT a game over -- you "FALL
' UNCONSCIOUS" and revive.
'   creatureAtk    = ds:2264   (rolled at encounter start -- 0 in the EXE)
'   playerDefense  = ds:2266   (ComputeEquippedPower)
'   S4(12)         = ds:1B96 elem 0x0C  (a region difficulty/scaling word)
'   hitCount 1F06   totalDmg 1F04   loopN 2262 = creaturesToFight (21FE)

    IF contextMode > 11 THEN EXIT SUB                 ' ds:1F2A            ' asm:3478-3480

    ' ---- the monsters' shared to-hit chance --------------------- asm:3486-3510
    '   toHitChance = MIN( 0.75 , creatureHP / (Dexterity*2 + 20) )
    '   -- scales with the CREATURE's HP, inversely with your Dexterity,
    '      capped at 0.75.
    toHitChance = creatureHP / (Dexterity * 2.0 + 20.0)   ' ds:280A, ds:24EA
    IF toHitChance > 0.75 THEN toHitChance = 0.75         ' ds:280E        ' asm:3497-3510

    hitCount = 0 : totalDmg = 0                                           ' asm:3484-3485
    FOR k = 1 TO creaturesToFight                     ' ds:2262           ' asm:3520-3584
        IF RND(1) <= toHitChance THEN                                     ' asm:3521-3530 '?ord
            hitCount = hitCount + 1                                       ' asm:3535
            blow = creatureAtk * (RND(1) + 0.4) * 1.7   ' ds:2476, ds:2812 ' asm:3536-3554
            totalDmg = (totalDmg + 0.5 + blow) _
                       * (S4(12) + 2) / (Endurance * (playerDefense + 2))  ' asm:3556-3574 '??
            '  the whole running total is re-scaled by
            '  (S4(12)+2) / (Endurance*(playerDefense+2)) each hit -- so
            '  earlier blows in the same round keep getting mitigated.
            '  S4(12) is a biggish region word (like the S4 sentinels);
            '  with it ~ Endurance*(pd+2) the scale is ~1 (purely additive). '?? (S4(12))
        END IF
    NEXT k
    totalDmg = INT(totalDmg)

    PRINT "ATTACKED BY "; creaturesToFight; " "; Creature$(creatureIndex); _
          PLURAL$(creaturesToFight)                                       ' asm:3597-3642
    PRINT "HITS: "; hitCount; "    DAMAGE: "; totalDmg                     ' asm:3644-3669

    hitPoints = hitPoints - totalDmg                                      ' asm:3670-3671
    IF hitPoints >= 1 THEN EXIT SUB                   ' survived           ' asm:3675-3695

    ' ---- knocked out ------------------------------------------- asm:3698-3760+
    RedrawAfterAction                                                     ' asm:3699
    PRINT "YOU FALL UNCONSCIOUS."                     ' ds:2844           ' asm:3700-3711
    contextMode = 0                                                       ' asm:3714
    hitPoints = INT( RND(1) * 50.0 + 60.0 )           ' ds:285E, ds:2862  ' asm:3716-3727
    '  revive with 60..110 HP.
    IF RND(1) >= 0.5 AND food > 50 THEN                ' ds:2482, ds:285E ' asm:3728-3752 '??
        food = food - foodPenalty                     ' a ration hit on waking
    END IF
    ' ... teleport to the nearest town / safe tile ...
END SUB


' ==========================================================================
'  STATUS
' ==========================================================================
'  SOLID -- operators pinned from LEGLIB.EXE; formulas verified against
'  Paul's DOSBox traces (to-hit is an EXACT float match) or self-consistent:
'   * op table = + - -rev * / /rev cmp  (README) ; FF2B = `^` reversed
'   * to-hit score  = Dex^0.8 * (weaponPower + 18) / (creatureHP * 11)
'                     [VERIFIED, two DOSBox traces; 0.8 = ds:2E3A]
'   * to hit        : HIT when RND(1) < hitScratch ; weakness-match forces 1.0
'   * RollEncounterMod  = INT( RND(1)*18 + 12.6 )               [12..30]
'   * base damage       = INT( Str * (wp/6 + 1/2) / (2*RND(1) + 1) )
'   * chip damage       = INT( 4*RND(1) + wp/1.3 + 1 )
'   * weakness-match dmg = INT( Str + 20*RND(1) )
'   * creatureHP   = creatureStatWord  MOD 256   (out.asm:3924-3943)
'   * creatureWeak = creatureWeakWord  MOD 256, 99 = none  (3944-3963)
'   * value-stack node: value at [node.ptrField]; top value at [ds:111C]
'
'   * ComputeEquippedPower = weaponPower = INT(weaponId + cond/2.8);
'     playerDefense = INT(cond/3.5 + armorTier)
'   * SpellAttack: fizzle when RND(1)*45 > Intelligence+20;
'     spell dmg = INT((selectedSpell - 22.5) * 15 * (RND(1)+1))
'   * CreatureDefeated: food/gold both = (RND(1)*0.6 + 0.7) * rewardTier
'     [* creatureCount for gold], rewardTier = A3(creatureIndex) \ 256;
'     NO experience award (overworld kills = food/gold/items only)
'   * CreatureAttack: monster to-hit = MIN(0.75, creatureHP/(Dex*2+20));
'     per hit  blow = creatureAtk * (RND(1)+0.4) * 1.7 , accumulated with a
'     per-hit re-scale by (S4(12)+2)/(Endurance*(playerDefense+2));
'     death -> "YOU FALL UNCONSCIOUS", revive at INT(RND(1)*50+60) HP
'
'  OPEN:
'   1. RollEncounterMod: is the 0.40 gate a < or >= (40% vs 60%)?  same
'      question for the CreatureDefeated food gate.
'   3. selectedSpell range (23..28 assumed from the -337.5 = -22.5*15 offset).
'   4. CreatureDefeated: the exact DropChance / GoldChance RND gates and the
'      S0(4) test at asm:7348.
'   5. CreatureAttack: S4(12) value (region difficulty word); the compounding
'      re-scale; the unconscious-revive teleport target + food penalty.
'   6. creatureAtk (ds:2264) -- rolled at encounter start; find where.
