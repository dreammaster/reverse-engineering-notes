# Roadmap

Working list of next steps, roughly in priority order. Check items off /
add notes as they're resolved — this file is meant to stay current
across sessions, unlike a one-off todo list.

## Decisions made

- **2026-09-13 (Paul's call): function naming is camelCase**, matching
  `ultima1` rather than `ultima2` (snake_case), despite this project's
  single-executable/single-IDB shape otherwise being closer to
  `ultima2`'s project template. See
  [overview.md](overview.md#naming-convention).
- **2026-09-13 (Paul's call): `source/` (the LairWare Mac remake) is
  untracked** from this git repo — kept on disk locally as a reference,
  `.gitignore`d, matching `ultima1`'s `ultima_old/` precedent for large
  reference-only trees that aren't part of the RE-notes history.
- **2026-09-13: `apply_renames.py`/`apply_structs.py` both start with
  `DRY_RUN = True`** (unlike `ultima2`'s eventual `DRY_RUN = False`
  convention for `apply_renames.py`) — no findings applied yet, so
  there's no track record to justify skipping the dry-run safety net.
  Revisit once a first batch of renames has been verified end-to-end.

## Immediate (scaffolding / IDB hygiene)

- [x] Set up `docs/`, `ida_scripts/`, `tools/` folders matching the
      `ultima1`/`ultima2` layout (2026-09-13).
- [x] Ported `run_ida_script.ps1`/`batch_run_and_export.py`/
      `identify.py` from `ultima2` (single-IDB template), confirmed
      working end-to-end via a real `identify.py -NoExport` run
      (2026-09-13): 57 functions, 3 named, 0 structs.
- [ ] Map `EXODUS.BIN`/`BOOTUP.BIN` — are they separate DOS executables
      chained/loaded the way Ultima I's overlays are, or raw data
      blobs loaded into a fixed buffer? `aBootupBin` is referenced as a
      string literal early in `start_0`'s auto-analysis, suggesting a
      file-open call worth tracing directly. Check both files' own
      headers (do they start with `MZ`?) before assuming either shape.
- [ ] Confirm `BLANK.IBM`/`EXOD.IBM`'s format — likely full-screen
      CGA/EGA image data (16,384 bytes matches a plausible full-screen
      bitmap size), analogous to Ultima II's `PIC*` files. Trace the
      loader that reads `aBlankIbm`/`aExodIbm`.
- [ ] Full `sub_XXXXX` sweep: 54 unnamed functions in a 36,692-byte COM
      file is a tractable amount to work through systematically —
      prioritize highly-called-into helpers first (call-site count),
      same triage approach `ultima2` used for its A-Z command-handler
      helpers.

## High value / next session

- [ ] Identify the top-level command dispatch loop (Ultima III's
      classic single-key command set — likely similar A-Z letter
      commands to Ultima I/II, per the game manual `ULTIMA3.TXT`).
      Cross-reference against LairWare's `UltimaMain.c` main loop for
      the expected command set/shape before assuming Ultima II's exact
      26-command table transfers over unchanged.
- [ ] Trace the overworld/town/dungeon map file loader — the `.ULT`
      files in `C:\games\ultima3` are a strong, concrete target
      (`SOSARIA.ULT` etc. at 4,648 bytes, dungeon/interior `.ULT` files
      at 2,192 bytes, `CNFLCT_*.ULT` combat arenas at 176 bytes) with
      obvious fixed-size-record structure to reverse from both ends
      (file layout + the code that reads them). See
      [file-formats.md](file-formats.md) for size tables to start from.
- [ ] Trace `ROSTER.ULT`/`PARTY.ULT` (save data, 1,280/274 bytes) —
      character record layout, party state. Compare against LairWare's
      `Player[21][65]`/`Party[64]` shapes mentioned in its source
      (Apple II original sizes — expect the DOS port's byte layout to
      differ, but the *field list* is a strong hint of what to look
      for).
- [ ] `SHAPES.ULT` (5,120 bytes) / `CHARSET.ULT` (2,048 bytes) — tile
      graphics and font. Cross-reference against Ultima II's
      already-solved `TILE_OFFSETS`-table + CGA-2bpp tile approach as a
      starting methodology, but verify independently — don't assume
      the exact byte layout transfers.
- [ ] `DUNGEON.DAT` (1,866 bytes) / `ANIMATE.DAT` (5,888 bytes) /
      `NAME.DAT` (640 bytes) / `MOVES.ULT` (1,024 bytes) — smaller,
      less obviously-structured data files, lower priority until the
      bigger map/save formats are settled.
- [ ] Define the core `Savegame`/player-state struct once enough
      fields are traced (mirrors `ultima1`/`ultima2`'s `Savegame`
      struct) — don't force this early; wait for real evidence per
      field like the sibling projects did.

## Open questions

- Does `ULTIMA.COM` chain to a second executable at any point (like
  Ultima I's multi-EXE overlay chain), or is everything — including
  Exodus/endgame content — handled within this one `.COM` plus its
  `EXODUS.BIN`/`BOOTUP.BIN` companion blobs? Not yet determined.
- Exact relationship between this DOS port (files dated 1991-10-01 for
  the 3 largest binaries, but most `.ULT`/`.DAT` data files dated
  1983-1987) and the original 1983 Apple II release — is this a
  straight port of an earlier DOS release, or the "gold" 1991
  re-release? Worth checking `ULTIMA3.TXT`'s copyright text and any
  version strings found during disassembly.
