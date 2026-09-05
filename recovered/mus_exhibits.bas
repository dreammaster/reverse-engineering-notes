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

    ' ---- which driver EXE / which coin does this exhibit need? --- asm:998-1044
    '   a staircase of threshold tests on exhibitId picks chainTargetIdx,
    '   which turns out to be EXACTLY the required coin's S2()/Item$()
    '   index (17..23) -- see recovered/quest_flags.bas S3 for the full
    '   exhibitId -> exhibit name -> coin table (0..13 -> 14 exhibits,
    '   7 coins, verified against the Amethyst/Stones-of-Wisdom save-diff).
    chainTargetIdx = 17
    IF exhibitId > 3  THEN chainTargetIdx = chainTargetIdx + 1
    IF exhibitId > 6  THEN chainTargetIdx = chainTargetIdx + 1
    IF exhibitId > 8  THEN chainTargetIdx = chainTargetIdx + 1
    IF exhibitId > 10 THEN chainTargetIdx = chainTargetIdx + 1
    IF exhibitId > 11 THEN chainTargetIdx = chainTargetIdx + 1
    IF exhibitId > 12 THEN chainTargetIdx = chainTargetIdx + 1        ' asm:999-1044

    exhibitProgress = S3(exhibitId)                   ' ds:2106          ' asm:1045-1051

    ' ---- "(INSERT <coin>)" prompt ---------------------------------- asm:1052-1072
    '   Coin$(chainTargetIdx) IS Item$(chainTargetIdx) -- the literal gem
    '   name (Jade/Topaz/Amethyst/Sapphire/Turquoise/Ruby/Diamond coin),
    '   not a random flavour string.  The coin is CONSUMED on use --
    '   confirmed in-game: an Amethyst coin (S2 slot 19) went 1 -> 0 after
    '   one Stones-of-Wisdom game (exhibitId 7 -> chainTargetIdx 19); Jade
    '   / Topaz coins survived (different exhibits).
    PRINT "(INSERT "; Item$(chainTargetIdx); ")"

    RandomizeStep exhibitId + 1                       ' rtm_FC           ' asm:1075-1077
    ' ... "WOULD YOU LIKE TO GO TO <exhibit>?" ...
    IF YesNo() THEN
        RequiredCoin = RequiredCoin(exhibitId) - 1    ' spend the coin
        S3(15) = S3(15) + 1                           ' entry count
        ChainToStory chainExeName                     ' -> STDRV / CELDRV / DUN / TWNDR
    END IF
END SUB


' --------------------------------------------------------------------------
SUB TestExhibitFlag                                   ' asm: mus.asm:1857 (testExhibitFlag)
' --------------------------------------------------------------------------
' An ALL-BITS-SET test, not a raw AND: flagTestResult is true only when
' EVERY bit in flagTestMask is set in questFlagWord (S4(11)).  The
' checkFlag_* family stage a mask first: 0x03 / 0x2B / 0xD0 / 0x0300 /
' 0x0800 / 0x1000 / 0x2000 -- the quest / story-progress bits that gate
' the museum's per-coin-group unlock ladder.  Full bit-by-bit mapping
' (which action sets which bit, which gate needs which) is in
' recovered/quest_flags.bas.
    flagTestResult = ( (questFlagWord AND flagTestMask) = flagTestMask )  ' asm:1857-1873
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
'   * bit 0x2000's setter -- inside caretakerOffer's untraced accept
'     branch, likely alongside the character-LEVEL increment (ds:1AE0),
'     still not located (see recovered/quest_flags.bas S4)
'   * Stones of Wisdom INT maths live in STDRV -- see recovered/stdrv_dice.bas
'   (the exhibitId -> required-coin mapping and the per-bit story-flag
'   gating are now SOLVED -- recovered/quest_flags.bas)
