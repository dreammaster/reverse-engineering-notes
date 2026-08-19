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
- [ ] Decode `playSound`'s 10-entry effect jump table (`off_1F94A`,
      handlers `0x1AE65`-`0x1AF37`) by cross-referencing `playFX` call
      sites' literal `effectNum` arguments against game context — see
      overview.md.
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

## Per-executable next steps (not yet started)

`ultima1_space`, `ultima1_gen`, `ultima1`, `ultima1_mondain` — untouched
this session beyond the initial `identify.py` cataloging in
overview.md. Pick up after `ultima1_out` is fully documented, per the
priority order above.
