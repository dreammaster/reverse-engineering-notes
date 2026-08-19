# Roadmap

Working list of next steps, roughly in priority order. Check items off /
add notes as they're resolved — this file is meant to stay current across
sessions, unlike a one-off todo list. See [overview.md](overview.md) for
the per-executable breakdown this is tracking against.

## Infra (done, 2026-08-19)

- [x] Set up headless IDA pipeline (`ida_scripts/run_ida_script.ps1` +
      `batch_run_and_export.py`), generalized for 5 IDBs (unlike
      `ultima2`'s single-file driver) by deriving export paths from
      whatever `.idb` `idat.exe` was actually pointed at.
- [x] Wrote `ida_scripts/identify.py`, ran it read-only against all five
      IDBs to catalog current state (root file, segments, function/struct
      naming progress) — see the table in overview.md.
- [x] Verified full export+save round-trip against `ultima1_space.idb`
      (re-ran `identify.py` without `-NoExport`; `.idc` byte-identical,
      `.asm` differs only in incidental ordering/whitespace, not content).
- [x] Fixed `.gitignore` (was still ultima2-specific filenames).
- [ ] Create `docs/file-formats.md` once the first on-disk format
      (savegame, map, etc.) is actually traced — don't stub it empty.

## Executable order

Not yet decided with Paul. Candidates, in order of how much prior work
exists (i.e. how close each is to "fully documented" already):

1. **`ultima1_out` (OUT.EXE)** — furthest along (266/353 functions, 11
   structs). Likely the biggest single module (overworld + towns +
   dungeons) so also the highest payoff to finish first.
2. **`ultima1_space` (SPACE.EXE)** — next furthest (156/210, 16 structs).
   Self-contained minigame, probably the fastest to actually *finish*
   even though it's not the most-complete-by-percentage.
3. **`ultima1_gen` (GEN.EXE)** — 44/113, 5 structs.
4. **`ultima1` (ULTIMA.EXE)** — 39/100, 4 structs. Probably just a
   launcher/title-screen/chainloader — worth confirming that hypothesis
   early since it may be small enough to finish quickly once looked at
   directly.
5. **`ultima1_mondain` (MONDAIN.EXE)** — 1/191, no structs. Essentially
   starting from scratch; role not yet confirmed (best guess: Mondain's
   castle / final confrontation, given the name).

Decided 2026-08-19 (Paul's call): start with **`ultima1_out` /
OUT.EXE**.

## OUT.EXE — next steps

- [x] Exported `.asm`/`.idc` for the first time this session (they
      didn't exist yet — only `ultima1_space` had been exported before).
- [x] Ran `ida_scripts/rank_unnamed_functions.py` to rank the 87
      remaining `sub_XXXXX` by call-site count.
- [x] First renaming pass: 7 functions confirmed and applied via
      `ida_scripts/apply_renames_out.py` — `getKeypressAndWaitRaw`,
      `toLowerLetter`, `_nmalloc`, `_nfree`, `_nheapgrow`, `readAmount`,
      `isDigit`. Full writeup in
      [overview.md](overview.md#outexe--findings-log).
- [x] Second renaming pass: confirmed the full CRT file-I/O layer
      underneath `readFile`/`writeFile`/`_fopen` was right — 21
      functions + 3 globals (`_fread`, `_fwrite`, `_fclose`, `_filbuf`,
      `_flsbuf`, `_openfile`, `_open`, `_flushall`, the `_dos_*` raw
      DOS primitives, `errno`/`_doserrno`/`_fmode`). Full writeup in
      [overview.md](overview.md#outexe-crt-file-io-layer-decoded).
- [x] Third renaming pass: decoded the shop-transaction cluster behind
      `transactWeapons`/`transactArmory`/`transactMagic`/
      `transactTransport` — 12 functions (`calcWeaponBuyPrice`,
      `calcWeaponSellPrice`, `drawWeaponShopLine`, `sellWeapons`, and
      the armor/magic/transport equivalents, plus `divmod32`). Full
      writeup in
      [overview.md](overview.md#outexe-shop-transaction-cluster-decoded).
      306/353 functions now named, 47 `sub_XXXXX` remain.
- [ ] Figure out `divmod32`'s exact role in `transactWeapons`'
      weapon-tier-availability gating (chained `divmod32` calls against
      `_moveCtr`, `0x7FFF`, and `1500`) — mechanically confirmed as
      32-bit division, but the game-mechanic meaning of the two chained
      calls isn't nailed down. See overview.md.
- [x] Fourth renaming pass: decoded the `get` command's castle
      item-theft mechanic — `getTownItem`, `getArmorTile`/
      `getWeaponTile`/`getFoodTile`, `denyGetNoPermission`,
      `checkCaughtStealing`, and the `_castleItemAllowance` global. 6
      renames. Full writeup in
      [overview.md](overview.md#outexe-get-command--castle-item-theft-decoded).
      312/353 functions now named, 41 `sub_XXXXX` remain.
- [x] Fifth renaming pass: `showPillarInscription` (an 8-way flavor-
      text jump table for entering pillars, containing a Shelley
      *Ozymandias* Easter egg) and `findWidgetAtPosition`
      (`attackPerson`'s "who's at this position" lookup, distinct from
      `getLocationWidgetAt`). 314/353 functions now named, 39
      `sub_XXXXX` remain.
- [x] Sixth renaming pass: the 3 hardware-specific `setVideoMode`
      initializers (`initVideoModeCGA`/`EGA`/`Tandy`, matching real
      `INT 10h` mode numbers 4/0Dh/9) plus `buildScanlineOffsetTable`,
      `waitTimerTicks`, `drawDeathGraphic`, `drawSelectItemPanel`. Full
      writeup in
      [overview.md](overview.md#video-mode-initializers-and-a-few-standalone-finds).
      321/353 functions now named, 32 `sub_XXXXX` remain.
- [x] Seventh renaming pass: `setCriticalErrorHandler`/
      `criticalErrorHandler` (the DOS `INT 24h` handler pair behind
      `insertDisk`'s custom disk-error prompt) and `_nheapinit`. Spot-
      checked but didn't individually name the rest of the CRT startup
      chain (10 functions) or 5 candidate-dead (`proc near`, 0 callers,
      not interrupt-reachable) functions at the time — **correction**:
      most of both groups turned out to be the EXE-chaining cluster
      (see the ninth pass below) once `sub_1908B`/`sub_190A6`'s calls
      into a mis-scoped chunk of `_nheapinit` were traced properly;
      only `sub_192AE` is genuinely dead. Full writeup in
      [overview.md](overview.md#critical-error-handler-and-heap-init).
      324/353 functions now named, 29 `sub_XXXXX` remain (92%).
- [x] Eighth renaming pass: decoded all 10 of `playSound`'s effect
      handlers (`soundEffectBump`/`Ack`/`Damage`/`MonsterAttack`/
      `Footstep`/`Success`/`Failure`/`Attack`/`8`/`9`) by walking every
      `playFX` call site back to its literal `effectNum`. Full writeup
      in [overview.md](overview.md#playsound-effect-table-decoded).
      334/353 functions now named, 19 `sub_XXXXX` remain (95%).
- [x] Ninth renaming pass: decoded the EXE-chaining mechanism —
      `writeInUseAndExit` chains to another executable via a **custom
      overlay loader, not DOS `INT 21h`/`4Bh` EXEC** (reads the target
      file directly, builds a PSP by hand, far-JMPs into it, never
      returns to DOS). 15 functions/locations named
      (`chainToExecutable`, `findExecutableFile`,
      `buildAndChainExecutable`, `execProgram`, `hasFileExtension`,
      `ensureFileExtension`, `_dos_getfileattr`, `strcpy`, `strlen`,
      `strncpy`, `_exit`, `atexit`, `_creat`, plus 2 relabeled
      locations). **Important for the reimplementation**: model
      executable-switching as an in-engine mode change, not process
      spawning — see
      [overview.md](overview.md#exe-chaining-mechanism-decoded).
- [x] Tenth renaming pass: `_read`/`_write`/`_lseek` (completing the
      CRT I/O layer) plus the last standalone finds
      (`checkRange19x9`, confirmed-dead `sub_192AE` left unnamed).
      **OUT.EXE renaming sweep complete: 352/353 functions named
      (99.7%)** — only one confirmed-dead function remains
      unnamed. Full writeup in
      [overview.md](overview.md#outexe-complete--final-pass-and-dead-code).
- [ ] Fix `word_1F95E` — currently a single `dw 1` plus raw `db` bytes,
      should be a proper `dw 4 dup(?)` (or named) powers-of-ten array.
      Structural fix (needs `apply_structs_out.py` or a one-off script),
      not a plain rename.
- [ ] Rename the `sg013A`/`sg0E82`/`seg002`/`seg003` segments to the
      `CODE`/`DATA`/... convention once their roles are confirmed (not
      done yet for this IDB, unlike the naming sweep itself).
- [ ] Cross-check the 5 structs shared by name with other IDBs
      (`STR15`, `Point`, `Rect`, `Savegame`, `Creature`) actually agree
      field-for-field with their same-named counterparts elsewhere —
      flagged as unverified in overview.md.
- [ ] **IDB hygiene**: `_nheapinit`'s proc boundary is wrong — it
      visually contains `execProgramEntry` and `translateDosErrorToErrno`
      (both nested `loc_` labels, not real children) purely because
      IDA merged contiguous code with no gap between them. Named the
      two locations directly rather than fixing the boundary (lower
      risk than `ida_funcs.add_func` structural surgery this session),
      but a future pass should properly split `_nheapinit` /
      `execProgramEntry` / `translateDosErrorToErrno` into 3 separate
      functions so the call graph and function list read correctly.
      See overview.md's EXE-chaining section.
- [ ] Same IDB-hygiene family: `sub_192AE` (dead, shares tail code with
      `divmod32`) confirms at least one more case of a linked-but-
      unused sibling function; not worth chasing further unless a
      future finding needs to call into the middle of one.

## OUT.EXE status: essentially complete

352/353 functions named (99.7%), all 11 pre-existing structs still in
place. Remaining open items above are all follow-up polish (an array
mis-typing, segment renames, struct cross-checks, 2 IDB-hygiene notes)
rather than unresolved game logic — every command, dialog, shop,
combat routine, and CRT subsystem in this executable has been traced
and named. Good stopping point to call this executable done and move
to the next one.

## ULTIMA.EXE — next steps

Switched focus here 2026-08-19 per Paul's request — title screen/
character-creation launch logic, likely to inform the reimplementation
early. Was 39/100 named; exported `.asm`/`.idc` for the first time.

- [x] First renaming pass: recognized this executable links the exact
      same CRT object files as OUT.EXE and transferred 22 functions +
      2 locations + 2 globals by direct-read confirmation (not just
      shape inference) — the full `_dos_*`/`_n{malloc,free,heapgrow,
      heapinit}`/`_open`/`_openfile`/`_fread`/`_fclose`/`_flsbuf`/
      `errno`/`_doserrno` cluster, plus the critical-error-handler pair.
      61/100 functions now named (61%). Full writeup in
      [overview.md](overview.md#ultimaexe-crt-layer-transferred-from-outexe).
- [ ] Still missing from the transferred CRT cluster: `_filbuf`,
      `_fwrite`, `allocFileBuffer`, `findFileHandleSlot`, `_flushall`,
      `_read`/`_write`/`_lseek`, `atexit`, `_creat` — likely present in
      the remaining 39 `sub_XXXXX` but not yet located; this executable
      does very little file I/O (just two fixed image reads) so some
      may simply be unused/uncalled here.
- [x] **Overlay-chaining hypothesis confirmed**: `showTrademarks`
      unconditionally chains to `"gen.exe"` (the only executable name
      referenced anywhere in this binary) via the exact same
      `chainToExecutable`/`execProgramEntry`/`findExecutableFile`/
      `buildAndChainExecutable`/`execProgram` shape as OUT.EXE. Full
      chain: `ULTIMA.EXE` → `GEN.EXE` → `OUT.EXE` ↔ `SPACE.EXE`.
      9 functions named. 70/100 functions now named (70%). Full
      writeup in
      [overview.md](overview.md#ultimaexe-chains-unconditionally-to-genexe).
- [x] Third renaming pass: `writeString2_mb` (a pre-existing but
      misleading name from before this session) turned out to be a
      `vsprintf`-family formatter, not multi-byte text handling —
      renamed to `printStartupMessage` along with its 4 helpers
      (`vsprintf`, `formatArg`, `fputs`, `putc`). 74/100 functions now
      named (74%). Full writeup in
      [overview.md](overview.md#writestring2_mb-was-a-misnamed-printf-not-multi-byte-text-handling).
- [x] Fourth renaming pass: completed the transferred CRT cluster
      (`_filbuf`/`_read`/`_lseek`/`_write`/`allocFileBuffer`/
      `findFileHandleSlot`, all matching OUT.EXE by exact byte size),
      resolved the `argv[1][0]=='C'` mystery (forces CGA video mode,
      overriding hardware auto-detection — confirmed via `init_video`),
      discovered `strncpy`/`toupper` are each duplicated at two
      addresses (`strncpy2`/`toupper2`, real static-linking artifact
      not a bug), finished the printf family (`formatHex`,
      `formatOctal`, `atoi`, `isdigit`), and named the remaining
      graphics/animation helpers (`videoDrawPoint`,
      `videoDrawPointAlt`, `drawLineInternal`,
      `buildScanlineOffsetTable`, `divmod32`, `drawLogoPixelRow`,
      `flushKeyboardBuffer`, `drawAnimatedCursor`). Full writeup in
      [overview.md](overview.md#ultimaexe-complete--100100-100).

## ULTIMA.EXE status: complete

**100/100 functions named (100%)**, 39 → 100 across 4 passes this
session (2026-08-19/20). Confirmed the full executable-chaining
architecture (`ULTIMA.EXE` → `GEN.EXE` → `OUT.EXE` ↔ `SPACE.EXE`) and
that this executable is purely the title/attract-mode screen with no
character-generation logic of its own. Nothing left to do here beyond
the general cross-IDB follow-ups already logged under OUT.EXE (struct
field cross-checks, segment renames).

## GEN.EXE — next steps

Started 2026-08-20 per Paul's direction. Was 44/113; exported
`.asm`/`.idc` for the first time.

- [x] First renaming pass: transferred the CRT cluster (8 functions,
      several exact-byte-size matches to OUT.EXE) and the `playSound`
      table, and confirmed + named the full exec-chain to `OUT.EXE`
      (`launchGame` launches roughly `OUT C 0` — video-mode letter and
      save-slot digit as `argv[1]`/`argv[2]`). 71/113 functions now
      named (63%). Full writeup in
      [overview.md](overview.md#genexe-crt-layer-and-exec-chain-to-outexe).
- [x] **Character-creation point-buy mechanic decoded**: 6 attributes
      (Strength/Agility/Stamina/Charisma/Wisdom/Intelligence — not the
      5 assumed), baseline 10, 30-point pool, hard range [10, 25].
      Named `decreaseAttribute`/`increaseAttribute`/
      `clearSelectionArrows`. 74/113 functions now named (65%). Full
      writeup in
      [overview.md](overview.md#character-creation-point-buy-mechanic-decoded).
- [ ] **IDB hygiene**: `_savegame._hits[si]` (used in `decreaseAttribute`)
      appears to be a stale/incorrect alternate struct-field name for
      the same memory `_savegame._strength[]` (confirmed correct via
      `updateAttribute`) actually resolves to — likely IDA attached
      the wrong overlapping field name to that one access. Needs a
      struct-definition fix (`apply_structs_gen.py`, not created yet),
      not a plain rename. See overview.md.
- [ ] Check OUT.EXE's `start`/`main` for `argv[1]`/`argv[2]` handling
      (the video-mode letter and save-slot digit GEN.EXE passes via
      `launchGame`) — likely undocumented there since OUT.EXE's own
      argument parsing wasn't traced during that session.
- [x] Third/fourth renaming passes: finished the CRT transfer
      (`_dos_*` primitives, `_write`/`_nmalloc`/`_nheapgrow`/`_nfree`/
      `_nheapinit`, `writeNumber`, `hasFileExtension`/
      `ensureFileExtension`/`strcpy`/`strncpy`), named
      `getKeypressRaw` (the real single-poll primitive — this IDB's
      pre-existing `getKeypress` is actually the wait-loop wrapper,
      left as-is since renaming it isn't worth the cross-reference
      churn), `insertDisk`, and confirmed the 4-character roster limit
      (`drawCharacterRoster`, `showCharacterReplacementMenu`). Full
      writeup in [overview.md](overview.md#genexe-complete--113113-100).

## GEN.EXE status: complete

**113/113 functions named (100%)**, 44 → 113 across 4 passes
(2026-08-20). Combined with ULTIMA.EXE and OUT.EXE, the full
`ULTIMA.EXE` → `GEN.EXE` → `OUT.EXE` ↔ `SPACE.EXE` chain is now named
end to end, plus the actual character-creation game rules (6
attributes, 10-25 range, 30-point pool, 4-character roster limit).
Remaining open item: the `_savegame._hits`/`_strength` struct-field
mixup noted above needs a real struct-definition fix, not a rename —
left for a future `apply_structs_gen.py` pass.

## SPACE.EXE — next steps

Started 2026-08-20 per Paul's direction. Was 156/210 already named
from substantial prior work.

- [x] First renaming pass: filled the remaining CRT cluster gaps (24
      functions, confirmed via `INT 21h` mapping) and cleaned up 2
      placeholder names (`"exec?"` → `execProgramEntry`, `"f"` →
      `_filbuf`) left by prior work. Full writeup in
      [overview.md](overview.md#spaceexe-crt-gaps-filled).
- [x] Second renaming pass: decoded `inform`'s sector-map icon
      dispatch (`drawSectorMapIcon`/`drawEnemyMarker`/
      `drawStationMarker`/`drawStarMarker`/`drawEmptyMarker`), named
      `isqrt`, and promoted 3 already-commented-but-unpromoted
      line-drawing functions. 189/210 functions now named (90%). Full
      writeup in
      [overview.md](overview.md#sector-map-icon-dispatch-and-other-game-specific-finds).
- [x] Third renaming pass: decoded the `status` screen (2-column
      scrolling message log, `calcExperienceLevel` formula) and the
      `view` cockpit's sector-boundary-crossing math
      (`updateSectorChangeX`/`Y`, `snapToNearestSector`). 204/210
      functions now named (97%).
- [x] Fourth renaming pass: closed out the last 6 — `spaceAce`'s
      dialog-frame helpers, `waitVerticalRetrace`, `parseParamNum`,
      `fputs`, and `exit2` (another duplicated CRT function, same
      pattern as `strncpy2`/`toupper2`/`_fread2`). **210/210 (100%)**.
      Full writeup in
      [overview.md](overview.md#spaceexe-complete--210210-100).
- [ ] **Still open**: double-check the already-existing comment on the
      "leave outer space" function (already named, not touched this
      session) that describes the chain back to `OUT.EXE` as "(DOS
      EXEC...)" — every other executable's equivalent turned out to
      use a custom overlay loader, not real `INT 21h`/`4Bh` EXEC, so
      this phrasing is probably just loose wording rather than a
      genuine architectural difference, but worth a quick read to
      confirm rather than assume.

## SPACE.EXE status: complete

**210/210 functions named (100%)**, 156 → 210 across 4 passes
(2026-08-20). All four of `ULTIMA.EXE`/`GEN.EXE`/`OUT.EXE`/`SPACE.EXE`
are now fully or near-fully named.

## Per-executable next steps (not yet started)

`ultima1_mondain` (1/191, essentially unstarted) is the only remaining
executable — pick up next.
