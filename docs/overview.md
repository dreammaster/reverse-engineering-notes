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
| `MUS.EXE` | 20,387 | Music player / jukebox. `MUSDATA.BSV`, `MUSMSG.TXT`, `MUSOBJ.BSV`. |
| `SDEFENDR.EXE` | 15,443 | "Space Defender" arcade minigame (one of the in-world arcade cabinets). `SDMAP.GLB`, `SDOBJ.GLB`, `SDMAP.GMP`. |
| `GMB1.EXE` / `GMB2.EXE` | 13,285 / 21,079 | Gambling / casino minigames. |
| `CELDRV.EXE` | 8,967 | Cel-animation driver. `CEL0.BSV`…`CEL3.BSV`. |
| `STDRV.EXE` | 24,923 | Driver (story? "ST"?). `STDRVSCR.DAT`. |
| `SAVER.EXE` | 5,903 | Savegame handler. |
| `CONFIGUR.EXE` | 10,349 | Standalone config utility — plain Microsoft C, **no LEGLIB dependency**. `DRCONFIG.DAT`. |

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
| `leglib.idb` | `LEGLIB.EXE` | 7 / 773 | 0 | Baseline 2026-08-30 via `identify.py`. 10 segments; `seg003` (53 KB) + `seg004` (18 KB) are the code. Auto-analysis found 773 funcs but named almost none. |
| `menu.idb` | `MENU.EXE` | 1 / 18 | 0 | Baseline 2026-08-30. Essentially raw: `seg000` (12 KB, the actual menu logic) is undisassembled `db`; only the `seg001`/`seg002` bootstrap got auto-analyzed. String block at `seg003:21D0h` = all on-screen text. |

(Counts via `ida_scripts/identify.py -NoExport`; re-run any time as a
sanity check.)

### Segment layout, per module

IDA's MZ loader leaves most segments tagged `'UNK'` even where they hold
code. `menu.idb`:

| Seg | Range | Contents |
|---|---|---|
| `seg000` | `10000`–`13160` | Root menu/intro code (compiled BASIC; ~890 `call far` sites, all routed through `seg001`). **Undisassembled** — needs forcing to code. |
| `seg001` | `13160`–`138F0` | `int 3Fh` overlay/RTM thunk table — one 5-byte entry per cross-module call site. IDA mis-parses parts as instructions. |
| `seg002` | `138F0`–`13F30` | BC 6.0 EXE bootstrap + RTM loader (`start` at `139CF`). "Error in loading RTM…" strings live here. |
| `seg003` | `13F30`–`1A1B0` | DGROUP data. Text block (menu items, credits, instructions, "poor peasant on the world of Tarmalon…" intro, MML music strings, chained-EXE names) at offset `21D0h`+ (file `0x6ED2`–`0x7F10`). |
| `seg004` | `1A1B0`–`1A9B0` | Stack. |

Segment names are **not** renamed to `CODE`/`DATA` — see the sibling
`ultima1` project's convention (Paul correlates segment names with the
DOSBox debugger at runtime).

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
  segments, naming progress, raw-vs-code byte coverage). Confirmed
  working end-to-end against `menu.idb` and `leglib.idb` 2026-08-30.
- **`ida_scripts/rank_unnamed_functions.py`** — read-only, ranks
  `sub_XXXXX` functions by call-site count (highest-value naming
  targets first).

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

## Findings log

Nothing yet — analysis starts here. Decided 2026-08-30 (with Paul): work
`LEGLIB.EXE` first (or alongside `menu`), since it's the shared payload
and everything downstream depends on its entry table being mapped.
