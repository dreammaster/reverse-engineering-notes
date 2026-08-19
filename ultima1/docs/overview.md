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

## OUT.EXE — findings log

Started 2026-08-19. Was 266/353 functions named before this pass;
ranked the remaining 87 `sub_XXXXX` by call-site count
(`ida_scripts/rank_unnamed_functions.py`) and worked top-down. First
pass renamed 7 (`ida_scripts/apply_renames_out.py`, applied
`DRY_RUN = False`), full detail below.

### `getKeypressAndWaitRaw` and `getKeypressAndWait` — duplicated poll loop

`getKeypressAndWait` (`0x1B94E`-ish, already named pre-session) and the
newly-named `getKeypressAndWaitRaw` (`0x1A81D`) implement the *same*
poll loop independently — not one wrapping the other:

```
loop:
    key = getKeypress(textColor)
    if key != 0: wait(1); break
    wait(3)
goto loop (implicit via the branch structure)
return key
```

`getKeypressAndWait` inserts one extra step (`_toupper` on the result)
that `getKeypressAndWaitRaw` doesn't. All 15 call sites of
`getKeypressAndWaitRaw` push the global `_textColor` — never a literal
or a different variable — even though the function takes `textColor` as
a real parameter, so in practice it's always "wait for a key in the
current text color." Some callers `_toupper` the raw result themselves
right after (matching what `getKeypressAndWait` does inline); others use
the raw case (e.g. before subtracting `'a'` for a menu index, where case
matters). This is genuine code duplication in the original binary, not
a naming artifact — both bodies exist independently in the disassembly.

### Near-heap allocator: `_nmalloc` / `_nfree` / `_nheapgrow`

A classic Microsoft C runtime near-heap allocator, not game logic.
Named with a leading underscore to match this codebase's existing
convention for runtime-library internals (`_fopen`, `_toupper`).

- **`_nmalloc`** (`0x1981D`) — walks a singly-linked free-list
  (`word_1EC56` = sentinel head, `word_1EC5A` = cursor/tail), splits a
  free block if it's big enough for the request, otherwise calls
  `_nheapgrow` to extend the heap and links the new space in via
  `_nfree` before retrying the walk.
- **`_nfree`** (`0x198C0`) — walks the same free-list, coalesces the
  freed block with whichever neighbor(s) are adjacent in memory.
- **`_nheapgrow`** (`0x19EA3`) — grows the near heap via DOS `INT 21h`/
  `AH=4Ah` (SETBLOCK, "adjust memory block size"). Only called from
  `_nmalloc`. Its disassembly has two extra `push bp; mov bp,sp; ...`
  prologues immediately after the first `retn`, with **zero incoming
  xrefs** — dead code, almost certainly sibling entry points (e.g. a
  realloc/msize variant) from the same linked library object that this
  program never calls. Left un-split since they're inert; worth
  revisiting only if some future finding turns out to call into the
  middle of this proc.

Not pursuing the individual free-list globals (`word_1EC52`,
`word_1EC54`, `word_1EC56`, `word_1EC58`, `word_1EC5A`, `word_1EC5C`)
further for now — CRT allocator internals, low value for the C++
reimplementation (which will just use normal `new`/`delete`), unlike
the game-specific globals elsewhere in this IDB.

### `readAmount` / `isDigit` — numeric text entry

`readAmount` (`0x1B0B5`, callers: `dropPence`, `transactCastle`,
`transactGrocer`) reads up to 4 digits at a screen position: filters
keypresses through `isDigit` (`0x1B094`, simple `'0'`-`'9'` range
check) via `getKeypressAndWaitRaw`, handles backspace (char `8`) to
delete the last digit, and on any non-digit/non-backspace key converts
the accumulated buffer to a number using `word_1F95E` and returns it.

**IDB hygiene note**: `word_1F95E` is a `{1, 10, 100, 1000}`
powers-of-ten table (confirmed by the multiply-and-sum loop that reads
it), but it's currently mis-typed in the IDB as a single `dw 1` word
immediately followed by raw `db` bytes rather than a proper 4-element
array — IDA's auto-analysis didn't recognize the boundary. Not fixed
yet; see roadmap.md.

### OUT.EXE CRT file-I/O layer, decoded

Second pass, same session. `readFile`/`writeFile`/`_fopen` were already
named going in; traced the full layer underneath them by reading each
function's body directly (not just caller/callee shape), confirming as
I went via the exact `INT 21h` subfunction each one issues. The bottom
layer matches Microsoft C's documented `<dos.h>` primitives closely
enough (including which `AH=` subfunction each one uses) that I'm
confident these are the right names even without symbol-table
confirmation:

```
_fopen (was already named)
  -> _openfile (0x1C791)         parses the "rb"/"wb"/"a+" mode string
       -> _open (0x1CA14)         composite: O_CREAT/O_TRUNC/O_EXCL flag
                                   dispatch (flag bit values match MSC's
                                   fcntl.h exactly: 0x100/0x200/0x400)
            -> _dos_open (0x19578)        AH=3Dh
            -> _dos_creat (0x1D37C)       AH=3Ch
            -> _dos_creatnew (0x1D38D)    AH=5Bh (O_CREAT|O_EXCL)
            -> _dos_creattemp (0x1D39A)   AH=5Ah

readFile -> _fread (0x1C8F2)  -\
writeFile -> _fwrite (0x1C960) -+-> _filbuf (0x1CC6B) / _flsbuf (0x1CE05)
                                        -> allocFileBuffer (0x1D30F)
                                        -> _dos_read/_dos_write/_dos_lseek

_fclose (0x1C6D4)
  -> _flsbuf (flush if dirty), _nfree (release buffer)
  -> releaseFileHandle (0x1C9CC)
       -> findFileHandleSlot (0x1D0DD)   internal handle-table lookup
       -> _dos_close (0x19589)           AH=3Eh

_flushall (0x190C0)   iterates the 20-slot static FILE table (base
                       0B124h, 14 bytes/slot -- almost certainly the
                       classic CRT _iob[] array, though the address
                       itself isn't renamed since IDA only sees it as
                       an arithmetic literal, not a labeled operand);
                       flushes any stream with unwritten data via
                       _dos_lseek + _dos_write. Called from
                       readSavegame's invalid-save-slot fallback and
                       from start2 -- i.e. this program calls CRT
                       _flushall directly at specific points, not just
                       implicitly at exit.
```

Also renamed 3 globals in this cluster, identified by the exact
numeric values stored into them:
- **`_doserrno`** (`word_1D4F1`) — raw DOS error code, set after every
  DOS-touching CRT call; `readFile`/`writeFile` poll it to decide
  whether to prompt `insertDisk` and retry (this is the "please
  reinsert the disk" floppy-era UX).
- **`errno`** (`word_1ED22`) — translated POSIX-style code. Confirmed
  by the values themselves: `9`=EBADF (handle not found),
  `0x11`=EEXIST / `0x16`=EINVAL (in `_open`'s create-flag branches),
  `0x18`=EMFILE (`_openfile`'s FILE table full), `0x1C`=ENOSPC (short
  `_dos_write`) — all exactly right per MSC's `errno.h`.
- **`_fmode`** (`word_1EC50`) — default text/binary mode, read by
  `_openfile` before the mode string's `'b'`on overrides it.

**Not pursued further**: `_dos_ioctl_get`/`_dos_ioctl_set` (the
`isatty()`-style device-info calls) and `allocFileBuffer` don't map to
a single confidently-known MSC symbol, so they got descriptive names
instead of guessed CRT names. `word_1EC60`/the `17E2h`-based handle
table itself isn't struct-ified yet — would need `apply_structs_out.py`
to do properly (a `FileHandleSlot { inUse; dosHandle }`-shaped array),
left for a future pass since it's CRT plumbing, not game logic.

### `playSound` / `playFX` — sound-effect jump table, not yet decoded

`playSound(effectNum)` (already named) dispatches through a 10-entry
jump table (`off_1F94A`, `effectNum` 0-9) to 10 short, single-caller
`sub_XXXXX` handlers (`0x1AE65` through `0x1AF37`) — almost certainly
individual PC-speaker effect routines (footstep, hit, death, etc.).
`playFX(effectNum)` is a thin `_savegame._soundOn`-gated wrapper around
it, called from ~40 sites across combat/movement/UI code. Left
unnamed: figuring out which of the 10 is which requires cross-
referencing every `playFX` call site's literal `effectNum` argument
against its game context (or just listening in an emulator), which
hasn't been done yet. Worth a dedicated pass — see roadmap.md.
