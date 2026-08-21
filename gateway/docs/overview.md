# Gateway (DOS) — Disassembly Overview

Working notes on the reverse-engineering effort for Legend Entertainment's
*Gateway* (1992), the first release on Legend's shared game engine. Goal:
fully document both DOS executables well enough to write a clean C++
reimplementation, then a ScummVM engine module — and to document the
underlying engine generally enough to ease reversing the other Legend
titles built on it (later goal, not started).

This file is the entry point into `docs/`. See also:
- [roadmap.md](roadmap.md) — prioritized list of what's investigated vs.
  still open, per executable.

**Nothing in either `.idb` should be presumed accurate.** Both databases
carry tentative, in-progress manual work from earlier sessions, done
before this pipeline existed — names, struct fields, and comments may be
wrong, half-finished, or simply guesses. Treat every existing name as a
hypothesis to re-confirm, not a fact, the same skepticism applied
throughout the sibling `ultima1` project (see that project's overview.md
for several examples of confidently-named-but-wrong functions caught by
actually reading the code).

## Two executables, two IDBs

| IDB | Root file | Functions named | Structs | Segments |
|---|---|---|---|---|
| `gate.idb` | `gate_decoded.exe` (entry `0x9b0`, cs=`0x1112`) | 177 / 502 (35%) | 21 | 32 |
| `gatemain.idb` | `gatemain_decoded.exe` (entry `0x76c`, cs=`0x2cf4`) | 807 / 3288 (25%) | 49 | 308 |

Counts captured 2026-08-21 via `ida_scripts/identify.py`. The `_decoded`
suffix on both input filenames suggests the on-disk `.exe`s were unpacked/
decompressed from Legend's original distribution format before analysis
started — worth confirming what tool produced that and whether the
original packed `.exe` is still available, since the packed form may
carry useful metadata (e.g. overlay structure) the decoded form lacks.

Paul's working understanding, to confirm as we go: `gate.idb` covers the
title screen and cutscenes; `gatemain.idb` covers the actual in-game
content. `gatemain.idb`'s much larger size (3288 functions, 308 segments,
almost 6.5x `gate`'s function count) and its struct list — `Room`,
`LogicSection2`..`LogicSection8`, `LogicIndexEntry`, `VocabEntry`,
`Parser_Data1`, `ParserHandlerEntry`, `Picture`/`PictureDecoder`,
`Thing`, `StateVocab` — are consistent with that: this looks like a
text-parser adventure engine in the vein of Sierra's AGI/SCI (rooms,
logic scripts, a vocabulary/parser layer, picture resources), rather than
a simple graphical overworld like `ultima1`. `gate.idb`'s struct list is
a strict subset in spirit (`REGS`, `VIDEO_MODE`, `FONT`, `SCREEN`,
`PIC_HEADER`/`PIC_DATA2`/`PIC_DATA`, `MESSAGE`, `VOCAB_FILE_REC`,
`VOCAB_ENTRY`) — shared engine plumbing, no `Room`/`LogicSection`/`Thing`/
parser structs, consistent with it being the smaller title/cutscene
executable and not the full game-logic runtime. Names differ in case
convention between the two IDBs (`VOCAB_ENTRY` vs. `VocabEntry`,
`STR16` vs. `Str16`) even for what look like the same concept — expect
these to need reconciling once both are actually cross-checked field by
field, same open item `ultima1` has for its own shared structs.

Both are 16-bit real-mode MS-DOS `MZ` executables. Segment naming is
still mostly IDA's auto-generated `seg###`/`sg####` convention in both
IDBs (only a handful of segments carry a real name like `dseg`) —
renaming to a `CODE`/`DATA` convention, once each segment's role is
confirmed, is a later cleanup pass, not blocking.

## Headless IDA pipeline

Set up 2026-08-21, ported directly from the `ultima1` project's
`ida_scripts/` (IDA Pro 8.3, `idat.exe`, no GUI required — the GUI is
incompatible with this flow since it locks the `.idb`). Same shape as
`ultima1`: the driver derives the target `.idb`'s (and matching
`.asm`/`.idc` export) paths from whatever `idat.exe` was actually pointed
at, so one driver script serves both of Gateway's IDBs without
hardcoding a filename (same reason `ultima1` needed this over `ultima2`'s
single-IDB hardcoded approach, and the same reason it wasn't rewritten
`ultima1`-specific here — copied verbatim).

- **`ida_scripts/run_ida_script.ps1`** — entry point.
  ```powershell
  .\run_ida_script.ps1 -Idb gatemain -ScriptName identify.py -NoExport
  .\run_ida_script.ps1 -Idb gate -ScriptName apply_renames_gate.py
  ```
  `-Idb` takes a bare stem (`gate` or `gatemain`, resolved against the
  two known `.idb`s) or an explicit path. Refuses to run if the target
  `.idb` is already open elsewhere (e.g. the IDA GUI) rather than racing
  it. `-NoExport` skips the `.asm`/`.idc` export and `save_database`
  step, for read-only discovery/report scripts.
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
- **`ida_scripts/rank_unnamed_functions.py`** — read-only report,
  ranks still-unnamed `sub_XXXXX` functions by call-site count. Not yet
  run against either Gateway IDB (large `gatemain` output — 2481
  unnamed — worth piping through something other than raw stdout when
  it's actually used).

Confirmed working end-to-end 2026-08-21 against both `gate.idb` and
`gatemain.idb` in report mode (`identify.py -NoExport`). Full export+save
round-trip not yet exercised against either Gateway IDB (only done
against `ultima1_space.idb` when this pipeline was first built) — worth
a smoke-test export+save before the first real rename pass, same as
`ultima1` did.

## Conventions (carried over from `ultima1`/`ultima2`)

- **Renames accumulate per-executable**, not in one shared file:
  `apply_renames_<stem>.py` (e.g. `apply_renames_gatemain.py`), each a
  growing, git-diffable list of `(ea, new_name, note)` tuples rather
  than a new one-off script per finding. Created on demand as work
  starts on each executable — neither exists yet.
- **Struct edits** go in `apply_structs_<stem>.py` (separate from
  renames — different IDA API, addresses a struct definition rather
  than a linear address).
- **`DRY_RUN` defaults to `True`** for both, until/unless it's clear a
  given executable's pass is stable enough to flip it off.
- **One-off structural scripts** (splitting an array, building a jump
  table, decoding a resource format) get their own dedicated script.
- Findings get written up here (or in a future `file-formats.md` for
  on-disk resource formats — rooms/logic/pictures/vocab, given the
  AGI/SCI-like structure hypothesized above) with enough detail that the
  `note` field in a rename entry can stay short and just point back to
  the section.

## Open questions before the first real analysis pass

- Confirm (rather than assume) `gate.idb`'s role as title/cutscenes and
  `gatemain.idb`'s as the main game — Paul's own framing going in, not
  yet independently verified by reading `_main`'s top-level flow the way
  `ultima1`'s `ULTIMA.EXE` pass did.
- Figure out whether the two executables chain into each other the way
  all five `ultima1` executables did (custom overlay loader, not DOS
  `INT 21h`/`4Bh` EXEC) — worth checking early since it shapes how the
  ScummVM engine module should model switching between them, same
  lesson learned from `ultima1`'s `writeInUseAndExit` finding.
- Locate and read any pre-existing analysis comments left by the earlier
  manual sessions before renaming over them — `ultima1`'s `SPACE.EXE`
  pass found several unpromoted-but-correct comments worth promoting to
  real names rather than re-deriving from scratch.
- No `docs/file-formats.md` yet — worth starting once the room/logic/
  picture/vocab resource formats implied by `gatemain.idb`'s struct list
  are actually traced.
