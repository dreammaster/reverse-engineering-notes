' ==========================================================================
'  CASDR.EXE  --  castle / fortress interiors                         [v2]
'  reconstructed from casdr.asm ; see recovered/README.md for the model + tags
'
'  SUBs: DoFight (player attack), CastleEnemyTurn / EnemyAttack,
'        AttackHit (incoming melee), GasDamage,
'        WarlordConfrontation / WarlordAttack,
'        DescribeRoom + DescribeChest / DescribeGasRoom /
'        DescribePotionShop / DescribeLockedDoor,
'        OpenCommand / UseKey / ResolveUseKey / TakeChestItem (doors +
'        the castle box), FortressSelfDestruct, WarlordConfrontation
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
SUB WarlordConfrontation                              ' asm: casdr.asm:5104 (warlordConfrontation)
' --------------------------------------------------------------------------
' The climactic scene -- triggered by moveBlocked when you walk into the
' final wall on the fort's last level.  A scripted cinematic, then the
' boss fight.
'
'   targetSlot(ds:1F24) = &hFF
'   PRINT "YOU SEE THE "; warlord$
'   ' -- the monologue, one line per Pager() call (sub_1270F = print+hold),
'   '    interleaved with rt_FE54 tones + rt_FE28 flashes:
'   '      "SONIC MAGIC..."   "YOU CAN'T MOVE."
'   '      "THE WARLORD APPEARS AT THE WALL."   (patches map tiles to show him)
'   *** hitPoints(ds:1ADA) = 28  (&h1C) ***       ' the sonic paralysis leaves
'                                                 ' you at near-death
'   '      "YOU FOOL!"  "YOU CAN'T STOP ME!"  "AS YOU"
'   '      "STAND HELPLESS, I'LL USE THIS SCROLL"
'   '      "TO CAST THE SPELL OF DEATH.  ALL LIFE"
'   '      "OUTSIDE THIS FORTRESS WILL CEASE."
'   questMarkState(ds:22E6) = &hFF                ' the forearm-mark quest state
'   ' -- draw the Warlord sprite (sub_13E33 with ds:1F04..1F12 screen params),
'   '    run the approach animation loops --
'   ds:20B6 = 1                                   ' the "Warlord fight active" flag
'
' You now fight the Warlord in melee at 28 HP (DoFight in / WarlordAttack
' out).  Winning -> FortressSelfDestruct.


' --------------------------------------------------------------------------
SUB WarlordAttack                                     ' asm: casdr.asm:6048 (warlordAttack)
' --------------------------------------------------------------------------
' The Warlord's blow during that fight -- announced ("WARLORD ATTACK -
' BLOW n"); the subtraction from hitPoints is applied by the caller
' (doWalk / castleTurnUpdate) with this value.
    warlordBlow = INT( RND(1) * 99.0 + 80.0 )     ' ds:28A0 99 ; ds:2B94 80 ' asm:6050-6076
    '  80 .. 178 damage per blow  (confirmed: RND -> FF4E 99 -> FF44 80 -> INT).
    '  -- at 28 HP a single blow can one-shot you: the fight is meant to be
    '     won fast (Invisibility / Weaken / a big first strike).
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
'                 [FF49 = TOS / const CONFIRMED (leglib static) ; so
'                  K = Dex/26 is real, and with K in the DENOMINATOR of
'                  toHit below, higher Dex genuinely LOWERS the castle
'                  hit-rate -- an apparent original quirk, not a mis-read]

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
SUB CastleEnemyTurn                                   ' asm: casdr.asm:5621 (sub_127C8)
' --------------------------------------------------------------------------
' The per-turn castle-enemy update -- doWalk calls it after the player
' moves (CASDR's answer to DUN's moveMonsters).  enemyAtk = ds:20B8.
'
'   IF S2(15) > 0 THEN EXIT SUB                 ' hold the Compendium -> no guards
'   IF enemyAtk = 0 THEN                        ' no enemy -> SPAWN one
'       enemyAtk = 140                          ' ds:20B8 <- &h8C
'       guardScale(ds:230C) = INT( S2(3) \ 9 )  ' a castle-progress scaled value
'       EXIT SUB                                ' (no blow the turn it appears)
'   END IF
'   EnemyAttack                                 ' the enemy is here -> it hits you
' A guard is "killed" / leaves elsewhere by zeroing ds:20B8 ; the Weaken
' item multiplies ds:20B8 by 0.96 per cast (see UseKey) until it drops
' to <= &h50, then "THE ATTACK STOPS."


' --------------------------------------------------------------------------
SUB EnemyAttack                                       ' asm: casdr.asm:5683 (enemyAttack)
' --------------------------------------------------------------------------
' A non-Warlord castle enemy's blow.  Entered mid-expression from
' CastleEnemyTurn, which has already computed  half = enemyAtk \ 2.
'
'   r      = RND(1)
'   raw    = r * enemyAtk                       ' FF4C
'   damage = INT( raw - INT(raw)\2 - half )     ' rt_14 halves INT(raw); FF21/FF28/FF22
'          ' ~= enemyAtk * (1 - r) / 2   ->   0 .. enemyAtk\2
'          ' enemyAtk 140 (fresh guard) -> damage 0..70, mean ~35
'   PRINT enemyName$; " ATTACK - BLOW "; damage; " H.P."     ' ds:2B5C / ds:28D0
'   Delay &h17
'   hitPoints = hitPoints - damage              ' (message tail / caller)
'
' NOTE: the value-stack ops are now settled -- FF23 POPS, FF28 = TOS-TOS1
' (see leglib_runtime.c).  But this SUB is entered mid-expression and the
' FP stack has ONE operand where FF28 wants two, so a leftover operand from
' the caller's expression is unaccounted for.  The magnitude
' (~ enemyAtk*(1-RND)/2) is the sane reading ; the exact `raw - INT(raw)\2
' - half` shape needs a live dump of the FP stack at the FF28 call.
' *derived, shape-uncertain*


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


' --------------------------------------------------------------------------
SUB OpenCommand                                      ' asm: casdr.asm:13651 (openDoor)
' --------------------------------------------------------------------------
' The "OPEN" command -- handles doors, the big castle box, and boxes.
' castleOrFort = ds:20C0 (1 = fort, 2 = castle).   tileAhead = ds:1F02.
'
'   ' --- permanently locked (need a key -- see UseKey) ---
'   IF fort  AND tileAhead IN (&hC0..&hC2, &hCB, &hCC, &hDA) THEN
'       PRINT "DOORS LOCKED." : EXIT SUB
'   END IF
'
'   ' --- the big castle box (tile &hC3) : a 2x2 tile group ---
'   IF castle AND tileAhead = &hC3 THEN
'       IF bigBoxOpen(ds:20B2) > 0 THEN PRINT "BOX OPEN ALREADY." : EXIT SUB
'       ' reveal the 4 cells (map words 52AC/52AD/531C/531D), Delay &hE
'       bigBoxOpen = 1                       ' now walk up + TAKE (tile &hDF)
'       EXIT SUB
'   END IF
'
'   ' --- ordinary door ---
'   ResolveOpenDoor                          ' rewrites the tile ; sets doorResult ds:1F04
'   SELECT CASE doorResult
'   CASE 0 : PRINT "NOTHING OPENS"           ' (castle only)
'   CASE 1 : PRINT "... ALREADY OPEN."
'   CASE 3 : PRINT "-TOO FAR."
'   CASE 2 : ' a NOISY open --
'       FacePlayerDirection : CheckLineOfSight
'       SpottedByGuard                       ' "YOU ARE SPOTTED NOW!"
'       IF RND(1) < 0.15 AND S5(tileAhead) > 99 AND S5(tileAhead) < 400 THEN
'           ' S5 (ds:1BF2) IS used by CASDR -- the per-tile door/lock data
'           ' table (NOT the dead shop-price table it is in TWNDR).
'           ...                              ' a further reaction (rtm_FE38)
'       END IF
'   END SELECT
END SUB


' --------------------------------------------------------------------------
SUB UseKey  /  ResolveUseKey                         ' asm: casdr.asm:13ad5 / 14f74
' --------------------------------------------------------------------------
' The "USE <item>" command.  itemKind = ds:1ADC :
'     3   Healing herbs -> hitPoints = MIN(maxHP, hitPoints + maxHP\2)
'     4..7 a KEY        -> ResolveUseKey  (match the door ahead)
'     8   Invisibility  -> "YOU'RE INVISIBLE"
'     &hC Compendium    -> (reveals info ; doorResult = 0)
'     &hE Weaken        -> enemy attack (ds:20B8) *= 0.96 per cast until it
'                          drops <= 0x50, then "THE ATTACK STOPS."
'     else              -> "NO EFFECT"
'   On use the charge is spent: S2(itemKind) -= 1  (0 clears the cursor).
'
' ResolveUseKey(x, y, keyKind) -- the door <-> key table (tileAhead ->
' required key kind):
'     &hC0 -> key 4      &hC1 -> key 7      &hDA -> key 7
'     &hCB -> key 8      &hE6 -> key 5      &hE7 -> key 6
'     (any other tile -> doorResult 0 -> "THIS KEY DOES NOTHING HERE.")
'   keyKind = requiredKey  -> doorResult 1 -> "UNLOCK DOOR." + open anim
'   keyKind <> requiredKey -> doorResult 2 -> "THIS KEY DOES NOTHING HERE."


' --------------------------------------------------------------------------
SUB TakeChestItem                                    ' asm: casdr.asm:134f6 (takeItem, tile &hDF)
' --------------------------------------------------------------------------
' After OpenCommand has revealed the castle box, walk onto it and "TAKE".
'     IF S2(15) > 0 THEN PRINT "NOTHING TO TAKE" : EXIT SUB   ' already looted
'     IF enemyActive(ds:20B8) > 0 THEN PRINT "YOU CAN'T HOLD IT." : EXIT SUB
'     PRINT "YOU GRAB THE "; Item$(15); "."     ' Item$ index 15 = the Compendium
'     ' a grab animation ; patch two map tiles ; big redraw
'     S2(15) = 1                                ' the "holds Compendium" bit (once)
'     ds:20BA = &h320                           ' (a post-grab timer / alarm)
' The castle has NO gold economy -- the chest yields exactly this one
' quest item, never gold.


' --------------------------------------------------------------------------
SUB FortressSelfDestruct                             ' asm: casdr.asm:11bc8 (+ sub_11CBA)
' --------------------------------------------------------------------------
' Triggered the moment the Warlord dies ("** WARLORD KILLED **").
'     PRINT banner
'     FOR i = 0 TO 4 : Delay &h13 : NEXT
'     PlayKlaxon 900                            ' rt_FE54
'     Delay &h1B
'     Pager "SECURITY ALERT...** "              ' sub_11CBA = print line + hold,
'     Pager "OUR LEADER HAS BEEN KILLED.  BLOCK"'   a keypress skips it (with a beep)
'     Pager "ALL DOORS.  EXPLOSIVE CHARGES SET. "
'     Pager "SELF-DESTRUCTION IN 5 MINUTES!"
'     ds:20BC = &h5F00                          ' the escape allowance (turn units)
'     Sub_14F0C &hBE
'     FacePlayerDirection
' After this you must reach the castle exit before the allowance runs out;
' the guards "block all doors" (the door state flags flip).  The "5
' MINUTES" is the on-screen text -- the real limit is the ds:20BC budget
' decremented per turn (exact per-turn cost not pinned).


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
'   * CHEST (castle box): OPEN reveals a 2x2 tile group (tile &hC3),
'     then TAKE (tile &hDF) grants Item$(15) = the Compendium, gated once
'     by S2(15) ; blocked while an enemy is active ; NO gold ever
'   * LOCKED DOORS: fort tiles &hC0..&hC2/&hCB/&hCC/&hDA = "DOORS LOCKED"
'     on a plain OPEN ; UseKey matches a specific key kind per tile
'     (&hC0->4, &hC1/&hDA->7, &hCB->8, &hE6->5, &hE7->6) -> "UNLOCK DOOR."
'   * S5 (ds:1BF2) IS live in CASDR -- the per-tile door/lock data table
'     (it is only dead in TWNDR)
'   * WARLORD CONFRONTATION (moveBlocked -> the final wall): scripted
'     monologue ("SPELL OF DEATH...") ; FORCES hitPoints = 28 (&h1C) ;
'     questMarkState = &hFF ; ds:20B6 = 1 ; spawns the Warlord for melee.
'     Warlord blow (warlordAttack) = INT( RND(1)*99 + 80 ) = 80..178.
'   * SELF-DESTRUCT: Warlord death -> "** WARLORD KILLED **" cinematic +
'     "SELF-DESTRUCTION IN 5 MINUTES!" ; ds:20BC = &h5F00 escape budget,
'     ticked down per turn ; guards block the doors
'   * Weaken item: enemy attack (ds:20B8) *= 0.96 per cast (ds:3162)
'   * noisy-door spot roll: RND(1) < 0.15 (ds:3054)
'   * CASTLE ENEMY: sub_127C8 spawns a guard (enemyAtk = 140) when none
'     is active ; EnemyAttack blow ~= INT( enemyAtk * (1 - RND(1)) / 2 )
'     -> 0..70 for a fresh guard (mean ~35), no armour/Endurance mitigation
'
'  RESOLVED (leglib static, 2026-09-06 -- see leglib_runtime.c)
'   * FF1F compare = reversed (Jcc tests TOS <cmp> TOS1) -- confirmed
'   * FF49 = /rev, immediate form = TOS / const -- confirmed.  So
'     ds:2214 = Dex/26 IS a genuine division ; whether it lands in the
'     numerator or denominator of the to-hit ratio (making higher Dex
'     better or worse) is a placement question, not an FF49 polarity one
'   * FF22 / FF23 pop their operand
'
'  OPEN
'   * gas damage `base` (stack leftover)
'   * the per-turn ds:20BC decrement (self-destruct budget cost)
'   * EnemyAttack's exact `raw - INT(raw)\2 - half` shape -- FP stack is
'     one operand short at the FF28 call (a leftover from the caller's
'     mid-expression entry) ; needs a live FP-stack dump
'   * ds:230C = INT(S2(3)\9) purpose (written, not obviously read)
