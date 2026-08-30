# Roadmap

Working list of next steps, roughly in priority order. Check items off /
add notes as they're resolved — this file is meant to stay current across
sessions, unlike a one-off todo list. See [overview.md](overview.md) for
the per-executable breakdown this tracks against.

## Infra (done, 2026-08-30)

- [x] Set up headless IDA pipeline
      (`ida_scripts/run_ida_script.ps1` + `batch_run_and_export.py`),
      copied from the sibling `ultima1` project. Path-derivation driver
      (works for any `<stem>.idb`), pre-flight GUI-lock check.
- [x] `ida_scripts/identify.py` (+ raw-vs-code byte coverage) and
      `rank_unnamed_functions.py`. Confirmed working end-to-end against
      `menu.idb` and `leglib.idb` (report mode).
- [x] Identified the toolchain: **Microsoft BASIC Compiler 6.0**, with
      `LEGLIB.EXE` as a shared BRUN-style run-time module that every
      other module loads via an `int 3Fh` thunk table. Not C.
- [x] Confirmed `.BSV`/`.GLB`/`.GMP`/`.BS1`/`.BS2` are Microsoft BASIC
      `BSAVE` images (`0xFD` + seg + off + len header) — see
      [file-formats.md](file-formats.md).
- [x] `.gitignore` already correct (commits `.asm`/`.idc`, ignores
      `.idb` and unpacked-db files).

## Executable order

Decided 2026-08-30 (with Paul): **`LEGLIB.EXE` first**, or alongside
`MENU.EXE`. Rationale: `LEGLIB` is the shared payload — the BASIC runtime
plus the common engine (`bmXXXX` graphics, file I/O, input). Every
per-module `.EXE` is a thin `call far` stream into it, so nothing
downstream is readable until `LEGLIB`'s entry points are mapped. `MENU`
is small and makes a good pipeline shakedown / first consumer to
validate the thunk-table → `LEGLIB` cross-reference.

Rough size order of the rest (biggest engine payoff first): `OUT` (done,
2026-08-30) → `DUN` → `TWNDR` → `CASDR` → `MUS` → minigames (`SDEFENDR`,
`GMB1`, `GMB2`) → drivers (`CELDRV`, `STDRV`, `SAVER`) → `CONFIGUR`
(standalone, not BASIC).

**All four formerly-packed modules — `OUT` / `DUN` / `TWNDR` / `CASDR` —
are built** (2026-08-30/31), each via the same pipeline: `idat -B` from a
copy of the UNP-unpacked exe → `resolve_thunks` → `coerce_code` →
`resolve_thunks` → `dump_strings` → `apply_renames_<m>`. `DUN`/`TWNDR`/
`CASDR` each link a 2nd compiled-BASIC code segment ("bmDUNG",
"bmTNCALB") — re-run `coerce_code` with `$env:COERCE_SEG='seg001'` for
it. `TWNDR`/`CASDR` share the identical `bmTNCALB` `seg001` module.

Once `leglib.idb` gets more `B$…` names, re-run `resolve_rtm_leglib.py`
to refresh `rtm_map.py` — the `-> name` comments propagate to every
module on the next `resolve_thunks` pass.

`mus.idb` built 2026-08-31 — and **`MUS.EXE` turned out to be the MUSEUM
driver, not music** ("MUS" = Museum; the hub, exhibits = portals; chains
to `TWNDR`/`DUN`/`STDRV`/`CELDRV`).

`stdrv.idb` built 2026-08-31 — and **`STDRV.EXE` turned out to be the
"Stones of Wisdom" dice game** (a Liar's-Dice / Perudo variant vs. the
"DEALER"), not a story driver. It's the museum's *Stones of Wisdom*
exhibit minigame; the match result adjusts the character's INTELLIGENCE.
`STDRVSCR.DAT` = the rules text it narrates. Single code segment, 467
thunks, 100% coerced; 7/39 named.

`celdrv.idb` built 2026-08-31 — and **`CELDRV.EXE` is the endgame victory
cinematic**, not a general cel player: "AGAINST ALL ODDS!" + the
scrolling victory-story recap (hero-name substitution, over music) + end
credits. Loads `CEL0`–`CEL2`/`DIS9`/`CEL3.BSV`. Chained from `CASDR`.

`saver.idb` built 2026-08-31 — **`SAVER.EXE` is the save-game handler**:
"SAVE THE GAME NOW IN PROGRESS?", validates the character disk, writes
`CHAR.DAT`, then ESC → DOS / else re-exec `OUT` / `DUN`. Confirms
`CHAR.DAT` doubles as the in-progress save (no separate file).

`sdefendr.idb` built 2026-08-31 — **`SDEFENDR.EXE` is the combat-training
school** (ARMOR / WEAPONS training arena, a 360° fireball-defense shooter
reached from a town; raises ARMOR/WEAPON/ENDURANCE, 50 gold/session,
7 levels, chains back to `TWNDR`). First module with a hand-written
**asm** code segment (`seg001` = the real-time arena engine).

`gmb1.idb` built 2026-08-31 — **`GMB1.EXE` is the BlackJack table**
("GMB" = gamble; hit/stay vs. the dealer, natural pays double, five-card
win, 17+ dealer stop, bet 0 to quit, broke → 5-gold stake, break the
bank → house closes). Loads `BJCHR.GLB`, chains back to `TWNDR`.

`gmb2.idb` built 2026-08-31 — **`GMB2.EXE` is "Flip-Flop Parlour"**, a
Plinko / pachinko betting game (drop a ball, it bounces bumper-to-bumper
into one of 6 buckets; bet the bucket number and/or colour; outer
buckets pay even / double / 5×; the bumpers "flip-flop"). Uses
`BIGNUM.DAT`, chains back to `TWNDR`. NOT a card game — it doesn't use
`BJCHR.GLB`.

`configur.idb` built 2026-08-31 — **`CONFIGUR.EXE` is the floppy-drive /
disk-layout config utility** (edits `DRCONFIG.DAT`: which drive letters
hold the game floppies, "to reduce disk swaps"). Standalone MSC, not
packed; only `_main` + six BIOS screen/keyboard wrappers are app code.

**All 14 executables now have an IDB.** Remaining work is depth per
module, `leglib` `rtm_*` → `B$…`, on-disk formats, then the
reimplementation.

## DUN.EXE — open questions

- [x] Build `dun.idb` (2026-08-30). 6 segments, two compiled-BASIC code
      segs (`seg000` "bmDUN" main + `seg001` "bmDUNG" graphics), thunk
      table `seg002`. Both coerced ~100%, 0 bad insns.
- [x] Map the DUN engine state vars (2026-08-31, `apply_dsvars_dun.py`;
      DUN's DGROUP is **seg004**). ~16 named: `dungeonLevel` (1ACA),
      `hitPoints` (1ADA), `playerX`/`playerY` (20CE/20D0), `tileAhead`
      (20C4), `selectedSpell` (1E24), `dungeonArrayPtr` (2274 far ptr),
      `levelProgressFlags` (1AE2), `actionPhase` (20EC); `turnActionFlag`
      (212E) and `chainDestType` (1F16) are the **same DGROUP slots as
      OUT**. `hpDisplayScratch` (20EA) explicitly flagged as scratch, not
      HP. `featureUnderfoot` / `scanTile` / `moveDelta` tentative.
- [~] Name `seg000` functions (`apply_renames_dun.py`): **38 / 72**
      (2026-08-31 second pass). New: `processTileFeature` (was
      `sub_12536` — the per-turn feature handler with the trap-name
      table), `moveMonsters`, `drawDungeonHud`, `doLookSearch`,
      `clearTurnFlag`, `setActionPhase_1/2/3`, plus tentatives
      (`stepMonsterToward`, `rebuildLevelView`, `updateLevelState`,
      `rollChestContents`, `monsterSpecialAttack`, `redrawDungeonView`).
- [x] Name `seg001` "bmDUNG" (2026-08-31) — it's the first-person
      **dungeon-view renderer** (7 funcs): `renderDungeonView` loops the
      depth bands calling `drawViewWallBand{Near,Mid,Far}` +
      `drawViewSprite`, all via the `blitViewCell` primitive (rtm_FE2A).
      Which band is which is a guess. MUS's `bmMUSDUNG` (6 funcs) is the
      same renderer for the dungeon-exhibit rooms — named identically.
- [ ] Confirm the DUN tentatives; identify the ~28 remaining `sub_`
      (many are runtime-dispatched 6–26 byte stubs).

## TWNDR.EXE / CASDR.EXE — open questions

- [x] Build `twndr.idb` (2026-08-31, 41/98 named) and `casdr.idb`
      (2026-08-31, 34/102 named) from the unpacked exes. Each: `seg000`
      main + `seg001` "bmTNCALB" (**shared, identical** between the two),
      thunk table `seg002` (431 entries).
- [x] Map the TWNDR engine state vars (2026-08-31,
      `apply_dsvars_twndr.py`; DGROUP = **seg004**). `partyGold` (1AD2)
      and `hitPoints` (1ADA) are the **same DGROUP slots as OUT/DUN**.
      Town-specific: `townServiceId` (1F22 — `townServiceDispatch`'s
      SELECT CASE), `tileAhead` (1F02), `guardHitPoints` (216E),
      `townArrayPtr` (278C far ptr); `menuChoice` / `turnFlag` /
      `shopWorkQty` / `viewMode` tentative.
- [~] `twndr` names: **51 / 98** (2nd pass 2026-08-31). The shops
      (`foodShop`, `weapon`/`armorShopEntry`, `buyBackShop`,
      `sellShopIntro`), money (`borrowMoney`, `loanRepayment`,
      `fortuneTeller`), crime/jail (`robCommand`, `arrestedByGuards`,
      `jailScene`, `jailRelease`, `fightGuard`), `townServiceDispatch`
      (~6 KB), `mailDeliveryJob`, `spendGold`, `enterTownService` (the
      ENTER/USE command -> dispatch), `facePlayerDirection`,
      `checkLineOfSight`, `redrawTownView`; `offerGuardBribe` /
      `initGuardCombat` / `grabTileItem` / `computeSellValue` tentative.
- [x] Map the CASDR engine state vars (2026-08-31,
      `apply_dsvars_casdr.py`; DGROUP = **seg004**). `partyGold` (1AD2),
      `hitPoints` (1ADA), `tileAhead` (1F02), `targetSlot` (1F24),
      `turnFlag` (1F2A) are the **shared LEGLIB slots** (4th module to
      confirm). Castle-specific: `playerX`/`playerY` (1B00/1B04),
      `enemyHitPoints` (2222), `castleArrayPtr` (25B0 far ptr);
      `castleOrFort` (20C0, 1/2) / `viewLevel` (2084) / `mapStride`
      (1F26) tentative.
- [~] `casdr` names (endgame content): **47 / 102** (2nd pass
      2026-08-31). `warlordConfrontation`, `kingConfides` (the
      guardians-of-the-scroll quest + forearm mark), `potionWizard`,
      `doFight`, `describeRoom`/`describeObjects` (incl. "THE COMPENDIUM
      IS THERE!"), `loadCastleLevel` (CASTLE.BS1/2, FORT.BS1/2),
      `exitCastle`, `gasRoomTrap`, `useKey`, spell handlers,
      `fortressSelfDestruct` ("SELF-DESTRUCTION IN 5 MINUTES!"), the
      `describeRoom` cases (`describeChest`, `describeLockedDoor`,
      `describeGasRoom`, `describePotionShop`), `facePlayerDirection`,
      `checkLineOfSight`; `castleTurnUpdate` / `redrawCastleView` /
      `resolveOpenDoor` / `resolveUseKey` / `jailPlayer` tentative.
- [x] Name the shared `bmTNCALB` `seg001` module (2026-08-31,
      `apply_renames_tncalb.py` — keyed by seg001 offset, byte-identical
      structure in both, applied to each). 13/26: the town/castle
      *interior* engine — `drawInteriorTiles`, `refreshView`, `tileAt`,
      `scanLineOfSight`, `findObjectTile`, `moveActor`, `stepByDirection`,
      `dirBetween`, `viewFaceDirection`, `setViewport`, `drawActor`,
      `traceCombatLine`.
- [~] `bmTNCALB`: **21/26** (2026-08-31). Added `refreshTileGraphic`
      (the shared rtm_FE19 single-tile blit), `stepLineOfSight` /
      `sightBlockedBy`, `traceCombatRay` / `combatRayResult`,
      `placeNpcSprite`, `drawViewFrame`, `tileAtOffset` (several
      TENTATIVE). 5 tiny stubs (<20 b, runtime-dispatched) left `sub_`.
- [x] `twndr` `sub_11ED0` -> `enterTownService` (the ENTER/USE command
      that locates the adjacent service tile and jumps to
      `townServiceDispatch`).
- [x] Map the CASDR state vars (2026-08-31, `apply_dsvars_casdr.py`) --
      done; see the CASDR section above.
- [x] Map the MUS.EXE state vars (2026-08-31, `apply_dsvars_mus.py`;
      `seg004`). `exhibitId` (20FE), `chainExeName` (210C string),
      `flagTestMask`/`flagTestResult` (2136/2138); shared slots as
      usual. 45/109 named (added `testExhibitFlag`, `checkFlag_*`).
- [x] **State-var mapping complete for all play modules.** `ds:1AD2` =
      partyGold and `ds:1ADA` = hitPoints in every module; `ds:1F02`,
      `ds:1F24`, `ds:1F2A`, `ds:212E`, `ds:1F16` are the other
      LEGLIB-fixed slots. Remaining: use them to finish naming the
      per-module `sub_` helpers. (`seg001` graphics helpers -- `bmDUNG`,
      `bmMUSDUNG`, `bmTNCALB` -- now named, 2026-08-31.)

## STDRV.EXE — open questions

- [x] Build `stdrv.idb` (2026-08-31). Single code seg `seg000` "bmSTDRV"
      (39 funcs), thunk table `seg001` (467), DGROUP `seg003`. 100%
      coerced, 0 bad insns, 467 thunks resolved.
- [~] Name `seg000` functions (`apply_renames_stdrv.py`): 7/39 —
      `stdrv_entry` (builds the "NO"/"ONE"…"NINE" number-word table),
      `stonesOfWisdomMain` (loads `STDRVSCR.DAT`, "INSTRUCTIONS?", the
      per-match / play-again-for-gold loop), `playerBidTurn` ("HOW MANY
      DICE?", "OF WHAT VALUE?", "CHALLENGE!!"), `resolveChallenge`
      (reveal dice, win/lose, "YOUR INTELLIGENCE / INCREASES BY"),
      `formatBidText`. `dealerTurn` / `evalDiceOdds` = the dealer AI
      (no player text — **tentative**, confirm from the call graph).
- [x] Map the STDRV state vars (2026-08-31, `apply_dsvars_stdrv.py`;
      DGROUP `seg003`). `partyGold` (1AD2) / `menuChoice` (1E22) shared
      slots; `intelligenceStat` (1AF0 -- what resolveChallenge adjusts),
      `stdrvArrayPtr` (29CA far ptr), `diceCount` (28FE), `playerBid`
      (2108) / `dealerBid` (2106), `gameScore` (1EF2 dword, role
      unclear). Most of the other 30-odd DGROUP words are one-function
      drawString layout params.
- [ ] The ~30 remaining `sub_` helpers are all value-stack number
      crunching for the dice math -- no distinguishing text. `diceCount`
      / `stdrvArrayPtr` split them into "tally" and "table lookup"
      groups; naming each precisely needs the dice ruleset traced.
- [ ] Field-decode `STDRVSCR.DAT` (6192 bytes) — is it the same
      screen-string record pool as the in-EXE text, or a flat blob?
- [ ] How the INTELLIGENCE delta (`intelligenceStat`) is written back to
      the character record (shared with `SAVER` / `CHAR.DAT`?).

## CELDRV.EXE — open questions

- [x] Build `celdrv.idb` (2026-08-31). Single code seg `seg000`
      "bmCELDRV" (16 funcs, 2 KB), thunk table `seg001` (373 entries),
      DGROUP `seg003`. 99.5% coerced, 0 bad insns, 373 thunks resolved.
- [~] Name `seg000` functions (`apply_renames_celdrv.py`): 13/16 —
      `celdrv_entry` (loads `CEL0`–`CEL2`/`DIS9`/`CEL3.BSV` via `rt_FE07`,
      relocates their offset tables, "AGAINST ALL ODDS!", the story
      crawl), `scrollStoryText`, `runCreditsCrawl` + `showCreditIbmVersion`
      / `showCreditMusic` / `showCreditArtwork` / `showCreditArtworkCont`,
      `serviceMusic`, `delayWithMusic` / `waitKeyWithMusic` (tentative),
      `celAnimStep` / `blitCelFrame` (tentative). `sub_10777` is a NOP
      sled + `jmp` (dead).
- [x] Map the CELDRV state vars (2026-08-31, `apply_dsvars_celdrv.py`;
      DGROUP `seg003`). `storyLine` (20B2, the 0..999 line counter --
      997 → credits), `celBank` (208A, 0..4), `celRelocBase` (208C),
      `celFrame` (20BE, 1..5), `displayDuration` (20F8). No shared
      LEGLIB slots -- CELDRV has no partyGold / hitPoints (pure
      cinematic). The rest of DGROUP is drawString layout scratch.
- [ ] Confirm this is *only* the ending — does `MUS`'s `chainToCel` ever
      invoke `CELDRV` for a mid-game exhibit animation, or is that path
      dead? (All 54 string records here are ending / credits content.)
- [ ] `DIS9.BSV` is loaded here (the `DIS*.BSV` set was "display screens?"
      in file-formats — this ties one of them to the ending).
- [ ] The 999-line victory-story text: is it in `STDRVSCR`-style records
      inside the EXE, or a `.BSV`? (`rt_46` array access on a DGROUP
      table at `ds:1E58h`.)

## SAVER.EXE — open questions

- [x] Build `saver.idb` (2026-08-31). Single code seg `seg000` "bmSAVER"
      (5 funcs, 1.5 KB), thunk table `seg001` (373), DGROUP `seg003`.
      99.8% coerced, 0 bad insns, 373 thunks resolved.
- [~] Name `seg000` functions (`apply_renames_saver.py`): 3/5 —
      `saver_entry` (the "SAVE THE GAME NOW IN PROGRESS?" flow +
      character-disk validation + quit/continue prompt), `saveRosterToDisk`
      (writes `CHAR.DAT` — `rt_73`/basStrBuild record build, `rt_FE35` /
      `rt_FE39` file I/O), `chainBackOrQuit` (ESC → DOS, else re-exec
      `OUT.EXE` / `DUN.EXE`). `sub_10411` is a 1-byte artifact.
- [ ] Which module SAVER came from — how does it know to re-exec `OUT`
      vs `DUN`? (a flag in `CHAR.DAT` / `LEGACY.DAT`, or an env var?)
      Only `OUT.EXE` and `DUN.EXE` are named as chain targets — what
      about saving from a town / castle / the museum?
- [ ] `CHAR.DAT` field layout — `saveRosterToDisk` is the write side,
      `readLegacyDat` / the menu's roster screens the read side.
- [ ] The "character disk" checks ("is not on this / character disk",
      "empty") imply a multi-disk / per-character-slot save scheme —
      confirm.

## SDEFENDR.EXE — open questions

- [x] Build `sdefendr.idb` (2026-08-31). Two code segs: `seg000`
      "bmSDEFENDR" (compiled BASIC) + `seg001` (hand-written asm arena
      engine); thunk table straddles `seg001`/`seg002` (328 entries).
      `seg000` 99.8% coerced, 0 bad insns.
- [~] Name functions (`apply_renames_sdefendr.py`): ~15 —
      `trainingSchoolMain` (mode select + rating + gold + `TWNDR`
      hand-off), `showBriefing`, `runTrainingLevel`, `runPractice`,
      `showWaveIntro`, `showWaveScore`, `drawScorePanel`, `drawFramedBox`;
      `arenaGameLoop` + its 8 step routines (`arenaInitPlayfield`,
      `pollPlayerTurn`, `firePlayerArrow`, `moveFireballs`,
      `arenaStepEndCheck`, `drawArenaSprites` — all **tentative**, from
      the loop structure + `ds:` byte-var usage, not yet verified).
- [ ] The `seg001` asm engine: it works on `ds:` bytes `0Ch/0Eh` (seg004
      playfield ptr), `11h` (turn key), `15h` (fire/shift), `16h`/`22h`
      (cooldown timers). Map the full arena state block in `seg004`.
- [ ] `SDMAP.GLB` / `.GMP` / `SDOBJ.GLB` field layout (the arena
      playfield + fireball/arrow sprites).
- [ ] Confirm the stat-change math: the "40, 31, 22, 19, 16, 14, 12"
      table (per-level hit thresholds?) vs. the " INCREASE: + / DECREASE:
      -" applied to `ARMOR,WEAPON,ENDUR`.
- [ ] Which town building launches it, and how the trained stat writes
      back to `CHAR.DAT`.

## GMB1.EXE / GMB2.EXE — open questions

- [x] Build `gmb1.idb` (2026-08-31). Single compiled-BASIC code seg
      `seg000` "bmGMB1" (21 funcs), thunk table `seg001` (431), card
      graphics `seg004`. 99.8% coerced, 0 bad insns.
- [~] Name `gmb1` functions (`apply_renames_gmb1.py`): 14/21 —
      `blackjackMain` (the whole game + outcomes + bankroll handling +
      `TWNDR` hand-off), `showInstructions`, `showWagerRules`,
      `pressKeyToContinue`, `showGoldLine`, `shuffleDeck`, `drawFromDeck`;
      `dealCardToHand` / `dealInitialHands` / `revealDealerCard` /
      `drawGoldAndBet` / `drawDealerArea` / `clearPromptLine` /
      `drawHandSprites` (all **tentative** — no distinctive text).
- [ ] `gmb1`: the hand-scoring logic — it leans hard on the `leglib`
      `rtm_FF4B`/`FF27`/`FF44`/`FF22`/`FF1F` value-stack cluster.
      Identifying those `B$…` primitives would clarify `dealCardToHand`.
- [ ] `gmb1` `ds:` vars: `ds:1F1Ah` (shoe pointer), `ds:1F04h`,
      `ds:21A4h`, the hand struct at `ds:1C7Ch`.
- [ ] `BJCHR.GLB` sprite-sheet layout (52 cards + backs).
- [x] Build `gmb2.idb` (2026-08-31) — it's **"Flip-Flop Parlour"**, a
      Plinko / pachinko game, *not* cards (doesn't touch `BJCHR.GLB`).
      Single code seg `seg000` "bmGMB2" (20 funcs), 467 thunks, 100%
      coerced, 0 bad insns.
- [~] Name `gmb2` functions (`apply_renames_gmb2.py`): 14/20 —
      `flipFlopMain`, `showInstructions`, `playRound`,
      `playPracticeRound`, `dropBallAndBounce`, `computePayout`,
      `drawBumpers`, `playTune`; `promptYesNo` / `playBounceSound` /
      `playWinChime` / `drawBigNumberPanel` / `drawBallAnim` /
      `stepBallPhysics` (tentative).
- [ ] `gmb2`: the ball-physics / bumper model in `dropBallAndBounce` +
      `stepBallPhysics` — how "flip-flop" bumper state biases the drop,
      and how `computePayout` (struct at `ds:1B96h`) maps bucket → odds.
- [ ] `gmb2`: does it read a data file for the bumper layout, or is the
      whole board the hard-coded DRAW-macro set in `drawBumpers`?
- [ ] Both games: how the gold delta is written back to `CHAR.DAT`.

## CONFIGUR.EXE — open questions

- [x] Build `configur.idb` (2026-08-31). Standalone Microsoft C — IDA's
      C loader + FLIRT recovered the whole MSC CRT; 3 segments; not
      packed. Named the only 6 app helpers + commented `_main`
      (`apply_renames_configur.py`).
- [x] It's a **disk-drive** config tool (drive letters for the game
      floppies / "reduce disk swaps"), NOT graphics/sound.
- [ ] `DRCONFIG.DAT` (1015 B) exact field layout — `_main` reads it into
      a stack buffer and checks bytes for `'0'`/`'1'`/`'2'` + drive
      letters near the start; the other ~1000 bytes are unexamined.
- [ ] Who else reads `DRCONFIG.DAT` at runtime (the LEGLIB file loader?
      each module's `BLOAD` path builder?).

## LEGLIB.EXE — open questions

- [x] Map the `int 3Fh` thunk-table entry format and build a resolver
      (2026-08-30). `resolve_rtm_leglib.py` + `rtm_map.py`;
      `resolve_thunks.py` on the client side. Mechanism written up in
      [overview.md](overview.md#int-3fh-run-time-dispatch-decoded-2026-08-30).
- [x] Extended the FE table to `FE70` (2026-08-30) — `out.exe` uses
      `FE6E`–`FE70`; `FE71`+ is past the table.
- [~] Attach real names to `rtm_*` (`apply_renames_leglib.py`). Done: 14
      hot ones — `basProcEnter`/`basProcLeave` (SUB frame enter/leave, cx
      = frame size), `basProcExit1`/`basProcExit2` (outer exit wrapper),
      `basStrAssign` (164x in out), `basStrConcat`, `basStrClear`,
      `basStrBuild`, `basArrayCopy`, `basPlayMusic`, `basScreenInit`
      (provisional), `drawString`/`drawStringInner`/`screenRefresh`
      (engine text). Continue with the FF-cluster `out` leans on
      (`FF4B` 154x, `FF20`, `FF1F`, `FF44`, `FF4E`, `FF50` — value/screen
      stack ops around `valueStackPtr` = `ds:111Ch`).
- [~] Partial DGROUP map (2026-08-31, `apply_dsvars_leglib.py`):
      `videoSegment` (0876, inits to 0xB800), `valueStackPtr` (111C),
      `screenFlags` (0EFA), `nestLevel` (0118) + tentatives
      (`vsScratchA`/`B`, `textAttr`, `ioChannel`, `gfxTempA`/`B`,
      `fmtBufPos`). ~460 DGROUP words total; the rest need the
      surrounding `rtm_*`/`sub_` clusters named first. `dsvars.py`
      output is the working list.
- [ ] Verify the 65 `[mid-func: verify]` `rtm_*` entries — confirm
      they're genuine shared-tail / multi-entry routines, not resolution
      errors.
- [ ] Separate stock BASIC runtime from LotA-specific engine code in
      `seg007`/`seg008` (the `bm*` graphics — `bmCOMBLIB`, `bmTCASANIM`).
- [ ] Find and name the `bmXXXX` bitmap/graphics primitives (the
      `bmMENU`, `bmREADY`, … names are already visible as strings).
- [ ] Find the `BLOAD`/`BSAVE` implementation and the file-I/O layer —
      the key to decoding every `.BSV`/`.GLB` file.
- [ ] Input handling (keyboard poll used by the menu state machine).
- [ ] `seg003` (53 KB) vs `seg004` (18 KB): which is runtime, which is
      game engine?
- [ ] Confirm graphics mode support (CGA / EGA / Tandy). NB `CONFIGUR.EXE`
      turned out to be **disk-drive** config only, not graphics — and the
      `*DR.EXE` files (`TWNDR`, `CASDR`, `STDRV`, `CELDRV`) are game
      *drivers*, not hardware drivers. Where the video mode is actually
      chosen is still open (LEGLIB startup? a `DIS*.BSV`?).

## MENU.EXE — open questions

- [x] Name all 467 `seg001` thunks + cross-reference to `leglib`
      (2026-08-30, `resolve_thunks.py`).
- [x] Force `seg000` to code (2026-08-30, `coerce_code.py`):
      99.5%, 0 bad insns, 25 functions, full call graph.
- [x] Name the 25 `seg000` functions (2026-08-30, `apply_renames_menu.py`).
      6 CHAR.DAT-record helpers left `sub_` (hard to distinguish).
- [x] Cosmetic block-chopping: post-process fixed via fall-through crefs
      past each `call far` (`apply_renames_menu.py`, final step). 1914 → 141.
- [x] Map the MENU DGROUP vars + name the 6 helpers (2026-08-31,
      `apply_dsvars_menu.py` + `apply_renames_menu.py`): `menuHighlight`
      (1F5C), `charCount` (2138), `rosterIndex` (1B0A), `charRecordSize`
      (211C, the CHAR.DAT `imul` stride); `sub_11A15` -> `pressAnyKey`,
      `sub_12055` -> `readCharDat`, `sub_12778` -> `writeCharDat`,
      `sub_10150` -> `menuStartup`, `sub_128A9` -> `updateCharDatEntry`,
      `sub_1210E` -> `enumerateRoster`. Only `sub_11A1E` still `sub_`.
- [ ] Confirm `playMusicTick` / `showTitleScreen` naming (the LEGACY.DAT
      touch in `playMusicTick` is unexplained).
- [ ] `menu.idb` input path reads `C:\dev\lota\menu.exe` (a copy; the
      original `.idb` was lost and rebuilt via `idat -B`). Harmless, but
      re-point at `C:\games\lota\MENU.EXE` if a full rebuild is ever
      needed.
- [ ] Mark up the `seg003:21D0h`+ text block as strings.
- [ ] Walk the menu state machine (main menu → play / instructions /
      credits / sound toggle; character management screens).
- [ ] Confirm whether `MENU.EXE` was unpacked before import (oversized
      header, entry `038F:00DF`), and whether the EA "Installation
      Program" is separable from the Quest menu program.
- [ ] Trace the chain-out to `OUT.EXE` / `MUS.EXE` and how the selected
      character is handed over (`CHAR.DAT`? `LEGACY.DAT`?).

## OUT.EXE — open questions

- [x] Build `out.idb` (2026-08-30). First built from the still-packed
      `OUT.EXE` (unreliable — un-relocated far ptrs, BSS DGROUP), then
      **rebuilt from the UNP-unpacked `OUT.EXE`**: 5 clean segments like
      menu, `seg000` coerced to 100%, 0 bad insns, ~97 functions, 467
      thunks (== menu namespace), 1297 run-time calls resolved. The
      `apply_renames_out.py` EAs carried over (reloc-only packing =
      byte-stable code).
- [~] Name the `seg000` functions (`apply_renames_out.py`): **67 / 121**
      (2026-08-31, second pass off the state vars + call graph). Movement
      pipeline (`doMovement` → `resolveMoveTarget` → `classifyLocationTile`
      / `identifyLocationObject` / `readTileObject` →
      `enterLocationOrChain`), overworld load (`enterOverworld` →
      `loadOverworldData` → `drawOverworldViewport`), combat
      (`beginEncounterView`, `resolvePlayerAttack`, `creatureDefeated`,
      `awardFoundItem`), events (`pegasusFlightAnim` / `pegasusFlyStep` /
      `showPegasusLanding`, `banditAmbushEvent`), `redrawAfterAction`.
      3rd pass (2026-08-31, all state vars named): **104 / 121** --
      `combatBeat_1..7`, `stageSfx_*` (named by trigger: hit / move /
      bump / attack / item / talk / event), `stageShopItem_1..3`,
      `setScenePosY_1..5`, `useCompass`, `showIndexedRemark`,
      `handleOverworldArrival`, `setupPromptScreen`, the resolveMoveTarget
      sub-tree. ~17 obscure sub-20-byte helpers left `sub_`.
- [ ] Confirm the tentatives: `addFoodDays` / `spendFoodDays` /
      `drawFoodGauge` (1F04/231C usage), `identifyLocationObject` /
      `readTileObject` (the `resolveMoveTarget` sub-tree), `rollCreatureStats`.
- [ ] `ds:1F04` — reused as a scratch/subcode word everywhere; in the
      combat stubs it pairs with `combatPhase` as an animation/message
      id, in movement it is the "blocked" result. Worth pinning down.
- [x] Fixed the call-far fragmentation merge (2026-08-30) — was
      orphaning code when a fragment's successor wasn't adjacent. Now
      merges only truly-adjacent fragments + re-sweeps.
- [x] Decode the screen-string pool format (2026-08-30) — see
      [file-formats.md](file-formats.md#screen-string-pool-in-the-exe-not-a-file--decoded-2026-08-30).
      `dump_strings.py` recovers + annotates it; drove ~25 `out`
      function names.
- [~] Map the OUT engine state variables (2026-08-31,
      `dsvars.py` + `apply_dsvars_out.py`). 227 DGROUP words are touched
      by the code; ~15 are real cross-function state and are now named:
      `partyGold` (1AD2, 32-bit), `hitPoints` (1ADA), `playerX`/`playerY`
      (1B02/1B06), `contextMode` (1F2A), `subMode` (2146), `combatPhase`
      (2192), `encounterActive` (21FE), `questFlags` (2234),
      `chainDestType` (1F16), `enteredLocationId` (1F02),
      `turnActionFlag` (212E), `overworldArrayPtr` (24E6, far ptr);
      `tileAhead` / `activeCreaturePtr` / `targetSlot` tentative. The
      remaining ~200 are per-call BASIC scratch temps. Next: use these to
      name the ~55 `sub_` helpers, and confirm the tentative three.
- [ ] Combat helper cluster: `sub_1232F` / `sub_13D98` / `sub_14054`
      (all poke `activeCreaturePtr` + `combatPhase`) — the encounter
      resolution / talk / trade routines.
- [ ] Pin down the trailing `! # $ &` control codes in `drawString`.
- [ ] The post-thunk RTM-loader stub (`seg000:16E8C`+) is left unswept
      (`$`-terminated DOS strings + boilerplate) — disassemble if needed.
- [ ] Trace the `BLOAD` sites for `OUTDATA.BSV` / `OUTM*.BSV` / `OUTOBJ.BSV`.

## Data formats

- [ ] Field-level decode of `LEGACY.DAT` (no BSAVE header — the odd one
      out; likely game progress / roster / config).
- [ ] `CHAR.DAT` (3444 bytes) — character roster the menu edits.
- [ ] Overworld map (`OUTDATA.BSV` / `OUTM*.BSV`), once `OUT.EXE`'s
      `BLOAD` sites are traced.
- [ ] Town / castle / dungeon layouts.
- [ ] `TITLE.GLB` / `TITLE.GMP` (title-screen graphics) — good first
      target since `MENU` loads them early.
- [ ] Graphics pixel packing (CGA/EGA) and the `.GLB` palette/tile
      layout.
- [ ] Music format (`MUSDATA.BSV` + the MML strings in `MENU`).

## ScummVM engine (future)

Not started. Same end goal as `ultima1`/`ultima2`: once the modules are
documented, a clean C++ reimplementation, then a ScummVM engine module.
