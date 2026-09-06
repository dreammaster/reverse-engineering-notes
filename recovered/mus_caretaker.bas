' ==========================================================================
'  MUS.EXE  --  the museum caretaker: how character LEVEL actually works [v2]
'  reconstructed from mus.asm (useCommand / sub_10B59 / checkFlag_2000 /
'  caretakerOffer / caretakerPraise / sub_12CAC) ; see recovered/README.md
'
'  ds:1AE0 (character level) is written in EXACTLY ONE place in the whole
'  game: sub_12CAC, called (only) from caretakerOffer's two branches. It
'  was invisible until now because sub_12CAC (and caretakerPraise,
'  sub_12AF4, sub_12C67) each carried a raw, un-coerced `db` byte run --
'  the same "IDA chops the block after every far call" issue documented
'  in ida_scripts/fix_dun_coerce_gaps.py, now fixed here by
'  ida_scripts/fix_mus_caretaker_gaps.py.
'
'  THE MECHANIC, top to bottom:
'    character level = the highest museum "exhibit-coin-group RANK" you
'    have proven yourself worthy of (per quest_flags.bas's S4(10) rank
'    ladder), capped and FINALISED at level 10 once you clear rank 8 (all
'    7 coin groups).  The caretaker only ever RAISES it -- never lowers,
'    never re-grants a rank you already hold.
' ==========================================================================


' --------------------------------------------------------------------------
'  Step 1 -- useCommand: "is a new rank ready?"      (calls sub_10B59)
' --------------------------------------------------------------------------
'   sub_10B59 (mus.asm:10B59): if S4(10) (byte 0x14 = the persistent
'   caretaker-rank counter, 0..7) is already 7 -> RETURN (no rank 8 here).
'   Otherwise it runs  ON (S4(10) + 1) GOSUB {the 8 rank-check arms}  --
'   each arm sets flagTestResult (ds:2138): 0 = requirements MET.
'
'        IF flagTestResult = 0 THEN S4(10) = S4(10) + 1     ' PROMOTE
'        candidateRank = S4(10) + 1                          ' ds:1AEE
'        IF (S3(3) = 0) OR flagTestResult <> 0 THEN candidateRank = candidateRank - 1
'        IF characterLevel < candidateRank THEN     ' ds:1AE0 < ds:1AEEh
'            museumWalkState = candidateRank         ' ds:1AC6h  <- KEY
'            S3(14) = candidateRank                  ' parallel copy
'        END IF
'   i.e. ds:1AC6 only gets updated to a NEW value when you QUALIFY for a
'   rank higher than your current level -- otherwise it's untouched (and
'   checkFlag_2000, step 2, reads whatever stale value it already held).
'
' --- THE 8 RANK-CHECK ARMS (index = S4(10) + 1) ---------------------------
'   -> rank 1  sub_10C1F     : questFlagWord(S4(11)) AND 3 <> 0   AND S3(3) <> 0
'   -> rank 2  checkFlag_2B  : exhibit bits &h2B all set  AND S3(3) <> 0
'                              AND  S4(37) <> 0     *** the "visited a dungeon" gate
'   -> rank 3  checkFlag_D0  : exhibit bits &hD0 all set
'   -> rank 4  checkFlag_0300: exhibit bits &h0300 all set  AND S2(12) <> 0
'   -> rank 5  sub_10CCE     : S4(18) >= 2          *** the "main quest >= stage 2" gate
'                              (NO exhibit flags -- this rank is purely quest-gated)
'   -> rank 6  checkFlag_0800: exhibit bits &h0800 all set
'   -> rank 7  sub_10CF3     : S2(15) <> 0          (hold the Compendium)
'   -> rank 8  sub_10D0F     : flagTestResult := 1  -- ALWAYS BLOCKED
'                              (rank 8 is never auto-promoted here; it is the
'                               explicit rank-8 branch of CaretakerOffer)
'
' --- S4(37)  (S4 byte 0x4A = word 37)  "has entered a dungeon" -----------
'   Set to 1 at the very top of dunMain (dun.asm:88) on EVERY dungeon turn;
'   never cleared; rides the CHAR.DAT checkpoint.  So you cannot rank past
'   1 with the caretaker until you have set foot in a dungeon.
'
' --- S4(18)  (S4 byte 0x24 = word 18)  main-quest stage counter ----------
'   0 = not started ; advanced by the museum's kingConfides chain ; TWNDR's
'   townServiceDispatch reads it (== 3 -> "FIND A WAY TO THE FORTRESS!") ;
'   CASDR's exitCastle sets it to 4 on leaving the fortress.  Rank 5 needs
'   S4(18) >= 2 -- the caretaker will not promote you to rank 5 until the
'   main quest has reached at least stage 2.


' --------------------------------------------------------------------------
'  Step 2 -- useCommand: "have you already taken the top offer?"
'            (calls checkFlag_2000)
' --------------------------------------------------------------------------
    IF questFlagWord AND &h2000 = &h2000 THEN            ' ds:2136=0x2000
        ' "already done" -- show dialog ':' (0x3A), no state change.
        ' [bit 0x2000's SETTER is still not located -- see OPEN]
    ELSE
        museumMenuChoice = 0                              ' ds:1E22
        caretakerRankArg = museumWalkState                ' ds:20B6 = ds:1AC6
        CaretakerOffer
    END IF


' --------------------------------------------------------------------------
SUB CaretakerOffer                                    ' asm: mus.asm:3116
' --------------------------------------------------------------------------
' caretakerRankArg (ds:20B6, entering = ds:1AC6) selects the branch:

    IF S3(0) >= 2 THEN
        ' a separate, later-game caretaker interaction (loads exhibit
        ' graphics via sub_12AF4) -- not part of the level ladder.  OPEN.
        EXIT SUB
    END IF

    SELECT CASE caretakerRankArg
        CASE IS < 1
            ' dialog only -- no state change.  Picks ONE OF TWO fixed
            ' lines (dialog id 0x45 or 0x4B) purely from whether
            ' characterLevel > 7 -- flavour text, not a gate.
        CASE 1 TO 7
            CaretakerPraise                           ' "YOU HAVE DONE WELL..."
        CASE 8
            ' ---- the FINAL offer: ascend to max level ----- asm:~1150A
            S3(0) = 1
            S2(15) = 0                     ' Compendium consumed
            hitPoints = 3000                          ' ds:1ADA = 0xBB8
            partyGold = MIN(partyGold, 50000)         ' ds:1AD2:1AD4
            caretakerRankArg = 10                      ' ds:20B6 = 0x0A
            Sub_12CAC                                   ' -> level = 10
        CASE ELSE
            ' no-op (candidateRank never exceeds 8 in practice)
    END SELECT
END SUB


' --------------------------------------------------------------------------
SUB CaretakerPraise                                   ' asm: mus.asm:6457
' --------------------------------------------------------------------------
' "YOU HAVE DONE WELL SINCE I LAST SAW..." -- reached for rank 1..7.
' caretakerRankArg (ds:20B6) is STILL the rank passed in (1..7): no
' immediate reward text beyond the line itself, then:
    Sub_12CAC                                          ' -> level = that rank
END SUB


' --------------------------------------------------------------------------
SUB Sub_12CAC                                         ' asm: mus.asm:6785 --
'                                                        THE LEVEL WRITE
' --------------------------------------------------------------------------
' The ONE place ds:1AE0 (character level) is ever assigned in the game.

    characterLevel = caretakerRankArg                 ' ds:1AE0 = ds:20B6  [SOLID]
    PRINT "..."; characterLevel                       ' asm:~6798-6824

    Sub_12C67                                          ' a keypress-gate helper
                                                        ' (unrelated to level)

    ' ---- recompute MAX HIT POINTS for the new level ------------- [SOLID,
    '      exact -- matches every L1..L7 value in game-logic.md S2]
    maxHP = 200
    FOR i = 1 TO characterLevel
        maxHP = maxHP + (i - 1) * 100
    NEXT i                                  ' = 200 + 50*L*(L-1)
    IF characterLevel > 5 THEN maxHP = maxHP - 100
    S4(19) = maxHP                                     ' ds:1B96 elem 0x13

    '   L1=200  L2=300  L3=500  L4=800  L5=1200  L6=1600  L7=2200  <- EXACT
    '   L10 = 200 + 50*10*9 - 100 = 4600  (the immediate ds:1ADA=3000 heal
    '   on ascension is a SEPARATE, smaller freebie -- true max HP at L10
    '   is 4600, not 3000; the old "L10 3000" note conflated the two)
END SUB


' ==========================================================================
'  SOLID
'   * ds:1AE0 (character level) = ds:20B6 (the incoming rank argument),
'     written ONLY inside sub_12CAC -- the sole level-up site in the game
'   * level = the rank ladder's S4(10)+1, i.e. how many exhibit-coin-groups
'     you've been gated into (quest_flags.bas S4) -- monotonic, never re-
'     granted, only advances when useCommand's sub_10B59 detects a new
'     qualifying rank
'   * reaching rank 8 (all 7 coin groups cleared) is the FINAL offer:
'     consumes the Compendium, sets HP=3000 (an immediate heal/bonus,
'     distinct from max HP), caps gold at 50000, and sets level = 10
'   * max HP formula, exact for L1..L7: `200 + 50*L*(L-1) - (100 if L>5)`
'     -- confirms every value in game-logic.md S2's level table; L10 =
'     4600 by the same formula (the "3000" in the old note was the
'     ascension heal amount, not the true max-HP baseline)
'   * the 8 rank-check arms are fully mapped (see Step 1 above):
'     ranks 1/2/3/4/6 gate on cumulative exhibit-flag bit groups
'     (3 / 2B / D0 / 0300 / 0800), rank 7 on holding the Compendium,
'     rank 8 is never auto-promoted
'   * S4(37) (byte 0x4A) = "visited a dungeon" (dunMain sets it to 1);
'     gates rank 2
'   * S4(18) (byte 0x24) = main-quest stage counter (kingConfides advances
'     it, exitCastle -> 4, TWNDR reads == 3); rank 5 needs S4(18) >= 2
'
'  RESOLVED (was open)
'   * questFlagWord bit 0x2000 ("already took the caretaker's final
'     offer") is NEVER SET -- a latent bug, no gameplay effect.  Every
'     other exhibit's handler ends with `call sub_11D02` (which ORs in
'     `2^(exhibitId-1)`); `checkFlag_2000` (exhibit 14's handler) omits
'     that on its success path, and its only call into the setter is a
'     dead "bit already set" branch.  So `checkFlag_2000` always routes
'     to `caretakerOffer`, and the level-up logic is monotonic, so
'     re-offering is harmless.  Full evidence in
'     recovered/quest_flags.bas section 3c.  A port drops the branch.
'   * caretakerOffer's `S3(0) >= 2` fast-path is dead the same way
'     (`S3(0)` is only ever written to 1).
'   * the `S3(0) >= 2` branch (sub_12AF4, loads exhibit graphics) --
'     a separate late-game caretaker interaction, not traced
' ==========================================================================
