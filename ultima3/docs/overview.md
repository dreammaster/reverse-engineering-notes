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

**Follow-up pass, same session**: 5 more commands confirmed —
`cmdExchange` ('M', swaps two party members' full combat records),
`cmdPeer` ('P', spends a Gem to view the dungeon/overworld layout),
`cmdQuit` ('Q', save-and-quit, restricted to the overworld surface),
`cmdSteal` ('S', ~25% success chance with a guard-alert failure
consequence, same pattern as `ultima2`'s `alert_town_guards`), and
`cmdUnlock` ('U', spends a Key on a locked-door tile). These
independently confirm three more `RosterEntry` fields via real
gameplay arithmetic: `_gems` (`+0x25`), `_keys` (`+0x26`), and
`_powder` (`+0x27`, via `combatCmdNegateTime`) — all matching
`docs/file-formats.md`'s externally-sourced field list exactly.

**Follow-up pass, same session**: 'D'/'K' turn out to be disabled on
the overworld (`cmdDisabledOnSurface` — both just fall through to the
invalid-command trampoline), consistent with Descend/Klimb being
dungeon-only commands reached through a different jump table
(`jpt_18389`, referenced repeatedly across this session's finds as an
alternate dispatch table but not yet itself identified — presumably
the in-dungeon command set). `cmdIgniteTorch` ('I') confirms a further
`RosterEntry` field, `_torches` (`+0x0F`), independently of external
documentation.

18 of 33 overworld commands are now confirmed. 15 letters (A, F, G, H,
J, L, N, O, R, T, W, Y, Z) remain — a concrete, bounded checklist for
continuing (see [roadmap.md](roadmap.md)), much more tractable than
reading `sub_17B54` linearly since each handler's address and trigger
key are now known.

### Session 2026-09-14: `dungeonMainLoop` found — the entire dungeon command system

`jpt_18389`, referenced repeatedly throughout the previous session's
finds but never itself identified, turns out to be **`DUNGEON_COMMAND_TABLE`**
— the dungeon counterpart to `OVERWORLD_COMMAND_TABLE`, dispatched from
**`dungeonMainLoop`** (structurally identical to `mainGameLoop`: idle-
animation poll, key lookup, jump-table dispatch), entered via
**`initDungeonState`** right after `cmdEnter`'s dungeon branch loads the
dungeon map + `DUNGEON.DAT`. Confirmed both structurally *and*
semantically: `dungeonMainLoop` checks `byte_115CE` — the exact flag
`cmdIgniteTorch` sets — and prints **"It's dark!"** every turn it's
unlit, the classic Ultima need-a-lit-torch mechanic.

`DUNGEON_COMMAND_TABLE` shares many handlers directly with the
overworld table (Pass, Cast Spell, Ignite Torch, Exchange, Toggle
Sound all work identically in both places) but disables **10** letters
outright — Board/Enter/eXit-vehicle/Quit-and-save/**Steal** disabled
underground all make immediate sense; Attack/Fire/Locate/Transact/
Unlock being disabled too is still unexplained.

**Correction, same session**: an initial dump of the table's addresses
for K/D/the arrow keys/S was wrong — a bug in a disposable exploratory
script (not one of the accumulating `apply_renames_*.py` files, so
nothing wrong was ever actually applied to an IDB) read past the
table's real 33-entry boundary into the adjacent `off_1778C` table,
producing plausible-looking but incorrect addresses. Caught and fixed
by re-deriving both tables' contents fresh with a careful,
double-checked script before writing anything down as fact — the
**real** 6 dungeon-specific handlers (K, D, and both arrow-turn/arrow-
move pairs; S turned out to be disabled, joining the shared stub) are
a beautifully clean, fully-confirmed classic first-person Ultima
dungeon movement system: `cmdKlimb`/`cmdDescend` (ladder-up/-down tile
checks, matching `docs/file-formats.md`'s dungeon tile encoding
exactly and confirming a new `_dungeonLevel` counter), `cmdTurnLeft`/
`cmdTurnRight` (rotate `_facingDirection` mod 4, no position change),
and `cmdMoveForward`/`cmdMoveBackward` (step in the facing direction
via a confirmed 4-entry `_dungeonFacingDeltaX`/`_dungeonFacingDeltaY`
table — `[0,1,0,-1]`/`[-1,0,1,0]`, a standard clockwise N/E/S/W
convention — blocked by a Wall tile, matching the file-formats.md
tile encoding again). **Lesson**: an exploratory one-off dump script
is not held to the same rigor as the accumulating rename scripts, but
its *output* still needs the same skepticism before being written into
documentation as fact — a second, careful pass caught this before it
became a permanent wrong claim.

**Follow-up, same session**: named `DUNGEON_COMMAND_LABELS`
(`off_1778C`, medium confidence — a per-command prompt-string table
loaded into `si` right before every dungeon command dispatches,
explaining why individual handlers don't always show their own
`lea si,aXxx` before their first `printGameText` call) and 3 more
overworld commands: `cmdAttack` ('A', prompts a direction and jumps
straight into `beginCombatEncounter` — the same combat path
`updateMonsterAI` uses automatically), `cmdFire` ('F', ship-only
cannon fire, steps a projectile up to 3 tiles), and `cmdGet` ('G',
picks up gold/treasure off the map). **21 of 33 overworld commands are
now confirmed** — 12 letters remain (H, J, L, N, O, R, T, W, Y, Z, plus
completing H's partial read).

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

**Three more overworld commands closed out, 2026-09-14**:
`cmdZtats` (`0x12068`, 'Z', index 10) calls the same `sub_16DC1` helper
combat's Ztats display uses, with the same `dh = playerIndex + 0x30`
calling convention — an out-of-combat character stat sheet.
`cmdWear` (`0x17EE4`, 'W', index 22, tentative) does a player-select
plus alive-check then calls `sub_17EFA`, not yet traced past that call
— named by elimination against the classic Ultima III command set and
slot position rather than confirmed effect.

`cmdYell` (`0x17458`, 'Y', index 31) is the more interesting one: it
prompts `"Word: "`, reads up to 9 characters via `readLine`, and looks
the entered word up against a keyword table (`byte_164FA`) via
`sub_1740A`/`sub_17423`. On a successful match it checks **bit `0x40`
of the selected character's record at offset `+0x0E`** (and additionally
requires `_partyPosition == 0x0A`, i.e. a specific map location) before
doing anything — the classic Ultima III "Yell a word" mechanic (e.g.
yelling "RAMA" at a specific spot to trigger something). This is the
first real-code confirmation of `docs/file-formats.md`'s
externally-sourced "Marks/cards bitmask" field at that offset (previously
carried in `RosterEntry` as an unlabeled gap between `_name` and
`_torches`) — added to the struct as `_marksAndCards`
(`ida_scripts/create_roster_struct.py`, applied to both
`ultima_bootup.idb` and `ultima_exodus.idb`). Individual bit meanings
beyond `0x40` aren't decoded yet.

**32 of 33 overworld commands are now confirmed.** Only 'H'
(`loc_11E55`) remains unresolved — see roadmap.md's open-items list for
what's known about it (prompts two players, recursively re-enters the
dispatcher `sub_17B54` if they differ, purpose not pinned down).

**All 33 of 33 overworld commands confirmed, 2026-09-14** — 'H' closed
out, plus one earlier misnaming caught along the way. Rather than keep
guessing at 'H' from its code alone, wrote a small read-only dump
script (`ida_scripts/dump_overworld_labels.py`) to read the per-command
prompt-string table directly: `sub_17B54`'s dispatcher does
`mov si, [bx+18C9h]` right before `jmp OVERWORLD_COMMAND_TABLE[bx]`,
loading a prompt string pointer from a 33-entry word table parallel to
`OVERWORLD_COMMAND_TABLE`/`OVERWORLD_COMMAND_KEYS` — the overworld
counterpart of `DUNGEON_COMMAND_LABELS`. **Gotcha worth recording**:
the raw operand `18C9h` looks like it should be a linear address in
this segment but isn't — `[bx+18C9h]` is DS-relative, and this IDB's
load segment is `0x1000` (per `identify.py`'s "entry point: 0x100
(cs=0x1000)"), so the real linear address is `0x1000*16 + 0x18C9 =
0x118C9`, comfortably inside the loaded segment range
(`0x10100`-`0x1ADCA`). The table's own word entries are near pointers
under the same convention (`linear = 0x10000 + ptr`). Reading all 33
entries against the already-known `OVERWORLD_COMMAND_KEYS` gave a
full, direct confirmation of every command's on-screen prompt text in
one pass:

| Idx | Key | Prompt | Idx | Key | Prompt |
|---|---|---|---|---|---|
| 0 | (arrow) | "North" | 17 | T | "Who will\nTransact? " |
| 1 | A | "Attack-" | 18 | U | "Unlock-" |
| 2 | (arrow) | "South" | 19 | I | "Ignite a torch" |
| 3 | (arrow) | "East" | 20 | Q | "Quit & Save" |
| 4 | (arrow) | "West" | 21 | R | "Ready for #" |
| 5 | (space) | "Pass" | 22 | W | "Wear for #" |
| 6 | B | "Board" | 23 | C | "Cast by whom-" |
| 7 | X | "X-it" | 24 | F | "Fire" |
| 8 | V | "Volume" | 25 | J | "Join gold to:" |
| 9/14 | (alt) | "Volume" | 26 | D | "Descend" |
| 10 | Z | "Ztats for #" | 27 | N | "Negate Time!\nWhose Powd? " |
| 11 | E | "Enter" | 28 | G | "Get Chest!\nPlr to search-" |
| 12 | L | "Look-" | 29 | P | "Peer at gem!\nWhose gem? " |
| 13 | M | "Modify order!\nPlayer: " | 30 | S | "Steal Chest!\nPlayer? " |
| 15 | K | "Klimb" | 31 | Y | "Yell, whom? " |
| 16 | H | "Hand Equipment!\nFrom Player: " | 32 | O | "Other command!\nWhose action? " |

Two results stood out:

- **Index 16, key 'H': "Hand Equipment!\nFrom Player: "** — finally
  explains the otherwise-mysterious double player-select in
  `loc_11E55` (now renamed `cmdHandEquipment`): it prompts a "from"
  player, then (via the already-known `aToPlayer` string, "  To
  Player: ") a "to" player. If they're the same player it takes the
  shared `loc_17DBA` no-op path; if different, it makes a genuine
  re-entrant `call near ptr sub_17B54` — a call back into the *top* of
  the overworld command dispatcher itself. Confirmed this isn't a
  parameter-passing trick: `sub_17B54`'s prologue unconditionally
  pushes `ax`/`bx`/`cx`/`dx`/`bp`/`si`/`di`/flags, so nothing survives
  the recursive call in registers. The actual weapon/armour hand-off
  between the two selected characters must therefore happen through a
  global "hand mode" flag (not yet located) that some subsequent
  single-key command — `cmdWear` is the obvious candidate, given the
  shared equipment-slot theme — checks to redirect its normal
  single-player behavior into a transfer between the two
  globally-remembered players. This follow-on mechanism is flagged as
  the next concrete lead, not guessed at further.
- **Index 32, key 'O': "Other command!\nWhose action? "** — caught a
  real misnaming. `cmdOrder` (`0x174D5`) had been named on the guess
  that "typed keyword command" plus letter 'O' meant party-order
  related; the actual prompt has nothing to do with order at all, and
  the genuine party-reorder command is `cmdExchange` (`0x11E9B`, key
  'M', prompt "Modify order!\nPlayer: ", which does a confirmed
  byte-for-byte swap of two characters' records). **Renamed `cmdOrder`
  → `cmdOtherCommand`** to match the real prompt. Its mechanism is
  otherwise unchanged from what was already known: player-select,
  alive-check, prompt "Cmd: ", read a 10-character line, and look it
  up via `sub_1740A` — the *same* keyword-lookup helper `cmdYell` uses,
  against a *different* table (`[bx+6540h]`, linear `0x16540`, vs.
  `cmdYell`'s `byte_164FA`) and without the `_marksAndCards` bit-check.
  Reads like a second, more general "say a keyword" interaction
  distinct from Yell's location-gated one — not fully traced past the
  lookup itself.

`ida_scripts/dump_overworld_labels.py` is kept in the repo (read-only,
no IDB modifications) as living documentation of this table, the same
way the other one-off `fix_*`/`dump_*` scripts are kept.

**`DUNGEON_COMMAND_LABELS` confirmed, same session**: ran the same
technique against the dungeon side (`ida_scripts/
dump_dungeon_labels.py`) — `dungeonMainLoop`'s dispatcher does
`lea si, DUNGEON_COMMAND_LABELS; mov si, [bx+si]` right before `jmp
DUNGEON_COMMAND_TABLE[bx]`, so no manual segment math was needed for
the table base this time (only its near-pointer string entries, same
`linear = 0x10000 + ptr` convention). This closes the "worth
confirming and naming next" item flagged in the dungeon-command
correction writeup above — the table is exactly the per-command prompt
string array hypothesized, and it double-confirms two earlier findings
independently:

- All 10 dungeon-disabled letters (`B`, `A`, `E`, `F`, `L`, `Q`, `T`,
  `U`, `X`, `S` — `cmdDisabledInDungeon`) share one prompt string,
  `"Not a DNG cmd!\n"` — consistent with them all routing to the same
  handler.
- Every enabled dungeon command's prompt matches its already-confirmed
  handler exactly, including the 6 movement handlers whose addresses
  were the subject of this session's self-caught correction (see
  above): `cmdKlimb`="Klimb", `cmdDescend`="Descend",
  `cmdTurnRight`="Turn right", `cmdTurnLeft`="Turn left",
  `cmdMoveForward`="Advance", `cmdMoveBackward`="Retreat" — an
  independent, prompt-text-based confirmation that the corrected
  addresses are right, from a completely different table than the one
  that originally caught the bug.

'H' (`cmdHandEquipment`) and 'O' (`cmdOtherCommand`) both share their
overworld prompts in the dungeon context too (dungeon indices 3 and 8
respectively), i.e. Hand Equipment and the typed "Other command" both
work underground as well as on the surface.

**All 32 spell slots named, 2026-09-14** (`WIZARD_SPELL_TABLE`/
`CLERIC_SPELL_TABLE`, 16 entries each). `castSpell` restricts the typed
spell letter to `'A'..'P'` (`cmp ah,41h` / `cmp ah,50h` at
`castSpell+89`/`+92`) before subtracting `'A'` for a 0-based index —
this is what actually proves both tables are exactly 32 bytes, not an
assumption from where a following label happens to sit. Worth
recording: an earlier version of this dump (`dump_spell_tables.py`)
walked each table's byte range until IDA's next *named* symbol and
silently over-read `CLERIC_SPELL_TABLE` by 16 bytes into an unrelated,
unlabeled table (`aStrength`/`aDexterity`/`aIntelligence`/`aWisdom`
pointers happening to sit right after it) — the exact same class of
mistake as the dungeon-command-table mixup earlier this session,
caught the same way: re-derive the boundary from the dispatching code
itself, not from data layout.

Each spell's magic word is directly readable from the binary via a
combined 32-entry name-pointer table, `SPELL_NAME_TABLE` (linear
`0x1590B`), which `castSpell` reads via `mov si, [si+590Bh]` (index =
local index + 0 for wizard, +0x10 for cleric) to print the spell's name
before dispatching to its effect routine via `jmp word ptr [di]`. All
32 words were cross-checked against `C:\games\ultima3\ULTIMA3.TXT`'s
in-game spellbook manual and match exactly (one likely manual typo:
"SANTU MANI" in the text vs. the binary's "Sanctu Mani"):

| Ltr | Wizard word | Effect (per manual) | Ltr | Cleric word | Effect (per manual) |
|---|---|---|---|---|---|
| A | Repond | Dispel Orcs/Goblins/Trolls | A | Pontori | Dispel Undead |
| B | Mittar | Magic missile attack | B | Appar Unem | Open chest safely |
| C | Lorum | Light (short) | C | Sanctu | Minor heal |
| D | Dor Acron | Descend 1 dungeon level | D | Luminae | Light (short) |
| E | Sur Acron | Ascend 1 dungeon level | E | Rec Su | Ascend 1 dungeon level |
| F | Fulgar | Fireball attack | F | Rec Du | Descend 1 dungeon level |
| G | Dag Acron | Random teleport (surface) | G | Lib Rec | Teleport within dungeon |
| H | Mentar | INT-scaled mind attack | H | Alcort | Cure poison |
| I | Dag Lorum | Light (long) | I | Sequitu | Recall to surface |
| J | Fal Divi | "Unlocks Cleric book" (flavor) | J | Sominae | Light (long) |
| K | Noxum | Multi-target attack | K | Sanctu Mani | Heal near-death |
| L | Decorp | Single-target kill | L | Vieda | Reveal surroundings |
| M | Altair | Stop time | M | Excuun | Single-target kill |
| N | Dag Mentar | Multi-target INT attack | N | Surmandum | Resurrect (risk: ashes) |
| O | Necorp | Powerful attack | O | Zxkuqyb | Powerful "anti-creation" kill |
| P | *(unused — see below)* | | P | Anju Sermani | Restore from ashes (-5 Wisdom) |

Two things stood out enough to flag explicitly:

- **6 of the 32 slots share their effect routine with the other
  class's same-purpose spell** (confirmed via `check_spell_addrs.py`,
  reading each table's raw pointers and diffing addresses — no other
  collisions found): Wizard "Lorum"/Cleric "Luminae" (both short
  lights), Wizard "Dor Acron"/Cleric "Rec Du" (both descend-a-level),
  Wizard "Sur Acron"/Cleric "Rec Su" (both ascend-a-level), Wizard "Dag
  Lorum"/Cleric "Sominae" (both long lights), Wizard "Decorp"/Cleric
  "Excuun" (both single-target kills), and — the more interesting one
  below — Wizard's unused 'P' slot and Cleric "Zxkuqyb". Makes sense:
  the game only needed one implementation per *effect*, reused across
  whichever spell letters/classes call for that effect.
- **Wizard letter 'P' is not a documented spell** — `ULTIMA3.TXT`'s
  wizard spell list stops at 'O', and `SPELL_NAME_TABLE`'s entry for
  Wizard index 15 (letter P) resolves to an empty string. But
  `castSpell`'s letter check is a generic `'A'..'P'` range test with no
  per-class upper bound, so selecting 'P' as a Wizard is structurally
  possible — and its table entry points at the exact same routine as
  Cleric's "Zxkuqyb", described in the manual as the most powerful
  attack spell in the game ("the second words of anti-creation...
  [that] will end this life, and all other potential lives within
  them"). Whether this is a real, exploitable quirk of the shipped
  game (a free/mistakenly-costed instant-kill for Wizards) or just an
  artifact of how the two tables happen to be packed in memory isn't
  confirmed either way — flagged as a concrete, interesting lead, not
  asserted as a discovered bug.

The MP-cost formula was also confirmed directly from `castSpell`'s own
arithmetic (not from the manual): `cost = localIndex * 5`, computed via
`mul`+`aam` and packed to BCD, checked against `RosterEntry+0x19`
(`_magicPoints`) before a BCD-subtract — so within either class's own
16-spell list, letter A costs 0 MP and letter P costs 75 MP BCD,
increasing by 5 per letter.

All 32 spell-effect addresses plus `SPELL_NAME_TABLE` are now named
(`spellRespond`, `spellMittar`, ... — see
`ida_scripts/apply_renames_exodus.py` for the complete list with
per-entry evidence). None of them were recognized by IDA as function
starts (same situation as the overworld/dungeon command handlers) —
renamed as plain labels via `idc.set_name`, same as those. **Still
open**: each routine's actual mechanic is sourced from the manual's
flavor text only, not independently confirmed by reading the routine's
own code — a well-bounded next target now that every address has a
name, and LairWare's `UltimaSpellCombat.c` is available as a secondary
cross-reference for expected behavior.

**Combat damage-resolution fully traced, same session**: read the code
around `updateMonsterAI`'s attack-resolution chunk end-to-end rather
than guessing from shape alone. The 8-combatant combat arena turns out
to be a set of parallel byte arrays at a fixed base (`[reg+24C4h]`,
DS-relative): `+0x80`=X, `+0x88`=Y, `+0x90`=display tile,
`+0x98`=occupied-flag/HP-or-count — a smaller, combat-local cousin of
`updateMonsterAI`'s own 32-slot overworld monster arrays
(`+0x1280`/`+0x12A0`/`+0x12C0`/`+0x12E0`), not the same structure.
Named 4 helpers:

- **`findCombatantAtPosition`** (`0x18E46`) — given an (X,Y), searches
  the 8 slots and returns the matching one's index via a small trick:
  `lea bx, entryFromBootup; sub si, bx` — `entryFromBootup`'s own
  near-offset within this segment happens to equal the arena array's
  base offset, so the subtraction cancels it out and leaves the plain
  0-7 loop index in `bx`, without a separate constant. Returns
  `0xFFFF` if no occupied slot matches.
- **`fireProjectileAcrossArena`** (`0x18E7A`) — called from
  `combatCmdAttack`'s own chunk (ranged weapons) and from
  `updateMonsterAI` (monster ranged attacks). Steps a projectile's
  position by a fixed `(ch,cl)` delta per iteration, redrawing via
  `drawLogoTileGrid` and testing `findCombatantAtPosition` after each
  step, stopping when either axis reaches `0x0B` (leaving the 11×11
  arena — matches `drawTileGrid`'s confirmed dimensions) or a
  combatant is found.
- **`applyCombatDamage`** (`0x18F5A`) — subtracts a damage amount from
  a target slot's `+0x98` field as **plain binary, not BCD**
  (confirmed: no `das` follows the `sub`, and `beginCombatEncounter`
  initializes this same field via `or dl, 0Fh` — an invalid BCD
  nibble, ruling out a decimal interpretation); on death, prints
  "Killed! Exp.+", clears the slot, and calls `addExperienceClamped`
  with a BCD amount read from `MONSTER_EXP_TABLE` (see
  `dump_monster_tables.py`, below) indexed by `_conflictMonsterClass &
  0xFh` (`[bx-79EBh]`, linear `0x18615`). Has one unexplained special
  case: skipped entirely when `_conflictMonsterClass == 0x13` — flagged, not
  investigated (possibly a scripted/indestructible monster, maybe
  Exodus itself, but not confirmed).
- **`applyRandomGroupDamage`** (`0x15EAA`) — confirmed shared by
  `spellRespond` and `spellNoxum`: its two `CODE XREF` comments
  (`seg000:5FCBh`/`seg000:60B1h`) resolve to linear addresses
  `0x15FCB` and `0x160B1`, which fall *inside* `spellRespond`'s
  (`0x15F9F`-`0x15FD3`) and `spellNoxum`'s (`0x160A5`-`0x160B9`) own
  address ranges respectively — i.e. those two spells call this helper
  from within their bodies. It loops all 8 arena slots and, for each
  occupied one, rolls `and dl,3` and applies damage via
  `applyCombatDamage` unless the roll is exactly 0 — roughly a 3-in-4
  chance per occupied slot, not the 1-in-4 a naive reading of "and
  with 3" might suggest. Matches Noxum's manual description ("the
  first of the multi-pronged attacks") well; for Respond ("dispel
  Orcs/Goblins/Trolls"), the monster-type filtering the manual implies
  isn't visible in this shared helper, so it must happen in
  `spellRespond`'s own code before the call — not independently
  confirmed.

The **melee** to-hit/damage formula itself (inline in
`updateMonsterAI`'s attack-resolution chunk, not a separate function,
so not given its own name) is now fully traced: to-hit is a BCD dice
roll compared against the defender's `_dexterity`
(`RosterEntry+0x13`) — higher Dexterity means harder to hit. On a hit,
damage is computed as:

```
strength = decimal(RosterEntry[+0x12])          ; BCD -> binary via AAD
damage   = 4 + weaponIndex*3 + floor(strength/2) + random(0 .. strength|1)
```

(`weaponIndex` from `RosterEntry+0x30`), then applied via
`applyCombatDamage`. This is a complete, evidence-based combat formula
— a solid foundation for the eventual C++/ScummVM reimplementation.

**"Level vs. Experience" offset conflict resolved, same session** —
there was never a real conflict, just two different displays of the
same bytes. `ultima_bootup.idb`'s `showCharacterDetails` does `mov ax,
[bx+1Eh]; call printHexWord` under the label "Experience:" — a plain
word read of the entire `+0x1E`/`+0x1F` pair, confirming `_experience`
really is the full word already modeled in `RosterEntry`.
`ultima_exodus.idb`'s `drawPartyStatusBar` separately does `mov al,
[bx+1Fh]; add al,1; daa; ...clamp to 99h...; call printHexByte` under
the label "L:" — reading only the **high byte** of that same word
(little-endian, so `+0x1F` holds Experience's top BCD digit-pair) and
showing it as a derived "Level" indicator, `Level = high-byte(
Experience) + 1`. No separate stored Level field exists, on disk or in
any "live" in-memory copy — Level is simply computed fresh from
Experience's leading digits every time the status bar redraws (with
Experience clamped to 9999, this is a clean `floor(exp/100)+1` curve,
1-100, all without spending a dedicated byte on it). The originally
floated hypothesis (a live-copy field overwriting Experience's high
byte) doesn't hold up and wasn't needed — the two reads simply overlap
by design.

**Shrine entry traced and two small tables resolved, same session**:
followed up on the earlier spell-table over-read bug by dumping the
exact addresses it had wrongly attributed to `CLERIC_SPELL_TABLE`
(`ida_scripts/dump_small_tables.py`), rather than assuming what they
were. Both turned out to be real, useful tables: `0x1598B`
(`FACING_DIRECTION_NAME_TABLE`, 4 entries — 'North'/'-East'/'South'/
'-West') and `0x15993` (`SHRINE_ATTRIBUTE_NAME_TABLE`, 4 entries —
'Strength'/'Dexterity'/'Intelligence'/'Wisdom'). Tracing their callers
resolved two more roadmap items:

- **`drawDungeonStatusBar`** (`0x162FD`) — the dungeon HUD line,
  "LVL:"+`_dungeonLevel`+1 and "Head-"+`FACING_DIRECTION_NAME_TABLE`
  [`_facingDirection`].
- **`enterShrine`** (`0x16366`) — the shrine-visit handler. Prompts a
  player, loads `SHRINE.IMG`, sets the game-mode byte to 4, and
  announces the shrine by name via
  `SHRINE_ATTRIBUTE_NAME_TABLE[_partyPosition & 3]` — confirming
  Ultima III's 4 shrines map 1:1 to the 4 primary attributes (a nice,
  clean design fact). It then prompts an "Offering*100-" gold amount,
  refuses one above a shrine-specific maximum ("You can't cheat the
  Gods!"), spends the gold from `_gold`, and raises one of the
  character's 4 primary attributes before printing "Shazam!". **Left
  open deliberately**: the code selects *which* attribute to raise via
  a lookup of the character's race (`[bx+16h]`) against a 5-entry
  table (`byte_158CD`) combined with the shrine index in a way this
  pass didn't fully untangle, and the per-shrine maximum-offering
  table (`[bx+di+58D2h]`) isn't independently confirmed either — noted
  as unresolved rather than asserted from a partial read.

`ida_scripts/fix_monster_tables.py` was generalized into
`fix_array_boundaries.py` (a growing list of address/name/size
entries) once it needed to fix these two tables' boundaries too, on
top of the two monster tables from earlier — same
`del_items`+`create_data` pattern each time.

**Overworld dragon breath attack traced, same session**: followed the
roadmap's flagged-but-unchased `'t'`/`'<'` monster-type special case in
`updateMonsterAI` all the way through. `monsterBreathAttack`
(`0x1232F`) fires on a 50% per-turn roll, using `computeStepTowardParty`
(`0x1633B`) to get a single-step aim direction toward the party
(wrapping correctly on the overworld's 64-tile-per-axis map — the same
wraparound convention seen elsewhere), then walks a `'='` breath tile
outward from the monster's position toward a fixed `(5,5)` — the
party's own fixed screen-center position in the 11×11 visible viewport
(distinct from the 11×11 *combat* arena traced earlier) — stopping at
impassable terrain (tile values `4`/`0x23`/`0x24`) or, on actually
reaching the party's tile, calling `damagePartyAll` (`0x182C6`). That
function loops all 4 live party members and hits each alive one with
`random(0..0x77) + (_dungeonLevel+1)*8` BCD damage via
`damageCharacterHP` (`0x16BC9`, the general-purpose BCD HP-subtract
primitive — also called from `processPartyTurnEffects`, so poison/
hunger presumably route through it too, not independently confirmed).
`damageCharacterHP` handles death by setting `_status` to `'D'` and
zeroing `_hitPoints`, then calls an untraced `sub_16B91` — worth
following up, since it's also called from `checkPartyWipedOut` and
looks like it could be the actual party-wipe/game-over trigger.

One loose thread worth flagging: `damagePartyAll`'s
dungeon-depth-scaled damage term fires even when reached from this
overworld attack, which reads oddly unless `_dungeonLevel` simply
holds a stale/irrelevant value on the surface (most likely) or this
helper turns out to be shared with an as-yet-unfound dungeon-dragon
caller — not resolved either way this pass.

**Auto-save-on-death confirmed, same session**: followed
`damageCharacterHP`'s death branch into `sub_16B91`
(→ **`autoSaveOnDeath`**) and its two callees, which turned out to be
exactly what their save-file arguments say: `sub_1207D`
(→ **`saveSosariaAndParty`**) calls `saveFile` on `SOSARIA.ULT` with
`cx=0x1228` (4648 bytes — matching the confirmed overworld/town map
file size exactly) then on `PARTY.ULT`; `sub_12097`
(→ **`savePartyFile`**) alone calls `saveFile` on `PARTY.ULT` with
`cx=0x112` (274 bytes — matching its confirmed size too), sourced from
`_currentTransport`, the live party struct's own base address (PARTY.
ULT's first byte per `file-formats.md`). `autoSaveOnDeath` picks
between the two based on game mode: a full save in the overworld or a
specific combat sub-state, `PARTY.ULT` alone otherwise (dungeon/town,
where the overworld map itself hasn't changed). This confirms a real
design decision worth replicating exactly in any reimplementation:
Ultima III permanently commits a character's death to disk
*immediately*, not only at an explicit Quit & Save — there's no "undo"
by quitting without saving after a death.

This also clarified `checkPartyWipedOut` (already named, from an
earlier session): it's the actual game-over check, looping all 4
party slots and, if every one is dead, printing "All Players Out!",
calling `autoSaveOnDeath`, and jumping to `loc_17252` — which turns
out to be nothing more than `jmp short loc_17252`, a literal
self-jump. **Ultima III's game-over "screen" is a deliberate infinite
hang**: after the message and auto-save, the game just loops forever
in place, and the player has to reboot or restart `ULTIMA.COM` by
hand. Authentically period-correct, and a fully resolved dead end —
no further tracing needed there.

**Movement-blocked checks named, same session**:
`isShipMovementBlockedByWind` (`0x17233`) confirms one of Ultima
III's best-known sailing mechanics directly from the disassembly: a
ship can't move at all while becalmed, and can't sail directly into
the wind — checked against `word_12A92`'s low byte, independently
confirmed elsewhere (via `updateWindDisplay`) to be the live wind
direction, matching `WIND_DIRECTION_TABLE`'s own convention.
`checkTerrainMovementBlocked` (`0x17254`) is the more general,
transport-dependent tile-passability check `cmdMoveNorth`'s handler
family calls — most blocked tiles simply refuse the move, but a few
specific values instead play a distinct warning sound with a timed
delay (an extra one when mounted) while still allowing the move,
reading like an audio hazard cue rather than a hard block; the exact
tile-to-hazard mapping wasn't chased down further.

**`canMoveToTile`'s real helper found, and a stale guess corrected,
same session**: `findMonsterAtPosition` (`0x17F96`) loops
`updateMonsterAI`'s 32-slot overworld monster arrays looking for an
occupant at a candidate (X,Y) — `canMoveToTile` calls it to refuse a
move onto a tile another monster already occupies, the overworld
counterpart to combat's `findCombatantAtPosition`. Along the way, an
earlier roadmap note's guess that `sub_128F2` was `canMoveToTile`'s
*other* helper turned out to be wrong — reading `canMoveToTile`
end-to-end shows it only calls `findMonsterAtPosition`. `sub_128F2`
is something else entirely, and a useful find in its own right:
**`getDungeonTileAt`**, the dungeon-map counterpart to `getMapTileAt`
— reads a tile byte from a loaded per-level dungeon buffer at a
computed offset (packed X/Y in one byte, plus `_dungeonLevel<<8`,
based at a fixed `0x900`). Worth remembering as a general lesson: a
speculative note written once, without re-reading the actual call
site, can go stale — always re-verify against the code before trusting
an old guess, not just when something looks suspicious.

**`SHAPES.ULT`/`CHARSET.ULT` load sites found, and the old
`drawCharGlyph`-zero-buffer mystery finally resolved, same session**:
`entryFromBootup` (`EXODUS.BIN`'s entry point) loads `SHAPES.ULT` into
a buffer at `0x12B39` (`0x1400` = 5,120 bytes) and, immediately after,
`CHARSET.ULT` into `0x13B39 + 0x400`. Those two addresses are the same
linear address (`0x12B39 + 0x1400 = 0x13F39 = 0x13B39 + 0x400`) — the
two files load back-to-back into one contiguous 7,168-byte
graphics-asset region. This explains a mystery flagged all the way
back during the `ULTIMA.COM` phase: `drawCharGlyph`'s glyph buffer
reads as all zeros in both `ULTIMA.COM`'s and `ultima_bootup.idb`'s
own static file images simply because neither executable ever loads
`CHARSET.ULT` — only `EXODUS.BIN` does, at runtime, into memory shared
across the whole chain-loaded process. Since both earlier executables
display real text successfully before `EXODUS.BIN` ever runs, they
must be using a different text-output path than `drawCharGlyph` for
their own (pre-graphics-mode) screens — not confirmed which, but the
zero-buffer question itself is fully explained.

**Temple interactions named, same session**: `showTempleMenu`
(`0x1A692`) is the "Clerical Healing\nSacraments:" screen, reached
from `cmdEnter` when the location type is a temple. It dispatches
through a 4-entry `TEMPLE_COMMAND_TABLE` to `templeCure`, `templeHeal`,
`templeResurrect`, and `templeRecall` — the temple-visit counterparts
to `spellAlcort`/`spellSanctuMani`/`spellAnjuSermani`. Each follows
the same shape: print a location-specific cost message, confirm via a
newly-named shared `promptYesNo` helper, then pay via
`deductGoldIfAffordable` (a shared cost-check-and-pay primitive that
BCD-subtracts `_gold` if affordable and refuses otherwise) before
presumably applying its effect. The actual character-state change past
the payment gate wasn't independently re-traced against each
corresponding spell's own code for this pass — assumed equivalent by
name and structure, not confirmed byte-for-byte.

**Two smaller loose ends addressed, same session**:
`_locationTypeTable` was renamed to `_locationType` after confirming
it's a plain scalar (`db 0`, never accessed with an index anywhere in
the binary) — the 19-byte-parallel-array possibility an earlier note
flagged for confirmation doesn't hold up. And the "why is Unlock
disabled in dungeons?" question got a partial answer: `cmdUnlock`
only ever checks for one hardcoded tile value,
`getMapTileAt() == 0xB8`, which reads like an overworld/town-only
tile ID — if dungeons simply never use that value, reusing the same
handler there would always be a silent no-op, making it cheaper to
just disable the letter outright. Doesn't confirm whether dungeon
locked doors exist or how they'd open if so — left as a genuinely
open question, just a better-informed one.

**Two frequently-cited helpers finally named, same session**:
`selectPlayer` (`0x16C76`) and `invertScreenRegion` (`0x17176`) had
both been referenced *by address* throughout this entire session's
evidence notes (in `cmdCastSpell`, `enterShrine`, `showTempleMenu`,
`applyHungerTick`, and more) without ever actually being renamed —
fixed. `selectPlayer` is the player-selection keypress prompt used
everywhere a command needs "which character?" (returns the character
pointer in `bx`). `invertScreenRegion` XOR-inverts a CGA video-memory
region across both interlaced banks — mechanically confirmed, but
reused for at least two different purposes across callers (character
portrait highlighting during `enterShrine`, and a screen-flash effect
synced to a sound cue elsewhere), so it's named for the shared
mechanism rather than one asserted purpose.

**Full per-turn party effects cycle traced**: `processPartyTurnEffects`
(already named from an earlier session) turned out to run a complete
cycle every turn — class-gated MP regeneration, hunger, poison, and
healing — and its two remaining unnamed helpers are now
`applyHungerTick` (`0x170E4`) and `computeMaxMagicPointsFromAttribute`
(`0x17149`). Tracing `applyHungerTick` turned up a genuine, previously
unlabeled `RosterEntry` field: offset `0x20`, a 1-byte gap between
`_experience` and `_food`, is `_foodSubCounter` — a fixed-point
accumulator that only lets the visible `_food` counter drop once this
sub-counter itself underflows, making hunger progress slower than 1
unit per turn. Added to the struct in both `ultima_bootup.idb` and
`ultima_exodus.idb`. The same pass confirmed poison deals 1 damage per
turn with a "Poisoned!\n" message and a status-bar flash, starvation
deals 5 damage the same way once `_food` bottoms out, and HP
naturally regenerates by 1 (BCD) toward `_maxHitPoints` each turn a
character isn't poisoned — a nice, complete confirmation of Ultima
III's core survival-mechanics loop.

**Keyword-matching and transition helpers named, same session**: an
earlier note had described "the keyword-lookup helper sub_1740A/
sub_17423" as one unit without distinguishing them — reading both
end-to-end shows they're two different steps: `uppercaseBuffer`
(`0x1740A`) is a case-normalization pre-pass (converts a buffer to
uppercase in place), and `matchKeywordAtDelimiter` (`0x17423`) is the
actual comparison `cmdYell`/`cmdOtherCommand` use, matching a typed
word against a dictionary word up to a shared space delimiter. Also
named `exitToSosaria` (`0x16458`, directly from its own string "Exit
to Sosaria!\nPlease wait...") — the general "leave a
dungeon/town/castle/shrine, return to the overworld map" transition —
and its `flushInputBuffer` helper (`0x16C53`), which discards any
pending keystrokes before a new screen starts responding.

**`showZtats` finally named**, along with its two small helpers:
`showZtats` (`0x16DC1`) is the shared character-stats display both
`cmdZtats` and `combatCmdZtats` call — referenced by address in this
session's notes ever since `cmdZtats` was first identified back at the
very start of this stretch of work, but never actually renamed until
now. It highlights the selected character's row (`invertCharacterCell`)
and prints their stats page by page, gated by the newly-named
`waitForContinueOrCancel` (`0x16FB1`, Enter/Down/Space to continue,
Escape to cancel). Also named `setCursorForPartyRow` (`0x16D81`,
`drawPartyStatusBar`'s per-row helper — measures a character's name
length to center it horizontally and sets the cursor row from the
party slot).

**Two more core utilities named**: `incrementMoveCounter` (`0x17E02`)
increments a 4-byte BCD counter (`byte_114BD`-`byte_114C0`) with carry
cascading through all 4 bytes -- matches `file-formats.md`'s
`PARTY.ULT` "Move count (BCD)" field (4 bytes at file offset `0x03`)
exactly, so this is that counter, ticked once per party move.
`tryBcdAddClamped` (`0x17D74`) is a generic, side-effect-free "BCD add
with a limit" calculator -- given a current value, a limit, and an
amount, it reports success/already-over-limit/would-overflow without
writing anywhere itself, leaving the actual commit to its many call
sites scattered directly in `sub_17B54`. Distinct from
`addGoldClamped`/`addExperienceClamped`, which are simpler and commit
their result directly to a `RosterEntry` field.

**Town shops and NPCs identified — a big batch**: found by reading
`cmdEnter`'s building-tile branch (entering a town building, tile
`'@'` = `0x40`) all the way through. It dispatches through a
newly-named `TOWN_BUILDING_TABLE` (`0x18028`, 8 entries) indexed by
`_partyPosition`'s Y byte `& 7` — which of 8 shop/NPC types a given
town building is turns out to depend purely on its Y coordinate mod 8,
one fixed pattern reused across every town in the game, not anything
town-specific. Every one of the 8 handlers was identified from its own
on-screen welcome text (not guessed from position):

- **`showTavernMenu`** (`0x1A5AC`) — "Welcome to\nthe Pub!" Buy drinks
  for gold; below a minimum cost you're kicked out ("Leave my shop!
  You scum!!"), and each drink tier prints a different rumor/lore line
  from a small lookup table indexed by cost. Loops via "Another?",
  farewell "It's been a\npleasure!!" — the classic Ultima
  tavern-rumors mechanic.
- **`showGrocerMenu`** (`0x1A630`) — "Ye local\nGrocer\n\nRations:".
  Buys food, BCD-adding to `_food` (confirming that offset again from
  a completely different angle than `showCharacterDetails`), refusing
  a purchase that would overflow it ("Too much to\ncarry!").
- **`showWeaponsShopMenu`**/**`showArmourShopMenu`** (`0x1A8A5`/
  `0x1AA25`) — structurally identical to each other (shared prompt
  strings and byte-table layout), buy/sell weapons or armour.
- **`showGuildMenu`** (`0x1AB93`) — "The Guild shop:\n Keys 50gp\n
  Torc[hes]...", sells Keys/Torches/Powders/Gems.
- **`showOracleMenu`** (`0x1ACFC`) — "    Radrion:\nProphet of Life!",
  the game's cryptic-hint NPC. Its full dialogue is a direct, valuable
  lore confirmation: rhyming riddles about the 4 Marks, the 4 Shrines,
  playing-card suits, and seeking "the Lord of Time" in the dungeons —
  ties straight back to the `_marksAndCards` `RosterEntry` field found
  earlier this session via `cmdYell`. Takes a gold offering (100gp
  increments) per hint.
- **`showStableMenu`** (`0x1AD70`) — "\n\nEquine Emporium:\n\n", buys
  horses for the whole party at `partySize * 200gp`.

This one batch moved function-naming from 99/144 to 106/144. Left
open: `showGrocerMenu`'s cost-calculation call, `sub_17D1A` (likely a
shared "prompt a quantity, compute the price" utility other shops
probably reuse too), and the weapons/armour shops' own
inventory-listing internals weren't individually traced this pass.

**The win condition and ending sequence found**, while chasing down
`showGrocerMenu`'s remaining cost-calc helper (now named
`promptForQuantity`, `0x17D1A` — a 4-digit number-entry prompt similar
in shape to `promptForNumberEntry`). Right nearby, `victorySequence`
(`0x1A4B9`) turned out to be Ultima III's entire win screen: it prints
"Congratulations!\n Thou hast\n compleated\nExodus: Ultima 3\n in",
the party's move count (`printMoveCount`, `0x1A46F`, printing the same
4-byte BCD counter `incrementMoveCounter` ticks), "Report thy feat!",
then a 21-flash screen fanfare — `xorScreenRegionWithPattern`
(`0x1A491`), a generic large-region CGA XOR helper parameterized by a
caller-supplied pattern, structurally similar to `invertScreenRegion`
but bigger and not hardcoded to `0xFFFF` — before printing the
complete epilogue, line by line, at fixed screen coordinates:

> And so it came to pass that on this day EXODUS, hell-born incarnate
> of evil, was vanquished from Sosaria. What now lies ahead in the
> ULTIMA saga can only be pure speculation! Onward to ULTIMA IV!

It then calls `autoSaveGameState` and sets game mode to 1. That call
is itself a small but important finding: the function had been named
`autoSaveOnDeath` on the assumption it only fired on character death
or a full party wipe — finding it ALSO called here, at the moment of
victory, showed that name was too narrow. Renamed to
`autoSaveGameState`, reflecting its real role as a general "commit
game state to disk at a major transition" primitive, not something
death-specific.

Not yet traced: what actually *triggers* entry into `victorySequence`
in the first place — i.e., the specific "Exodus has been defeated"
condition check, presumably somewhere in the combat-resolution code
path. A good, well-scoped next target.

**Found it immediately after — Ultima III's legendary Exodus puzzle,
confirmed directly from the disassembly.** Tracing `victorySequence`'s
own trigger backward led straight to it. This is one of gaming
history's most famous "you can't just fight the boss" mechanics, and
it's right there in the code:

- **`obtainCard`** (`0x17607`) fires only when `_locationType ==
  0x3Eh`. It sets one bit of the acting character's `_marksAndCards`
  (`RosterEntry+0x0E`) based on `_partyPosition & 3` — which of the 4
  Cards you get depends on where in the world you pick it up — and
  prints "A card, with\nstrange marks!\n".
- **`attemptExodusSequence`** (`0x1764A`) is the actual puzzle
  mechanic. You stand on one of 4 special control-panel tiles (`'|'`,
  `0x7C`) — reached by prompting "Direct? " and checking the targeted
  tile — and choose a letter from a "D, S, L, M:\n" menu. That only
  advances a step counter (`word_164A0`, starting at 0) if THREE
  things all check out for the current step: the chosen letter
  matches an expected value, the panel's position matches an expected
  value, and — this is the part that makes it a real puzzle, not just
  a lookup table — the acting character's `_marksAndCards` has the
  bit for the Card that step actually requires. Get any of the three
  wrong and you just get a flash-and-beep ("wrong answer"); get all
  four steps right, in order, each with the matching Card already in
  hand, and `word_164A0` reaches 4 — which jumps straight into
  `victorySequence`.

This directly explains `showOracleMenu`'s riddle dialogue found
earlier this same session: Radrion the Prophet's rhyme about "The
cards their\nsuits do number..." and seeking guidance "Unto the
Montors" was always a hint toward exactly this mechanism — the game's
central puzzle is confirmed end-to-end, from the Oracle's cryptic hint
text through to the winning condition check, entirely from reading the
code. The precise letter-to-card/position mapping (which of D/S/L/M
pairs with which of the 4 steps) wasn't decoded digit-by-digit this
pass — the mechanism is confirmed, the exact walkthrough sequence
isn't spelled out.

**`cmdWear`'s effect finally traced**: `wearArmour` (`0x17EFA`) had
been left as "not traced past the call" since the very first session
covering EXODUS.BIN's command letters. It prints "Armour:\n", looks up
the character's class letter against a combined string
`'FCWTPBLIDARDirect? '` (confirming an 11-class roster) to find that
class's max wearable-armour tier, shows a menu of owned armour types,
and on a valid choice sets `_armourIndex` and prints "Readied!\n" (the
same confirmation text weapon-readying uses). This also rules out one
piece of speculation from `cmdHandEquipment`'s note: `wearArmour`
shows no "hand mode" flag check of any kind, so it's not the follow-on
half of that mechanic after all — a self-contained, single-character
equip screen with nothing cross-character about it. The real
item-hand-off mechanism `cmdHandEquipment` implies is still not
located.

Also named `printNameByIndex` (`0x16BFA`), a generic "look up a string
by index in a shared table, print it" helper reused across very
different contexts (`cmdLook`'s tile descriptions, and at least 3
combat-related callers) — named for its confirmed mechanism rather
than a single asserted purpose, since different callers clearly use
it for different kinds of names.
