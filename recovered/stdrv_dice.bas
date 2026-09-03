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
'  Rules (from the in-game help text):
'    - both players start a match with 5 dice
'    - each round both roll secretly; players alternate BIDS ("N dice
'      showing face F"); each bid must exceed the last (higher count, or
'      same count + higher face; 1s are wild in the tally)
'    - "CHALLENGE" ends the round: reveal all dice, count face F (+wilds).
'      >= bid  -> the challenger loses the round ; < bid -> the bidder does
'    - round loser gives up one die ; a player at 0 dice loses the MATCH
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
'  OPEN
'   * dealer bid/challenge thresholds (evalDiceOdds constants)
'   * whether a coin is consumed win-or-lose (looks like: yes, on entry)
'   * exact "matchLostByPlayer" flag origin in resolveChallenge
