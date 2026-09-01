' ==========================================================================
'  OUT.EXE  --  overworld combat, player-attack path            [PILOT v3]
'  reconstructed from out.asm ; see recovered/README.md for the model + tags
' ==========================================================================
'
'  v3: the leglib op-dispatch table was read straight out of LEGLIB.EXE
'  (ds:0F7C, 8 entries).  It is NOT the canonical BASIC operator order --
'  it is  + - -rev * / /rev cmp  (see README).  So the earlier "\ MOD ^"
'  guesses were wrong.  Every operator below is now pinned; what remains
'  open is the to-hit (resolvePlayerAttack is entered with a value already
'  on the stack, and its first op `FF2B` is a non-table binary op).
'
'  Verified against Paul's DOSBox trace (knife vs neural cloud):
'     Dex 16, Str 19, weaponPower 2, creatureHP 50, creatureWeak 3,
'     weaponId 1  ->  "ENEMY HIT BY BLOW OF 6"  /  a separate "MISSES"
'
'  leglib op table (ds:0F7C, index = 0/4/8/C/10/14/18/1C):
'     0x00  +            FF44 (imm) / FF42 (stack)
'     0x04  -            (a - b)
'     0x08  - reversed   FF53                 (b - a)
'     0x0C  *            FF4E (imm) / FF4C (stack)
'     0x10  /            FF47                 (TOS1 / TOS)
'     0x14  / reversed   FF49                 (TOS / TOS1; as imm: TOS / imm)
'     0x18  compare      FF1F
'  RND(1) = `push ds:24E8 : push ds:24E6 : call B$RND`  (ds:24E6 = SINGLE 1.0)
'
'  SINGLE constant pool (decoders/dgroup_consts.py OUT.EXE -- Paul: please
'  spot-check ds:2C26, ds:279C, ds:2C32, ds:2E6C in DOSBox):
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
SUB ResolvePlayerAttack                               ' asm: out.asm:6686 (resolvePlayerAttack)
' --------------------------------------------------------------------------

    ' ---- 1. to-hit score  --  NOT YET SOLVED --------------------------- asm:6687-6714
    '   push CSNG(Dexterity)                                    ' asm:6687
    '   FF2B                    <- binary, POPS a 2nd operand   ' asm:6691  '??
    '   push (weaponPower + 18)                                 ' asm:6694
    '   FF4C  (*)                                               ' asm:6698
    '   push creatureHP                                         ' asm:6702
    '   FF4E ds:2C26 (*)   -> creatureHP * 11.0                 ' asm:6706
    '   FF47 (/)          -> (...) / (creatureHP * 11)          ' asm:6710
    '   pop -> hitScratch (ds:208E)                             ' asm:6713
    '
    '   With the table as read, this is
    '       hitScratch = (X {FF2B} Dex) * (weaponPower+18) / (creatureHP * 11)
    '   where X is whatever the caller (creatureAttack / doAttackOrCast,
    '   both partly collapsed) left on the value stack, and FF2B is a
    '   non-dispatch-table op (leglib handler seg004:0x3954 -- looks like
    '   `\` or MOD: it zero-checks both exponents and copies both operands
    '   to work buffers).  Paul's trace: hitScratch = 0.334167 for the
    '   inputs above -- the (Dex*(wp+18))/(creatureHP*11) part alone gives
    '   0.5818, so the FF2B operand/op accounts for the rest.             'CHECK
    hitScratch = ToHitScore()                                 '?? ' asm:6687-6714

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


' ==========================================================================
'  STATUS
' ==========================================================================
'  SOLID (operators pinned from LEGLIB.EXE; formulas below verified or
'  self-consistent):
'   * op table = + - -rev * / /rev cmp   (README)
'   * RollEncounterMod   = INT( RND(1)*18 + 12.6 )              [12..30]
'   * base damage        = INT( Str * (wp/6 + 1/2) / (2*RND(1) + 1) )
'   * chip damage        = INT( 4*RND(1) + wp/1.3 + 1 )
'   * weakness-match dmg  = INT( Str + 20*RND(1) )
'   * creatureHP  = creatureStatWord MOD 256   (out.asm:3924-3943)
'   * creatureWeak = creatureWeakWord MOD 256, 99 = none  (3944-3963)
'   * to hit: HIT when RND(1) < hitScratch; weakness-match forces 1.0
'
'  OPEN:
'   1. the to-hit score.  Needs: (a) the value on the stack when
'      resolvePlayerAttack is entered -- break at out.asm:6687, dump the
'      value stack (12-byte slots from [ds:0FAC] up to [ds:111C]); (b) the
'      FF2B op (leglib seg004:0x3954).  Then one more trace: break at the
'      store to ds:208E (out.asm:6713) and read it as a 4-byte float, plus
'      dump ds:2C26 / ds:279C / ds:2C32 / ds:2E6C to confirm the pool.
'   2. RollEncounterMod: is the 0.40 gate a < or >= (40% vs 60%)?
'   3. spell damage (doAttackOrCast:6507-6523) -- structure known, needs
'      ds:231C / ds:2502 / ds:2DD0.
'   4. ComputeEquippedPower (sub_12823) -- redo with the corrected ops.
'   5. CreatureAttack (out.asm:3468, collapsed) -- the monster half.
