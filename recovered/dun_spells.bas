' ==========================================================================
'  DUN.EXE  --  the dungeon spell system                             [v1]
'  reconstructed from dun.asm (useMagicMenu / castSpell + the effect arms)
'  ; see recovered/README.md for the model + tags
'
'  The "M"agic command -> useMagicMenu.  Two menus:
'    - the DIRECT list: Magic flame, Firebolt   (attack spells)
'    - an "OTHER" option (shown only if you hold a Befuddle / Psycho
'      strength / Kill flash charge) -> falls through into castSpell.
'  Seek (LEGACY spell index 5, S2 slot 29) is NOT implemented in the
'  dungeon -- it is an overworld-only spell; castSpell just hides its
'  charge from the OTHER menu.
'
'  Spells map 1:1 to S2() charge slots (S2 = ds:1BC4, the item+spell
'  bitmap/count array; slots 24..29 = the 6 spell charges):
'      24 Magic flame   25 Firebolt   26 Befuddle
'      27 Psycho strength   28 Kill flash   29 Seek (unused here)
'  selectedSpell (ds:1E24) holds the S2 slot index.  Every cast does
'  S2(selectedSpell) -= 1.
'
'  DGROUP vars:
'     selectedSpell 1E24     Intelligence 1AF0     hitPoints 1ADA (cap 250)
'     targetRange   2140  (tiles to the current target; 5 = none/max)
'     monsterInView 20BE  (>= 0x10 means a monster is targetable)
'     confuseTimer  1AE6  ( >0 monster befuddled, <0 PLAYER befuddled;
'                           counts back toward 0 one step per turn )
'     psychoBuff    1AE8  (turns of +50% melee damage remaining)
'     spellDmg      21CC
'  constants (DUN.EXE DGROUP base 0x5F00):
'     ds:26F2 45.0   ds:270E 18.0   ds:22A2 0.05
'     ds:2302 10.0   ds:2772 25.0   ds:2516 20.0   ds:276E -5.0   ds:2752 0.93


' --------------------------------------------------------------------------
'  ATTACK SPELLS  --  Magic flame (24) / Firebolt (25)   [derived]
'  asm: useMagicMenu, dun.asm loc_11FE7 .. loc_12147
' --------------------------------------------------------------------------
SUB CastAttackSpell
    S2(selectedSpell) = S2(selectedSpell) - 1          ' consume a charge
    PRINT "SHOOT "; Spell$(selectedSpell); " AT ..."
    ClearTurnFlag

    ' ---- fizzle rolls (both must pass) --------------------------
    '   NOTE: leglib FF1F compares TOS vs the deeper operand (reversed
    '   from the arithmetic thunks).
    IF RND(1) > (Intelligence + 15) / 45.0 THEN GOTO Fizzle   ' Int 15 -> ~67% ok
    IF RND(1) < 0.05                        THEN GOTO Fizzle   ' flat 5% misfire

    Delay IIF(selectedSpell = 25, 23, 22)                       ' stun ticks

    IF monsterInView < &h10 THEN
        PRINT "THERE IS NO EFFECT." : EXIT SUB
    END IF

    ' ---- damage --------------------------------------------------
    spellDmg = INT( (45.0 / (targetRange + 1) + 18.0) _
                    * (RND(1) + 1.0) _
                    * IIF(selectedSpell = 25, 2, 1) )           ' Firebolt = x2
    '   range 1 (adjacent): (45/2 + 18) = 40.5  ->  x1..2  ->  ~40..81
    '   range 5 (far)      : (45/6 + 18) = 25.5  ->  ~25..51
    '   Firebolt doubles all of the above.
    ApplyDamageToMonster spellDmg
    EXIT SUB

Fizzle:
    IF targetRange < 2 THEN ds208C = 0                          ' (adjacent quirk)
    Delay IIF(selectedSpell = 25, 25, 24)
    PRINT "ATTACK FIZZLES"
END SUB


' --------------------------------------------------------------------------
'  BEFUDDLE  (26)   [derived]
'  asm: castSpell ON-GOTO arm 1 (= j_clearTurnFlag, the shared resume) ->
'       dun.asm loc_1234C (backfire) / loc_123BB (normal)
' --------------------------------------------------------------------------
SUB CastBefuddle
    S2(26) = S2(26) - 1
    PRINT "CAST BEFUDDLE"
    ClearTurnFlag
    RedrawDungeonView

    ' ---- backfire check ---------------------------------------- asm:loc_12337
    IF hitPoints >= 250 AND RND(1) <= 0.93 THEN
        PRINT "THE SPELL BACKFIRES!"
        confuseTimer = INT( -5.0 - RND(1) )        ' ~ -5 / -6  -> YOU are befuddled
        FlashEffect                                ' sub_10336
        EXIT SUB
    END IF

    ' ---- normal: confuse the monster -------------------------- asm:loc_123BB
    confuseTimer = INT( confuseTimer \ 2 + RND(1) * 10.0 + 25.0 )   ' ~25..40 turns
    IF monsterInView > &h0F THEN PRINT Monster$; " LOOKS CONFUSED"
END SUB
' Per turn, dunMain: confuseTimer < 0 -> "YOU ARE BEFUDDLED." + confuseTimer++;
'                    confuseTimer > 0 -> the monster skips its turn + confuseTimer--.


' --------------------------------------------------------------------------
'  PSYCHO STRENGTH  (27)   [derived]
'  asm: castSpell ON-GOTO arm 2 = psychoStrengthSpell (dun.asm:5184)
' --------------------------------------------------------------------------
SUB CastPsychoStrength
    S2(27) = S2(27) - 1
    PRINT "CAST PSYCHO STRENGTH"
    ClearTurnFlag
    IF psychoBuff > 0 THEN
        PRINT "(PSYCHO STRENGTH SPELL ALREADY IN EFFECT)"
        S2(27) = S2(27) + 1                        ' refund the charge
        EXIT SUB
    END IF
    PRINT "YOU FEEL VERY STRONG!"
    psychoBuff = INT( RND(1) * 10.0 + 20.0 )       ' 20..29 turns of +50% melee
    Delay &h1B
END SUB
' While psychoBuff > 0, DoAttack's damage is x1.5 (dun_combat.bas).
' psychoBuff is zeroed on every dungeon-level change (processTileFeature).


' --------------------------------------------------------------------------
'  KILL FLASH  (28)   [derived]
'  asm: castSpell ON-GOTO arm 3 = sub_124FE (dun.asm:5270)
' --------------------------------------------------------------------------
SUB CastKillFlash
    S2(28) = S2(28) - 1
    PRINT "CAST KILL FLASH"
    ClearTurnFlag
    Delay 6
    KillFlashAnim 26                               ' sub_12ACA -- 26 flash frames
    ClearViewObjects                               ' removes ALL 8 monster slots
                                                    ' from the map + view
    Delay 6
    ScreenFlash                                    ' rt_FE01
END SUB
' No damage roll, no save -- every monster currently loaded in the view
' is simply gone.  (New monsters can still wander in afterwards.)


' ==========================================================================
'  SOLID
'   * spell <-> S2 slot: 24 flame / 25 Firebolt / 26 Befuddle /
'     27 Psycho strength / 28 Kill flash / 29 Seek (Seek unused in DUN)
'   * every cast: S2(slot) -= 1
'   * attack-spell fizzle: RND(1) > (Int+15)/45  OR  RND(1) < 0.05
'   * attack-spell damage: INT( (45/(range+1) + 18) * (RND(1)+1)
'                               * (Firebolt ? 2 : 1) )
'   * Befuddle: monster confused INT( old\2 + RND*10 + 25 ) turns; at full
'     HP a 0.93 roll backfires -> confuseTimer = INT(-5 - RND) (YOU befuddled)
'   * confuseTimer (ds:1AE6): >0 monster, <0 player; steps toward 0/turn
'   * Psycho strength: psychoBuff (ds:1AE8) = INT( RND*10 + 20 ) turns of
'     +50% melee; refunds the charge if already active; cleared per level
'   * Kill flash: clearViewObjects -- wipes all 8 monster slots, no roll
'
'  OPEN
'   * the exact rt_FE57 menu row<->spell mapping (menuChoice + 15 = slot;
'     row 9=flame, 10=Firebolt, 11=OTHER/cancel -- inferred, not traced
'     through selectAbove)
'   * ds:208C = 0 on an adjacent-target fizzle -- purpose unclear
'   * whether Befuddle's backfire condition is really "hitPoints >= 250"
'     (the DUN HP cap) or the compare polarity is flipped
' ==========================================================================
