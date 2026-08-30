# Legacy of the Ancients (DOS) — Disassembly Overview

Working notes on the Legacy of the Ancients reverse-engineering effort.
Goal: document each DOS executable (and the shared run-time module) well
enough to write a clean C++ reimplementation, then a ScummVM engine
module — same approach as the sibling [`ultima1`](../../ultima1) and
[`ultima2`](../../ultima2) projects.

This file is the entry point into `docs/`. See also:
- [roadmap.md](roadmap.md) — prioritized list of what's investigated vs.
  still open, per executable.
- [file-formats.md](file-formats.md) — on-disk data formats (maps, saves,
  graphics, music, etc.). Barely started.

## The game is compiled Microsoft BASIC, not C

The startup splash credits three copyrights:

```
Game Program Copyright (c) 1987 - 1989 Quest Software, Inc.
Installation Program Copyright (c) 1989 Electronic Arts
Program Compiler Copyright (c) 1982 - 1988 Microsoft Corp.
```

The "Program Compiler" line is **Microsoft BASIC Compiler 6.0** (1988 —
the release between QuickBASIC 4.5 and BASIC PDS 7.0, sometimes called
BASCOM 6.0), **not** the Microsoft C compiler. Proof is embedded in
`LEGLIB.EXE`:

```
"Microsoft (R) BASIC Compiler Runtime Version 6.00"
"Copyright (C) Microsoft Corp 1982-1988. All rights reserved."
"Illegal function call"  "Subscript out of range"  "Overflow"  "Redo from start"
```

— the stock Microsoft BASIC runtime banner and error-message table. The
C-looking `start` stub in each module (SETBLOCK, PSP/`PATH=` scan, MZ
header check, `C_FILE_INFO`) is just the BC 6.0 EXE bootstrap; BASIC PDS
shares a lot of runtime plumbing with MSC, which is why it reads as C at
first glance.

Consequence for RE: compiled BASIC 6 turns nearly every source statement
into a `call far` into the runtime. The per-module `.EXE`s are therefore
*thin* — mostly `push args` / `call far` streams routed through an
`int 3Fh` thunk table into `LEGLIB.EXE`. Once `LEGLIB`'s entry points are
mapped to `B$…` runtime names + game-specific engine routines, every
module's call stream becomes readable pseudo-BASIC. IDA's FLIRT has good
MSC coverage but weak BASIC-runtime coverage — expect to identify `B$…`
entry points by hand, cross-referenced against QuickBASIC 4.5 / BASCOM 6
/ BASIC PDS 7 documentation and the QB reverse-engineering community's
notes on BRUN.

## Architecture: one shared RTM + thin per-subsystem modules

| File | Size | Role (best guess unless noted) |
|---|---|---|
| `LEGLIB.EXE` | 109,716 | **Shared run-time module.** Custom BRUN60 (BASIC runtime) bundled with the game's common engine — the `bmXXXX` bitmap/graphics system, file I/O, input, etc. The real payload. Confirmed: contains the BASIC runtime banner; loaded as the "run-time module" by every module below. |
| `MENU.EXE` | 45,056 | Main menu / launcher. Also carries the EA 1989 "Installation Program". Chains to `OUT.EXE` / `MUS.EXE` / `DUN.EXE`. Oversized header (3664 bytes, 909 relocs) and entry `038F:00DF` vs. the others' `xxxx:0010` — may have been unpacked before import. |
| `OUT.EXE` | 37,109 | Overworld / outdoor engine. References `MUS.EXE`, `SAVER.EXE`, `TWNDR.EXE`, `CASDR.EXE`, `DUN.EXE`. |
| `DUN.EXE` | 25,353 | Dungeon engine. `DUNDATA.BSV`, `DUNM*.BSV`, `DUNMON*.BSV`, `DUNOBJ.BSV`. |
| `TWNDR.EXE` | 47,315 | Town driver. `TWNMSG.TXT`, `TOWN0.BSV`…`TOWNB.BSV`. |
| `CASDR.EXE` | 36,845 | Castle driver. `CASTLE.BS1`/`.BS2`, `TCASOBJ.BSV`. |
| `MUS.EXE` | ~29,568 (unpacked) | **The MUSEUM driver** ("MUS" = Museum, *not* music). The Tarmalon Museum is the game's central hub; its display cases are portals into the world — chains to `TWNDR` (town exhibits), `DUN` (dungeon exhibits), `STDRV` (story), `CELDRV` (cel animations). `MUSDATA.BSV`, `MUSOBJ.BSV`, `MUSMSG.TXT`. |
| `SDEFENDR.EXE` | 15,443 (34,368 unpacked) | **The combat-training school** minigame — a 360° "defender"-style shooter reached from a town. Pick ARMOR or WEAPONS training, survive waves of fireballs approaching from all sides (turn keys + shift to fire), and doing well raises ARMOR / WEAPON / ENDURANCE. 50 gold/session, seven levels. Chains back to `TWNDR`. `SDMAP.GLB`, `SDOBJ.GLB`, `SDMAP.GMP`. |
| `GMB1.EXE` / `GMB2.EXE` | 13,285 / 21,079 (26,208 / 31,808 unpacked) | Town gambling minigames ("GMB" = gamble). **`GMB1` = BlackJack / "21"** (hit/stay vs. the dealer, natural pays double, five-card win, house stakes you 5 gold if you go broke). **`GMB2` = "Flip-Flop Parlour"** — a Plinko / pachinko game: drop a ball, it bounces bumper-to-bumper into one of 6 buckets, bet on the bucket number and/or colour; outer buckets pay more (even / double / 5×). Both chain back to `TWNDR`. |
| `CELDRV.EXE` | 8,967 (17,024 unpacked) | **Endgame victory cinematic** ("cel" = the cel-animation image banks). Shows "AGAINST ALL ODDS!", the scrolling victory-story narration (hero-name substitution, over music) and the end credits. `CEL0.BSV`…`CEL3.BSV`, `DIS9.BSV`. Chained to from `CASDR` after the Warlord falls. |
| `STDRV.EXE` | 24,923 | **"Stones of Wisdom" dice game** — a Liar's-Dice / Perudo variant played against the "DEALER" as the museum's *Stones of Wisdom* exhibit (`MUS` chains to it). Bid (quantity, value) pairs, challenge, loser drops a die, last with dice wins; the match result changes the character's INTELLIGENCE; each replay costs gold. NOT a "story driver" despite the name. `STDRVSCR.DAT` = the rules text. |
| `SAVER.EXE` | 5,903 (13,888 unpacked) | **Save-game handler.** Chained to from `OUT` / `DUN` on a save-or-quit request: "DO YOU WANT TO SAVE THE GAME NOW IN PROGRESS?", validates the character disk, writes the roster to `CHAR.DAT`, then either exits to DOS or re-execs the calling module. |
| `CONFIGUR.EXE` | 10,349 | **Floppy-drive / disk-layout config utility.** Standalone Microsoft C (no LEGLIB, no int-3Fh). Edits `DRCONFIG.DAT` to tell the game which drive letter(s) hold the floppy disks (or that it runs from hard disk / HD floppy), to reduce disk swaps. Does **not** configure graphics or sound. |

`LEGACY.BAT` is 4 bytes: `menu` — it just runs `MENU.EXE`.

All are 16-bit real-mode MS-DOS `MZ` executables. The `.GLB`/`.GMP`/`.BSV`/
`.BS1`/`.BS2`/`.DAT`/`.GLB` data files are engine-wide — see
[file-formats.md](file-formats.md).

## IDBs

One `.idb` per executable, created on demand as work starts on it. No
cross-IDB symbol sharing in IDA, so a `LEGLIB` routine identified in one
module's context still has to be independently confirmed in
`leglib.idb`. Because `LEGLIB` is shared, though, work done there is the
one place that pays off across every module.

| IDB | Root file | Functions named | Structs | Notes |
|---|---|---|---|---|
| `leglib.idb` | `LEGLIB.EXE` | ~445 / 773 (14 real, rest `rtm_*` provisional) | 0 | 10 segments; `seg003` (53 KB, 810 funcs) + `seg004` (18 KB, 279 funcs) are the code, `seg007` "bmCOMBLIB" / `seg008` "bmTCASANIM" the `bm*` graphics, `seg000`+`seg001` the DGROUP, `seg009` the BASIC error strings. Every int-3Fh run-time entry resolved; the 14 hot BASIC-runtime primitives named (`apply_renames_leglib.py`). DGROUP map (`apply_dsvars_leglib.py`, ~26 named): `dgroupSeg` (`ds:0101`), `nestLevel` (`ds:0118`), `stopFlag` (`ds:0136`), `valueStackPtr` (`ds:111C`, init `0xFAC`), `vsWorkA`/`B` (`ds:0C62`/`C64`), `videoSegment` (`ds:0876`, init `0xB800`), `screenFlags` (`ds:0EFA`), `screenCols`/`Rows` (`ds:0E68`/`0E6B`), `dirtyRectA`/`B` (`ds:1E86`/`1E88` — the FE42 refresh rect), `pagerLineCount` (`ds:1FEC`), `viewOriginX`/`Y` + `interiorDrawX`/`Y` (the FE1x tile engine) + more. |
| `menu.idb` | `MENU.EXE` | ~31 / ~34 seg000 funcs (+ 467 `rt_*` thunks) | 0 | `seg000` coerced + named. Layout: `seg000` code, `seg001` thunk table, `seg002` RTM bootstrap, **DGROUP `seg003`**, `seg004` stack. State vars (`apply_dsvars_menu.py` — MENU holds almost none, being a launcher): `menuHighlight`, `charCount` (0..8), `rosterIndex`, `charRecordSize` (CHAR.DAT stride), `menuChoice`. + the CHAR.DAT helpers `readCharDat` / `writeCharDat` / `menuStartup` / `pressAnyKey`. |
| `out.idb` | `OUT.EXE` | 104 / 121 seg000 funcs (+ `rt_*` thunks) | 0 | Overworld/towns/dungeons engine; chains to `MUS`/`SAVER`/`TWNDR`/`CASDR`/`DUN`. Rebuilt from the UNP-unpacked OUT.EXE (5 clean segments like menu); `seg000` coerced to 100%, 1297 run-time calls resolved. Functions named from the screen text (`dump_strings.py`) **and** the engine state vars (`apply_dsvars_out.py` — `partyGold`, `hitPoints`, `playerX/Y`, `combatPhase`, `questFlags`, …): `doMovement`, `resolveMoveTarget`, `enterOverworld`, `loadOverworldData`, `beginEncounterView`, `creatureDefeated`, `banditAmbushEvent`, `chainTo*`, the `combatBeat_*` / `stageSfx_*` / `stageShopItem_*` stub families, …. ~17 obscure sub-20-byte helpers left `sub_`. |
| `dun.idb` | `DUN.EXE` | 38 / 72 seg000 funcs (+ `rt_*` thunks) | 0 | Dungeon engine; chains back to `OUT`/`MUS`/`SAVER`. UNP-unpacked; 6 segments — **two** compiled-BASIC code segs: `seg000` "bmDUN" (main) + `seg001` "bmDUNG" (the first-person **dungeon-view renderer**, 7 funcs, all named — `renderDungeonView`, `drawViewSprite`, `blitViewCell`, wall-band drawers), thunk table `seg002`, **DGROUP `seg004`**. Both coerced to ~100%. Named from the screen text + engine state vars (`apply_dsvars_dun.py` — `dungeonLevel`, `hitPoints`, `playerX/Y`, `tileAhead`, `selectedSpell`, `dungeonArrayPtr`, `turnActionFlag`/`chainDestType` [shared slots with OUT]): `dunMain`, `processTileFeature`, `moveMonsters`, `drawDungeonHud`, `doLookSearch`, `openChest`, `castSpell`, `loadDungeonLevel`, …. |
| `twndr.idb` | `TWNDR.EXE` | 51 / 98 seg000 (+ 13 `bmTNCALB` seg001) funcs (+ `rt_*` thunks) | 0 | Town driver (entered from `OUT` board; chains back). UNP-unpacked; 6 segments — `seg000` "bmTWNDR" (98 funcs) + `seg001` "bmTNCALB" (town/castle anim, 26 funcs), thunk table `seg002` (**431**), **DGROUP `seg004`**. Both ~100% coerced. Named from the shop/NPC text + engine state vars (`apply_dsvars_twndr.py` — `partyGold`/`hitPoints` [OUT slots], `townServiceId`, `tileAhead`, `guardHitPoints`, `townArrayPtr`): `foodShop`, `weaponShopEntry`, `borrowMoney`, `loanRepayment`, `fortuneTeller`, `jailScene`/`jailRelease`, `townServiceDispatch` (~6 KB), `spendGold`, `enterTownService`, …. |
| `casdr.idb` | `CASDR.EXE` |  47 (+ 13 `bmTNCALB`) / 102 seg000 funcs (+ `rt_*` thunks) | 0 | Castle / fortress driver — **endgame** content (the Warlord, the Compendium, the king's quest). UNP-unpacked; `seg000` "bmCASDR" + `seg001` "bmTNCALB" (the **same** helper module TWNDR uses), thunk table `seg002` (431), **DGROUP `seg004`**. Both ~100% coerced. Named from the story text + engine state vars (`apply_dsvars_casdr.py` — `partyGold`/`hitPoints`/`tileAhead`/`targetSlot`/`turnFlag` [shared slots], `playerX/Y`, `enemyHitPoints`, `castleArrayPtr`): `warlordConfrontation`, `kingConfides`, `potionWizard`, `doFight`, `describeRoom`, `loadCastleLevel`, `fortressSelfDestruct`, `describeChest`/`describeLockedDoor`/`describeGasRoom`/`describePotionShop`, …. |
| `mus.idb` | `MUS.EXE` | 45 / 109 seg000 funcs (+ `rt_*` thunks) | 0 | **The MUSEUM driver** (the game's hub — display cases are portals). `seg000` "bmMUS" (109 funcs) + `seg001` "bmMUSDUNG" (6, all named — the **same first-person view renderer** `bmDUNG` uses, for the dungeon-style exhibit rooms), thunk table `seg002` (431), **DGROUP `seg004`**. Both ~100% coerced. Named from the screen text + engine state vars (`apply_dsvars_mus.py` — `partyGold`/`hitPoints`/`playerX/Y`/`menuChoice` [shared slots], `exhibitId`, `chainExeName`, `flagTestMask`): `enterExhibit`, `describeMuseumRoom`, `readPlaque`, `caretakerOffer`, `useCommand`, `chainToTown`/`Dungeon`/`Story`/`Cel`, `testExhibitFlag` + `checkFlag_*`, ~15 `exhibitName_*`. |
| `configur.idb` | `CONFIGUR.EXE` | 6 helpers + `_main` (of 65; rest are MSC CRT) | 0 | **Floppy-drive config utility** — standalone Microsoft C, no LEGLIB/thunks (IDA's C loader + FLIRT recovered the CRT). 3 segments (`seg000` code, `dseg` data, `seg002`). Only app code is `_main` (1.2 KB) + `setTextColor` / `readKeyUpper` / `getVideoPage` / `clearScreen` / `gotoXY` / `clearRegion` (BIOS int-10h/16h wrappers). Edits `DRCONFIG.DAT`. **No application `ds:` state** — `_main` is entirely stack-based; all of `dseg` is MSC CRT state (`apply_dsvars_configur.py` names `_errno` / `_doserrno` / `_osversion` / `_savedDS` / `_STKHQQ` / `_nfile` = 20, + the `colorRegs` REGS union). |
| `gmb2.idb` | `GMB2.EXE` | 14 / 20 seg000 funcs (+ `rt_*` thunks) | 0 | **"Flip-Flop Parlour"** (Plinko / pachinko betting game). Single code seg `seg000` "bmGMB2" (20 funcs), thunk table `seg001` (467), DGROUP `seg003`. **100.0%** coerced, 0 bad insns, 467 thunks, 80 string records. Named `flipFlopMain`, `showInstructions`, `playRound`, `playPracticeRound`, `dropBallAndBounce`, `computePayout`, `drawBumpers`, `playTune` + tentative helpers. Uses `BIGNUM.DAT` + GW-BASIC DRAW macros for the bumpers. |
| `gmb1.idb` | `GMB1.EXE` | 14 / 21 seg000 funcs (+ `rt_*` thunks) | 0 | **BlackJack table.** Single code seg `seg000` "bmGMB1" (21 funcs), thunk table `seg001` (431), DGROUP `seg003`, card graphics `seg004` (`BJCHR.GLB`). 99.8% coerced, 0 bad insns, 431 thunks, 49 string records. Named `blackjackMain`, `showInstructions`, `pressKeyToContinue`, `showGoldLine`, `shuffleDeck`, `drawFromDeck` + hand/deal/render helpers (tentative). |
| `sdefendr.idb` | `SDEFENDR.EXE` | ~15 game funcs named (+ `rt_*` thunks) | 0 | **The combat-training school minigame.** **Two** code segs: `seg000` "bmSDEFENDR" (compiled BASIC — framing: mode select, briefing, wave/score screens, rating + stat change, 50-gold economy, `TWNDR` hand-off) + `seg001` (hand-written **asm** — the real-time arena engine: `arenaGameLoop` over 8 step routines, playfield data in `seg004`). `seg000` 99.8% coerced, 0 bad insns; 328 thunks. Named `trainingSchoolMain`, `showBriefing`, `runTrainingLevel`, `runPractice`, `showWaveScore`, `drawScorePanel`, `arenaGameLoop` + engine steps (`pollPlayerTurn`, `firePlayerArrow`, `moveFireballs`, …, tentative). |
| `saver.idb` | `SAVER.EXE` | 3 / 5 seg000 funcs (+ `rt_*` thunks) | 0 | **The save-game handler.** Tiny — single code seg `seg000` "bmSAVER" (5 funcs, 1.5 KB), thunk table `seg001` (373), DGROUP `seg003`. 99.8% coerced, 0 bad insns, 373 thunks resolved, 19 string records. 3 named: `saver_entry` (the "SAVE THE GAME NOW IN PROGRESS?" flow), `saveRosterToDisk` (writes `CHAR.DAT`), `chainBackOrQuit` (ESC → DOS, else re-exec `OUT`/`DUN`). State vars (`apply_dsvars_saver.py`): `rosterIndex` (`ds:1B0A`, same slot as MENU), `menuChoice` (`ds:1E22`), `returnTarget` (`ds:1ACA` — carries the OUT-vs-DUN chain decision between the two functions). |
| `celdrv.idb` | `CELDRV.EXE` | 13 / 16 seg000 funcs (+ `rt_*` thunks) | 0 | **The endgame victory cinematic.** Tiny — single code seg `seg000` "bmCELDRV" (16 funcs, 2 KB), thunk table `seg001` (373), DGROUP `seg003`. 99.5% coerced, 0 bad insns, 373 thunks resolved, 54 string records. 13/16 named: `celdrv_entry` (loads `CEL*`/`DIS9.BSV`, "AGAINST ALL ODDS!", story crawl), `scrollStoryText`, `runCreditsCrawl` + `showCredit*`, `serviceMusic`/`delayWithMusic`, `celAnimStep`/`blitCelFrame`. State vars (`apply_dsvars_celdrv.py`): `storyLine` (`ds:20B2`, 0..999 -- passing 997 → credits), `celBank` (`ds:208A`, 0..4 = the 5 image banks), `celRelocBase` (`ds:208C`), `celFrame` (`ds:20BE`, 1..5), `displayDuration` (`ds:20F8`). |
| `stdrv.idb` | `STDRV.EXE` | 9 / 39 seg000 funcs (+ `rt_*` thunks) | 0 | **The "Stones of Wisdom" dice game** (a museum minigame, not a story driver). Single code seg `seg000` "bmSTDRV" (39 funcs), thunk table `seg001` (467), **DGROUP `seg003`**. 100% coerced, 0 bad insns, 467 thunks resolved. Named from the screen text + state vars (`apply_dsvars_stdrv.py` — `partyGold`/`menuChoice` [shared slots], `intelligenceStat` [`ds:1AF0`, what `resolveChallenge` adjusts], `stdrvArrayPtr`, `diceCount`, `playerBid`/`dealerBid`): `stdrv_entry`, `stonesOfWisdomMain`, `playerBidTurn`, `resolveChallenge`, `formatBidText`, `dealerTurn`/`evalDiceOdds`/`scoreDiceHand` (tentative). |

(Counts via `ida_scripts/identify.py -NoExport`; re-run any time as a
sanity check.)

### Segment layout, per module

IDA's MZ loader leaves most segments tagged `'UNK'` even where they hold
code. `menu.idb`:

| Seg | Range | Contents |
|---|---|---|
| `seg000` | `10000`–`13160` | Root menu/intro code (compiled BASIC). Coerced to code + all 25 functions named 2026-08-30. Header (`0`–`0x31`) is the `bmMENU` name + BSS descriptor. |
| `seg001` | `13160`–`138F0` | `int 3Fh` run-time thunk table (467 entries) + a few small resident helpers at the top. Decoded — see "int 3Fh run-time dispatch" below. |
| `seg002` | `138F0`–`13F30` | BC 6.0 EXE bootstrap + RTM loader (`start` at `139CF`). "Error in loading RTM…" strings live here. |
| `seg003` | `13F30`–`1A1B0` | DGROUP data. Text block (menu items, credits, instructions, "poor peasant on the world of Tarmalon…" intro, MML music strings, chained-EXE names) at offset `21D0h`+ (file `0x6ED2`–`0x7F10`). |
| `seg004` | `1A1B0`–`1A9B0` | Stack. |

`out.idb` (rebuilt from the **UNP-unpacked** `OUT.EXE`, 2026-08-30) has
the same 5-segment layout as menu:

| Seg | Range | Contents |
|---|---|---|
| `seg000` | `10000`–`167E0` | BASIC code (26 KB). Coerced to 100%. |
| `seg001` | `167E0`–`16E80` | `int 3Fh` thunk table (467 entries — matches menu's namespace exactly). |
| `seg002` | `16E80`–`174C0` | BC 6.0 EXE bootstrap + RTM loader. |
| `seg003` | `174C0`–`1A690` | DGROUP — screen-string pool (`~0x2150`+, in place) + engine state. |
| `seg004` | `1A690`–`1AE90` | Stack. |

### Packing

`OUT` / `DUN` / `TWNDR` / `CASDR` ship **packed** — a "Packed file is
corrupt" stub, `relocs=0`, entry `:0010`. The code/data payload is
literal but the relocation table is compressed, so `idat -B` loads them
raw with **no relocations applied** and the far-pointer segment words are
wrong (and DGROUP is BSS). Unpack with `UNP.EXE` (via DOSBox) first.
Paul unpacked all four in place in `C:\games\lota` on 2026-08-30
(`OUT.EXE`: 37 KB → 49 KB, 0 → 1512 relocs, entry `:00DF`). Because the
packing is reloc-only, the rebuilt `out.idb` is **byte-stable at the
code EAs**, so the `apply_renames_out.py` entries carried straight over.
`MENU.EXE` was already unpacked (a different packer, unpacked earlier);
`LEGLIB.EXE` is plain (1392 relocs); `CONFIGUR.EXE` is plain Microsoft C
(20 relocs, entry `02D4:05CE`) — not packed after all; built directly.

`SDEFENDR` / `GMB1` / `GMB2` / `CELDRV` / `STDRV` / `SAVER` were all
shipped packed too and unpacked in place alongside the first four (they
now show `relocs>0`, entry `:00DF`).

Segment names/numbers are **not** renamed or restructured — Paul
correlates them with the DOSBox debugger at runtime (sibling `ultima1`
convention).

## Headless IDA pipeline

Set up 2026-08-30, copied from `ultima1/ida_scripts` (IDA Pro 8.3,
`idat.exe`, no GUI — the GUI locks the `.idb`).

- **`ida_scripts/run_ida_script.ps1`** — entry point.
  ```powershell
  .\run_ida_script.ps1 -Idb leglib -ScriptName identify.py -NoExport
  .\run_ida_script.ps1 -Idb menu   -ScriptName apply_renames_menu.py -NoExport
  ```
  `-Idb` takes a bare stem (`leglib`, `menu`, `out`, …) or an explicit
  path. Refuses to run if the `.idb` is open in the GUI. `-NoExport`
  skips the `.asm`/`.idc` export + `save_database` (for read-only
  scripts).
- **`ida_scripts/batch_run_and_export.py`** — the driver
  (`idat.exe -A -S"batch_run_and_export.py <target.py> [noexport]"`).
  Execs the target script (as if under Alt+F7), then exports
  `<idb-stem>.asm`/`.idc` and saves. Everything — including the target
  script's stdout and any traceback — goes to
  `ida_scripts/batch_run_and_export.log` (idat's own console output in
  `-A` mode isn't reliably flushed).
- **`ida_scripts/identify.py`** — read-only report (root file, hash,
  segments, naming progress, raw-vs-code byte coverage).
- **`ida_scripts/rank_unnamed_functions.py`** — read-only, ranks
  `sub_XXXXX` functions by call-site count.

### Per-module coercion pipeline (generic)

For each client module, in this exact order (all idempotent):

1. **`resolve_thunks.py`** — auto-locate the int-3Fh thunk table (own
   segment or embedded), register the frame selector if embedded, itemise
   + name each thunk `rt_<key>`, comment it with its `rtm_*` target from
   `rtm_map.py`.
2. **`coerce_code.py`** — auto-detect the BASIC code segment + header +
   thunk hole; anchor every `call far`, sweep the gaps, protect
   `$`/NUL-terminated strings, make called thunks returning functions,
   carve + merge functions, fold epilogue stubs, and add a fall-through
   cref past every `call far` (last, so the listing reads continuously).
3. **`resolve_thunks.py`** again — non-destructive; re-asserts the
   `-> rtm_*` comments now that step 2 promoted the hot thunks to
   functions (needs `set_func_cmt` to propagate).
4. **`apply_renames_<module>.py`** — names + repeatable comments only;
   never triggers a reanalysis (that would drop the crefs).

- **`ida_scripts/profile_module.py`** — read-only; per-function callers,
  callees, `rtm_*` calls, and resolved string immediates, to support
  naming.
- **`ida_scripts/dsvars.py`** — read-only; profiles the DGROUP
  (module-scope) variables the code touches — read/write counts, the
  constants stored, the functions that use each. High-traffic
  cross-function vars are the real engine state; write-once-by-one-func
  vars are BASIC scratch temps. Feeds `apply_dsvars_<module>.py`
  (`apply_dsvars_out.py` names ~15 OUT state vars: `partyGold`,
  `hitPoints`, `playerX`/`playerY`, `contextMode`, `combatPhase`,
  `questFlags`, `chainDestType`, `overworldArrayPtr`, …).
- `resolve_rtm_leglib.py` / `rtm_map.py` build the shared
  `(prefix,ordinal) → leglib address` map once from `leglib.idb`.

## Conventions (from `ultima1`/`ultima2`)

- **Renames accumulate per-executable** in `apply_renames_<stem>.py`
  (e.g. `apply_renames_leglib.py`), a growing git-diffable list of
  `(ea, new_name, note)` tuples — not a new script per finding. Created
  on demand.
- **Struct edits** go in `apply_structs_<stem>.py` (different IDA API).
- **`DRY_RUN` defaults to `True`** until a module's pass is clearly
  stable.
- **One-off structural scripts** (splitting an array, flattening the
  int 3Fh thunk table, fixing inline-data-after-CALL) get their own
  dedicated script.
- Findings get written up here (or in `file-formats.md` for on-disk
  formats) so the `note` field in a rename entry can stay short.
- **Segments are never renamed** (see above).

## int 3Fh run-time dispatch (decoded 2026-08-30)

Every cross-module call in a client `.EXE` is a `call far` into that
module's thunk segment (`menu.idb`: `seg001`), where each entry is a 3- or
4-byte trampoline:

```
CD 3F nn        bare ordinal nn         (nn = 0x00..0xFD, 254 of them)
CD 3F FF nn     FF-prefixed ordinal nn  (nn = 0x00..0x67)
CD 3F FE nn     FE-prefixed ordinal nn  (nn = 0x00..0x6D)
```

`FE` / `FF` are always prefixes, never bare ordinals. `menu.exe` has 467
thunks total; the `(prefix, ordinal)` namespace is **flat** (no ordinal
reused across a prefix) and **identical across every client module** —
they all link against the same `LEGLIB` — so a name learned in one module
applies everywhere.

`LEGLIB.EXE` installs the `int 3Fh` handler (`leglib.idb` `seg003:7383h`,
set via DOS `int 21h`/`AX=253Fh` from `~seg003:734Bh`). On the **first**
execution of each call site it:

1. reads the ordinal byte(s) after `CD 3F` in the caller;
2. resolves the target:
   - **bare `nn`** → `seg003 : word[seg003:73F6h + 2·nn]`
   - **`FF nn`** → `word[seg003:75F2h + 2·nn]`, in `seg004` when
     `0x19 ≤ nn < 0x62`, else `seg003`
   - **`FE nn`** → full `seg:off` far pointer from the 4-byte entry at
     `seg003:15Ch + 4·nn` (lands in `seg004` / `seg007` / `seg008` — the
     bitmap/graphics segments; `FE` ordinals are the `bm*` graphics calls)
3. **rewrites the caller's `CALL FAR` operands in place** and `retf`s, so
   every call site self-patches to a direct far call after first use.
   Nothing in `LEGLIB`'s own image is modified.

The bare/`FF` tables store offsets only; the handler supplies the segment
as the constant `0x2A9` / `0xF9C` (file value; `+ 0x1000` = IDA
paragraph of `seg003` / `seg004`). The `FE` table holds real relocated
far pointers.

### Tooling

- **`ida_scripts/resolve_rtm_leglib.py`** — reads the three tables in
  `leglib.idb`, turns each of the 468 targets into a named function
  (`rtm_<key>`, e.g. `rtm_C2`, `rtm_FE26`), and writes
  **`ida_scripts/rtm_map.py`** (`(prefix,ordinal) → {ea, seg, name}`).
  First run 2026-08-30: 344 functions created, 4 34 (+ 13 `bmTNCALB`) names, 49 were already
  function heads, 65 land mid-function (shared-tail / multi-entry
  routines — normal for a compiled-BASIC runtime; commented
  `[mid-func: verify]`).
- **`ida_scripts/resolve_thunks.py`** (generic) — see the per-module
  pipeline above. `probe_rtm_tables.py` is the read-only helper this was
  worked out with.

### Provisional names → real BASIC runtime names

`rtm_*` are placeholders. Next step is identifying the actual Microsoft
BASIC 6.0 runtime routines (`B$…`) behind the hot ordinals. From
`menu.idb` call counts:

| key | calls | `leglib` addr | shape (first look) |
|---|---|---|---|
| `rtm_C2` | 171 | `seg003:1B572` | `push bp` frame, 2 stack args, copies a 4-byte descriptor (`[bx]`,`[bx+2]`) — string assignment / `LET`? |
| `rtm_D1` | 149 | `seg003:1B9B0` | — |
| `rtm_FE26` | 98 | graphics seg | bitmap blit (menu draws heavily) |
| `rtm_AF` | 26 | `seg003:13608` | multi-entry (`xor al,al` / `mov al,0FFh` fall-through with `rtm_0F`) |
| `rtm_F0` `rtm_F4` | 21 each | `seg003:1BBA7` / `1BB7C` | adjacent — paired variants |

## Findings log

Decided 2026-08-30 (with Paul): work `LEGLIB.EXE` first (or alongside
`menu`), since it's the shared payload.

- **2026-08-30** — Decoded the `int 3Fh` run-time dispatch (above).
  `leglib.idb`: 7 → ~440 named (all `rtm_*` provisional). `menu.idb`: all
  467 `seg001` thunks named + cross-referenced to `leglib`.
- **2026-08-30** — `menu.idb` `seg000` coerced to code
  (`coerce_code.py`): 99.5% instruction coverage, 0 bad insns, 25
  functions. The `menu.idb` rebuild lost the original `.idb` (recreated
  via `idat -B` from a copy of `MENU.EXE` — input path in the DB now
  reads `C:\dev\lota\menu.exe`).
- **2026-08-30** — Named all 25 `seg000` functions
  (`apply_renames_menu.py`), from the `seg003` text each one prints:

  ```
  menu_main -> mainMenuLoop  (SELECT CASE dispatch, self-looping)
    +- showStartupSplash       the 3-copyright splash (called from menu_main)
    +- drawMainMenuScreen -> drawCancelOption
    +- showQuestCopyright      "...Quest Software, Inc."
    +- showGameCredits         the credits screen
    +- showInstructions        SIMPLE INSTRUCTIONS / COMMANDS / MOVEMENT
    +- showTitleScreen -> loadTitleImage (TITLE.GLB/GMP->B800h), playMusicTick
    +- readLegacyDat
    +- eraseCharacterMenu  -.
    +- startNewGameMenu     +-> showCharacterRoster, showEmptyCharacterSlots,
    +- restartGameMenu     -'   promptCharacterNumber, sub_12055/12778/128A9
         startNewGameMenu -> promptNewCharacterName, playIntroAndLaunchGame
           (the "poor peasant on Tarmalon" intro, then chains to OUT.EXE)
  ```

  6 small CHAR.DAT-record / string-input helpers (`sub_11A15`,
  `sub_11A1E`, `sub_1210E`, `sub_12055`, `sub_12778`, `sub_128A9`) left
  `sub_` — genuinely hard to tell apart.

  Cosmetic: added a fall-through cref past every `call far` so the
  listing reads continuously (IDA's `int 3Fh` overlay special-casing
  otherwise chops a block after each) — 1914 breaks down to 141.

- **2026-08-30** — Generalised the menu-specific scripts into
  `resolve_thunks.py` / `coerce_code.py` / `profile_module.py` (deleted
  `resolve_thunks.py`, `coerce_code.py`, `dump_thunk_table.py`,
  `profile_seg000_menu.py`). Verified menu output unchanged.
- **2026-08-30** — Built `out.idb` (OUT.EXE, overworld engine). First
  from the still-packed `OUT.EXE` — worked well enough to decode the
  thunks / strings / ~40 functions, but the far pointers were
  un-relocated and DGROUP was BSS. Paul then **UNP-unpacked** OUT/DUN/
  TWNDR/CASDR in place; `out.idb` rebuilt from the unpacked `OUT.EXE`:
  5 clean segments like menu, `seg000` **100%** coerced, 0 bad insns,
  ~97 functions, 467 thunks (== menu's namespace), 1297 run-time calls
  resolved. The `apply_renames_out.py` EAs carried over unchanged
  (reloc-only packing → byte-stable code).
- **2026-08-30** — Decoded the screen-string pool format
  ([file-formats.md](file-formats.md#screen-string-pool-in-the-exe-not-a-file--decoded-2026-08-30));
  `dump_strings.py` recovers + annotates it. Named ~14 LEGLIB runtime
  primitives (`basProcEnter`/`Leave`, `basStrAssign`, …,
  `apply_renames_leglib.py`) and ~40 `out` functions (`doMovement`,
  `creatureAttack`, `shopBuy`, `chainTo*`, `museumAccessPrompt`, …).
- **2026-08-30** — Built `dun.idb` (DUN.EXE, dungeon engine) from the
  unpacked exe. 6 segments — **two** compiled-BASIC code segs (`seg000`
  "bmDUN" + `seg001` "bmDUNG"), so `coerce_code.py` gained a
  `$env:COERCE_SEG` override for the 2nd pass. Both ~100% coerced; 24/72
  `seg000` functions named from the screen text (`dunMain`, `openChest`,
  `monsterAttack`, `useMagicMenu`, `castSpell`, `loadDungeonLevel`, …).
- **2026-08-31** — Built `twndr.idb` (TWNDR.EXE, town driver) from the
  unpacked exe. Same 2-code-segment shape as dun (`bmTWNDR` + `bmTNCALB`);
  thunk table only 431 entries. 41/98 `seg000` functions named from the
  rich shop/NPC text — the food shop, weapon/armor shops, buy-back shop,
  bank, two moneylenders, fortune teller, jail, `townServiceDispatch`
  (~6 KB), guard combat, mail-delivery quest.
- **2026-08-31** — Built `casdr.idb` (CASDR.EXE, castle/fortress driver —
  the **endgame**). Same shape as twndr and shares the identical
  `bmTNCALB` `seg001` module. 34/102 named from the story text:
  `warlordConfrontation` ("YOU CAN'T STOP ME! … THE SPELL OF DEATH … ALL
  LIFE OUTSIDE THIS FORTRESS WILL"), `kingConfides` (the guardians of the
  scroll + the secret forearm mark), `potionWizard`, `doFight`,
  `describeRoom`/`describeObjects` ("THE COMPENDIUM IS THERE!"),
  `loadCastleLevel`, `exitCastle`.
- **2026-08-31** — Named the shared `bmTNCALB` `seg001` module — the
  town/castle **interior engine** that `twndr` and `casdr` both link
  (byte-for-byte the same 26-function module, only relocated
  differently, so `apply_renames_tncalb.py` is keyed by seg001 offset
  and runs against either IDB). 13/26: `drawInteriorTiles`,
  `refreshView`, `tileAt`, `scanLineOfSight`, `findObjectTile`,
  `moveActor`, `stepByDirection`, `dirBetween`, `viewFaceDirection`,
  `setViewport`, `drawActor`, `traceCombatLine`. These drive the
  `rtm_FE1x` graphics primitives in `leglib` `seg004` and read the
  interior map array (empty tile = `0xFF`).
- **2026-08-31** — Named the remaining `seg001` graphics helpers.
  `bmTNCALB` (twndr/casdr) up to **21/26**: `refreshTileGraphic` (the
  shared `rtm_FE19` single-tile blit), `stepLineOfSight` /
  `sightBlockedBy`, `traceCombatRay` / `combatRayResult`,
  `placeNpcSprite`, `drawViewFrame`, `tileAtOffset`. `bmDUNG` (dun
  `seg001`, **7/7**) and `bmMUSDUNG` (mus `seg001`, **6/6**) are the
  first-person view renderer -- structurally the same code in both:
  `renderDungeonView` / `renderExhibitView` loops the depth bands
  calling `drawViewWallBand{Near,Mid,Far}` + `drawViewSprite`, all on
  the shared `blitViewCell` primitive (`rtm_FE2A`). Which wall band is
  which is still a guess (marked TENTATIVE).
- **2026-08-31** — Mapped the MENU.EXE DGROUP vars
  (`apply_dsvars_menu.py`, DGROUP `seg003`). MENU is a launcher so it
  barely has state (30 words touched, most one-function scratch):
  `menuHighlight` (`ds:1F5C`), `charCount` (`ds:2138`, 0..8),
  `rosterIndex` (`ds:1B0A`), `charRecordSize` (`ds:211C`, the CHAR.DAT
  `imul` stride), `menuChoice` (`ds:1E22`). Named the 6 CHAR.DAT /
  startup helpers that were left `sub_`: `menuStartup` (intro music +
  `LEGACY.DAT` + splash), `pressAnyKey`, `readCharDat`, `writeCharDat`,
  `updateCharDatEntry`, `enumerateRoster`. `menu.idb` is now essentially
  fully named -- **the state-var + helper mapping is complete for every
  client module.**
- **2026-08-31** — DGROUP map of `leglib.idb` (the shared runtime, so
  DGROUP = runtime internals not game state; base = `seg000`, extends
  into `seg001`). `dsvars.py` over seg003..seg008 finds **~740 words
  touched**. The layout that emerges:
    - `0x0002..0x0140` — the BASIC runtime control block: `dgroupSeg`
      (`ds:0101`, the runtime's stashed DS), `nestLevel` (`ds:0118`),
      `stopFlag` (`ds:0136`, 0xFF = end program), `procFlags` /
      `savedStackTop` / `ioChannel`, `chainCmdPtr` (`ds:0874`).
    - `0x0216..0x0302` — the interior tile-graphics engine (`rtm_60`/`61`,
      `sub_1A2xx`): `viewOriginX`/`Y` (`ds:02C4`/`02C8`),
      `interiorDrawX`/`Y` (`ds:0250`/`0252`).
    - `0x0C62..0x0C64` — `vsWorkA`/`B`, the value-stack CX-spill registers.
    - `0x0E46..0x0EFA` — screen geometry: `screenCols`/`Rows`
      (`ds:0E68`/`0E6B`), `screenFlags` (`ds:0EFA`), `keyModifiers`.
    - `0x111C` — `valueStackPtr` (init `0xFAC`, `rtm_FF4A`/`FF4B` push/pop).
    - `0x14FA..0x1FEE` — the bm*/FE graphics layer: `videoSegment`
      (`ds:0876`, init `0xB800`), `dirtyRectA`/`B` (`ds:1E86`/`1E88`, the
      `rtm_FE42` refresh rect flushed by `screenRefresh`),
      `pagerLineCount` (`ds:1FEC`, the `rtm_FE54` pager), `interiorViewBase`.
  ~26 named (`apply_dsvars_leglib.py`); many single-function scratch
  slots and not-yet-understood `rtm_*` clusters remain, waiting on real
  `B$…` names for the surrounding routines.
- **2026-08-31** — Built `mus.idb` and discovered **`MUS.EXE` is the
  MUSEUM driver, not a music player** ("MUS" = Museum). The Tarmalon
  Museum is the game's hub: display cases are portals — `MUS` chains to
  `TWNDR` (town exhibits like "THORNBERRY"), `DUN` ("THE FOUR JEWEL
  DUNGEON", "THE PIRATE'S LAIR"), `STDRV` (later found to be the "Stones
  of Wisdom" dice game), `CELDRV` (later found to be the endgame
  cinematic). 37/109 `seg000` functions named: `enterExhibit`
  (the coin-portal mechanic), `describeMuseumRoom`, `readPlaque`,
  `caretakerOffer`/`caretakerDialog`, `useCommand`, the `chainTo*`
  hand-offs, ~15 `exhibitName_*` setters. Fixed the architecture
  table + file-formats (`MUSDATA.BSV` etc = museum data; `STDRV` =
  story driver, confirmed).
- **2026-08-31** — Built `stdrv.idb` and discovered **`STDRV.EXE` is
  not a "story driver"** — it's the **"Stones of Wisdom" dice game**, a
  Liar's-Dice / Perudo variant played against the "DEALER" (the museum's
  *Stones of Wisdom* exhibit; `MUS` chains here). From the recovered
  text: "YOU AND THE DEALER BOTH RECEIVE FIVE DICE … EACH OF YOU WILL
  TAKE TURNS BIDDING ON THE DICE … SOMEONE CHALLENGES … THE LOSER OF A
  GAME GIVES UP ONE DIE. THE LAST PLAYER WITH ANY DICE LEFT WINS THE
  MATCH. YOUR INTELLIGENCE WILL BE CHANGED ACCORDINGLY". Costs gold to
  replay ("PLAY AGAIN FOR `<n>` GOLD"). Single code segment (39 funcs),
  467 thunks, 100% coerced; 7 named (`stonesOfWisdomMain` loads
  `STDRVSCR.DAT` — the rules text — and runs the match loop;
  `playerBidTurn`, `resolveChallenge`, `formatBidText`; `dealerTurn` /
  `evalDiceOdds` are the dealer AI, tentative). Corrected the
  architecture table + `file-formats.md` (`STDRVSCR.DAT`).
- **2026-08-31** — Built `celdrv.idb`. **`CELDRV.EXE` is the endgame
  victory cinematic**, not a general cel player: it BLOADs `CEL0`–`CEL2`
  / `DIS9` / `CEL3.BSV`, shows the "AGAINST ALL ODDS!" title card, then
  scrolls a past-tense recap of the whole quest ("THUS `<N>` ARRIVED AT
  THE GREAT MUSEUM … FLEW THE FABLED PEGASUS … VICTORIOUS, AND PLACED THE
  WIZARD'S COMPENDIUM FOR ETERNAL SAFEKEEPING … ADVENTURE AWAITS…") with
  `<N>` = hero name, over music, then the end credits. Chained to from
  `CASDR` after the Warlord. Tiny module — 16 funcs / 2 KB; 13 named
  (`celdrv_entry`, `scrollStoryText`, `runCreditsCrawl` + four
  `showCredit*`, `serviceMusic` / `delayWithMusic` / `waitKeyWithMusic`,
  `celAnimStep` / `blitCelFrame`). State vars mapped later
  (`apply_dsvars_celdrv.py`): `storyLine` (`ds:20B2`, the 0..999
  narration-line counter -- passing 997 is what hands off to the
  credits), `celBank` (`ds:208A`, 0..4 = `CEL0`/`CEL1`/`CEL2`/`DIS9`/
  `CEL3`), `celRelocBase` (`ds:208C`, the paragraph offset added to
  relocate a BLOADed bank), `celFrame` (`ds:20BE`, cycles 1..5),
  `displayDuration` (`ds:20F8`). Everything else in DGROUP is
  one-function drawString layout scratch.
- **2026-08-31** — Built `saver.idb`. **`SAVER.EXE` is the save-game
  handler** — a small module `OUT` / `DUN` chain to on a save-or-quit
  request: "DO YOU WANT TO SAVE / THE GAME NOW IN PROGRESS?", character-
  disk validation ("`<name>` is not on this / character disk", "empty"),
  writes the roster to `CHAR.DAT` ("SAVING TO DISK"), then "{TO QUIT} -
  Hit the ESC key." → exit to DOS, else re-exec `OUT.EXE` / `DUN.EXE`.
  5 funcs / 1.5 KB; 3 named (`saver_entry`, `saveRosterToDisk`,
  `chainBackOrQuit`). Confirms `CHAR.DAT` is both the roster (menu edits)
  **and** the in-progress save — there is no separate save file.
- **2026-08-31** — Built `sdefendr.idb`. **`SDEFENDR.EXE` is the
  combat-training school** (not a standalone "arcade cabinet"): reached
  from a town, pick "ARMOR TRAINING" / "WEAPONS TRAINING", get a briefing
  ("stand in the center of this stadium, magic fireballs will approach
  from all sides ... Use either shift key to fire arrows ... over if
  you're hit five times"), survive seven levels of waves; the rating
  raises or lowers ARMOR / WEAPON / ENDURANCE, 50 gold a session, then
  it chains back to `TWNDR`. First module with a **hand-written
  assembly** code segment — `seg001` is the real-time arena engine
  (`arenaGameLoop` cycling 8 step routines over playfield data in
  `seg004`), no BASIC frame; `seg000` is the usual compiled-BASIC
  framing. ~15 game functions named.
- **2026-08-31** — Built `gmb1.idb`. **`GMB1.EXE` is the BlackJack
  table** ("GMB" = gamble) — reached from a town, hit/stay against the
  dealer, "Natural BlackJack pays double", "You win with five cards
  under 21", "Dealer stops with 17 or more"; bet 0 to quit; go broke and
  the house stakes you 5 gold once, break the bank and "The house is
  closed". Loads `BJCHR.GLB` (card sprites into `seg004`), chains back to
  `TWNDR`. Single compiled-BASIC code seg; 14/21 functions named
  (`blackjackMain`, `showInstructions`, `shuffleDeck`, `drawFromDeck`, …).
- **2026-08-31** — Built `gmb2.idb`. **`GMB2.EXE` is "Flip-Flop
  Parlour"** — a Plinko / pachinko betting game, *not* another card game
  (it doesn't actually use `BJCHR.GLB`). "YOU DROP A BALL ... IT BOUNCES
  BUMPER TO BUMPER, AND FALLS INTO A BUCKET AT THE BOTTOM ... YOU WIN
  GOLD BY GUESSING WHICH BUCKET (1-6) THE BALL WILL FALL INTO ... ALSO
  WIN BY GUESSING THE CORRECT COLOR ... BUCKETS 1-2 [even] / 3-4 DOUBLE
  / 5-6 FIVE TIMES ... THE BUMPERS FLIP-FLOP BACK AND FORTH". `seg000`
  **100%** coerced; 14/20 named (`flipFlopMain`, `showInstructions`,
  `playRound`, `playPracticeRound`, `dropBallAndBounce`, `computePayout`,
  `drawBumpers`, `playTune`). Uses `BIGNUM.DAT` + GW-BASIC DRAW macro
  strings for the bumper/ball shapes.
- **2026-08-31** — Built `configur.idb`. **`CONFIGUR.EXE` is the
  floppy-drive / disk-layout config utility**, not a graphics/sound
  "driver config" as the file name suggested — it edits `DRCONFIG.DAT`
  so the game knows which drive letter(s) hold the floppies ("four 360K
  5.25\" floppy disks using drive %c:", "two 720K 3.5\" ...", "to reduce
  disk swaps", "Please enter the letter of the drive to use for Disk 1
  (A-Z)?"). Standalone Microsoft C — IDA's C loader + FLIRT recovered
  the entire MSC runtime; the only app code is `_main` plus six BIOS
  screen/keyboard wrappers (`clearScreen`, `gotoXY`, `readKeyUpper`, …),
  all now named. Also: it is **not** packed (plain MSC, 20 relocs) —
  earlier note corrected.
- **All 14 executables now have an IDB** (leglib + 13 game modules).
  Every compiled-BASIC module is coerced to ~100% with its screen text
  decoded and its lead functions named. Remaining work is depth, not
  breadth: (a) map the `ds:` engine state vars to name the remaining
  `sub_` helpers per module; (b) continue `rtm_*` → `B$…` in
  `leglib.idb` (the `FF4B`/`FF20`/… value-stack cluster — heavily used
  by both gambling games — and the `rtm_FE1x` interior-graphics
  cluster); (c) decode the on-disk data formats
  ([file-formats.md](file-formats.md)); (d) then the C++ / ScummVM
  reimplementation.
- **2026-08-31** — Mapped the OUT.EXE engine state variables
  (`ida_scripts/dsvars.py` profiler → `apply_dsvars_out.py`). Of the 227
  DGROUP words the code touches, ~15 are genuine cross-function engine
  state and are now named + commented: `partyGold` (`ds:1AD2`, 32-bit —
  shop/food deduct, rewards add), `hitPoints` (`ds:1ADA` — movement
  starvation + combat damage, `<=1` → unconscious), `playerX`/`playerY`
  (`ds:1B02`/`1B06` — trial coords staged in `ds:208C`/`208A`, validated
  by `sub_151B7`), `contextMode` (`ds:1F2A`), `subMode` (`ds:2146`),
  `combatPhase` (`ds:2192`), `encounterActive` (`ds:21FE`), `questFlags`
  (`ds:2234`), `chainDestType` (`ds:1F16` — 2/3/4/6 = castle/town/
  dungeon/museum), `enteredLocationId` (`ds:1F02`), `turnActionFlag`
  (`ds:212E`), `overworldArrayPtr` (`ds:24E6` far ptr — the main
  game-data array, pushed to almost every `rtm_` call). The other ~200
  are per-call compiled-BASIC scratch temps. This is the lever for
  naming OUT's ~55 remaining `sub_` helpers.
- **2026-08-31** — Second OUT naming pass off the state vars + call
  graph + screen text (`apply_renames_out.py`): **67 / 121** `seg000`
  functions named (was ~40). New: the overworld-load chain
  (`enterOverworld` → `loadOverworldData` [`OUTM*.BSV` / `OUTDATA.BSV`,
  keyed by `combatPhase`] → `drawOverworldViewport`); the move pipeline
  (`doMovement` → `resolveMoveTarget` → `classifyLocationTile` /
  `identifyLocationObject` / `readTileObject` → `enterLocationOrChain`);
  combat (`beginEncounterView`, `resolvePlayerAttack`, `creatureDefeated`
  with loot + "FLESH FOR FOOD", `awardFoundItem`); the Pegasus event
  (`pegasusFlightAnim` / `pegasusFlyStep` / `showPegasusLanding`) and
  `banditAmbushEvent` (the scripted Compendium theft); `redrawAfterAction`
  and the food/status helpers (several tentative). ~40 tiny
  runtime-dispatched `(combatPhase, subcode)` and coordinate-preset
  stubs left `sub_` -- not worth speculative names.
- **2026-08-31** — Third OUT pass, now that every state var is named:
  **104 / 121**. Named the stub families -- `combatBeat_1..7` (the
  `(combatPhase, subcode)` encounter-animation setters), `stageSfx_*`
  (each stages a `(param1, param2)` pair at `ds:215x` before a tone --
  named by the action: `stageSfx_hit` / `_move` / `_bump` / `_attack` /
  …), `stageShopItem_1..3`, `setScenePosY_1..5` -- plus `useCompass`,
  `showIndexedRemark`, `handleOverworldArrival`, `setupPromptScreen`,
  and the `resolveMoveTarget` sub-tree (`checkLocationEntry`,
  `classifyMapFeature`, `computeLocationOffset`, …, tentative). ~17
  obscure sub-20-byte helpers left `sub_`.
  DUN's DGROUP is **seg004**) and did a 2nd naming pass: **38 / 72**
  `seg000` functions (was 24). `dungeonLevel` (`ds:1ACA`), `hitPoints`
  (`ds:1ADA`, capped 250 -- `ds:20EA` is just the display scratch),
  `playerX`/`playerY` (`ds:20CE`/`20D0`), `tileAhead` (`ds:20C4`),
  `selectedSpell` (`ds:1E24`), `dungeonArrayPtr` (`ds:2274`); crucially
  `turnActionFlag` (`ds:212E`) and `chainDestType` (`ds:1F16`) are the
  **same DGROUP offsets as OUT** -- LEGLIB fixes those slots across
  modules. New functions: `processTileFeature` (the per-turn feature
  handler + trap-name table), `moveMonsters`, `drawDungeonHud`,
  `doLookSearch`, `clearTurnFlag`, `setActionPhase_1/2/3`.
- **2026-08-31** — Mapped the TWNDR.EXE state vars
  (`apply_dsvars_twndr.py`, DGROUP `seg004`) + 2nd naming pass: **51 /
  98**. `partyGold` (`ds:1AD2`) and `hitPoints` (`ds:1ADA`) confirmed at
  the **same DGROUP offsets as OUT and DUN** -- those two engine slots
  are fixed by LEGLIB across every client module. Town-specific:
  `townServiceId` (`ds:1F22` = the `townServiceDispatch` SELECT CASE),
  `tileAhead` (`ds:1F02`), `guardHitPoints` (`ds:216E`), `townArrayPtr`
  (`ds:278C`). New functions: `spendGold` (the shared "pay N gold"
  helper), `enterTownService` (the ENTER/USE command),
  `facePlayerDirection`, `checkLineOfSight`, `redrawTownView`.
- **2026-08-31** — Mapped the CASDR.EXE state vars
  (`apply_dsvars_casdr.py`, DGROUP `seg004`) + 2nd naming pass: **47 /
  102**. Confirms the LEGLIB-fixed slots a **fourth** time: `partyGold`
  (`ds:1AD2`, barely used — the castle has no economy), `hitPoints`
  (`ds:1ADA`), `tileAhead` (`ds:1F02` — the #1 read), `targetSlot`
  (`ds:1F24`), `turnFlag` (`ds:1F2A`). Castle-specific: `playerX`/`playerY`
  (`ds:1B00`/`1B04`), `enemyHitPoints` (`ds:2222`), `castleArrayPtr`
  (`ds:25B0`); `castleOrFort` / `viewLevel` / `mapStride` tentative. New
  functions: `fortressSelfDestruct` ("SELF-DESTRUCTION IN 5 MINUTES!"
  after the Warlord falls), the `describeRoom` cases (`describeChest`,
  `describeLockedDoor`, `describeGasRoom`, `describePotionShop`),
  `facePlayerDirection` / `checkLineOfSight` (same shapes as TWNDR).
- **2026-08-31** — Mapped the MUS.EXE state vars (`apply_dsvars_mus.py`,
  DGROUP `seg004`) + 2nd naming pass: **45 / 109**. Fifth module to
  confirm the shared slots (`partyGold` @ `ds:1AD2`, `hitPoints` @
  `ds:1ADA`, `playerX`/`playerY` @ `ds:1B02`/`1B06`, `menuChoice` @
  `ds:1E22`). Museum-specific: `exhibitId` (`ds:20FE` -- the
  `enterExhibit` SELECT CASE that routes to TWNDR / DUN / STDRV /
  CELDRV), `chainExeName` (`ds:210C` -- MUS passes the next module's
  name as a *string* rather than a `chainDestType` code), `flagTestMask`
  / `flagTestResult` (`ds:2136`/`2138` -- the exhibit-flag test idiom).
  New functions: `testExhibitFlag` + `checkFlag_03` / `_2B` / `_D0` /
  `_0300` / `_0800` / `_2000`.
- **The engine state-var map is now done for all six play modules**
  (OUT, DUN, TWNDR, CASDR, MUS — plus the trivial STDRV/CELDRV/SAVER).
  `ds:1AD2` = partyGold and `ds:1ADA` = hitPoints in every one;
  `ds:1F02` = tileAhead and `ds:1F2A` = a turn/context flag wherever
  they appear. See each module's `apply_dsvars_<m>.py`.
