' ==========================================================================
'  OUT.EXE  --  overworld combat, player-attack path            [PILOT v2]
'  reconstructed from out.asm ; see recovered/README.md for the model + tags
' ==========================================================================
'
'  v2 change: the value-stack arithmetic is SINGLE-PRECISION, and the
'  constant pool has been read straight out of OUT.EXE
'  (decoders/dgroup_consts.py).  What used to be opaque `c2476` markers are
'  now real numbers.  The operators are confirmed from leglib's dispatch
'  indices (README); operand ORDER for `-  /  \  MOD` is still the open
'  question and is tagged '?ord.
'
'  Call graph for one "Fight" command against an overworld creature:
'
'     overworldLoop            (sub_13C60)
'       -> ComputeEquippedPower   (sub_12823)          once, on encounter start
'       -> RollEncounterMod       (rollCreatureStats)   "        (TENTATIVE name)
'       -> ResolvePlayerAttack    (resolvePlayerAttack) per swing
'            -> CreatureDefeated    (creatureDefeated)  if the HP cell hits 0
'       -> CreatureAttack         (still collapsed in out.asm - TODO)
'
'  DGROUP variables (names from apply_dsvars_out.py + our analysis):
'     Dexterity      ds:1AC0 (int)     Strength      ds:1B08 (int)
'     hitPoints      ds:1ADA (int)     workDamage    ds:2192 (int)  <- the rolled hit
'     weaponId       ds:1AFE (int)     0..8, also indexes Weapon$() = ds:1D0A
'     weaponPower    ds:22D6 (int)     set by ComputeEquippedPower
'     defScratch     ds:2266 (int)     set by ComputeEquippedPower   '??
'     weaponSlot     ds:1AEA (int)     '?? slot into S0()/S1()  (< 8 guard)
'     armorSlot      ds:1AFC (int)     '?? slot into S0()/S1()  (< 8 guard)
'     creatureWeak   ds:22A6 (int)     creature's weak-weapon id, or 99 = "none"
'     creatureIndex  ds:2254 (int)     0..31, indexes Creature$() at 20E2:0A
'     enemyHpCell    ds:21FE (int)     index into viewObjectArray() (ds:1C7C)
'     enemyArmor     ds:21FC (int)     '?? the enemy stat used in the to-hit divisor
'     hitScratch     ds:208E (single)  computed to-hit fraction
'     encMod         ds:2274 (int)     RollEncounterMod result
'     S0() = ds:1B0C  equipment ids       S1() = ds:1B68  equipment condition 0..4
'
'  SINGLE-PRECISION constant pool  (verified, decoders/dgroup_consts.py OUT.EXE):
'     ds:2476 = 0.40    ds:247A = 0.67    ds:247E = 0.25   ds:2482 = 0.50
'     ds:24E6 = 1.00    ds:24EA = 20.0    ds:279C = 4.00
'     ds:280A = 2.00    ds:280E = 0.75    ds:2812 = 1.70
'     ds:2906 = 18.0    ds:290A = 12.6    ds:290E = 256.0
'     ds:2970 = 3.50    ds:2974 = 2.80
'     ds:2C26 = 11.0    ds:2C2A = 1.02    ds:2C2E = -6.0   ds:2C32 = 1.30
'     ds:2E6C = 6.00
'  (RND(1) below is  `push ds:24E8 : push ds:24E6 : call B$RND`  -- ds:24E6
'   is just the SINGLE 1.0 passed as RND's mode arg, not a pointer.)


' --------------------------------------------------------------------------
SUB ComputeEquippedPower                                  ' asm: out.asm:4335 (sub_12823)
' --------------------------------------------------------------------------
' Folds the equipped weapon/armour + their wear (S1(), 0=Shoddy..4=Superb)
' into the numbers combat reads.  Runs once when an encounter starts.

    IF weaponSlot < 8 THEN
        defScratch = (S1(weaponSlot) MOD kk2970) + (dsWord(&h1AEC) - 9)   '?ord ' asm:4344
        '  kk2970 = 3.5 ; ds:1AEC role still unknown (read as `1AEC - 9`)  'CHECK
    END IF

    weaponPower = weaponId                                                ' asm:4365
    IF armorSlot < 8 THEN
        weaponPower = weaponId + (S1(armorSlot) MOD kk2974)               '?ord ' asm:4377
        '  kk2974 = 2.8
    END IF
END SUB


' --------------------------------------------------------------------------
SUB RollEncounterMod                                  ' asm: out.asm:3723 (rollCreatureStats)
' --------------------------------------------------------------------------
' Rolls encMod.  Default is the sentinel -1000 ("no modifier"); a minority
' of encounters get a real value.  Also called from enterOverworld.

    encMod = -1000                                                        ' asm:3725  (0xFC18)

    IF RND(1) < 0.40 THEN                    ' ds:2476              '?ord ' asm:3727 (FF1F, jb)
        '  ~40% (or ~60% -- depends which way FF1F's compare runs)          '??
        encMod = INT( RND(1) / 18.0  +  12.6 )       ' ds:2906, ds:290A '?ord ' asm:3753
        '  as written this is ~12 every time; if the divide is 18.0/RND(1)
        '  instead it spreads wide.  PRIME DOSBOX TARGET.                    '??
    END IF
END SUB


' --------------------------------------------------------------------------
SUB ResolvePlayerAttack                               ' asm: out.asm:6686 (resolvePlayerAttack)
' --------------------------------------------------------------------------
' One swing at creatureIndex: flavour text, to-hit vs RND, damage roll
' (three variants keyed on the creature's weapon-weakness), subtract from
' the enemy HP cell, CreatureDefeated when it reaches 0.

    ' ---- 1. to-hit fraction -------------------------------------------- asm:6687-6714
    '   hitScratch =  ( CSNG(Dexterity) / (weaponPower + 18) )
    '                   op0x10
    '                 ( enemyArmor / 11.0 )                    ' ds:2C26 = 11.0
    hitScratch = (Dexterity / (weaponPower + 18)) \ (enemyArmor / 11.0)   '?ord ' asm:6687-6714
    '  op 0x10 is integer-divide in the README table, but hitScratch is
    '  compared to RND(1) below and you only HIT when it is SMALL, so it
    '  must end up a fraction -- op 0x10 here is more likely `-`.           '??

    IF creatureWeak = weaponId THEN                                       ' asm:6717
        hitScratch = hitScratch + 1.0                        ' ds:24E6    ' asm:6723
    END IF

    PRINT "ATTACK "; Creature$(creatureIndex)                             ' asm:6730 (2E3E)
    PRINT "WITH "; Weapon$(weaponId); "."                                 ' asm:6756 (2E4A + 27D6)

    ' ---- 2. roll to hit ---------------------------------------------- asm:6789-6808
    IF hitScratch >= RND(1) THEN                                          '?ord ' asm:6803 (FF1F, jnb)
        PRINT "YOUR ATTACK MISSES."                                       ' asm:6812 (2E54)
        Delay 4                                                           ' asm:6823 (ds:23A6 = 4)
        StageSfx_Attack                                                   ' asm:6827
        EXIT SUB
    END IF

    ' ---- 3. base damage --------------------------------------------- asm:6830-6874
    '   workDamage = ( CSNG(Strength) / ((weaponPower MOD 6.0) + 0.5) )
    '                   op0x10
    '                ( RND(1) / 2.0  +  1.0 )
    workDamage = INT( (Strength / ((weaponPower MOD 6.0) + 0.5)) _
                      \ (RND(1) / 2.0 + 1.0) )               '?ord ' asm:6831-6874
    '  ds:2E6C = 6.0 ; ds:2482 = 0.5 ; ds:280A = 2.0 ; ds:24E6 = 1.0

    ' ---- 4. weapon-weakness branches ------------------------------- asm:6875-6934
    IF creatureWeak < 99 AND creatureWeak <> weaponId THEN                ' asm:6875-6892
        ' wrong weapon against a creature that HAS a weakness -> chip hit
        workDamage = INT( RND(1) / 4.0 _
                          + (weaponPower MOD 1.3) + 1.0 )     '?ord ' asm:6895-6912
        '  ds:279C = 4.0 ; ds:2C32 = 1.3 ; ds:24E6 = 1.0   (~1..3 dmg)
    END IF
    IF weaponId = creatureWeak THEN                                       ' asm:6915
        ' matching the creature's weakness -> the "one blow" hit
        workDamage = INT( Strength + RND(1) / 20.0 )          ' ds:24EA = 20.0 ' asm:6921-6934
        '  i.e. workDamage = Strength
    END IF

    PRINT "ENEMY HIT BY BLOW OF "; workDamage                            ' asm:6938 (2E70)
    Delay 2                                                               ' asm:6970
    StageSfx_Attack                                                       ' asm:6976

    ' ---- 5. apply to the enemy HP cell --------------------------- asm:6979-7005
    ViewObject(enemyHpCell) = ViewObject(enemyHpCell) - workDamage         ' asm:6980-6991
    IF ViewObject(enemyHpCell) > 0 THEN EXIT SUB                          ' asm:6996

    PRINT "THE "; Creature$(creatureIndex); " DIES."                      ' asm:7005
    CreatureDefeated
END SUB


' ==========================================================================
'  SOLID  vs.  NEEDS A DOSBOX WATCH
' ==========================================================================
'
'  SOLID:
'   * every operand, every constant value, every branch, the whole
'     miss / hit / 3-way damage / kill structure
'   * operator IDENTITY per thunk (leglib dispatch indices: +0 -4 *8 /C \10
'     MOD14 ^18 cmp1C -- see README)
'   * "your weapon == the creature's weak weapon"  ->  damage = Strength
'     (the guide's "goes down with one blow")
'   * "creature has a weakness, wrong weapon"       ->  ~1-3 chip damage
'   * creatureWeak = 99 is the "no weakness" sentinel
'   * combat math is single-precision; to-hit compares a fraction to RND(1),
'     and you HIT when the fraction is the SMALLER side
'
'  NEEDS CONFIRMING  ('?ord / '??):
'   1. operand order for the non-commutative middle ops.  `A op B` vs
'      `B op A` changes to-hit and base damage completely.
'   2. op 0x10 in the to-hit and base-damage lines: the README says `\`
'      (int divide) but the result has to be fractional there -- likely `-`.
'      One DOSBox watch on the write to ds:2192 / ds:208E during a fight
'      settles 1 and 2 together.
'   3. RollEncounterMod: RND(1)/18 vs 18/RND(1); and the 40 vs 60 % gate.
'   4. ds:1AEC (ComputeEquippedPower) and the exact weaponSlot/armorSlot
'      /weaponId roles.
'   5. where encMod is consumed (not here -- probably CreatureAttack).
