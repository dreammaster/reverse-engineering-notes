# Ultima I (DOS) — Disassembly Overview

Working notes on the Ultima I reverse-engineering effort. Goal: fully
document each DOS executable well enough to write a clean C++
reimplementation, then a ScummVM engine module — matching the approach
used in the sibling [`ultima2`](../../ultima2) project.

This file is the entry point into `docs/`. See also:
- [roadmap.md](roadmap.md) — prioritized list of what's investigated vs.
  still open, per executable.
- [file-formats.md](file-formats.md) — on-disk data formats (maps, saves,
  dungeons, etc.), once documented. Not started yet.

## Five executables, five IDBs

Unlike Ultima II (one `.exe`, one `.idb`), Ultima I ships as five
separate DOS executables that overlay/chain into each other at runtime,
each with its own segments, code, and (mostly) independent symbol space.
Each gets its own `.idb` — there is no cross-IDB symbol sharing in IDA,
so a helper renamed in one executable's IDB has to be independently
identified and renamed in any other IDB it also happens to appear in.

| IDB | Root file | Role (best guess, to confirm) | Functions named | Structs |
|---|---|---|---|---|
| `ultima1.idb` | `ULTIMA.EXE` | Title/launcher — likely chains to the others | 39 / 100 | `STR15`, `Point`, `Rect`, `Savegame` |
| `ultima1_gen.idb` | `GEN.EXE` | Character generation | 44 / 113 | + `Creature` |
| `ultima1_out.idb` | `OUT.EXE` | Overworld/towns/dungeons (outdoor engine) | 266 / 353 | + `DungeonCell`, `DungeonColumn`, `DungeonMap`, `LocationWidget`, `MapLine`, `Map` |
| `ultima1_space.idb` | `SPACE.EXE` | Space combat minigame | 156 / 210 | + `DuneonRow`, `DungeonMap` (space variant), `SpaceMapShip`, `SpaceMapCell`, `SpaceMapY`, `SpaceMap`, `FightData`, `JumpEntry` |
| `ultima1_mondain.idb` | `MONDAIN.EXE` | Unknown — essentially unstarted | 1 / 191 | none |

Counts captured 2026-08-19 via `ida_scripts/identify.py` (see below) —
`ultima1_out` and `ultima1_space` have substantial prior work from
earlier sessions; `ultima1.idb` and `ultima1_gen.idb` have moderate
partial work; `ultima1_mondain.idb` is effectively virgin territory.

All five are 16-bit real-mode MS-DOS `MZ` executables, consistent with
the 1986/1987 DOS port. Each IDB's `dseg`/`sg*` segments haven't been
renamed to the `CODE`/`DATA` convention used in `ultima2` yet — worth
doing per-executable as each one is worked on.

Common structs (`STR15`, `Point`, `Rect`, `Savegame`, `Creature`) already
exist independently in multiple IDBs with the same names — check they
actually agree field-for-field before assuming they're interchangeable;
they were defined in separate sessions against separate databases.

## Headless IDA pipeline

Set up 2026-08-19, mirroring `ultima2/ida_scripts`'s approach (IDA Pro
8.3, `idat.exe`, no GUI required — the GUI is incompatible with this flow
since it locks the `.idb`). Key difference from `ultima2`: that project
hardcodes a single `.idb` path in its driver; this one has five, so the
driver derives the target `.idb` (and its matching `.asm`/`.idc` export
paths) from whatever `idat.exe` was actually pointed at.

- **`ida_scripts/run_ida_script.ps1`** — entry point.
  ```powershell
  .\run_ida_script.ps1 -Idb ultima1_out -ScriptName apply_renames_out.py
  .\run_ida_script.ps1 -Idb ultima1_space -ScriptName identify.py -NoExport
  ```
  `-Idb` takes a bare stem (resolved against the five known `.idb`s) or
  an explicit path. Refuses to run if the target `.idb` is already open
  elsewhere (e.g. the IDA GUI) rather than racing it. `-NoExport` skips
  the `.asm`/`.idc` export and `save_database` step, for read-only
  discovery/report scripts.
- **`ida_scripts/batch_run_and_export.py`** — the actual driver, invoked
  via `idat.exe -A -S"batch_run_and_export.py <target.py> [noexport]"`.
  Execs the target script's code (so it runs exactly as it would under
  Alt+F7 in the GUI), then exports `<idb-stem>.asm`/`.idc` next to the
  `.idb` and saves. Every step — including the target script's own
  captured stdout and any exception traceback — goes to
  `ida_scripts/batch_run_and_export.log`, since `idat.exe`'s own console
  output in `-A` mode is not reliably flushed before exit.
- **`ida_scripts/identify.py`** — read-only report script (root
  filename, input path/hash, segments, function-naming progress, struct
  list). Used to produce the table above; safe to re-run any time as a
  sanity check with `-NoExport`.

Confirmed working end-to-end 2026-08-19 against all five IDBs (report
mode) and against `ultima1_space.idb` (full export+save round-trip,
re-running `identify.py` without `-NoExport`).

## Conventions (carried over from `ultima2`, adapted for 5 IDBs)

- **Renames accumulate per-executable**, not in one shared file:
  `apply_renames_<stem>.py` (e.g. `apply_renames_out.py`), each a
  growing, git-diffable list of `(ea, new_name, note)` tuples rather
  than a new one-off script per finding. Created on demand as work
  starts on each executable.
- **Struct edits** go in `apply_structs_<stem>.py` (separate from
  renames — different IDA API, addresses a struct definition rather
  than a linear address).
- **`DRY_RUN` defaults to `True`** for both, until/unless it's clear a
  given executable's pass is stable enough to flip it off (`ultima2`
  eventually flipped `apply_renames.py` to `DRY_RUN = False` by
  deliberate choice once the pattern proved reliable there — no
  equivalent decision made for `ultima1` yet).
- **One-off structural scripts** (splitting an array, building a jump
  table, fixing an inline-data-after-CALL trick — see `ultima2` for
  examples of this DOS-era pattern) get their own dedicated script,
  same as in `ultima2`.
- Findings get written up here (or in `file-formats.md` for on-disk
  formats) with enough detail that the `note` field in a rename entry
  can stay short and just point back to the section.
