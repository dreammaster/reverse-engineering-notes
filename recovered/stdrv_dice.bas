' ==========================================================================
'  STDRV.EXE  --  "Stones of Wisdom"  (museum exhibit / arcade driver)  [v1]
'  reconstructed from stdrv.asm ; see recovered/README.md for the model
'
'  A Liar's-Dice / Perudo match against a dealer.  Reached from the museum
'  (MUS.EXE) by paying a gem coin at the Stones of Wisdom exhibit.  The
'  ONLY reason it matters to the RPG: winning / losing a MATCH changes the
'  player's INTELLIGENCE.  This is the game's main way to raise INT (the
'  others are quests / Stones-of-Wisdom gems).
'
'  Rules (from the in-game help text -- FULL text is in stdrv.asm:~22195;
'  it is complete and player-facing, and the code confirms every point):
'    - both players start a match with 5 dice, rolled secretly
'    - dice roll = INT( RND(1) * 6 + 1 )   uniform d6      ' sub_10448
'      (ds:2958 = 6 faces, ds:2954 = 5 dice/hand, both set at match start)
'    - players alternate BIDS ("N dice showing face F") = "I expect AT
'      LEAST N dice showing F between BOTH hands (all 10 dice)"
'    - each bid must exceed the last: MORE count of any face, OR the SAME
'      count of a HIGHER face  ("four ones beats three sixes";
'      "two fours beats two threes")
'    - "CHALLENGE" ends the round: reveal all dice; tally =
'          count of ( die = F  OR  die = 1 )         ' *** 1s ARE WILD ***
'      confirmed in scoreDiceHand (loc_120E2): inc when
'      (die == bidFace) OR (die <= 1).
'      tally >= bidCount  -> the BIDDER wins the round
'      tally <  bidCount  -> the CHALLENGER wins ("caught overbidding")
'    - the round LOSER gives up one die ; the loser bids first next round ;
'      a player reduced to 0 dice LOSES THE MATCH
'    - INTELLIGENCE changes once, at match end (see ResolveChallenge)
'
'  SUBs: stonesOfWisdomMain  (the match loop, ~1.1 KB, mostly presentation
'                             + STDRVSCR.DAT screen)
'        playerBidTurn / dealerTurn / dealerEvaluateBid   (turn handling)
'        scoreDiceHand / evalDiceOdds   (the dealer's bid / challenge AI --
'                             value-stack probability math, summarised below)
'        resolveChallenge  (reveal, round bookkeeping, and the INT delta)
'
'  DGROUP: intelligenceStat = ds:1AF0  (the resident character record's INT
'  slot -- same layout as CHAR.DAT ; shared with the caller, so the change
'  rides the normal save path).  diceCount = ds:2??? round/tally counter.
' ==========================================================================


' --------------------------------------------------------------------------
SUB ResolveChallenge                                 ' asm: stdrv.asm:1570
' --------------------------------------------------------------------------
' Called when either side challenges. Reveals dice, decides the round, and
' -- once the MATCH is over -- applies the Intelligence change.

    ' ... reveal both hands, tally face F including 1s as wild ...
    ' ... "- YOU WIN." / "- YOU LOST." , remove a die from the round loser ...
    ' ... if both still have dice: next round. else the match is decided: ...

    IF matchLostByPlayer THEN
        ' ---- "YOUR INTELLIGENCE DECREASES BY n" ---------------- asm:2338-2395
        Int = intelligenceStat
        IF    Int > 49 THEN delta = -3.0                   ' ds:2AD6
        ELSEIF Int > 39 THEN delta = -2.0                  ' ds:2A70
        ELSEIF Int >  9 THEN delta = -1.0                  ' ds:2ADA
        ELSE                delta =  0.0                   ' ds:29C6 (floor ~9)
        END IF
    ELSE
        ' ---- "YOUR INTELLIGENCE INCREASES BY n" --------------- asm:2398-2455
        Int = intelligenceStat
        IF    Int < 15 THEN delta =  3.0                   ' ds:2A74
        ELSEIF Int < 30 THEN delta =  2.0                  ' ds:2954
        ELSEIF Int < 60 THEN delta =  1.0                  ' ds:29CA
        ELSE                delta =  0.0                   ' ds:29C6 (cap 60)
        END IF
    END IF

    intelligenceStat = intelligenceStat + delta           ' asm:2459-2470
    PRINT "YOUR INTELLIGENCE "; IIF$(delta<0,"DE","IN"); "CREASES BY"; ABS(delta)
    ' redraw ; the new value rides home in the resident record (ds:1AF0)
END SUB


' --------------------------------------------------------------------------
'  Dealer AI  (scoreDiceHand / evalDiceOdds / dealerEvaluateBid) -- SUMMARY
' --------------------------------------------------------------------------
'  scoreDiceHand   -- tallies the dealer's own 5 dice by face (1s wild) and
'                     builds a per-face "how many total on the table are
'                     plausible" estimate: own count + expected share of the
'                     hidden dice ( hiddenDice / 3, i.e. P(face|wild) = 1/3 ).
'  dealerEvaluateBid -- given the current bid (count C, face F):
'                     expectedF = ownF + INT(hiddenDice / 3)
'                     if  C <= expectedF            -> raise (bid C+1 of F,
'                                                       or C of F+1)
'                     if  C  > expectedF + slack    -> CHALLENGE
'                     else                           -> make a minimal raise
'                     ("slack" widens as the dealer's remaining dice shrink;
'                      exact constants live in evalDiceOdds and are still
'                      value-stack math -- TENTATIVE.)
'  formatBidText   -- "DEALER BIDS <n> <face>S" / "LET'S CHALLENGE."
'
'  None of the AI feeds anything back to the RPG; only the win/loss result
'  and the INT delta above do.


' ==========================================================================
'  SOLID
'   * Intelligence change on match end -- the two 4-entry delta tables
'     (win: +3/+2/+1/0 below 15/30/60 ; loss: -3/-2/-1/0 above 49/39/9).
'     Values read from STDRV.EXE (ds:2A74/2954/29CA/29C6 and 2AD6/2A70/
'     2ADA/29C6).  Self-balancing toward the middle; hard cap 60, floor ~9.
'   * match = Perudo to zero dice ; INT changes ONCE per match, not per round
'   * the changed INT rides ds:1AF0 (resident char record) back to the museum
'
'  RESOLVED 2026-09-07
'   * dice roll = INT(RND(1)*6+1) uniform d6 (sub_10448)
'   * challenge tally counts (die == bidFace) OR (die == 1) -- 1s wild,
'     confirmed in scoreDiceHand
'   * bid ordering: strictly-more count wins; equal count needs higher face
'   * challenge outcome: tally >= bidCount -> bidder wins, else challenger
'   * coin: EnterExhibit spends the gem coin on entry (win or lose), like
'     every museum exhibit
'
'  OPEN (feel, not mechanics -- a standard Perudo-with-wilds AI is faithful)
'   * the exact dealer P-formula + raise/challenge thresholds live in
'     evalDiceOdds (stdrv.asm:4281, ~600 lines of value-stack binomial
'     math). Model: expectedF = ownWildCount + INT(hiddenDice/3), raise
'     while bidCount <= expectedF, challenge when bidCount exceeds it by a
'     margin that widens as the dealer's dice dwindle.
