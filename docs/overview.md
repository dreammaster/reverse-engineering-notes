# Ultima III (DOS) — Disassembly Overview

Working notes on the `ultima.idb` / `ultima.asm` reverse-engineering
effort. Goal: fully document the DOS executable well enough to write a
clean C++ reimplementation, then a ScummVM engine module — matching the
approach used in the sibling [`ultima1`](../../ultima1) and
[`ultima2`](../../ultima2) projects.

This file is the entry point into `docs/`. See also:
- [roadmap.md](roadmap.md) — prioritized list of what's investigated vs.
  still open.
- [file-formats.md](file-formats.md) — on-disk data formats (maps,
  saves, dungeons, combat arenas, etc.), cross-referenced against the
  disassembly and the original data files in `C:\games\ultima3`.

Status as of this pass (2026-09-13, via `ida_scripts/identify.py`): 57
functions total, 3 named, 54 still placeholder `sub_XXXXX`. No structs
defined yet. `ultima.asm` is ~113KB.

## Platform and executable shape

- **Single `.COM` file**: `ULTIMA.COM` (36,692 bytes on disk), MS-DOS
  tiny model, one segment (`seg000`, class `CODE`, `use16`), base
  address `1000h`, code/data range `10100h`-`19054h` (loaded length
  `8F54h`). Entry point `100h` relative to `cs=1000h`.
- Unlike Ultima I (5 chained executables, 5 IDBs) this is a **single
  IDB, single executable** — same shape as Ultima II, so `ultima2`'s
  simpler `ida_scripts/` template (hardcoded single `.idb` path) is
  what `ultima3`'s scripts are adapted from, not Ultima I's
  generalized 5-IDB driver.
- The game ships with substantial **overlay/companion binaries and
  data files** alongside `ULTIMA.COM` (see `C:\games\ultima3`):
  `EXODUS.BIN` (44,234 bytes) and `BOOTUP.BIN` (19,572 bytes) are
  loaded by name from inside `ULTIMA.COM` (`aBootupBin` at
  `10x115`-ish already shows up as a referenced string literal in the
  initial IDA auto-analysis — see below) — likely overlay code for the
  boot sequence and the Exodus/endgame content respectively, analogous
  to Ultima I's `chainToExecutable` overlay pattern but via a
  file-loaded blob rather than a second full `.EXE`. Not yet
  disassembled/mapped into the IDB — **open question**, see
  [roadmap.md](roadmap.md).
- `BLANK.IBM`/`EXOD.IBM` (16,384 bytes each, fixed-size) are also
  referenced by string literal in the initial auto-analysis
  (`aBlankIbm`, `aExodIbm`) — full-screen image/bitmap data, likely for
  title/intro and the Exodus encounter, akin to Ultima II's `PIC*`
  full-screen CGA art files (see that project's file-formats.md for the
  precedent).

## Naming convention

**Decided 2026-09-13 (Paul's call): camelCase** for function names,
matching Ultima I rather than Ultima II (snake_case) — despite this
project's single-IDB shape otherwise being closer to Ultima II's
project template. Everything else follows the shared cross-project
convention (see the sibling projects' docs for the full rationale):

- **Globals/statics**: leading underscore + camelCase (`_savegame`,
  `_playerX`). CRT-internal runtime-library functions also get a
  leading underscore regardless of casing style (`_fopen`, `_toupper`).
- **Structs**: PascalCase (`Savegame`, `Creature`, `Point`).
- **Struct fields**: underscore-prefixed camelCase (`_hits`,
  `_strength`).
- **Constant/lookup tables**: ALL_CAPS (`SPELL_NAMES`, `TILE_OFFSETS`).
- **Confidence discipline**: names only applied once mechanically *and*
  semantically confirmed, never guessed from shape alone. Low-confidence
  names get called out explicitly in the rename script's `note` field
  and here. Dead-but-real functions get named anyway (not left
  `sub_XXXXX`), noted as confirmed unreachable.
- **Duplicate CRT functions** from static linking get a numeric suffix
  (`strncpy2`, `toupper2`) rather than being merged.

## Headless IDA pipeline

Set up 2026-09-13, ported directly from `ultima2/ida_scripts` (single
IDB, same simpler shape). IDA Pro 8.3, `idat.exe`, no GUI required (the
GUI is incompatible with this flow since it locks the `.idb`).

- **`ida_scripts/run_ida_script.ps1`** — entry point.
  ```powershell
  .\run_ida_script.ps1 apply_renames.py
  .\run_ida_script.ps1 apply_structs.py
  .\run_ida_script.ps1 identify.py -NoExport
  ```
  Refuses to run if `ultima.idb` is already open elsewhere (e.g. the
  IDA GUI) rather than racing it — this bit the ultima2 project once
  (a silently-dropped last entry in an `apply_structs.py` run).
- **`ida_scripts/batch_run_and_export.py`** — the actual driver,
  invoked via `idat.exe -A -S"batch_run_and_export.py <target.py>
  [noexport]"`. Execs the target script's code (so it runs exactly as
  it would under Alt+F7 in the GUI), then exports `ultima.asm`
  (`OFILE_ASM`) and `ultima.idc` (`OFILE_IDC`) and saves, unless
  `noexport` was passed. Every step — including the target script's own
  captured stdout and any exception traceback — goes to
  `ida_scripts/batch_run_and_export.log`, since `idat.exe`'s own
  console output in `-A` mode is not reliably flushed before exit.
- **`ida_scripts/identify.py`** — read-only report script (root
  filename, input path/hash, segments, function-naming progress,
  struct list). Used to produce the status line above; safe to re-run
  any time with `-NoExport` as a sanity check.
- **`ida_scripts/apply_renames.py`** / **`apply_structs.py`** —
  accumulating, idempotent, re-runnable scripts for plain renames and
  struct-member edits respectively (separate files since they use
  different IDA APIs). Both currently empty (`DRY_RUN = True`) — no
  findings applied yet as of this scaffolding pass.

Confirmed working end-to-end 2026-09-13 (`identify.py`, report mode,
against the real `ultima.idb`).

## Reference materials

- **`C:\games\ultima3\`** — the real game install: `ULTIMA.COM` (the
  disassembly target), `EXODUS.BIN`/`BOOTUP.BIN` (overlay binaries, not
  yet mapped), all overworld/town `.ULT` map files, dungeon/interior
  `.ULT` files, 9 combat-arena `CNFLCT_*.ULT` files, `SHAPES.ULT`/
  `CHARSET.ULT` (tile/font graphics), `ROSTER.ULT`/`PARTY.ULT` (save
  data), and `ULTIMA3.TXT` (the game manual/story text, useful for
  in-game string cross-referencing). Full file listing and sizes in
  [file-formats.md](file-formats.md).
- **`source/`** (local-only, gitignored — not part of this repo's
  history, same treatment as ultima1's `ultima_old/`) — LairWare's
  Ultima III, a MIT-licensed Mac fan remake (Garriott-authorized). Not
  a disassembly port — its own engine/resources/graphics — but its C
  source embeds the **original Apple II 6502** memory addresses and
  zero-page variable names as inline comments throughout
  `UltimaDngn.c`, `UltimaSpellCombat.c`, `UltimaGraphics.c`,
  `UltimaMain.c` (e.g. `/* $8CE9 */`, `zp[0x1F]`). Useful as an
  algorithm/logic cross-reference (combat resolution, dungeon special
  tiles, spell effects, 64-byte roster record shape) even though the
  addresses themselves won't line up with this DOS x86 binary — the
  original release this port is based on is Apple II, and `ULTIMA.COM`
  is a later (1991, per file dates) DOS port.

## Findings log

Nothing identified yet beyond IDA's own initial auto-analysis (3 named
items: `start`/`start_0` and a handful of string literals). This
section will grow one entry per session, evidence-first (byte offsets,
`INT 21h` subfunction numbers, call-site counts, literal string values)
— see the sibling projects' `overview.md` for the expected level of
detail and citation style.
