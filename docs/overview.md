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
| `ultima1.idb` | `ULTIMA.EXE` | Title screen / attract mode, chains unconditionally to `GEN.EXE` | 100 / 100 | `STR15`, `Point`, `Rect`, `Savegame`, `Creature` |
| `ultima1_gen.idb` | `GEN.EXE` | Character generation / continue, chains to `OUT.EXE` | 113 / 113 | + `Creature` |
| `ultima1_out.idb` | `OUT.EXE` | Overworld/towns/dungeons (outdoor engine) | 352 / 353 | + `DungeonCell`, `DungeonColumn`, `DungeonMap`, `LocationWidget`, `MapLine`, `Map` |
| `ultima1_space.idb` | `SPACE.EXE` | Space combat minigame, chains back to `OUT.EXE` | 210 / 210 | + `DuneonRow`, `DungeonMap` (space variant), `SpaceMapShip`, `SpaceMapCell`, `SpaceMapY`, `SpaceMap`, `FightData`, `JumpEntry` |
| `ultima1_mondain.idb` | `MONDAIN.EXE` | The "confront Mondain" special encounter, chained from `OUT.EXE`'s `board` at the "Mondain's Gate to Hell" location, chains back to `OUT.EXE` on exit | 191 / 191 | none yet |

Counts captured 2026-08-20 via `ida_scripts/identify.py` (see below) —
`ultima1_out` and `ultima1_space` have substantial prior work from
earlier sessions; `ultima1.idb` and `ultima1_gen.idb` have moderate
partial work; `ultima1_mondain.idb` went from 1/191 (a BinDiff pass
against `ultima1_out` transferred 133 more names before this session's
work started) to fully named in one pass — see below.

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

### OUT.EXE shop-transaction cluster, decoded

Third pass, same session. Traced the buy/sell helper functions behind
`transactWeapons`/`transactArmory`/`transactMagic`/`transactTransport`
(all four already named going in). Each shop follows the same shape —
a `calc*BuyPrice(itemIndex)` formula, a `draw*ShopLine(dialogResult,
itemIndex)` line renderer for the buy list, and (except magic) a
`sell*` flow with its own `calc*SellPrice` formula:

| Shop | Buy price formula | Sell price formula |
|---|---|---|
| Weapons | `(255-INT) * index² / 256 + 5` | `(CHA+40) * index² / 256 + 1` |
| Armor | `(200-INT) / 4 * index` | `CHA / 4 * index` |
| Magic | `(200-WIS) / 32 * index` | *(not sellable)* |
| Transport | via the already-named `getExpense` | *(not sellable)* |

Notable: weapons price **quadratically** in item index (higher-tier
weapons cost disproportionately more), while armor and magic price
**linearly** — a genuine mechanic difference, not an IDB-reading
artifact (confirmed by reading each formula's body directly). Magic
has no sell path at all — `transactMagic`'s `'S'` branch prints
`"Sorry, we don't buy spells!"` instead of calling a sell helper,
which is why there's no `sellMagic` in this cluster.
`drawTransportShopLine` breaks the pattern by reusing `getExpense`
rather than having its own `calcTransportBuyPrice` — transport pricing
was evidently already handled by earlier work in this IDB, before this
session's pass.

Also named `divmod32` (`0x192D4`) — a full signed 32-bit
division/remainder routine (quotient in `ax:bx`, remainder in `cx:dx`,
classic 8086 shift-subtract long division), almost certainly a
compiler-emitted arithmetic helper rather than game logic. Named for
its confirmed mechanical operation only, **not** a guessed toolchain
symbol. Its actual role in `transactWeapons` — chained calls
`divmod32(_moveCtr, 0x7FFF)` then `divmod32(<that remainder>, 1500)`
feeding into the weapon-tier availability check — looks like a
playtime-gated unlock calculation (echoing the simpler
`compareTo(_moveCtr, 3000)` gate already seen in `drawArmorShopLine`'s
caller), but the exact formula isn't pieced together yet. Follow-up in
roadmap.md.

### OUT.EXE `get` command — castle item theft, decoded

Fourth pass, same session. The `get` command's town/castle handler
(`getTownItem`, `0x154FB`) reads the `_townCityMap` tile under the
player and dispatches to `getFoodTile`/`getArmorTile`/`getWeaponTile`
by tile code (`'9'`/`'7'`/`';'`) — these tile codes appear to be
castle-room dressing specifically (tables, armor stands, weapon
racks), matching the "king's permission" framing, not ordinary town
tiles.

Two limits gate a successful `get`:
- **`_castleItemAllowance`** (`word_26024`) — set to 9 on
  `enterCastle`, 0 elsewhere; decremented per successful grab. Hits 0
  → `denyGetNoPermission` ("Thou hast not the king's permission!").
- **`checkCaughtStealing`** (`0x15B8A`, shared by the already-named
  `findArmor`/`findWeapon`/`findFood`) — rolls 1-255; catches the
  player if the roll is low (`<0x26`) or guards are already hostile,
  **except** Thief-class characters (`_savegame._class==3`), who
  always succeed silently. A catch prints "Oh no! Thou wert caught!"
  and sets `_guardsHostile` — turning every guard in the castle
  hostile from that point on.

### `showPillarInscription` — a literary Easter egg

Fifth pass, same session. `enterPillar`'s flavor-text helper
(`0x14EA2`, an 8-way jump table keyed by pillar/`locationNum`) turned
out to include a real-world reference: one branch prints "MY NAME IS
OZYMANDIAS, KING OF KINGS: LOOK AT MY WORKS, YE MIGHTY, AND DESPAIR!"
— the closing line of Shelley's *Ozymandias*. Worth keeping in mind for
the reimplementation's text assets. Quest-completion bookkeeping for
pillars is separate, in the already-named `questCompleted`.

Also named `findWidgetAtPosition` (`0x122F3`) — `attackPerson`'s lookup
for "who's standing at this (x,y)", scanning `_locationWidgets`.
Distinct from the already-named `getLocationWidgetAt` (a different
implementation, used only by `cityCheckAt`) — both exist independently
in the binary rather than one calling the other.

### Video-mode initializers and a few standalone finds

Sixth pass, same session. `setVideoMode` (already named) dispatches to
one of three hardware-specific initializers by `_videoMode`, now
named by their actual `INT 10h` mode number:

- **`initVideoModeCGA`** (`_videoMode==0`) — mode `4` (CGA 320×200
  4-color), BIOS palette 1, plus a direct `OUT` to port `3D8h` (the
  CGA mode-control register) toggling the composite-color/high-res
  artifact trick.
- **`initVideoModeEGA`** (`_videoMode==1`) — mode `0Dh` (EGA/VGA
  320×200 16-color) with a full 17-byte palette-register load.
- **`initVideoModeTandy`** (`_videoMode==2`) — mode `9` (Tandy/PCjr
  320×200 16-color), same palette load as EGA.

`buildScanlineOffsetTable` (called once per `setVideoMode`, before the
dispatch above) precomputes a 200-entry row→framebuffer-offset table,
with genuinely different address math per mode — CGA's interleaved
even/odd scanline memory layout vs. the linear layout of the 16-color
modes. Useful reference for the reimplementation's renderer: this
program supports 3 real display targets, not just "CGA."

Also named this pass:
- **`waitTimerTicks`** — the real primitive under `wait`: installs a
  custom `INT 1Ch` (system timer tick, ~18.2 Hz on real hardware)
  handler, busy-waits for the requested tick count, restores the
  original vector.
- **`drawDeathGraphic`** — bitmap blitter for the death-screen graphic,
  plotting points via `videoDrawPoint` from a 16-word bitmap table.
- **`drawSelectItemPanel`** — fills/outlines the item-list panel for
  the `selectItem` dialog.

### Critical-error handler and heap init

Seventh pass, same session. `setCriticalErrorHandler`/
`criticalErrorHandler` (`0x1937F`/`0x193D7`) are a DOS `INT 24h`
install/handler pair — `criticalErrorHandler` is `proc far` and ends
in `iret`, which is why it showed 0 static callers: it's only ever
reached through the interrupt vector, never called directly. This is
what lets the program show its own "insert disk" prompt (the
`insertDisk` retry logic documented under the CRT file-I/O layer
above) instead of DOS's default Abort/Retry/Fail screen — a real,
concrete link between two clusters found in different passes this
session. `_nheapinit` (`0x19418`) is the one-time setup for the
`_nmalloc`/`_nfree`/`_nheapgrow` free-list globals, called once from
`start`.

**CRT startup chain, not pursued further**: the rest of the
`start`→`main` init chain (`sub_196AC`, `sub_19969`, `sub_199EA`,
`sub_19BED`, `sub_19E18`, `sub_19E2C`, `sub_19E41`, `sub_19E58`,
`sub_1908B`, `sub_1960C`, plus the remaining stdio internals
`sub_1D126`/`sub_1D1ED`/`sub_1D3AB`) is standard Microsoft C runtime
startup boilerplate (argv construction, environment parsing, and
similar) — spot-checked enough to confirm the pattern, not traced
function-by-function, since it's generic runtime plumbing the C++
reimplementation won't reproduce. Genuinely **dead code** (0 callers,
`proc near` not `proc far`, so not reachable via any interrupt vector
either): `sub_190A6`, `sub_192AE`, `sub_19626`, `sub_1C34C`,
`sub_1CC46` — almost certainly unused functions from the same linked
library objects as the two dead siblings found inside `_nheapgrow`
earlier. Left unnamed; see roadmap.md.

### EXE-chaining mechanism, decoded

Ninth pass, same session, and the most architecturally important
finding of the whole OUT.EXE sweep. `writeInUseAndExit` (already
named, called from `board`) turned out not to be a simple lock-file
writer — it's the entry point into how this program hands off control
to the game's *other* executables (e.g. boarding a ship chains to
`SPACE.EXE`).

**This is not DOS `INT 21h`/`4Bh` EXEC.** No such call appears anywhere
in the chain. Instead:

```
writeInUseAndExit(filename, tileNum)
  writes _savegame state to "inuse.u1" (a lock file)
  loops: chainToExecutable(filename, filename, _param1, _param2, 0)
           execProgramEntry(filename, &argv, envp)
             findExecutableFile(filename)      -- tries bare name,
                                                   then +".EXE", +".COM"
             buildAndChainExecutable(filename, argv)
               -- concatenates argv into a DOS command-tail buffer
               execProgram(filename, cmdTail, envSeg)
                 -- reads the target file directly (open/read/close,
                    NOT exec), sizes it, resizes this process's own
                    memory block, copies a 192-word PSP template into
                    the new segment, copies the loaded image into
                    place, then:
                 far JMPs directly into the loaded program
                 (`jmp dword ptr cs:byte_19BE1`) -- control never
                 returns to DOS, let alone back here
         (only reached if execProgram/findExecutableFile failed)
         insertDisk()   -- "please insert disk" prompt, then retries
```

`writeInUseAndExit` is correctly marked `noreturn` by IDA's own
analysis, consistent with control leaving via a raw far jump rather
than a call/return.

**Why this matters for the ScummVM reimplementation**: the 5
executables aren't really separate DOS *processes* chained via child-
process spawning — they're custom-loaded overlays that jump into each
other while staying resident in the same running program, sharing
whatever's in low memory at the jump. A faithful reimplementation
should model "switching modules" as an in-engine mode change (much
like ScummVM already does for other multi-executable games), not as
launching a subprocess — and it's worth double-checking whether any
state survives the overlay switch only via shared memory rather than
through the `inuse.u1`/savegame file, since that would be easy to miss
by only reading the savegame format.

Named 15 functions/locations in this cluster: `chainToExecutable` /
`chainToExecutableAlt` (unused sibling), `findExecutableFile`,
`buildAndChainExecutable`, `execProgram`, `hasFileExtension`,
`ensureFileExtension`, `_dos_getfileattr`, `strcpy`, `strlen`,
`strncpy`, `_exit`, `atexit` (confirmed real but never called in this
executable), `_creat` (ditto), plus two labeled locations
(`execProgramEntry`, `translateDosErrorToErrno`) that IDA had nested
inside `_nheapinit`'s proc range purely because they're contiguous
with no gap — a real IDB hygiene issue (see roadmap.md) not fixed here
since splitting proc boundaries is riskier surgery than a plain
rename.

### `playSound` effect table, decoded

Eighth pass, same session. Resolved all 10 of `playSound`'s effect
handlers by walking every one of `playFX`'s ~74 call sites back to its
literal `effectNum` argument (script-assisted — a small Python pass
over the exported `.asm`, not manual reading) and grouping by which
game action each site belongs to:

| # | Name | Evidence |
|---|---|---|
| 0 | `soundEffectBump` | `dungeonForward`, `impassable`, `moveCheck` — blocked-move sound |
| 1 | `soundEffectAck` | 34 sites, nearly every top-level command incl. denials — generic "command acknowledged" click, not a true error tone |
| 2 | `soundEffectDamage` | `attackPerson`, `damage`, `death`, `guardAttack`, ... — taking a hit |
| 3 | `soundEffectMonsterAttack` | `dungeonMonsterAttack`, `monsterAttack` — a monster's swing, distinct from taking the hit |
| 4 | `soundEffectFootstep` | `move`, `cityMove`, `guardMove`, NPC updates — shortest delay-loop of all 10 (`bl=8` vs. hundreds), consistent with a quick tick |
| 5 | `soundEffectSuccess` | spell/quest/money success — confirmed exactly in `castSpell`'s "not failed" branch |
| 6 | `soundEffectFailure` | spell failure — confirmed exactly in `castSpell`, played right before printing "Failed!" |
| 7 | `soundEffectAttack` | `attackPerson`, `dungeonAttack`, `guardAttack` — the player's own weapon swing |
| 8, 9 | `soundEffect8`/`9` | never reached by a literal `effectNum` anywhere in OUT.EXE — named only for index-consistency, role unknown |

All 10 handlers directly bit-bang the PC speaker via port `61h`
(square-wave toggle + busy-wait delay loop, no PIT channel 2
involved), confirming they're independent tone effects rather than
shared code with parameters.

### OUT.EXE complete — final pass and dead code

Tenth pass, same session, closing out OUT.EXE at **352/353 functions
named (99.7%)**. Named the last 3 CRT I/O internals, completing the
`_filbuf`/`_flsbuf` layer from an earlier pass: `_read` and `_write`
(the text-mode-aware layer — CR-stripping and Ctrl-Z EOF detection on
read, bare-LF→CRLF expansion on write — sitting above `_dos_read`/
`_dos_write`) and `_lseek` (the CRT-level seek both of them call on to
correct position after text-mode translation).

**Confirmed-dead code, deliberately left unnamed**: `sub_192AE`, a
sibling of `divmod32` sharing its tail code with zero incoming
references of its own — same "unused variant from the same linked
library object" pattern as the dead siblings found inside
`_nheapgrow` in an earlier pass. This is the one remaining
`sub_XXXXX` in OUT.EXE.

**Confirmed-real-but-unused functions, named anyway**: `atexit`
(registers `_exit`'s single exit-hook slot, but nothing in this
executable ever calls it) and `_creat` (a thin `_open` wrapper, same
situation) — genuine, identifiable CRT functionality that this
particular program's code path just never exercises. `checkRange19x9`
is the one exception with a hedged, low-confidence name: two clean
bounds checks (`[0,0x13)` and `[0,9)`) with no callers and no shared
tail code to lean on for context, so its actual purpose is genuinely
unknown — named for its mechanical behavior only, not asserted intent.

## ULTIMA.EXE — findings log

Started 2026-08-19, per Paul's request to switch focus here (title
screen / character-creation launch logic, likely to give useful
context before diving into GEN.EXE). Was 39/100 functions named
before this pass. First read through `_main`'s top-level flow: builds
the initial CRT `FILE` table for stdin/stdout/stderr, checks
`argv[1][0] == 'C'` (likely a color/mono video-mode override flag,
common for early-80s DOS games — not confirmed yet), then runs
`checkMem`, `showCopyrightTitle`, `loadLogo`, conditionally
`decodeCastleEGA`, then loops forever alternating `showTitle1`/
`showTitle2` — the attract-mode title screen. No character-generation-
specific names appear among the already-named functions, consistent
with `GEN.EXE` (a separate executable) owning that instead; expect
ULTIMA.EXE to hand off to `GEN.EXE` via the same overlay-chaining
mechanism decoded in OUT.EXE once a key is pressed at the title
screen.

### ULTIMA.EXE CRT layer transferred from OUT.EXE

`loadLogo` opens `"castle.4"`/`"castle.16"` (raw CGA/EGA framebuffer
dumps of the castle image, not compressed) via a `fopen` whose body
matched OUT.EXE's `_fopen`/`_openfile` FILE-table-walk shape exactly,
which was the tell: **ULTIMA.EXE links the same Microsoft C runtime
object files as OUT.EXE**, just at different addresses. Cross-checked
by mapping every `INT 21h` call in the executable (same technique
used for OUT.EXE) and confirmed by direct reads rather than shape
inference alone — every single candidate matched byte-for-byte,
several even matching OUT.EXE's exact function size (`_flsbuf`: 728
bytes in both; `_open`: 562 bytes in both).

Named 22 functions + 2 locations + 2 globals in one pass:
`_dos_open/close/read/lseek/write/ioctl_get/ioctl_set/creat/creatnew/
creattemp`, `_nheapinit`, `setCriticalErrorHandler`,
`criticalErrorHandler`, `translateDosErrorToErrno`, `_open`,
`_openfile`, `_fread`, `_fclose`, `_flsbuf`, `_nfree`, `_nmalloc`,
`_nheapgrow`, `releaseFileHandle`, `_exit`, `errno`, `_doserrno`. Same
IDB-hygiene quirk as OUT.EXE too: `criticalErrorHandler` and
`translateDosErrorToErrno` are both nested inside `_nheapinit`'s proc
range (IDA merged contiguous code with no gap) rather than being
recognized as separate functions — named as locations rather than
fixing the boundary, same tradeoff as before.

**Not yet found here**: `_fwrite`, `_flushall`, `atexit`, `_creat` —
this executable never writes files (just reads two fixed images) or
registers an exit hook, so those CRT pieces are apparently just not
linked in/reachable, consistent with OUT.EXE having several genuinely
unused CRT siblings too. Everything else in the CRT layer was found
in the fourth pass below.

### ULTIMA.EXE chains unconditionally to GEN.EXE

Second pass, same session — confirms Paul's hunch about this
executable's role and answers the open question from the OUT.EXE
session about how the overlay-chaining mechanism gets *triggered* in
the first place. Traced the exact same
`chainToExecutable`→`execProgramEntry`→`findExecutableFile`+
`buildAndChainExecutable`→`execProgram` shape found in OUT.EXE
(`execProgram` even matches OUT.EXE's exact 555-byte size), and this
time followed it all the way to the literal filename:

```
showTrademarks (displays the trademark screen, waits, slides logo)
  chainToExecutable("gen.exe", &argv, envp)
    execProgramEntry
      findExecutableFile("gen.exe")
      buildAndChainExecutable  -->  execProgram  -->  [far-JMP into GEN.EXE, never returns]
  (only reached if the chain attempt failed)
  "Insert ULTIMA I disk and press RETURN", loop back and retry
```

**`"gen.exe"` is the only executable filename referenced anywhere in
this binary** (checked via string search — no `out.exe`, `space.exe`,
or `mondain.exe` literal exists in ULTIMA.EXE). Combined with what's
already known about the other executables, the game's overall
structure is now clear:

```
ULTIMA.EXE (title/attract-mode screen)
  --always chains to--> GEN.EXE (character creation / continue)
                           --chains to--> OUT.EXE (main game: towns, dungeons, overworld)
                                            <--chains to--> SPACE.EXE (space combat)
```

`GEN.EXE` must itself decide whether to run character creation or
just load an existing save (or ULTIMA.EXE's `argv[1][0]=='C'` check
may be involved in that decision — still unconfirmed, see roadmap.md)
before chaining onward to `OUT.EXE`. Where `MONDAIN.EXE` fits into
this chain isn't known yet — worth checking once `GEN.EXE` or
`OUT.EXE` is worked on, since it may only be reachable from deep
inside the dungeon/endgame rather than from this top-level chain.

### `writeString2_mb` was a misnamed printf, not multi-byte text handling

Third pass, same session. The pre-existing name `writeString2_mb`
(from earlier work, before this session) suggested multi-byte
character-encoding handling. Tracing its two callees
(`sub_11AE7`/`sub_1153D`, both previously unnamed) instead found a
textbook C runtime `vsprintf` implementation: a `%`-format-string
walker dispatching to a 1245-byte per-specifier conversion routine
(`formatArg` — the largest function in this whole executable, sized
consistently with real `%d`/`%s`/`%x`-style width/precision/flag
handling), followed by `fputs`/`putc` writing the result to a fixed
`FILE*` at `0xC73A` (presumably `stdout`). Renamed to
`printStartupMessage` to reflect what it's actually for — its only
two callers are `checkMem` and `sub_104D0`, both early `_main`
startup diagnostics (e.g. a low-memory warning), not general in-game
text. **Note for future sessions**: renamed an existing name here
rather than leaving it, since the old name was actively misleading
about a real capability (multi-byte text) this function doesn't have
— worth double-checking any other pre-2026-08-19 names in this IDB
with similar skepticism if they stop making sense once their callees
are understood.

### ULTIMA.EXE complete — 100/100 (100%)

Fourth pass, same session, closes out ULTIMA.EXE entirely. Two
categories of findings:

**Completed the transferred CRT cluster** — the remaining stdio
internals (`_filbuf`, `_read`, `_lseek`, `_write`, `allocFileBuffer`,
`findFileHandleSlot`) all matched OUT.EXE's equivalents by **exact
byte size** (e.g. `_filbuf` is 410 bytes in both executables), on top
of the shape/role matches already established — strong enough evidence
to name confidently without re-deriving each one from scratch. Also
resolved the `argv[1][0]=='C'` mystery flagged at the end of the last
pass: `toupper(argv[1]) == 'C'` sets a flag that `init_video` uses to
force CGA mode, overriding hardware auto-detection — a command-line
compatibility switch, not anything game-logic-related.

**Found the same function duplicated at two addresses** — `strncpy`
and `toupper` each exist as two byte-for-byte identical
implementations at different addresses (`strncpy`/`strncpy2`,
`toupper`/`toupper2` — IDA requires unique global names, hence the
suffix). Not a bug or a naming mistake: static linking pulled in the
same small CRT utility function from two different `.obj` files
without deduplication, a real (if mildly wasteful) property of how
this binary was built.

**Rest of the printf family decoded**: `formatArg`'s `%x`/`%o`
conversions (`formatHex`, `formatOctal`, reached via a jump table) and
its width/precision parsing (`atoi`, `isdigit`). Also named
`videoDrawPoint` (the CGA/Tandy/EGA-aware pixel primitive underneath
`fillRect`/`drawLine`, matching OUT.EXE's equivalent in role) and its
`videoDrawPointAlt` sibling entry point (used only by the flag/logo
animation code, likely an XOR-style draw mode — not fully traced),
`drawLineInternal` (the Bresenham rasterizer under `drawLine`),
`buildScanlineOffsetTable` (matching OUT.EXE exactly), `divmod32`
(matching OUT.EXE's shift-subtract 32-bit division exactly),
`drawLogoPixelRow` (an ASCII-art-style bitmap renderer for the fading
logo graphic), `flushKeyboardBuffer` (a BIOS-generation-dependent
type-ahead-buffer clear), and `drawAnimatedCursor` (an 8×8, 4-frame
sprite blit called from the title screen's input poll — almost
certainly the blinking "press any key" prompt, though its exact glyph
data wasn't decoded).

**Session totals for ULTIMA.EXE**: 39 → 100 functions named (100%),
across 4 passes. Combined with confirming the full `ULTIMA.EXE` →
`GEN.EXE` → `OUT.EXE` ↔ `SPACE.EXE` chain architecture, this
executable is fully documented.

## GEN.EXE — findings log

Started 2026-08-20, per Paul's direction to move here after
ULTIMA.EXE. Was 44/113 functions named before this pass. The
already-named functions going in immediately confirmed this is the
real character-creation/continue-game executable:
`showMainMenu`, `selectRace`, `selectSex`, `selectClass`, `getName`,
`generateCharacter`, `updateAttribute`, `moveSelectedAttrUp`/`Down`,
`writeDefaultAttributes`, `readSavegameList`, `selectSaveSlot`,
`continuePreviousGame`, `accessSavegame`, `saveQuery`.

### GEN.EXE CRT layer and exec-chain to OUT.EXE

First pass. Same story as ULTIMA.EXE: this executable links the
identical Microsoft C runtime as OUT.EXE, confirmed by direct reads
(free-handle-table-walk shape) plus several exact byte-size matches
(`_flsbuf` 728 bytes, `_filbuf` 410 bytes, `_openfile` 353 bytes,
`execProgram` 555 bytes — all matching OUT.EXE precisely). Named 8 CRT
functions, the `playSound` 10-entry effect table (no semantic labels
this time — only one ambiguous `playFX` call site in this executable,
not enough evidence to name them like OUT.EXE's), and 2 graphics
primitives (`videoDrawPoint`, `drawLineInternal`, both also exact
byte-size matches to ULTIMA.EXE's copies).

**Confirmed the full exec-chain to OUT.EXE**, and how it's invoked —
this is the concrete mechanism behind what was only inferred from
ULTIMA.EXE's side last session. `launchGame` (called from
`continuePreviousGame`) prints "Please wait whilst thy game loads",
then loops calling `chainToExecutable` with:
- filename `"out"` (bare, no extension — `findExecutableFile` adds
  `.EXE`)
- `argv[1]` = a single video-mode letter: `'C'` (default/CGA), patched
  to `'E'` (EGA) or `'T'` (Tandy) based on `_videoMode` right before
  the call
- `argv[2]` = a single digit `'0'` + the save slot number

So GEN.EXE launches OUT.EXE roughly as **`OUT C 0`** (or `E`/`T`,
and whatever slot digit) — meaning **OUT.EXE receives the video mode
and save slot as command-line arguments** rather than rediscovering
them itself or reading them from a shared file. Worth checking
OUT.EXE's own `start`/`main` for how it parses `argv[1]`/`argv[2]`
when that executable gets a later pass — this may currently be
undocumented there since OUT.EXE's `main`'s `argc`/`argv` handling
wasn't traced in detail during that session. On failure, `launchGame`
calls `sub_115B3` (not yet named — likely the `insertDisk`
equivalent, matching the retry pattern in every other chain call
found so far) and retries.

### Character-creation point-buy mechanic, decoded

Second pass, same session — the highest-value finding for the
reimplementation so far, since this is pure game rule logic with no
CRT boilerplate involved.

**Ultima I has six character attributes, not five**: `writeDefaultAttributes`
(already named) displays them literally as `"Strength"`, `"Agility"`
(not "Dexterity"), `"Stamina"`, `"Charisma"`, `"Wisdom"`,
`"Intelligence"` — all starting at exactly **10**. The player then
distributes a pool of **30 points** across them using the arrow keys,
each point moving one attribute by 1, within a hard range of
**10 to 25**:

- **`decreaseAttribute`** (Left arrow) — refuses below 10; otherwise
  decrements the attribute and refunds a point to `_pointsRemaining`.
- **`increaseAttribute`** (Right arrow) — refuses above 25, and refuses
  if `_pointsRemaining` is already 0; otherwise increments and spends
  a point.
- Both redraw via the already-named `updateAttribute`. This session
  (2026-08-25) resolved the small IDB oddity flagged here in the
  original pass: **there is no real struct-field bug** — all three
  functions (`decreaseAttribute`, `increaseAttribute`,
  `updateAttribute`) index the exact same 7-word run of `Savegame`
  fields (`_hits`, `_strength`, `_agility`, `_stamina`, `_charisma`,
  `_wisdom`, `_intelligence`) via `[si]` with `si = attr*2` — i.e. one
  array starting at `_hits` (index 0), with the 6 point-buy attributes
  at indices 1-6. `decreaseAttribute`'s `_savegame._hits[si]` display
  was correct all along; `increaseAttribute` and `updateAttribute`
  showed the identical address as a raw `word ptr [si+0A46h]` purely
  because those 2 instructions were never given the "offset" operand
  type IDA needs to resolve a bare immediate back to a symbol (`0xA46`
  = `dseg`'s base `0x14260` + `_hits`'s struct offset `0x16`, byte-for-
  byte the same address `decreaseAttribute` references). Applied
  `idc.op_plain_offset` to both — now all three read identically as
  `_savegame._hits[si]`. Fixed via
  `ida_scripts/apply_structs_gen.py`. The original "stale/incorrect
  alternate field name" suspicion was a misdiagnosis of this
  display-only gap, not a genuine struct inconsistency.
- **`clearSelectionArrows`** blanks the `<`/`>` cursor at the old
  attribute row before `moveSelectedAttrUp`/`Down` (already named)
  redraws it at the new one via `writeSelectionArrows` (already
  named).

This is a complete, concrete spec for the reimplementation's character
creation screen: 6 named attributes, baseline 10, pool of 30, hard
range [10, 25] per attribute — no guessing needed.

### GEN.EXE complete — 113/113 (100%)

Third and fourth passes, same session, close out GEN.EXE entirely.

**Finished the CRT transfer**: the remaining `_dos_*` raw primitives,
`_write`/`_nmalloc`/`_nheapgrow`/`_nfree`/`_nheapinit`, `writeNumber`,
`hasFileExtension`/`ensureFileExtension`/`strcpy`/`strncpy` — all
confirmed by direct read and/or exact byte-size match to OUT.EXE, same
methodology as every prior pass this session. One correction along the
way: `sub_13382` looked at first like another `strncpy` copy, but a
direct read showed an *unbounded* `lodsb`/`stosb` loop — it's `strcpy`,
called from `findExecutableFile` and `ensureFileExtension` exactly like
OUT.EXE's `strcpy` usage, not the bounded-copy pattern.

**The `getKeypress` naming turned out inverted from OUT.EXE's
convention** — this IDB's pre-existing `getKeypress` (from work
before this session) is actually the *polling-loop* wrapper
(OUT.EXE's `getKeypressAndWait` role), while the real single-poll
primitive was unnamed. Named the primitive `getKeypressRaw` rather
than renaming the well-established, heavily-cross-referenced
`getKeypress` — unlike `writeString2_mb` in ULTIMA.EXE, this name
isn't factually wrong (it does return a keypress), just conventionally
different, so the churn wasn't justified.

**Confirmed a second real game-mechanic**: the savegame roster holds
a maximum of **4 characters**. `drawCharacterRoster` lists them
(shared by `continuePreviousGame`'s "who do you want to play" and the
new `showCharacterReplacementMenu`'s "the roster is full, choose who
to replace" flow) — both draw from the same 4-slot, `STR15`-sized name
array.

**More CRT/library duplication**: `sub_11B9B` turned out to be a
byte-for-byte second copy of the already-named `_fread` (named
`_fread2`) — the same static-linking-pulls-in-duplicate-`.obj`-copies
pattern already seen twice in ULTIMA.EXE (`strncpy2`/`toupper2`).

**Session totals for GEN.EXE**: 44 → 113 functions named (100%),
across 4 passes. Combined with ULTIMA.EXE and OUT.EXE, the game's
full startup-to-gameplay chain (title screen → character creation →
main game ↔ space combat) is now completely named end to end.

## SPACE.EXE — findings log

Started 2026-08-20, per Paul's direction after GEN.EXE. Was 156/210
already named from substantial prior work — including, notably, a
`.asm`/`.idc` export that already carried rich **unpromoted analysis
comments**: several `sub_XXXXX` functions had detailed multi-sentence
descriptions written directly above their `proc` line (e.g. `"drawLine
(x1,y1,x2,y2,color) - thin wrapper over the Bresenham core..."`) that
had never actually been turned into a rename. Recognizing and
promoting these was the fastest win of this pass. Also already
confirmed by prior work, in an inline comment on the "leave outer
space" function: SPACE.EXE hands control back to `OUT.EXE` the same
overlay-jump way already decoded for the other three executables,
passing `argv = {"out.exe", "S"}` — closing the loop on the
`OUT.EXE` ↔ `SPACE.EXE` half of the chain diagram from the OUT.EXE
session (that function was already named and wasn't touched this
pass; worth double-checking its "(DOS EXEC...)" phrasing isn't taken
too literally, since every other executable's equivalent function was
confirmed to use a custom loader, not real DOS `INT 21h`/`4Bh` EXEC).

### SPACE.EXE CRT gaps filled

First pass. Filled the remaining CRT cluster gaps using the same `INT
21h` subfunction mapping used throughout this project (not just
size-matching, to avoid the `strcpy`/`strncpy` mixup risk seen once in
GEN.EXE) — 24 renames. Also cleaned up two placeholder names left by
prior work that were clearly tentative rather than final:
`"exec?"` → `execProgramEntry` and the single-letter `"f"` →
`_filbuf`.

### Sector map icon dispatch and other game-specific finds

Second pass. `inform` (already named, one of `_main`'s jump-table
screens) turns out to be the space combat "long-range sensor" /
sector-overview map: `drawSectorMapIcon`, given sector coordinates,
checks `_spaceMapCell._enemyCount` / the docking anchor (`field_6`) /
the hazard-star position (`field_2`) in that priority order and
dispatches to `drawEnemyMarker` / `drawStationMarker` /
`drawStarMarker`, falling back to `drawEmptyMarker` — the four icon
types shown per sector on the scan screen. Also named `isqrt` (called
from `view`, the main cockpit screen — likely a proximity/targeting
distance calculation) and promoted the three already-commented
`drawLine`/`drawLineTo`/`drawLineInternal` line-drawing functions.

189/210 functions now named (90%).

### SPACE.EXE complete — 210/210 (100%)

Third and fourth passes, same session, close out SPACE.EXE entirely.

**The `status` screen decoded**: a scrolling message-log with a
two-column, 13-row grid layout (`setStatusEntryPos`), `.`-padded
4-digit numeric values (`writeStatusValue`), a `-- More --`
page-break prompt every 25 rows (`showMorePrompt`), and a genuine game
formula — `calcExperienceLevel` is `_savegame._experience / 1000 + 1`.

**The `view` cockpit screen's sector-tracking math**: `updateSectorChangeX`/`Y`
compute the per-frame sector-boundary-crossing delta from
`_cockpitSpeed`, snapped to the nearest sector via
`snapToNearestSector` and clamped to a max of 6 — the core of how the
ship's position updates as it flies.

**`spaceAce` decoded** (an already-present but unpromoted comment did
most of the work): the "Thou hast achieved the rank of Space Ace!"
screen, triggered once `_savegame._enemyVessels` reaches 20. Named its
two helpers, `drawDialogFrame`/`drawBorderBox`, shared with `inform`'s
sector-map screen.

**More duplicated CRT functions found**: `exit2` (byte-for-byte
identical to the already-named `exit()` at a different address) and
two placeholder-cleanup renames (`fputs`, `parseParamNum` — the latter
matching the same-named, separately-implemented function role already
established in ULTIMA.EXE/GEN.EXE for parsing a numeric command-line
argument).

**Session totals for SPACE.EXE**: 156 → 210 functions named (100%),
across 4 passes. All four of `ULTIMA.EXE`/`GEN.EXE`/`OUT.EXE`/
`SPACE.EXE` are now fully or near-fully named (only `OUT.EXE` has one
deliberately-unnamed dead function left). Only `MONDAIN.EXE` (1/191)
remains essentially unstarted.

## `MONDAIN.EXE` — findings log

### The whole remaining cluster is one thing: the Mondain encounter

Paul ran BinDiff against `ultima1_out` before this session and applied
the matches, bringing this IDB from 1/191 to 134/191 named — all of it
shared CRT/engine code byte-identical to OUT.EXE. The 57 functions
BinDiff couldn't match turned out to be almost entirely **one
self-contained cluster**: everything reachable from `start2`'s single
callee, a 680-byte far proc now named `mondainMainLoop`. OUT.EXE's
`board` function chains to `"mondain.exe"` (string `aMondain_exe`,
xref `board+109`) — the special final-confrontation location "Mondain's
Gate to Hell" (also present in SPACE.EXE's location table) boards into
this executable instead of the normal town/castle overlay. On exit,
`endEncounter` → `writeInUseAndExit` chains back to `"out.exe"`,
confirming the round trip.

### Decoding message text unlocked exact naming

Early reads of this cluster showed lots of `mov ax, <small hex>; push
ax; call writeString`-style calls where the "message" was just a bare
numeric immediate — no `offset aXxx` operand for IDA to have turned
into a string xref. Tracing `writeString`'s body
(`mov si, [bp+arg_0]` then dereferencing `[si]` with the default `DS`)
showed these immediates are meant to be read as near-pointer offsets
into `dseg`, which stays the active data segment for the whole program.
Wrote a throwaway helper (`ida_scripts/dump_msg_strings.py`, kept in
the repo since it's generically reusable) to resolve `dseg_base +
offset` and print the string — confirmed correct immediately (0x264 →
`"Blocked!"`, 0x287 → `"Hit Mondain! "`, 0x3FE → `"Board?"`, etc.).

This turned what would otherwise have been a lot of guesswork into
precise naming: the entire per-letter command dispatch table in
`mondainMainLoop` could be identified by the literal prompt text each
handler prints (`"Board?"`, `"Drop?"`, `"Get (Gem)"`, `"Cast "`, `"Quit
...is not allowed!"`, etc.), not just by structural shape.

### The encounter, mechanically

- A 19×9 (`0x13 × 9`) map, with the player at `(playerMapX,
  playerMapY)` and Mondain at `(mondainMapX, mondainMapY)` (his tile's
  occupant-state byte is always 6). **Correction** (found while
  writing [file-formats.md](file-formats.md#mondainexes-startup-file-loads--corrected)):
  the map's actual tile layout is *not* loaded from a file — the 3
  `readFile` calls at startup load the current savegame (`inuse.u1`),
  a partial slice of the shared overworld tile graphics, and the
  dedicated Mondain-encounter graphics, none of which are room-layout
  data. With only one Mondain encounter in the whole game, the layout
  is presumably hardcoded directly in this executable's data segment.
- Arrow-key movement (`moveOrMeleeAttack`) either walks into an empty
  cell, is blocked by Mondain's own tile, or — if some other occupant
  is there — deals damage and, once player and Mondain are adjacent
  (`isAdjacentToMondain`), sets `combatActiveFlag`, which is the gate
  that turns on Mondain's whole turn-based AI (`mondainTakeTurn`,
  `updateMondainState`).
- Most letter commands (`B`oard, `D`rop, `E`nter, `F`ire, `H`yperjump,
  `K`limb, `O`pen, `Q`uit, `S`teal, `T`ransact, `U`nlock, `V`iew,
  `X`it) are flatly disabled here — each just echoes its normal-game
  prompt and beeps (`cmdBoardDisabled`, `cmdTransactDisabled`, etc.),
  confirming this is a deliberately constrained arena distinct from
  the normal town/dungeon command set.
- Two real actions are functional: `G`et (`attackGem`, prompt `"Get
  (Gem)"`) damages Mondain directly by 3/4 of his current HP once
  standing next to him, eventually setting `gemDestroyedFlag`; and `C`
  (`useSelectedItem`, "Cast") fires one of 5 usable spells
  (`applySpellEffect` indices `{3,7,8,9,10}`: magic-missile attack,
  teleport self, place/remove a barrier tile, or the killing
  incantation `spellEffectInterficioNunc` — literally Latin for "I
  kill now", which **backfires and doubles Mondain's HP** if the
  player isn't close enough (`isWithinRange7`) — the exact precondition
  for it to actually kill him wasn't fully pinned down, see
  roadmap.md). `I`nform (`inspectMondainAura`) reports whether
  Mondain's protective aura is weakened, keyed off `gemDestroyedFlag`.
  Ranged attacks with the currently-readied wand (`attackWithWand`,
  `cityCastleAttackDir`'s `dir==7` special case) are also available —
  5 dispatch-table entries reach it with different tier arguments,
  exact per-key mapping unconfirmed.
- Mondain has his own HP-driven state machine (`updateMondainState`):
  healthy (`mondainPhase`=2) → wounded below 500 HP (phase 0xB,
  regenerates and tries to flee via `tryMoveMondain`) → dead at 0 HP
  (phase 4, triggers `mondainDefeatedFlag` if `gemDestroyedFlag` was
  also set, then `playMondainDefeatCutscene`). His turn
  (`mondainTakeTurn`) picks between a direct attack (`dungeonAttack`,
  pre-existing BinDiff-transferred name that turned out to still fit —
  it damages the player) and a special ability roll
  (`mondainSpecialAttack`: weak hit / drain-a-resource curse / a
  screen-flash "big" hit).
- Losing (`playerHits` or `playerFood` hitting 0) and presumably
  winning both funnel through `endEncounter` →
  `writeInUseAndExit`, which shows a disk-swap prompt loop
  (`insertDisk`) and chains back to `out.exe`. Whether the win path
  (`playMondainDefeatCutscene`) rejoins this same exit or has a
  separate continuation wasn't confirmed — see roadmap.md.

### One BinDiff-transferred name was wrong: `viewChange` → `cmdTransactDisabled`

BinDiff matched a function here to OUT.EXE's real `viewChange` purely
by code shape — `writeStringNewline` + a one-line-message-and-beep
helper, the same shape as several genuinely-disabled command stubs in
this cluster. Once the message text was decodable, it turned out to
print `"Transact...Mondain will" / "not negotiate!"` — the `T`ransact
command's refusal message, nothing to do with view-switching. Renamed
to `cmdTransactDisabled`; the real, previously-unnamed `"View?"` stub
is now `cmdViewDisabled`. Same category of issue as
`writeString2_mb`→`printStartupMessage` in ULTIMA.EXE: a
structurally-matched or coincidentally-plausible name that doesn't
survive checking against what the function actually does. Worth this
same skepticism check if any other BinDiff-transferred name in this
IDB stops making sense. (A third suspected case, GEN.EXE's
`_savegame._hits`/`_strength` "mixup", turned out on closer inspection
during the later struct-cleanup pass to not be a real issue at all —
see
[overview.md](overview.md#character-creation-point-buy-mechanic-decoded)'s
updated writeup.)

**Session total for MONDAIN.EXE**: 134 → 191 functions named (100%),
in one pass, plus one name fix (`viewChange` → `cmdTransactDisabled`)
and 18 global renames (`playerHits`, `mondainHits`, `mondainPhase`,
`combatActiveFlag`, etc.) All five executables are now fully named.
Remaining work: 0 structs defined in this IDB (see roadmap.md), and the
few open semantic questions flagged above (`spellEffectInterficioNunc`'s
real kill condition, the win-path exit sequence, `cmdHyperjumpDisabled`'s
stray `word_15B00 = 1` side effect).

## Cross-IDB struct cleanup — `Savegame` brought up to date and synced

With all 5 executables fully named, moved to the first item on the
cross-IDB follow-up list: the shared `Savegame` struct (the on-disk/
in-memory save-file format, referenced as `_savegame` in
`ULTIMA.EXE`/`GEN.EXE`/`OUT.EXE`/`SPACE.EXE` — `MONDAIN.EXE` never
references it at all, see below). Since IDA has no shared type library
across separate IDBs, each executable's copy had been named
independently over many sessions and had drifted out of sync with each
other. Wrote `ida_scripts/dump_struct.py`/`dump_savegame_full.py` to
compare all 4 copies side by side.

**What was actually wrong, in order of severity:**

- **`ULTIMA.EXE`'s copy was badly offset-misaligned** from `+0x20`
  onward — missing 8 fields entirely (`_wisdom`/`_intelligence`/
  `_coins`/`_experience`/`_food`/the 3 ready-item fields), which shifted
  every subsequent field 16 bytes out of position (`_position` sat at
  `+0x24` instead of the real `+0x34`, etc.), degrading into a wall of
  unnamed `field_XX` placeholders and a totally undefined 642-byte gap
  from `+0xB0` to `+0x332`. Confirmed via `grep -i savegame
  ultima1.asm` returning **zero matches** that this struct is never
  actually applied to any variable in this executable — makes sense,
  `ULTIMA.EXE` is just the title screen and chains to `GEN.EXE` before
  any savegame data would be touched. Since nothing depended on the old
  (wrong) layout, rebuilt the whole struct from scratch
  (`ida_scripts/apply_structs_savegame_ultima.py`) to match the other
  3 IDBs exactly, including importing the `Creature` struct (which
  `ULTIMA.EXE` didn't have at all).
- **`SPACE.EXE` had already independently worked out 5 fields the
  other 3 IDBs still had as raw `field_XX`**: `_readyWeapon`/
  `_readySpell`/`_readyArmor` (the other 3 called them
  `_equippedWeapon`/etc.) and `_shipFuel`/`_shipShield` (the other 3
  had these as unnamed `field_B0`/`field_B2`). Standardized on
  `SPACE.EXE`'s names — `_ready*` matches the game's own command
  terminology (the `R`eady command, named `ready` everywhere it
  exists), and propagated `_shipFuel`/`_shipShield` to
  `ULTIMA.EXE`/`GEN.EXE`/`OUT.EXE`.
- **`_overworldWidgets` (Paul's example — see below) turned out to
  hold more than monsters.** Investigated via
  `ultima1_out.asm` cross-references before renaming anything:
  `_type` gets compared against the already-named `TILE_FIRST_MONSTER`
  constant to decide whether an entry *is* a monster (implying
  non-monster entries exist), and `exitLocation` re-adds the *same*
  widget — with `_data` set to the tile that was underneath it —
  back onto the overworld map at the player's position when leaving a
  town/dungeon, which is how a ship/raft left docked outside a town
  persists. So "monsters" would have been actively wrong, the same
  category of mistake as `viewChange`/`writeString2_mb` above — this
  is genuinely a general overworld-object slot list (monsters *and*
  transports/markers left behind), renamed to **`_overworldEntities`**
  instead. The 40-element array's per-slot struct is still `Creature`
  (already reasonably named, and monsters remain its primary use).
- **`field_AA` → `_overworldEntityCount`**: confirmed via both
  `saveGame` and `writeInUseAndExit`, which both do
  `_savegame.field_AA = _creaturesCount` immediately before writing the
  whole 820-byte struct to disk (`mov bx, size Savegame`), and OUT.EXE's
  startup code restoring `_creaturesCount = _savegame.field_AA` right
  after loading the town map on the load path — a save/restore pair
  for the live entity-count global, not raw padding.
- **`_quests` → `_questStatus`**: confirmed as a 9-word array (18
  bytes) indexed by `_castleIndex * 2`, holding per-castle quest
  *state* (`-1` = not yet offered, `1` = accepted, `0` = reward
  claimed) — a status table, not a list of quest objects, hence the
  clearer name.
- **The `_X_array` fields** (`_armor_array`/`_weapons_array`/
  `_spells_array`/`_transports_array`, briefly `_armorSlot0`/
  `_weaponSlot0`/`_spellSlot0`/`_transportSlot0`) were actively
  misleading: each is a genuinely distinct 2-byte memory slot sitting
  2 bytes *before* the first individually-named item in its category
  (e.g. `_armor_array` at `+0x54`, `_leatherArmor` at `+0x56`), not —
  as the `_array` suffix implied — the base of an array whose first
  element is one of the already-named fields.

Also renamed `addOverworldWidget` → `addOverworldEntity` in `OUT.EXE`
(its only home) for consistency with the field rename.

## Item-slot-0 and `Creature` padding, resolved

Follow-up pass (2026-08-25) on the two questions the struct-cleanup
pass above left open.

**What item is slot 0 in each category?** Found the answer in
`OUT.EXE`'s pre-existing item-name tables — `ARMOR`, `WEAPONS_LOWERCASE`,
`SPELL_NAMES`, `TRANSPORTS` (all already named, just not cross-checked
against the `Savegame` field order before now). Each has a real,
named entry at index 0:

| Table | Index 0 | Renamed field |
|---|---|---|
| `ARMOR` | `"Skin"` | `_armorSlot0` → **`_skin`** |
| `WEAPONS_LOWERCASE` | `"hands"` | `_weaponSlot0` → **`_hands`** |
| `SPELL_NAMES` | `"Prayer"` | `_spellSlot0` → **`_prayer`** |
| `TRANSPORTS` | `"Foot"` | `_transportSlot0` → **`_foot`** |

So these were never unused padding — they're each category's "nothing
special equipped" baseline: bare skin, bare hands, a basic prayer
instead of a real spell, on foot instead of a vehicle. Confirmed for
armor specifically by `dropArmor`'s drop-menu loop, which explicitly
starts iterating at index 1 (`var_A = 1`) rather than 0 — you can't
drop your own skin. The same tables also explain a detail noticed
during the earlier `apply_structs_gen.py` fix: `ATTRIBUTES[0]` is
`"Hit Points"`, confirming `_hits` is genuinely index 0 of that
struct's 7-word attribute array (see the character-creation section
above) — the exact same "index 0 is a real baseline value, not
padding" pattern, just for attributes instead of items.

**What are `Creature.field_A`/`field_C`/`field_E`?** Confirmed
genuinely unused — not just "unreferenced by name" (the kind of gap
that produced the false `_hits`/`_strength` alarm in GEN.EXE), but
unreferenced by *any* instruction. Wrote
`ida_scripts/find_creature_padding_refs.py` to compute the raw
immediate value an `[si]`-indexed array access would need to reach
each field and scan every instruction's raw operand values for a
match across the entirety of both `OUT.EXE` and `SPACE.EXE` (the two
executables that actually use the `Creature` array) — zero hits in
either. The array is indexed via `shl ax, 4` (× 16) rather than an
`imul` by the struct's real 10-byte size (5 words: `_type`/`_data`/
`_x`/`_y`/`_hits`), which is presumably *why* the padding exists: 16 is
a power of 2, letting every array access use a fast shift instead of a
multiply, at the cost of 6 wasted bytes per 40-entry array (240 bytes
total). Renamed `field_A`/`field_C`/`field_E` → `_unused1`/`_unused2`/
`_unused3` to reflect the now-confirmed (not just assumed) status.

Both fixes applied via the same `ida_scripts/apply_structs_savegame.py`
used for the original sync pass, across all 4 struct-owning IDBs.

**`MONDAIN.EXE` intentionally excluded**: confirmed via `grep -i
savegame ultima1_mondain.asm` that this executable never references
`_savegame` at all — its encounter state (`playerHits`, `playerFood`,
etc., named in the MONDAIN.EXE pass above) lives in its own standalone
globals, not a `Savegame`-typed variable, so there's nothing to sync
there.

All 4 struct-owning IDBs (`ULTIMA.EXE`/`GEN.EXE`/`OUT.EXE`/`SPACE.EXE`)
now have **byte-identical, identically-named** `Savegame` (820 bytes)
and `Creature` (16 bytes) struct layouts, verified via a full
side-by-side dump after applying. Scripts: `ida_scripts/dump_struct.py`
(single struct), `ida_scripts/dump_savegame_full.py` (Savegame +
every struct-typed member's own layout, for cross-IDB diffing),
`ida_scripts/apply_structs_savegame.py` (GEN/OUT/SPACE — plain
renames), `ida_scripts/apply_structs_savegame_ultima.py` (ULTIMA.EXE —
full rebuild, safe because the struct was unused there).

## `word_1F95E` fixed — it's a powers-of-ten table

Picked up the next roadmap item: `word_1F95E` had been flagged as
"currently a single `dw 1` plus raw `db` bytes, should be a proper
array". Its only reference, `readAmount` (the 4-digit numeric-input
reader behind `dropPence`/`transactGrocer`/etc.), does
`mul word_1F95E[bx]` with `bx = digitPosition*2` while accumulating a
typed number one digit at a time working backward from the last digit
— classic ASCII-to-integer conversion via a powers-of-ten lookup. The
6 raw bytes following the initial `dw 1` decode as `0x000A`, `0x0064`,
`0x03E8` — 10, 100, 1000. Confirmed the byte content matched exactly
before touching anything, then re-defined the 8-byte range as a proper
4-element word array and renamed it `POWERS_OF_TEN` (matching this
IDB's existing ALL_CAPS convention for lookup tables — `ARMOR`,
`WEAPONS_LOWERCASE`, `SPELL_NAMES`, etc.). Now displays cleanly as
`POWERS_OF_TEN dw 1, 0Ah, 64h, 3E8h` instead of a named word followed
by 6 bytes of unlabeled data. Fixed via
`ida_scripts/fix_powers_of_ten.py`.

## `_nheapinit`'s proc boundary split — and a dead sibling found in the process

Next roadmap item: `_nheapinit`'s function boundary in OUT.EXE had
silently swallowed 3 unrelated pieces of code that happened to sit
contiguously after it with no gap (a long-flagged IDB hygiene issue,
first noticed while decoding the EXE-chaining mechanism). Mapped the
exact split points with `idautils.CodeRefsTo`/`get_func`/manual
boundary inspection before touching anything:

| Range | What it actually is |
|---|---|
| `0x19418`-`0x1949D` | the real `_nheapinit` (heap-init / `SETBLOCK` dance) |
| `0x1949E`-`0x194CE` | **a genuinely dead, uncalled block** (see below) |
| `0x194CF`-`0x1950C` | `execProgramEntry` (already named as a location, not a real function) |
| `0x1950D`-`0x19568` | `translateDosErrorToErrno` (ditto) |

The middle block was the interesting find: it loops through a
candidate-name list and calls `execProgramEntry`, but
`idautils.CodeRefsTo(0x1949E)` returns **zero** hits — no code ref, no
data ref, and the instruction immediately before it is a `retn`, so it
can't even be reached by fallthrough. It's genuinely unreachable dead
code, the same family as the already-documented dead `sub_192AE`
(shares tail code with `divmod32`, also zero callers). Left it with
its auto-generated name (`sub_1949E`) rather than guessing an
identity from zero evidence — it may be the compiled body of an
alternate calling convention for `chainToExecutableAlt` (the other
confirmed-unused sibling in this same cluster) that never actually got
wired up, but that's speculation, not a finding.

Deleted the merged function and re-added the 4 correct boundaries via
`ida_funcs.del_func`/`add_func` (`ida_scripts/fix_nheapinit_boundary.py`).
All 3 previously-`loc_`-attributed names (`_nheapinit`,
`execProgramEntry`, `translateDosErrorToErrno`) kept their existing
names — this was purely a boundary fix, not a rename — and every XREF
comment throughout the file now correctly attributes callers to the
right function (e.g. `buildAndChainExecutable`'s caller comment changed
from `_nheapinit+DF` to the accurate `execProgramEntry+28`).
**356 total functions now (was 353), 354 named** — the 2 remaining
`sub_XXXXX` are this newly-split dead block and the pre-existing dead
`sub_192AE`, both confirmed-dead, not gaps in the naming sweep.

One incidental, unplanned improvement rode along: deleting/re-adding
the function boundaries triggered IDA's own re-analysis of a couple of
nearby operands, which noticed `word_1D48A` and `word_1D48C` (two
previously-separate 2-byte globals, already treated as one 4-byte far
pointer at their only real use site — `les ax, dword ptr word_1D48A`)
should actually be a single `dword_1D48A`. Verified no dangling
references to the old `word_1D48C` name remained after the merge.

`ULTIMA.EXE`/`GEN.EXE` have a related-but-different situation
(`translateDosErrorToErrno` already lives in a separate function
*chunk*, not literally merged into `_nheapinit`'s contiguous byte
range) — not touched here since the roadmap item was scoped to
OUT.EXE specifically; worth a look in a future pass if it turns out to
have the same hygiene problem in a different shape.

## SPACE.EXE's "(DOS EXEC...)" comment confirmed stale, corrected

Last item on the polish-list check: whether `execWithEnvp`'s function
comment — "exec?() itself is presumably the real DOS EXEC (INT 21h
AH=4Bh)..." — described a genuine architectural difference from every
other executable's confirmed custom-loader chain-exec mechanism, or
was just loose/outdated wording. It's the latter, and the evidence was
immediate: the comment's own text refers to the callee only as the
placeholder `exec?()`, but that function has long since been named
`execProgramEntry` in this same IDB — the exact same name (and,
tracing it, the exact same reads-the-file/builds-a-PSP/far-JMPs-in
shape) already confirmed in `OUT.EXE`/`ULTIMA.EXE`/`GEN.EXE` to be the
custom overlay loader, never real DOS `INT 21h`/`4Bh` EXEC. The
comment simply predates that naming/tracing work and was never revised
afterward.

Traced `execWithEnvp`'s only caller while at it: SPACE.EXE's own
`exit()` (the "leave outer space" function, called from `death` and
`checkForLanding`) — on non-zero hits it drains ship fuel/shields
first, then unconditionally saves `_savegame` to `"inuse.u1"` and
calls `execWithEnvp("out.exe", "S", 0)`, retrying via
`promptDiskSwapRetry` on failure. Exactly the same
save-then-chain-with-retry shape as every other executable's exit
path. Corrected the stale function comment on `execWithEnvp` in place
(`ida_scripts/fix_execwithenvp_comment.py`) rather than leaving the
misleading guess to confuse a future reader.

This closes out the last open item from the SPACE.EXE session and the
cross-IDB polish-list follow-ups (aside from segment renames, which
are explicitly out of scope — Paul uses the existing segment names to
correlate with DOSBox Debugger).
