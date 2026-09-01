' ==========================================================================
'  OUT.EXE  --  overworld combat, player-attack path            [PILOT]
'  reconstructed from out.asm ; see recovered/README.md for the model + tags
' ==========================================================================
'
'  Call graph for one "Fight" command against an overworld creature:
'
'     mainDispatch
'       -> ComputeEquippedPower        (sub_12823)      once, on encounter start
'       -> RollCreatureStats           (rollCreatureStats)   "
'       -> ResolvePlayerAttack         (resolvePlayerAttack) per swing
'            -> CreatureDefeated        (creatureDefeated)   if it drops
'       -> CreatureAttack              (still collapsed in out.asm - TODO)
'
'  DGROUP variables used here (names from apply_dsvars_out.py + our analysis):
'     Dexterity      ds:1AC0      Strength     ds:1B08
'     hitPoints      ds:1ADA      workInt      ds:2192   (= the rolled damage)
'     partyGold      ds:1AD2 (dword)
'     weaponId       ds:1AFE      '?? id 0-8, also indexes the weapon-name array 1D0A$()
'     weaponPower    ds:22D6      set by ComputeEquippedPower
'     armorPower     ds:2266      set by ComputeEquippedPower   '??
'     weaponSlot     ds:1AEA      '?? slot into S0()/S1()  (< 8 guard)
'     armorSlot      ds:1AFC      '?? slot into S0()/S1()  (< 8 guard)
'     targetWeaponId ds:22A6      the weapon the CURRENT enemy is vulnerable to '??
'     creatureIndex  ds:2254      0..31, indexes OUTDAT.DAT creature names / stats
'     enemySlot      ds:21FE      index into viewObjectArray() (ds:1C7C) = enemy HP cell
'     hitScratch     ds:208E      temp: the computed to-hit value
'     encMod         ds:2274      rolled per-encounter modifier (RollCreatureStats)
'     S0()  = ds:1B0C  equipment ids        S1() = ds:1B68  equipment condition (0-4)
'     Weapon$() = ds:1D0A   Armor$() = ds:1D38   (8 leglib string arrays)
'
'  Named integer constants live in OUT's DGROUP; their VALUES are not in the
'  packed EXE, so they show here as  c<addr>  and must be dumped from an
'  unpacked OUT.EXE or watched in DOSBox.   'CHECK


' --------------------------------------------------------------------------
SUB ComputeEquippedPower                                  ' asm: out.asm:4335
' --------------------------------------------------------------------------
' Turns the equipped weapon/armour + their condition into the two power
' numbers combat uses.  Called once when an encounter begins.

    IF weaponSlot < 8 THEN
        armorPower = (S1(weaponSlot) MOD c2970) + (weaponSlot@1AEC - 9)   '?op ' asm:4344
        '  NOTE: ds:1AEC is read here (`1AEC + FFF7h` = 1AEC - 9); role unclear,
        '        possibly "weapons carried" or a second cursor.            'CHECK
    END IF

    weaponPower = weaponId                                                ' asm:4365
    IF armorSlot < 8 THEN
        weaponPower = weaponId + (S1(armorSlot) MOD c2974)                '?op ' asm:4377
    END IF
END SUB


' --------------------------------------------------------------------------
SUB RollCreatureStats                                 ' asm: out.asm:3723
' --------------------------------------------------------------------------
' Rolls the per-encounter modifier `encMod`.  Also called from enterOverworld.

    encMod = -1000                                                        ' asm:3725
    IF RND < c2476 THEN                                                   '?op ' asm:3727 (compare, jb)
        encMod = (RND / c2906) + c290A                                    '?op ' asm:3753
    END IF
    '  So most encounters leave encMod = -1000 (a sentinel meaning "none"),
    '  and a minority get a random positive bump.                          '??
END SUB


' --------------------------------------------------------------------------
SUB ResolvePlayerAttack                               ' asm: out.asm:6686
' --------------------------------------------------------------------------
' One swing of the player's weapon at creatureIndex.  Prints the flavour
' lines, rolls to-hit, rolls damage, subtracts it from the enemy's HP cell,
' and calls CreatureDefeated if that cell drops to <= 0.

    ' ---- 1. to-hit threshold ------------------------------------------- asm:6687
    '   hitScratch = ( Dexterity {coerced}  ?  (weaponPower + 18) )
    '                    op10
    '                ( enemySlotState@21FC  /  c2C26 )
    hitScratch = (Dexterity + (weaponPower + 18)) \ (state@21FC / c2C26)  '?op ' asm:6687-6714
    IF targetWeaponId = weaponId THEN                                     ' asm:6717
        hitScratch = hitScratch + overworldArrayLo@24E6                   '?? ' asm:6723
        '  (adds a bonus when the equipped weapon matches the one this
        '   creature is vulnerable to -- see the guide's "vulnerable to a
        '   specific weapon" note)                                         '??
    END IF

    PRINT "ATTACK "; Creature$(creatureIndex)                             ' asm:6730
    PRINT "WITH "; Weapon$(weaponId); " ..."                              ' asm:6756

    ' ---- 2. roll to hit ---------------------------------------------- asm:6789
    IF RND >= hitScratch THEN                                             '?op ' asm:6791-6807 (FF1F, jnb)
        PRINT "YOUR ATTACK MISSES."                                       ' asm:6812
        Pause 4                                                           ' asm:6823
        StageSfx_attack                                                   ' asm:6827
        EXIT SUB
    END IF

    ' ---- 3. roll damage -------------------------------------------- asm:6830
    '   workInt = ( (Strength  MOD-or-op14  weaponPower)  +  c2482 )  / 12
    workInt = ((Strength MOD weaponPower) + c2482) / 12                   '?op ' asm:6831-6847
    workInt = workInt + (RND \ c280A) + overworldArrayLo@24E6            '?op ' asm:6849-6871

    ' ---- 4. special-case tweaks to the damage --------------------- asm:6875
    IF weaponPower < 99 AND targetWeaponId = weaponId THEN               '?? ' asm:6875-6892
        '  weaponPower still below its cap AND the "right" weapon:
        workInt = RND \ (weaponPower * c2C32) + overworldArrayLo@24E6    '?op ' asm:6901-6912
    END IF
    IF weaponId = targetWeaponId THEN                                    ' asm:6915
        workInt = Strength \ RND                                        '?op ' asm:6922-6934
        '  (a flat "vulnerable weapon" hit: damage scales with Strength)  '??
    END IF

    PRINT "ENEMY HIT BY BLOW OF "; workInt                                ' asm:6938
    Pause 2                                                               ' asm:6970
    StageSfx_attack                                                       ' asm:6976

    ' ---- 5. apply to the enemy HP cell --------------------------- asm:6979
    ViewObject(enemySlot) = ViewObject(enemySlot) - workInt               ' asm:6980-6991
    IF ViewObject(enemySlot) > 0 THEN EXIT SUB                            ' asm:6996

    PRINT "THE "; Creature$(creatureIndex); " DIES."                      ' asm:7005 -> creatureDefeated
    CreatureDefeated
END SUB


' ==========================================================================
'  WHAT IS SOLID vs. WHAT NEEDS CONFIRMING
' ==========================================================================
'
'  SOLID (structure + operands + control flow):
'   * the miss / hit / damage / kill sequence and its branches
'   * to-hit reads Dexterity + weaponPower (+18) and something at ds:21FC;
'     vulnerable-weapon match adds a bonus
'   * damage reads Strength + weaponPower + an RND term + constants, then
'     the "right weapon vs this creature" path replaces it with a
'     Strength-scaled hit  (matches the guide: "each one should go down
'     with one blow" once your weapon/level are high)
'   * damage is subtracted straight from viewObjectArray(enemySlot);
'     <= 0  => CreatureDefeated
'   * ComputeEquippedPower folds item condition (S1, 0-4) into weaponPower
'
'  NEEDS CONFIRMING  ('CHECK):
'   1. the value-stack operators tagged '?op  (+ vs * vs / vs \ vs MOD).
'      Fix by dumping leglib's [ds:0F7C] op-dispatch table, OR one DOSBox
'      watch: break on the write to ds:2192 during a fight, log the inputs.
'   2. the constants c2476 c2906 c290A c2C26 c2C32 c2E6C c2482 c280A c2970
'      c2974 -- dump from an unpacked OUT.EXE (the .idb has them) .
'   3. weaponSlot(1AEA) / armorSlot(1AFC) / weaponId(1AFE) exact roles --
'      the PAULA save-diffs are ambiguous here; a save taken right after
'      equipping a known weapon+armour, with the fight then watched, settles it.
'   4. ds:1AEC and ds:21FC semantics.
'   5. RollCreatureStats: does encMod feed to-hit, damage, or enemy HP?
'      (not referenced inside ResolvePlayerAttack -- probably CreatureAttack).
