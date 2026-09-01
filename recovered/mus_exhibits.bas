' ==========================================================================
'  MUS.EXE  --  museum exhibits                                       [v1]
'  reconstructed from mus.asm ; see recovered/README.md for the model + tags
'
'  SUBs: EnterExhibit  (the display-case SELECT CASE)
'        TestExhibitFlag + the checkFlag_* family (quest-flag gates)
'        ChainToStory  (hand off to the sub-game driver EXE)
'
'  MUS is mostly dispatch + chaining -- little arithmetic.  The exhibits'
'  actual mechanics live in the driver EXEs they chain to.
' ==========================================================================
'
'  DGROUP vars (MUS):
'     exhibitId 20FE   chainTargetIdx 2104   chainExeName 210C (STRING)
'     S3() = ds:1B3A  = the museum-progress array (17 words)
'       S3(exhibitId) -> whether/how far this exhibit is unlocked
'       S3(15) = museum entry count (MUS bumps it each entry)
'     museum flag word lives at [ds:1B96 desc + 0x16] (an S4() word), NOT
'       in DGROUP -- testExhibitFlag masks it into flagTestResult
'     Intelligence 1AF0   (adjusted by STDRV, not here)


' --------------------------------------------------------------------------
SUB EnterExhibit                                      ' asm: mus.asm:985 (enterExhibit)
' --------------------------------------------------------------------------
' The player steps up to display case `exhibitId`.

    ' ---- which driver EXE does this exhibit run? ---------------- asm:998-1044
    '   a staircase of threshold tests on exhibitId picks chainTargetIdx:
    '   exhibitId > 3 / > 6 / > 8 / > 0x0A / > 0x0B / > 0x0C each shift it.
    chainTargetIdx = 17
    IF exhibitId > 3  THEN chainTargetIdx = chainTargetIdx + 1
    IF exhibitId > 6  THEN chainTargetIdx = chainTargetIdx + 1
    IF exhibitId > 8  THEN chainTargetIdx = chainTargetIdx + 1
    IF exhibitId > 10 THEN chainTargetIdx = chainTargetIdx + 1
    IF exhibitId > 11 THEN chainTargetIdx = chainTargetIdx + 1
    IF exhibitId > 12 THEN chainTargetIdx = chainTargetIdx + 1        ' asm:999-1044

    exhibitProgress = S3(exhibitId)                   ' ds:2106          ' asm:1045-1051

    ' ---- "(INSERT <coin>" prompt ---------------------------------- asm:1052-1072
    '   each exhibit requires a specific gem coin (World- / Stone- flavour
    '   names).  The coin is CONSUMED on use -- confirmed in-game: an
    '   Amethyst coin (S2 slot) went 1 -> 0 after one Stones-of-Wisdom game;
    '   Jade / Topaz coins survived (different exhibits).
    PRINT "(INSERT "; Coin$(chainTargetIdx); ")"

    RandomizeStep exhibitId + 1                       ' rtm_FC           ' asm:1075-1077
    ' ... "WOULD YOU LIKE TO GO TO <exhibit>?" ...
    IF YesNo() THEN
        RequiredCoin = RequiredCoin(exhibitId) - 1    ' spend the coin
        S3(15) = S3(15) + 1                           ' entry count
        ChainToStory chainExeName                     ' -> STDRV / CELDRV / DUN / TWNDR
    END IF
END SUB


' --------------------------------------------------------------------------
SUB TestExhibitFlag                                   ' asm: mus.asm:1815 (testExhibitFlag)
' --------------------------------------------------------------------------
' flagTestResult = museumFlagWord AND flagTestMask.  The checkFlag_* family
' stage a mask first: 0x03 / 0x2B / 0xD0 / 0x0300 / 0x0800 / 0x1000 / 0x2000
' -- the quest / story-progress bits that gate which exhibits respond.
    flagTestResult = MuseumFlagWord AND flagTestMask                     ' asm:1815-1832
END SUB


' ==========================================================================
'  SOLID
'   * each exhibit chains to a driver EXE (STDRV = Stones of Wisdom,
'     CELDRV = cell, DUN, TWNDR); chainTargetIdx from an exhibitId staircase
'   * each use CONSUMES the exhibit's required gem coin
'   * S3(15) = museum entry count, bumped per entry
'   * exhibit responses gated on quest-flag bits (testExhibitFlag)
'
'  OPEN
'   * the exhibitId -> required-coin mapping (needs the OUTDAT/museum data)
'   * Stones of Wisdom INT maths live in STDRV (see the memory notes:
'     resolveChallenge adjusts ds:1AF0 -- INT<30 -> +2/win else +1, -1/loss)
'   * the museum caretaker level-up (raises ds:1AE0) -- locate it
