' ==========================================================================
'  GMB1.EXE / GMB2.EXE  --  the casino minigames                       [v1]
'  reconstructed from gmb1.asm / gmb2.asm ; see recovered/README.md
'
'  Two standalone gambling games chained from a town (TWNDR.EXE) casino.
'  Both share the party's gold dword  ds:1AD2:1AD4  with the caller, so any
'  net change rides the normal save path.  Neither touches any other stat.
'  On entry, if ds:1AC0 = 0 (uninitialised session) gold is seeded to 1000.
'
'  These are optional and self-contained -- a port can implement the RULES
'  below and skip the sprite/physics code entirely.
' ==========================================================================


' ==========================================================================
'  GMB1.EXE  --  Blackjack ("21")            gmb1.asm : blackjackMain (~3.2 KB)
' ==========================================================================
'  Standard casino blackjack vs. a dealer.
'    - "Enter your bet.  Enter 0 to quit."   (bet -> ds:1F02)
'    - deal 2 + 2 ; player hits / stays ; dealer draws to >= 17
'    - outcomes (exact strings): "Dealer has BlackJack." / "You're over 21
'      - you lose." / "It's a tie." / "You Win!" / "Dealer busts with <n>" /
'      / "Five cards without going over 21!" (5-card Charlie, auto win) /
'      "Natural BlackJack pays double."
'    - broke: "Come back when you have some gold." -> chains to TWNDR.EXE
'
'  Payout (gmb1.asm:955 / :1086 / :1101) -- TENTATIVE accounting:
'    win           partyGold += 2 * bet        ' asm:952-958  (bet back + even money)
'    natural / 5-card    partyGold += 3 * bet   ' the "pays double" line
'    push          partyGold += bet            ' bet returned
'    loss / bust   partyGold += 0  (bet lost)  ' asm:1086-1092
'  (whether the bet is escrowed at wager time or only settled at the end is
'   not 100% pinned; the NET effects above are what the strings imply.)


' ==========================================================================
'  GMB2.EXE  --  "Flip-Flop"  (a Plinko / bagatelle drop)
'                             gmb2.asm : flipFlopMain / computePayout / physics
' ==========================================================================
'  "FLIP-FLOP PARLOUR".  Place a bet, choose a release bucket 1..6, a ball
'  drops through flippers/bumpers (LT.BLUE bounce left, others right) and
'  lands in a basket.  Basket -> payout multiplier (from the help text):
'
'      basket 1-2 :  win = 1 * bet
'      basket 3-4 :  win = 2 * bet
'      basket 5-6 :  win = 5 * bet
'      (else / miss)  lose the bet
'
'  *** The parlour is RIGGED *** -- `computePayout` (gmb2.asm:4626):
'      ratio = S4(14) / S4(15)          ' running  totalWon / totalWagered
'      IF ratio > ds:2B3C  THEN         ' player is ahead of the house target
'          S4(14) = 99 : S4(15) = 99                  ' reset the running tally
'          IF landingBasket < 8 THEN landingBasket += 1
'      ELSEIF ratio < ds:2B40 THEN      ' player is behind
'          S4(14) = 99 : S4(15) = 99
'          IF landingBasket > 1 THEN landingBasket -= 1
'      END IF
'  i.e. the outcome basket is nudged +-1 to drag your win-rate back inside
'  the band [ds:2B40, ds:2B3C].  The in-game NPC even hints at it
'  ("...pretend you chose bucket 6.  BETTER LUCK NEXT TIME.").  ds:2B3C /
'  ds:2B40 not yet read cleanly -- TENTATIVE (a target win-rate ~ 1 : some
'  ratio just under 1).
'
'  playPracticeRound -- a free no-stakes mode; computePayout is skipped.


' ==========================================================================
'  SOLID
'   * both games only ever move ds:1AD2:1AD4 (party gold); no other stat
'   * blackjack outcome set + 5-card-Charlie / natural rules
'   * flip-flop basket -> 1x / 2x / 5x payout tiers
'   * flip-flop's computePayout rigs the landing basket toward a target
'     totalWon/totalWagered band
'
'  OPEN
'   * blackjack: bet escrow vs. end-settlement; the exact push handling
'   * flip-flop: ds:2B3C / ds:2B40 band values ; the ball physics constants
'     (stepBallPhysics / drawBumpers) -- not RPG-relevant
' ==========================================================================
