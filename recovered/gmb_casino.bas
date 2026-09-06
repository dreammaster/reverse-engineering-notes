' ==========================================================================
'  GMB1.EXE / GMB2.EXE  --  the casino minigames                       [v2]
'  reconstructed from gmb1.asm (blackjackMain) / gmb2.asm (flipFlopMain,
'  computePayout, playRound) ; see recovered/README.md
'
'  Two standalone gambling games chained from a town (TWNDR.EXE) casino.
'  Both share the party's gold dword  ds:1AD2:1AD4  with the caller, so any
'  net change rides the normal save path.  Neither touches any other stat
'  EXCEPT: GMB2 also uses S4(14)/S4(15) (byte 0x1C/0x1E) as its rigged
'  win-rate ledger, and both read ds:1AE0 (character level) for the cap.
'  On entry, if ds:1AC0 = 0 (uninitialised session) gold is seeded to 1000.
'
'  *** THE "BREAK THE BANK" CAP  (both games)  ***
'  At session start each game snapshots your gold and computes
'      winCap = 250 * characterLevel + 750          ' 0xFA*ds:1AE0 + 0x2EE
'  After every settled round it checks  (gold - startGold):
'      > winCap  -> "You broke the bank!  The house is closed."  (keep it
'                   all, set the exit flag, chain to TWNDR.EXE)
'  So a level-1 party is cut off at +1000 net; level 10 at +3250.
'  gmb1: startGold ds:2092:2094 ; gmb2: startGold ds:20D2:20D4,
'  cap ds:20D6:20D8, running win total ds:209A.
'
'  These are optional and self-contained -- a port can implement the RULES
'  below and skip the sprite / ball-physics code entirely.
' ==========================================================================


' ==========================================================================
'  GMB1.EXE  --  Blackjack ("21")            gmb1.asm : blackjackMain (~3.2 KB)
' ==========================================================================
'  Standard casino blackjack vs. a dealer.  bet -> ds:1F02.
'    - "Enter your bet.  Enter 0 to quit."   (bet is clamped to your gold)
'    - deal 2 + 2 ; player hits / stays ; dealer draws to >= 17
'    - outcome strings: "Dealer has BlackJack." / "You're over 21 - you
'      lose." / "It's a tie." / "You Win!!" / "Dealer busts with <n>" /
'      "Five cards without going over 21!" (5-card Charlie, auto win) /
'      "Natural BlackJack pays double."
'
'  DECK + SHUFFLE -- FULLY DECODED 2026-09-07, the game is FAIR (no rig):
'    * The deck is 52 static (rank, suit) words in seg004 @ 0x101E, in
'      fixed order K..A x suit for suits 3,2,1,0 (rank 1..13, suit 0..3).
'    * shuffleDeck (sub_11966) = a real FISHER-YATES over all 52, using a
'      per-game 32-bit rotate-add LCG sub_1199E:
'          state = ((state <<< 1) + 0x39EF) ; return state>>1 ; state at
'          seg004 [0x1018:0x101A]         (NOT B$RND -- its own generator)
'      then resets the draw cursor and ds:1F1A (cards left) = 52.
'    * drawFromDeck (sub_11934): take deck[cursor], cursor += 2,
'      dec ds:1F1A ; if ds:1F1A hits 0 -> shuffleDeck ("Shuffling...").
'
'  HAND SCORING -- dealCardToHand (gmb1.asm:1708) :
'      cardVal = MIN(rank, 10)                    ' J/Q/K -> 10
'      IF rank = 1 (ACE) AND handTotal < 11 THEN  ' 11 would not bust
'          handTotal = handTotal + 11             ' soft ace
'      ELSE
'          handTotal = handTotal + cardVal        ' ace as 1
'      END IF
'    (matches the rules screen: "Aces count as one or eleven.")
'  The dealer's draw is a loop in the same SUB: keep hitting while
'  handTotal <= 16 (i.e. stop at 17+), also guarded by "not already bust"
'  and a cap (ds:20F6).  The player's HIT calls it once per keypress.
'
'  SETTLEMENT -- the bet is NOT escrowed; the round is settled net at the
'  end against the gold value from the START of the round:
'      loss / bust        partyGold = partyGold - bet        ' gmb1.asm:loc_10A08
'      tie / push         partyGold unchanged                ' (neither path taken)
'      ordinary win       partyGold = partyGold + bet        ' gmb1.asm:loc_10A2B
'      5-card Charlie     partyGold = partyGold + bet        ' (routes to the win path)
'      natural BlackJack  partyGold = partyGold + 2 * bet    ' gmb1.asm:952-958 (shl bet,1)
'
'  BROKE:
'    - gold hits 0 mid-session -> "You're out of gold."  If you had > 9
'      gold at session start, the house stakes you: "Rotten luck.  Here's
'      five [gold]" (partyGold += 5) so you can keep playing; otherwise
'      "Come back when you have some gold." -> chains to TWNDR.EXE.
SUB BlackjackSettle                                  ' asm: gmb1.asm ~loc_10A08 / loc_10A2B
    SELECT CASE outcome
    CASE LOSS, BUST      : partyGold = partyGold - bet
    CASE TIE            : ' unchanged
    CASE WIN, FIVE_CARD  : partyGold = partyGold + bet
    CASE NATURAL        : partyGold = partyGold + 2 * bet
    END SELECT

    IF partyGold - startGold > 250 * characterLevel + 750 THEN
        PRINT "You broke the bank!  The house is closed."
        exitFlag = 1                                  ' ds:1F2A -> chain to TWNDR
    END IF
END SUB


' ==========================================================================
'  GMB2.EXE  --  "Flip-Flop Parlour"   (a Plinko / bagatelle drop)
'                    gmb2.asm : flipFlopMain / playRound / computePayout
' ==========================================================================
'  "** FLIP-FLOP PARLOUR **".  Bet an amount (ds:2098) on a bucket COLOUR
'  (ds:2092), then in dropBallAndBounce type a bucket NUMBER to release
'  the ball, WATCH the flip-flop bumpers, and STEER a 1..6 selector with
'  the LEFT / RIGHT arrow keys, ENTER to lock in your guess.
'
'  *** THERE IS NO BALL PHYSICS ***  (2026-09-07)
'  stepBallPhysics just copies (x,y) into display vars ds:20A4/ds:20A6 --
'  it computes no bounce.  The landing bucket = whatever you steered
'  ds:209C to (clamped 1..6).  drawBumpers is a static GW-BASIC DRAW-macro
'  picture; the "flip-flop" is cosmetic animation only.  The instruction
'  screen says so outright:  "FLIP-FLOP IS AS MUCH SKILL AS CHANCE. WATCH
'  THE FLIPPERS AND CHOOSE BUCKETS CAREFULLY!"  and  "* IF TIME RUNS OUT,
'  WE'LL DROP THE BALL AND PRETEND YOU CHOSE BUCKET 6."
'
'  TIMER:  the drop animation runs 6 frames ([bp-0Ch]=6, one dec/frame).
'  If you don't press ENTER before it expires AND this is not the first
'  drop -> forced landingBucket = 6.  The FIRST drop of a session is
'  untimed ([bp-1Ah] = -1, set when S4(16) is still 0).
'
'  DATA TABLES (parsed from text by rtm_AD -- gmb2.asm:~9998):
'    ds:2100(1..6) = 1, 1, 2, 2, 5, 5      ' *** the bucket payout multipliers ***
'    ds:210C(1..8) = 245,224,203,182,168,147,126,112   ' ball-drop start X per
'                                          '   bucket (screen px) -- COSMETIC
'    ds:211C(1..8) = 26,22,17,14,12,10,9,8 ' bumper sprite X -- COSMETIC
'    then ~15 more DATA lines: the triangular bumper (x,y) grid
'    (rows at y = 16/32/48/64/80/96/112), two "flip-flop" state vectors
'    (0/1 per bumper), the 5 drop-frame Y coords (250,315,380,445,510),
'    and per-frame animation offset pairs -- ALL COSMETIC.
'  Only ds:2100 matters to game logic.
'
'  PAYOUT  --  playRound, gmb2.asm:loc_11F4D :
'      mult = multTable(landingBucket)          ' ds:2100(b) : 1 / 2 / 5 / 0
'         mult 1 -> "EVEN MONEY."   (buckets 1-2)
'         mult 2 -> "DOUBLE."       (buckets 3-4)
'         mult 5 -> "FIVE TIMES."   (buckets 5-6)
'         else   -> no win          (buckets 7-8 / miss)
'      baseWin = mult * bet
'      IF ballColour = calledColour THEN                       ' ds:2092 vs [bp-14]
'          colourBonus = INT( baseWin * ds:2AA6 )              ' ds:2AA6 ~= 0 (see below)
'      ELSE
'          colourBonus = 0
'      END IF
'      ' *** ds:2AA6 does NOT hold a clean single in the packed EXE (reads
'      '     ~2.8e-13) -- so INT(baseWin * ds:2AA6) = 0 always.  The
'      '     advertised "COLOR BONUS" (playRound only prints it when the
'      '     value > 0) is effectively DEAD -- a zeroed / never-initialised
'      '     constant.  ds:2AA2 = 0.5 sits just before it in the pool. ***
'      totalWin  = baseWin + colourBonus
'      partyGold = partyGold + totalWin                        ' gmb2.asm:loc_1202A
'      sessionWon (ds:209A) += totalWin
'      '  feed the rigged ledger (scaled by ds:2ABC = 99.0):
'      S4(14) = INT( S4(14) + totalWin / 99.0 )    ' won-side accumulator
'
'  LOSS (landingBucket <> betBucket)  --  playRound, gmb2.asm:loc_1219B :
'      partyGold  = partyGold - bet                ' the bet is NOT escrowed
'      sessionWon = sessionWon - bet
'      S4(15) = INT( S4(15) + bet / 99.0 )         ' wagered-side accumulator
'      IF S4(15) > 30000 THEN                      ' ledger overflow guard
'          S4(14) = 99 : S4(15) = INT( bet/99 + ds:2AC0 )
'      END IF
'  (So: net win = bet*mult, net loss = -bet.  Buckets 1-2 are an even-money
'   game, 3-4 pay 2:1, 5-6 pay 5:1.)
'  Then the break-the-bank check (sessionWon vs 250*level + 750).
'
'  *** THE PARLOUR IS RIGGED ***  --  computePayout (gmb2.asm:4626), run
'  BEFORE the ball is scored.  FF1F is CONFIRMED reversed (Jcc tests
'  TOS <cmp> TOS1), so with the stack [const, ratio]:
'      ratio = S4(14) / S4(15)              ' realised payback = won / wagered
'      IF ratio > 0.94  (ds:2B3C)  THEN     ' you're getting too much back  [ja]
'          S4(14) = 99 : S4(15) = 99        ' wipe the ledger (ratio -> 1.0)
'          IF landingBucket < 8 THEN landingBucket += 1   ' nudge toward the 7-8 LOSS zone
'      ELSEIF ratio < ds:2B40  THEN         ' [jb]
'          S4(14) = 99 : S4(15) = 99
'          IF landingBucket > 1 THEN landingBucket -= 1    ' nudge toward the 1-2 small-win zone
'      END IF
'  *** ds:2B40 also does NOT hold a clean single (reads ~7e-13 ~= 0), and
'  `ratio` (won/wagered) is always >= 0, so `ratio < ds:2B40` is NEVER
'  true -- the "player is behind, give them a break" branch is DEAD.
'  The rig is ONE-DIRECTIONAL: it only ever shoves your bucket toward a
'  loss, whenever your realised payback exceeds ~94 %.  ds:2B38 = 1.4 and
'  ds:2B3C = 0.94 are the only two clean band constants in the pool. ***
'  The NPC hints at it ("...pretend you chose bucket 6.  BETTER LUCK NEXT
'  TIME.").
'
'  playPracticeRound -- a free no-stakes mode ("WANT PRACTICE?");
'  computePayout is skipped and gold never moves.


' ==========================================================================
'  SOLID
'   * both games: session ends ("broke the bank" / house closed) once
'     (gold - startGold) > 250*characterLevel + 750
'   * both only move ds:1AD2:1AD4 (party gold), plus GMB2's S4(14)/S4(15)
'     rig ledger
'   * blackjack net settlement: -bet loss/bust, +bet win/5-card,
'     +2*bet natural, 0 tie ; bet never escrowed
'   * blackjack broke: +5 "here's five" pity stake if startGold > 9
'   * flip-flop payout = multTable{1,2,5}(bucket) * bet  (the "COLOR
'     BONUS" is dead -- ds:2AA6 ~= 0 in the packed EXE)
'   * flip-flop computePayout: ONE-DIRECTIONAL rig -- when realised
'     payback S4(14)/S4(15) > 0.94 (ds:2B3C) it shoves the bucket +1
'     toward a loss ; the "player behind -> nudge -1" branch is dead
'     (ds:2B40 ~= 0, ratio always >= 0).  ledger resets to 99/99.
'   * flip-flop ledger accumulation is scaled by ds:2ABC = 99.0
'
'  RESOLVED 2026-09-07
'   * BLACKJACK is FAIR: 52 static (rank,suit) words in seg004 @ 0x101E,
'     real Fisher-Yates (sub_11966) via a per-game rotate-add LCG
'     (sub_1199E: state=((state<<<1)+0x39EF)>>1, seed at seg004 0x1018).
'     Ace = 11 if handTotal < 11 else 1 (dealCardToHand).  Dealer hits
'     while handTotal <= 16.  No rig; only the break-the-bank cap.
'   * FLIP-FLOP loss path: partyGold -= bet (loc_1219B) ; win path: += bet*mult
'     (loc_1202A).  sub_12A25 = the right-justified gold-string redraw
'     (rt_D3 STR$ + rt_42 LEN), not gameplay.
'   * DATA tables: ds:2100 = {1,1,2,2,5,5} (multipliers) ; ds:210C / 211C /
'     the rest = bumper-grid + flip-flop-state + animation coords, all
'     COSMETIC.  There is NO ball-physics sim (stepBallPhysics only moves
'     a display sprite) -- the outcome bucket is the player's steered pick,
'     or 6 on a 6-frame timeout (first drop untimed).
' ==========================================================================
