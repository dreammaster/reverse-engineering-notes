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

Status as of this pass (2026-09-13, via `ida_scripts/identify.py`):
**`ultima.idb` and `ultima_bootup.idb` 100% function-named** (58/58 and
73/73); **`ultima_exodus.idb` created and its entire shared low-level
runtime named** (47/142 — the remaining 95 are `EXODUS.BIN`-specific
game logic: overworld/town/dungeon movement, combat, spells, shops,
temples, the ending sequence — a much larger follow-up effort, see
[roadmap.md](roadmap.md)). One struct defined so far (`RosterEntry`, in
`ultima_bootup.idb`).

## Three executables, three IDBs (one pending)

Following the "ULTIMA.COM's real role" finding below, this project has
the same shape as `ultima1` (one IDB per DOS executable) rather than
`ultima2`'s single-IDB shape it was originally scaffolded to match —
`ida_scripts/run_ida_script.ps1`/`batch_run_and_export.py` were
generalized 2026-09-13 to take a `-Idb` parameter (ported directly from
`ultima1`'s equivalent driver) rather than staying hardcoded to
`ultima.idb`.

| IDB | Root file | Role | Functions named |
|---|---|---|---|
| `ultima.idb` | `ULTIMA.COM` | Title screen / boot loader, chains to BOOTUP.BIN | 58 / 58 (100%) |
| `ultima_bootup.idb` | `BOOTUP.BIN` | Character creation / party management, chains to EXODUS.BIN | 73 / 73 (100%) |
| `ultima_exodus.idb` | `EXODUS.BIN` | The overworld/town/dungeon/combat/spell/shop/temple engine and ending sequence | 47 / 142 (shared runtime only — game logic pending) |

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
  code, per the naming convention doc there). Both `apply_renames_ultima.py`
  and `apply_renames_bootup.py` are complete (`DRY_RUN = False`) —
  every function/global/table/string in both IDBs. `apply_structs_ultima.py`
  is empty (this executable has no player-state data); `apply_structs_bootup.py`
  is also currently empty since `RosterEntry` was created directly via
  the one-off `create_roster_struct.py` (struct *creation* isn't in
  scope for `apply_structs_*.py`, which only edits an existing struct's
  members — see that script's docstring) — future `RosterEntry` member
  edits belong in `apply_structs_bootup.py`.
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
`note` in `apply_renames_ultima.py`. Additionally, 3 regions of code
exist that IDA never recognized as functions at all (no `proc`/`endp`
boundary) — a numeric hex/decimal-entry prompt (calls
`accumulateInputDigit`/`readLine`), a raw `INT 13h` disk-sector-read
routine (plausibly a copy-protection check, given the "Wrong
Diskette!" theme elsewhere), and an FCB-write routine mirroring
`loadFile`. One (`printHexWord`) has since been fixed via
`ida_funcs.add_func()` and confirmed against `ultima_bootup.idb`'s
identical, properly-bounded copy — see the next section and
[roadmap.md](roadmap.md) for the other two, still open.

### Session 2026-09-13: `ultima_bootup.idb` created and fully swept (73/73 named)

Following the finding above, created a second IDB for `BOOTUP.BIN` (see
"Two executables, two IDBs" above) and read the entire exported
`ultima_bootup.asm` (4,058 lines) top to bottom. Headline results:

**`BOOTUP.BIN` is the character-creation and party-management program**
— its own title screen (same intro-story-plus-boot-animation shape as
`ULTIMA.COM`'s, loading `CHARSET.ULT`/`SHAPES.ULT`/`DEMO.ULT`/
`MOVES.ULT`/`ROSTER.ULT` this time instead of the `.IBM`/`ANIMATE.DAT`
title assets) leads into `showMainMenu`: **Return to the View** / **Organize
a Party** / **Journey Onward**. "Organize a Party" (`showPartyOrganizationMenu`)
drills into `showRegister` (list all 20 roster entries), `handleCreateCharacter`,
`handleFormParty`, `handleDisperseParty`, `handleTerminateCharacter`, and
`showCharacterDetails` ("Look at a Character"). None of this is the
overworld/combat/dungeon engine — see "Chained executables" below for
where that actually lives.

**The intro credits resolve a dating question**: `titleScreenAndMainMenuLoop`
(formerly `start_0`) displays "(C)-1983 By James R. Van Artsdalen and
Lord British" — James R. Van Artsdalen is a known Origin Systems-era DOS
programmer, confirming this codebase's authorship dates to 1983 despite
the 1991-10-01 file dates on disk (a later recompile/re-release, not a
rewrite) — see [roadmap.md](roadmap.md)'s open questions.

**Confirms the shared-runtime hypothesis completely.** Roughly 40 of
`BOOTUP.BIN`'s 73 functions are structurally identical, line-for-line,
to functions already named in `ultima.idb`: `writeString`/
`writeCharacter`/`drawCharGlyph`/`drawTileGrid`/`readLine`/`loadFile`/
`openFileWithRetry`/`printHexByte`/`printHexNibble`, the entire boot/idle
animation cluster (`stepTimeSeededPrng`, `swapAnimTableRows`,
`updateLogoAnimationA`-`D`, `runIdleAnimationTick`, `pollKeypressAndAnimate`,
`getKeypressAndWaitRaw`), `updateWindDisplay`/`WIND_DIRECTION_TABLE`
(same 5-string Calm/North/East/South/West mechanism, different
addresses), and the full 12-entry `playSoundEffect`/`SOUND_EFFECT_TABLE`
cluster. Same source, recompiled/relinked into a separate executable —
not the same bytes at the same addresses, but the same logic verified
line-by-line, not assumed from names alone. Two functions found here
have **no proc boundary in `ultima.idb`** (`printHexWord`,
`promptForNumberEntry`) but a real, confirmed caller here
(`showCharacterDetails`'s stat display; `getEntryNumber`'s "Entry#"
prompts respectively) — resolving those as genuine shared-runtime code
rather than dead weight (see `ultima.idb`'s "hard-won lesson" section
and [roadmap.md](roadmap.md)). `saveFile` (the FCB-write counterpart to
`loadFile`) is new here — `ULTIMA.COM`'s title screen never writes a
file, `BOOTUP.BIN` writes `PARTY.ULT`/`ROSTER.ULT` from 5 confirmed
sites.

**`RosterEntry` struct defined** (`ida_scripts/create_roster_struct.py`,
64 bytes), every field confirmed against a real `[bx+N]` access in
`showCharacterDetails` or `handleCreateCharacter` — matches
[file-formats.md](file-formats.md)'s externally-sourced ROSTER.ULT byte
layout exactly, field for field, for every offset checked (`_name`,
`_partyMember`, `_status`, the 4 attributes, `_race`, `_class`, `_sex`,
`_hitPoints`/`_maxHitPoints`, `_experience`, `_food`, `_gold`,
`_armourIndex`/`_armourOwned`, `_weaponIndex`/`_weaponOwned`). This is
the first time that external documentation has been confirmed against
real code rather than just matched by file size. `handleCreateCharacter`
also confirms the exact starting values for a new character: Status='G'
(Good), HP=Max HP=Food=Gold=150 (BCD), Weapon=Armour=index 1 with 1
owned, 50-point attribute pool with per-attribute min/max range checks
(`gatherCharacterCreationInput`) — real, played-out Ultima III character
creation rules, not guessed.

**Chained executables, now three deep.** `handleJourneyOnward` (formerly
`sub_11437`) confirms `EXODUS.BIN` is loaded and chained into exactly
the way `ULTIMA.COM` chains to `BOOTUP.BIN` — DTA set to `0x100`, FCB
opened, record size set to the whole file, self-modifying `INT 21h`
stub — but with one twist: instead of falling straight through to
offset `0x100` afterward, it does an **indirect** jump
(`JMP WORD PTR [1328h]`) through a 2-byte vector stored at a fixed
offset inside `EXODUS.BIN`'s own freshly-loaded bytes. `EXODUS.BIN`
self-describes its own entry point rather than just starting at byte 0
— worth keeping in mind when disassembling it (don't assume execution
starts at the segment's first instruction). Since neither `ULTIMA.COM`
nor `BOOTUP.BIN` contains any overworld/combat/dungeon code, `EXODUS.BIN`
is now the clear next target and holds the actual game-world engine —
see [roadmap.md](roadmap.md).

### Session 2026-09-13: `ultima_exodus.idb` created, shared runtime named (47/142)

Created the third IDB the same way as `ultima_bootup.idb` (copy to a
temp `.com` file for IDA's loader auto-detection — confirmed loading at
the same `0x10100`-`0x1ADCA` range convention). This one is far bigger:
**144 functions, ~14,600 `.asm` lines** — roughly 2x `BOOTUP.BIN` and
5x `ULTIMA.COM` combined.

**The real entry point had no function boundary at all**, unlike the
previous two IDBs' `start_0`-equivalents. `BOOTUP.BIN`'s
`handleJourneyOnward` reaches `EXODUS.BIN` via an indirect jump through
a vector at file offset `0x1228` (see the previous section) — since
nothing *within EXODUS.BIN itself* calls that address, IDA's
auto-analysis had marked it as plain data (`byte_124C4`), not code.
Fixed manually: `idc.plan_and_wait()` over a 0x400-byte range to force
disassembly, then `ida_funcs.add_func()`, then named
`entryFromBootup`. Confirms the vector points at a real, coherent boot
routine (same `INT 23h`/`24h` vector-patching and stack setup as
`titleScreenAndChainToBootup`/`titleScreenAndMainMenuLoop`), loading
`CHARSET.ULT`/`SHAPES.ULT`/`PARTY.ULT` and going straight into gameplay
— no decorative title/intro screen this time, unlike the previous two.

**Confirms the shared-runtime hypothesis a third time, exhaustively.**
Read through the entire low-level runtime cluster (`writeString`
through the sound-effect table, ~40 functions) line-by-line and found
it byte-for-byte structurally identical to `ultima_bootup.idb`'s copy —
same text/graphics primitives, same file I/O, same boot/idle animation
cluster, same 5-string wind display, same 12-entry sound table. All
named directly from the known correspondence rather than re-derived
from scratch (`ida_scripts/apply_renames_exodus.py`). One new function
found here that's an anonymous orphan in the other two IDBs:
`clearFramebuffer` (clears both CGA banks + resets cursor) has a real
proc boundary in this IDB, so it got a name here for the first time.

**A genuine memory-reuse trick, not a disassembly error**:
`drawLogoTileGrid`'s 11×11 source tile-index buffer is `entryFromBootup`
itself — the boot routine's own code bytes, deliberately read as
scratch tile-index data once the boot routine has run exactly once.
Worth remembering if `entryFromBootup`'s bytes ever look overwritten
during later analysis.

**The string table confirms this is the complete game-world engine** —
overworld/town/dungeon map filenames (`SOSARIA.ULT` through
`DARDIN.ULT`, all 19 `.ULT` map files), all 9 `CNFLCT_*.ULT` combat
arenas, `DUNGEON.DAT`, the 4 special-location `.IMG` files
(`SHRINE`/`TIME`/`FOUNTAIN`/`BRAND`), movement (`North`/`South`/`East`/
`West`, "Mount Horse!", "Board Frigate!"), a large single-key overworld
command dispatcher (evidenced by dozens of strings all attributed to
one function, `sub_17B54`, at huge internal offsets — "Craft", "Cmd: ",
"D, S, L, M:", "F, G, E, W, A:", trap messages, NPC interaction
prompts), a separate combat-specific dispatcher (`sub_123A5` —
"Attack", "Missed!", "Ready a weapon!", "Cast Spell!", "Negate Time!",
"Ztats", "Pass"), spellcasting (`sub_15D83` — "Not a mage!", "Spell
type W/C-", "Cleric spell-", "Wizard spell-", "M.P. too low!"), shops
(`sub_1A630` — "Ye local Grocer..."), temples (`sub_1A692` — "Cure/
Heal/Resurrect/Recall whom?"), and the game's ending sequence text
("EXODUS:", "Seek ye out the...", "Shrines of Knowledge", "Fountains
fair..."). This resolves a `docs/file-formats.md` naming discrepancy
noted earlier: both `AMBROSIA.ULT` (on disk) and `FAWN.ULT`/
`EXODUS.ULT` (referenced here as strings, not present on disk in this
release) exist in the string table, suggesting a version/release
difference rather than an error in either source — not fully resolved,
noted in file-formats.md.

**Scope reality check**: this executable alone is comparable in size to
`ultima1`'s or `ultima2`'s entire sibling projects (which took many
sessions each, per their git history). The remaining functions
include several genuinely massive ones (`sub_17B54` spans thousands of
bytes by itself) — full identification is a multi-session effort, not
a single pass. See [roadmap.md](roadmap.md) for the prioritized plan.

### Session 2026-09-13 (continued): combat, spellcasting, and the overworld main loop

A single extended pass (65/144 named by the end) found and traced four
major subsystems end to end:

**The combat encounter system, in full.** `beginCombatEncounter`
(triggered when `updateMonsterAI` moves a monster onto
`_partyPosition`) selects and loads one of all 9 `CNFLCT_*.ULT` arena
files via a decision tree on monster type, places party and monsters
in the confirmed 11×11 arena, and hands off to `combatTurnLoop` —
per-player turns dispatched through a genuine 33-entry
`COMBAT_COMMAND_TABLE` (`readCombatCommandKey`), with 8 commands
identified: Pass, 4 movement/flee directions, Ready a weapon, Ztats,
Negate Time, Cast Spell, Attack (the large damage-resolution case), and
an invalid-command catch-all.

**Spellcasting, in full.** `castSpell` gates spell access by character
class (Druid/Ranger choose Wizard-or-Cleric; Cleric/Paladin/Illusionist
locked to Cleric; Wizard/Lark/Alchemist locked to Wizard; everyone else
"Not a mage!"), charges a BCD magic-point cost against a newly-found
`RosterEntry` field (`_magicPoints`, offset `0x19` — filled a real gap
in the struct, confirmed here and back-filled into `ultima_bootup.idb`
too, since IDA structs are per-database), and dispatches to one of two
per-type spell-effect jump tables (`WIZARD_SPELL_TABLE`/
`CLERIC_SPELL_TABLE`, individual spells not yet traced).

**`printGameText`, the game's real text-output workhorse** — word-wrap
and window-scroll logic distinct from the lower-level `writeString`,
called from nearly every subsystem. Also found and named 3 BCD
stat-arithmetic helpers (`healHitPoints`, `addExperienceClamped`,
`addGoldClamped`) that independently confirm `_maxHitPoints`/
`_experience`/`_gold`'s struct offsets via real arithmetic rather than
just display code, and a per-turn party-upkeep function
(`processPartyTurnEffects`: class-gated magic-point regeneration, plus
poison/hunger effects not fully traced).

**The overworld main game loop, found and largely mapped.**
`mainGameLoop` is confirmed as the top-level "wait for a command,
dispatch it" loop, driving a genuine 33-entry
`OVERWORLD_COMMAND_TABLE`/`OVERWORLD_COMMAND_KEYS` pair (the key table
was originally misdecoded by IDA as garbage x86 instructions — it's
word-sized scancode:char entries, not bytes, fixed via
`fix_command_key_tables.py`). 8 of the 33 slots are confirmed by
key/letter:

| Key | Handler | Role |
|---|---|---|
| Up/Down/Right/Left | `cmdMoveNorth` + 3 unconfirmed | movement (only North fully traced; South/East/West are `loc_11C9E`/`loc_11CBF`/`loc_11CE0`, same shape, not yet renamed) |
| Space | `cmdPass` | end turn, no other effect |
| B | `cmdBoard` | board a vehicle (confirmed via the "Mount Horse!"/"Board Frigate!" strings) |
| X | `cmdExitVehicle` | dismount/disembark — confirms `_currentTransport` (formerly `byte_114BA`) and its `0x3F`=on-foot convention, matching the external PARTY.ULT "Transport" field exactly |
| E | `cmdEnter` | enter a location — shrine entry confirmed (calls the already-known shrine function); dungeon/town/castle entry presumably shares this command, not traced past the shrine branch |
| V (+2 alt bindings) | `cmdToggleSound` | mute toggle, shared with an unidentified menu screen's jump table |

**Follow-up pass, same session**: confirmed the 3 remaining movement
directions (`cmdMoveSouth`/`cmdMoveEast`/`cmdMoveWest`, identical shape
to `cmdMoveNorth`), and made a major find tracing `cmdEnter` fully:
it's not shrine-only as first thought, it's **the entire dungeon/town/
castle entry system**. It scans a new 19-entry `LOCATION_TILE_TABLE`
against `_partyPosition` to identify which named location the party is
standing on, looks up the location kind in `_locationTypeTable`
(5=Dungeon, 6=Towne, 7=Castle), sets the appropriate starting position
and game mode, and loads the location's data — confirming two file
sizes exactly against `docs/file-formats.md` in the process (town/
castle maps: `0x1228` = 4,648 bytes, matching `SOSARIA.ULT`'s
documented size; dungeon maps: `0x890` = 2,192 bytes). Also discovered
that entering a dungeon loads **two** files — the dungeon's own numbered
map *and* a separate `0x800`-byte read of `DUNGEON.DAT` — confirming
`DUNGEON.DAT` is an auxiliary data file, not itself a map. `cmdCastSpell`
('C') is the overworld counterpart to `combatCmdCastSpell`, found while
tracing a neighboring table lookup.

22 command letters (A, D, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T,
U, W, Y, Z) still point at unnamed `loc_XXXXX` handlers — a concrete,
bounded checklist for continuing (see [roadmap.md](roadmap.md)), much
more tractable than reading `sub_17B54` linearly since each handler's
address and trigger key are now known.

**First game-specific function identified, and a correction to the
string-table-based guess above**: `updateMonsterAI` (formerly
`sub_123A5`) is **not** the combat command dispatcher the string scan
suggested — that was a false lead caused by IDA merging a far-away
(~0x6500 bytes distant), separately-located code chunk containing the
actual "Attack"/"Cast Spell"/"Ztats"/"Pass" combat-menu logic into this
function's chunk list (a real IDA behavior: a function can have
disjoint chunks at distant addresses, usually from a compiler placing
a cold path elsewhere, and IDA's own analysis attributed the strings'
DATA XREF comments to the chunk owner rather than the chunk itself).
`updateMonsterAI` itself is the per-turn monster/NPC movement AI:
iterates 32 slots over 4 parallel byte arrays at fixed offsets from a
shared base — `+0x1280` (monster type, 0 = empty slot), `+0x12A0`
(display tile), `+0x12C0` (X), `+0x12E0` (Y) — the same parallel-array
convention `ultima2` uses for its own monster tracking, not an
array-of-structs. Rolls a movement chance via `stepTimeSeededPrng`/
`adjustAnimSpeed`, checks the candidate position via two new low/
medium-confidence helpers (`getMapTileAt`, `canMoveToTile`), and
updates position + redraws on success. The **real** combat command
dispatcher — the absorbed chunk, around absolute address `0x18D0B` — is
now the actual next target, not `sub_17B54` as first assumed for
combat specifically (though `sub_17B54` remains the overworld command
dispatcher).
