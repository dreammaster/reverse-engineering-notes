' ==========================================================================
'  SDEFENDR.EXE  --  the Training School  (museum exhibit / arcade driver) [v1]
'  reconstructed from sdefendr.asm ; see recovered/README.md
'
'  An arena shooter (fire arrows, dodge / block fireballs) reached from the
'  museum for a gem coin.  Menu: ARMOR, WEAPON, ENDURANCE, DEXTERITY, BLOCK,
'  SHOOT.  "As you repeat training, ..." / "CONGRATULATIONS! ... seven
'  levels of training" / "END OF TRAINING SESSION".
'
'  RPG-relevant: the ENDURANCE and DEXTERITY disciplines change those
'  attributes based on your arena score vs. your previous best for that
'  discipline.  (ARMOR / WEAPON / BLOCK / SHOOT teach technique -- effect on
'  the resident state not yet pinned; likely the same score-tracking on a
'  different slot, or pure practice.)
'
'  DGROUP:  disciplineSel = ds:1AEE   (0 => Endurance, else Dexterity)
'           sessionScore  = ds:1F12
'           bestScore(d)  = S(ds:1B96) elem  0x14 + d   (per-discipline best)
'           Endurance     = ds:1ACC        Dexterity = ds:1AC0
' ==========================================================================


' --------------------------------------------------------------------------
SUB TrainingSchoolMain                                ' asm: sdefendr.asm:trainingSchoolMain
' --------------------------------------------------------------------------
' After a session (runTrainingLevel x up to 7 waves -> sessionScore):

    d          = disciplineSel                        ' 0 = END, else DEX
    prevBest   = bestScore(d)
    bestScore(d) = sessionScore                       ' always overwrites

    IF sessionScore >= prevBest THEN
        gain = sessionScore - prevBest                                ' asm:772-788
        IF d = 0 THEN Endurance = Endurance + gain     ' asm:794-795
                 ELSE Dexterity = Dexterity + gain     ' asm:799-801
    ELSE
        loss = prevBest - sessionScore                                ' asm:690-703
        IF d = 0 THEN Endurance = Endurance - loss     ' asm:709-710
                 ELSE Dexterity = Dexterity - loss     ' asm:714-716
    END IF
    ' net: the attribute tracks your best-ever arena score for that
    ' discipline -- improving raises it by the margin, a worse run lowers it.
    ' (Endurance cap 0x24 = 36 is enforced elsewhere, cf. the potion wizard.)
END SUB


' --------------------------------------------------------------------------
'  Arena game (not RPG-relevant -- summary)
' --------------------------------------------------------------------------
'   arenaInitPlayfield / arenaGameLoop / pollPlayerTurn / firePlayerArrow /
'   moveFireballs / drawArenaSprites / arenaStepEndCheck / showWaveScore
'   -- a turn/tick shooter: the player looses arrows up a lane, fireballs
'   descend, contact costs a life, clearing a wave advances a level (7
'   levels).  Score = hits - times hit, roughly.  runPractice = free mode,
'   no stat change.  Physics / scoring constants not reversed (no game-rule
'   payoff beyond sessionScore).


' ==========================================================================
'  SOLID
'   * ENDURANCE / DEXTERITY training: attribute += (sessionScore - prevBest)
'     for the chosen discipline; a worse-than-best run subtracts the margin
'   * per-discipline best score kept in the S4-style array (ds:1B96)
'
'  OPEN
'   * ARMOR / WEAPON / BLOCK / SHOOT discipline effects (slots / whether
'     they touch the resident record at all)
'   * arena scoring formula ; whether the coin is consumed regardless
' ==========================================================================
