' ==========================================================================
'  SDEFENDR.EXE  --  the combat Training School                        [v2]
'  reconstructed from sdefendr.asm ; see recovered/README.md
'
'  A 360-degree fireball-defence arena shooter, reached from a TOWN (the
'  training-school building -- a townServiceDispatch service in TWNDR, cost
'  50 gold/session, "SESSION COST: 50 GOLD.").  NOT a museum exhibit.
'  TWNDR takes the 50 gold, sets ds:1F20 = the chosen mode, and chains to
'  SDEFENDR.EXE; SDEFENDR runs the arena and applies the stat change, then
'  chains back to TWNDR -> the normal save path (no direct CHAR.DAT write).
'
'  MODE  --  ds:1AEE, binary:  0 = "ARMOR TRAINING"  ->  raises ENDURANCE
'                              1 = "WEAPONS TRAINING" ->  raises DEXTERITY
'  (The name list "ARMOR,WEAPON,ENDURANCE,DEXTERITY,BLOCK,SHOOT" at
'   sdefendr.asm:7521 is just the message-name string array ds:1DC2; the
'   menu itself only offers the two modes above.)
'
'  DGROUP:
'    mode          ds:1AEE     (0 armour / 1 weapon)
'    sessionRating ds:1F12     (this session's score, see below)
'    ds:1F18       levelReached (0..7) ; ds:1F1A = arena hit score
'    storedRating  S4(20 + mode)  -- S4 byte 0x28 + mode*2 ; the per-mode
'                                    "best so far" record (two new S4 slots:
'                                    S4(20) = armour, S4(21) = weapon)
'    Endurance     ds:1ACC      Dexterity = ds:1AC0   (resident char record)
' ==========================================================================


' --------------------------------------------------------------------------
SUB TrainingSchoolMain                                ' asm: sdefendr.asm:80 (trainingSchoolMain)
' --------------------------------------------------------------------------
' Runs up to 7 arena levels, then rates the session and applies the change.

    ' ---- this session's rating (sdefendr.asm:522-549) -------------------
    t = 3 * levelReached (ds:1F18) + hitScore (ds:1F1A) - 4
    sessionRating = t _
                  + (1 IF t > 0) + (1 IF t > 3) + (1 IF t > 6)   ' 3-step staircase
    '  ds:1DC2 arena config table = DATA "40, 31, 22, 19, 16, 14, 12"
    '  (the 7 per-level fireball counts -- arena difficulty ramp, cosmetic)

    ' ---- rate + apply (sdefendr.asm:572-909) ---------------------------
    prevBest = S4(20 + mode)

    IF sessionRating = prevBest THEN
        PRINT "You made no real gains.   No change in "; skillName$
        '  S4(20+mode) unchanged, no stat change
    ELSEIF sessionRating < prevBest THEN
        PRINT "You've wasted our time."                          ' asm:650
        drop = prevBest - sessionRating
        S4(20 + mode) = sessionRating                            ' record worsens
        IF mode = 0 THEN Endurance = Endurance - drop            ' asm:709-710
                    ELSE Dexterity = Dexterity - drop            ' asm:714-716
        PRINT skillName$; " DECREASE: -"; drop
    ELSE
        PRINT "Your training has gone well."                     ' asm:~loc_10637
        gain = sessionRating - prevBest
        S4(20 + mode) = sessionRating                            ' new best
        IF mode = 0 THEN Endurance = Endurance + gain            ' asm:794-795
                    ELSE Dexterity = Dexterity + gain            ' asm:799-801
        PRINT skillName$; " INCREASE: +"; gain
    END IF
    ' Then: PRINT "Current "; skillName$; ": "; (Endurance or Dexterity)   asm:855-909

    ' ---- repeat? -------------------------------------------------------
    ' "Train more for 50 gold?" -> back to TWNDR to pay again, or "You
    ' don't have the gold." ; after level 7 -> "CONGRATULATIONS! ... seven
    ' levels of training".  Endurance is still capped at 0x24 = 36
    ' elsewhere (cf. potionWizard).
END SUB


' --------------------------------------------------------------------------
'  The arena game (sdefendr.asm seg001 = a hand-written asm engine)
' --------------------------------------------------------------------------
'  arenaInitPlayfield / arenaGameLoop / pollPlayerTurn / firePlayerArrow /
'  moveFireballs / arenaStepEndCheck / showWaveScore -- a tick shooter: you
'  aim around a ring and loose arrows; fireballs close in from the rim at ~6
'  scale steps; a hit costs a life; clearing a wave advances a level (7
'  levels, fireball counts 40/31/22/19/16/14/12).  levelReached (ds:1F18)
'  and hitScore (ds:1F1A) are the only outputs that matter -- they feed
'  sessionRating above.  runPractice = free mode, no stat change.  The
'  per-tick arena state block lives in seg004 (bytes 0x0C/0x0E playfield
'  ptr, 0x11 turn key, 0x15 fire, 0x16/0x22 cooldowns) -- a port can
'  reimplement the shooter to taste and just produce (level, score).


' ==========================================================================
'  SOLID (2026-09-07)
'   * launched from a TOWN (50 gold/session), NOT the museum
'   * two modes only: ARMOR -> Endurance (ds:1ACC), WEAPON -> Dexterity (ds:1AC0)
'   * sessionRating = staircase( 3*levelReached + hitScore - 4 )
'   * stat change = sessionRating - S4(20+mode) (the stored best); a worse
'     session subtracts the margin AND lowers the stored best
'   * S4(20) = armour-training rating, S4(21) = weapon-training rating
'   * everything rides ds:1ACC / ds:1AC0 / S4 back to TWNDR -> normal save
'
'  OPEN (arena "feel" only, no RPG payoff beyond level+score)
'   * exact arena tick physics / aiming / life count / wave-clear rule
'   * which town tile / townServiceId is the school (a town-map data lookup)
' ==========================================================================
