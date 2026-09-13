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

Status as of this pass (2026-09-13, via `ida_scripts/identify.py`): **57
functions total, 57 named, 0 still placeholder `sub_XXXXX`** — the full
function-naming sweep is complete. No structs defined yet (this
executable turns out not to need any — see "ULTIMA.COM's real role"
below). `ultima.asm` is ~113KB.

## Two executables, two IDBs

Following the "ULTIMA.COM's real role" finding below, this project now
has the same shape as `ultima1` (one IDB per DOS executable) rather
than `ultima2`'s single-IDB shape it was originally scaffolded to
match — `ida_scripts/run_ida_script.ps1`/`batch_run_and_export.py` were
generalized 2026-09-13 to take a `-Idb` parameter (ported directly from
`ultima1`'s equivalent driver) rather than staying hardcoded to
`ultima.idb`.

| IDB | Root file | Role | Functions named |
|---|---|---|---|
| `ultima.idb` | `ULTIMA.COM` | Title screen / boot loader, chains to BOOTUP.BIN | 57 / 57 (100%) |
| `ultima_bootup.idb` | `BOOTUP.BIN` | The actual game (character creation, main loop, etc. — hypothesis, not yet confirmed) | 4 / 73 |

`ultima_bootup.idb` was created 2026-09-13 by copying `BOOTUP.BIN` to a
temporary `.com`-extensioned file so IDA's automatic loader detection
would treat it exactly like `ULTIMA.COM` (tiny-model COM file, same
paragraph-`1000h`/offset-`100h` load address) rather than falling back
to the generic "Binary File" loader with no base-address knowledge —
the scratch copy is deleted immediately after (not needed once the IDB
exists; IDA embeds its own copy of the input bytes). This is why the
IDB's "root filename"/"input file" fields read `_bootup_scratch.com`
rather than `BOOTUP.BIN` — cosmetic only, doesn't affect the analysis.
Confirmed loading at the identical address range convention as
`ULTIMA.COM` (`0x10100`-`0x14D74`, 19,572 bytes) — consistent with the
FCB-read chain-load trick overwriting `ULTIMA.COM`'s memory in place.

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
- `BLANK.IBM`/`EXOD.IBM` (16,384 bytes each, fixed-size) are also
  referenced by string literal in the initial auto-analysis
  (`aBlankIbm`, `aExodIbm`) — full-screen image/bitmap data, likely for
  title/intro and the Exodus encounter, akin to Ultima II's `PIC*`
  full-screen CGA art files (see that project's file-formats.md for the
  precedent).

## ULTIMA.COM's real role: a title-screen loader, not the game

**Major finding, resolves why this IDB only has 57 functions.**
`ULTIMA.COM` is not the game itself — it's a self-contained title-
screen/attract-mode program whose entire job is to show the boot logo
animation and the wind-direction display, wait for a keypress, then
**load and chain execution into `BOOTUP.BIN`**, which is presumably
where character creation, the main game loop, and everything else
actually lives (undisassembled, no IDB yet — see
[roadmap.md](roadmap.md)).

The chain mechanism (in `titleScreenAndChainToBootup`, formerly
`start_0`, asm ~213-246) is a distinct trick from Ultima I's
`chainToExecutable`, worth documenting precisely since it's a neat
piece of 1980s DOS programming:

1. DOS's Disk Transfer Area (DTA) is set to offset `100h` — this
   program's **own code start** (`INT 21h AH=1Ah`).
2. `BOOTUP.BIN` is opened via FCB (`INT 21h AH=29h` parse, `AH=0Fh`
   open) through `openFileWithRetry`, which prompts "Wrong Diskette!"
   and retries on failure rather than erroring out — floppy-swap UX.
3. The FCB's record-size field is set to the **file's own total size**
   (copied from its file-size field) — so a single "sequential read"
   record covers the whole file.
4. A 2-byte `INT 21h` opcode (`CD 21`) is hand-written directly into
   the PSP at offset `FEh`, `SP` is set to `FEh`, `AH=14h` (FCB
   sequential read), then execution jumps to offset `FEh` — running
   the just-written `INT 21h` instruction.
5. That `INT 21h` call reads the **entire file** (one record = whole
   file size) directly over this program's own code at offset `100h`.
   Since `SP=FEh` when the `INT 21h` instruction executes, the CPU's
   pushed return address gets popped on the handler's `IRET` and
   execution resumes at offset `100h` — which now holds `BOOTUP.BIN`'s
   code, not `ULTIMA.COM`'s.
6. If `BOOTUP.BIN` isn't found, falls through to `jmp near ptr 0` (the
   PSP's built-in `INT 20h` stub at offset 0 — a clean terminate,
   rather than an error message).

This means **`BOOTUP.BIN` shares this program's entire low-level
runtime**: `drawTileGrid` (the generic 64-byte-tile-per-cell blitter,
whose 11×11-grid usage here exactly matches the `CNFLCT_*.ULT` combat
arena dimensions — see [file-formats.md](file-formats.md)) and
`playSoundEffect`'s 12-entry `SOUND_EFFECT_TABLE` (only 1 of 12 effects,
`playErrorBeep`, has a confirmed call site within `ULTIMA.COM` itself —
the other 11 are presumably called from `BOOTUP.BIN`) both look like
shared library code linked into both executables, not code exclusive
to the title screen. `EXODUS.BIN` (44,234 bytes) is a separate,
still-unexamined file — not chained to from anywhere found in this
IDB; per the external documentation in file-formats.md it holds fixed
game-world coordinate tables (castles/towns/dungeons/moongates) rather
than being another chained executable, though that's not yet confirmed
against its own bytes (does it start with `MZ`?). See
[roadmap.md](roadmap.md).

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
- **`ida_scripts/apply_renames_<stem>.py`** / **`apply_structs_<stem>.py`**
  (`ultima`/`bootup`) — accumulating, idempotent, re-runnable scripts
  per executable for plain renames and struct-member edits
  respectively (separate files since they use different IDA APIs; one
  set per IDB since ultima1's precedent showed function/global renames
  don't carry across IDBs automatically even when the executables share
  code, per the naming convention doc there). `apply_renames_ultima.py`
  holds the full 54-entry `ULTIMA.COM` function-naming sweep plus 12
  global/table/string renames (`DRY_RUN = False`, flipped once the
  first batch verified clean — see roadmap.md); the `_bootup` variants
  are freshly scaffolded, both empty (`DRY_RUN = True`), pending
  `BOOTUP.BIN`'s own identification pass.
- **`ida_scripts/fix_wind_string_array.py`** — one-off structural fix,
  kept per the sibling-project convention of not deleting one-off
  scripts: splits the 5 wind-direction strings (see `updateWindDisplay`
  below) out of one undifferentiated IDA data blob into 5 separately
  addressable byte-array items, which `idc.set_name` requires before
  4 of the 5 could be named (only the blob's start was nameable
  beforehand). `create_strlit` unexpectedly returned `False` for all 5
  even after `del_items` — fell back to a plain `FF_BYTE` array via
  `idc.create_data`, which worked; the `STRTYPE_C` failure mode wasn't
  root-caused, flagged in roadmap.md in case it recurs.

Confirmed working end-to-end 2026-09-13: full identify → rename →
export → re-verify cycle, including one round-trip through a real
address mistake (a stale custom label's numeric suffix didn't match
its actual address — see "A hard-won lesson" below) and one structural
fix (the wind-string array split).

### A hard-won lesson: don't trust a label's numeric suffix as its address

Twice during this session, an *already-named* symbol's own name
implied a wrong address: `start_0` (a custom name from a prior
session) is actually at `0x15CE0`, not the `0x15D63` guessed from
manual byte-counting through the `.asm` text; `funcs_18E91` (an
IDA-auto-generated array name) is actually at `0x18E61`, not `0x18E91`.
Both were caught by dry-running `apply_renames.py` first (the "current
name" printed for the target address came back empty/wrong) rather
than assuming the guess was right. **Lesson for future sessions**:
when deriving an address by counting bytes through the `.asm` text
rather than reading it directly off a label IDA re-generated fresh
this session, verify with a throwaway `idc.get_name_ea_simple()` /
`idc.get_word()` lookup script before trusting it in `apply_renames.py`
— manual hex arithmetic through a few hundred lines of listing is
exactly the kind of thing that's off by a byte, and a stale custom
name's suffix is not guaranteed to match its address at all (only a
genuine, never-renamed `sub_XXXXX`/`byte_XXXXX`/`word_XXXXX` auto-name
is guaranteed accurate, since IDA generates those directly from the
address every time it's displayed).

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

### Session 2026-09-13: full function-naming sweep, 54/54 named

Read the entire `ultima.asm` (2,845 lines) top to bottom and traced
every one of the 54 previously-unnamed functions. Full evidence for
each is in `ida_scripts/apply_renames.py`'s `note` field; summarized by
subsystem here.

**Text/graphics output primitives** (matching ultima1/ultima2's naming
where the role is identical): `writeString`, `writeStringPreserveCx`,
`swapCursorPos`, `writeCharacter`, `drawCharGlyph`, `readLine` (an
interactive line editor with backspace/Ctrl-Backspace/Del/F3 handling),
`initGraphicsMode` (CGA mode set + an EGA-vs-CGA memory probe that
relocates the framebuffer segment on non-CGA hardware), `drawTileGrid`
(a generic N×M 64-byte-CGA-tile blitter — see "ULTIMA.COM's real role"
above for why this is significant), `plotPixel2bpp`, `drawSparkleBox`
(a shimmering box-reveal effect with a synchronized PC-speaker click).

**Boot-sequence orchestration**: `titleScreenAndChainToBootup` (was
`start_0` — see "ULTIMA.COM's real role" above), `waitFrames`,
`drawAnimatedPixelPath`, `runBootFlagAnimation`, `drawAnimationFrameRow`,
6 `drawTitleBoxN` wrappers over `drawSparkleBox` (exact visual roles
not confirmed without running the game — flagged in roadmap.md), plus
5 periodic idle-animation updaters (`updateLogoAnimationA`-`D`,
`swapAnimTableRows`) driven by `runIdleAnimationTick` /
`pollKeypressAndAnimate` / `getKeypressAndWaitRaw` while waiting for a
keypress.

**Resolved a real Ultima III game mechanic**: `updateWindDisplay`
(formerly `sub_18E0A`) cycles a wind-direction indicator ("Calm Wind"/
"North Wind"/"East Wind"/"South Wind"/"West Wind") every 25 ticks via
`WIND_DIRECTION_TABLE`, a 5-entry pointer table at `0x18E00` indexing 5
strings at `0x1623D`-`0x1627D` (`aCalmWind`/`aNorthWind`/`aSouthWind`/
`aEastWind`/`aWestWind`). The selection order the table encodes (Calm,
North, East, South, West) deliberately differs from the strings'
in-memory layout order (Calm, North, South, East, West) — confirmed by
reading the table's actual word values via `ida_bytes.get_word()`, not
just inferred from position. This is Ultima III's classic wind
mechanic (affects ship movement), shown decoratively on the title
screen ahead of the real game.

**Sound**: `playSoundEffect` dispatches effect IDs `0F4h`-`0FFh` through
`SOUND_EFFECT_TABLE` (12 entries, at `0x18E61` — NOT `0x18E91`, see "A
hard-won lesson" below) to 12 tone-generator functions. Only
`playErrorBeep` (`0FEh`) has a confirmed call site in this file (used
by `readLine`, the orphaned numeric-input prompt, and
`openFileWithRetry`'s "Wrong Diskette!" prompt); the other 11
(`playToneF4`-`playToneFF` minus `FE`) are named generically by effect
ID pending a `BOOTUP.BIN` cross-reference, per the confidence-discipline
convention — see "ULTIMA.COM's real role" above for why they're
plausibly reachable only from there.

**File I/O**: `loadFile` (FCB-based loader, same architectural role as
ultima2's `access_file`, but no inline-data-after-CALL trick — the
filename is passed normally, not embedded in the code stream) and
`openFileWithRetry` (the "Wrong Diskette!" floppy-swap retry loop).

**A correction to file-formats.md**: `drawAnimatedPixelPath` (formerly
`sub_15E68`) reads its data directly from the `NAME.DAT`-loaded buffer
as a stream of (x,y) coordinate pairs, plotting a path pixel by pixel
with a wait between each — the classic hand-drawn logo-reveal effect.
This **contradicts** file-formats.md's external-source-derived guess
that `NAME.DAT` is a "random name-generator table" — direct
disassembly evidence now says otherwise. Updated in file-formats.md.

**Open, not forced**: 4 functions/regions got low-confidence names with
explicit uncertainty notes (`checkDebugModeFlag`, `adjustAnimSpeed`,
`computeAnimTableByte`, and the 6 `drawTitleBoxN` wrappers' exact
visual roles) rather than invented specific meanings — see each one's
`note` in `apply_renames.py`. Additionally, at least 2-4 regions of
code exist that IDA never recognized as functions at all (no `proc`/
`endp` boundary) — a numeric hex/decimal-entry prompt (~`0x18C79`,
partially named via `accumulateInputDigit`/`printHexByte`/
`printHexNibble`, which it calls), a raw `INT 13h` disk-sector-read
routine (~`0x1878C`, plausibly a copy-protection check, given the
"Wrong Diskette!" theme elsewhere), and at least one FCB-write "save
file" routine (~`0x18CD9`) mirroring `loadFile`. These need
`ida_funcs.add_func()` structural fixes before they can be named — see
[roadmap.md](roadmap.md).
