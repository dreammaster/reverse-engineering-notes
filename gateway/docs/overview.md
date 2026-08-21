# Gateway (DOS) — Disassembly Overview

Working notes on the reverse-engineering effort for Legend Entertainment's
*Gateway* (1992). Goal: fully document both DOS executables well enough
to write a clean C++ reimplementation, then a ScummVM engine module —
and to document the underlying engine generally enough to ease reversing
the other Legend titles built on it (later goal, not started).

**Engine lineage (per Paul, correcting an earlier draft of this file):**
Legend Entertainment shipped two distinct shared engines across its
adventure game catalog, not one:
- **"Early" engine** — a hybrid of text-adventure parser input with
  graphics. In release order: *Spellcasting 101* (1990, the earliest),
  *Spellcasting 201*, *Timequest*, *Spellcasting 301*, **Gateway**,
  *Gateway II*, and *Eric the Unready*.
- **"Later" engine** — more graphically oriented: still a static scene
  view, but action selection from a menu/verb list and point-and-click
  on visible inventory/scene items, no text parser. *Companions of
  Xanth*, *Death Gate*, *Superhero League of Hoboken*, *Shannara*.

*Gateway* was chosen for this project as one of the **last** Early-engine
titles (not the first — an earlier draft of this file had that backwards)
released, plus personal preference for the game's setting. Worth keeping
this lineage in mind once cross-referencing findings against any other
Legend title: `gatemain.idb`'s parser/vocab-shaped structs
(`VocabEntry`/`Parser_Data1`/`ParserHandlerEntry`/etc., see below) are
consistent with an Early-engine game specifically — the Later engine's
titles would not be expected to share that same parser layer.

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
| `gate.idb` | `gate_decoded.exe` (entry `0x9b0`, cs=`0x1112`) | 180 / 502 (36%) | 21 | 32 |
| `gatemain.idb` | `gatemain_decoded.exe` (entry `0x76c`, cs=`0x2cf4`) | 1519 / 3288 (46%) | 49 | 308 |

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
- Findings get written up here (or in
  [file-formats.md](file-formats.md) for on-disk resource formats —
  rooms/logic/pictures/vocab, given the AGI/SCI-like structure
  hypothesized above; started 2026-08-21 with the Huffman compression
  primitive and `VOCAB.DAT`) with enough detail that the `note` field in
  a rename entry can stay short and just point back to the section.

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
- `docs/file-formats.md` started 2026-08-21 (Huffman compression +
  `VOCAB.DAT`, see that file) — room/logic/picture formats implied by
  `gatemain.idb`'s struct list still not traced.

## `gate.idb` — findings log

Started 2026-08-21, per Paul's direction to start with the smaller
executable first. First full export+save round-trip for this IDB
(`gate.asm`/`gate.idc`, committed alongside this writeup).

### `_main` confirms the title/intro role, and the gate→gatemain handoff is real DOS EXEC

Read `_main`'s full body directly (`gate.asm:97`). Flow: `memory_init` →
`set_file_prefix("GATE")` → `check_graphic_params`/`Font_init`/
`unk1_init` → `check_sound_params` → (if sound check fails) print
copyright and exit early, else → `show_intro` → branch on
`current_section` to run additional per-section setup (`sub_1F2F4`,
conditionally `init_graphics` + `sub_1F284` when
`current_section==3`) → draw/message calls → **hand off to
`GATEMAIN.EXE`**.

**This confirms Paul's working hypothesis that `gate.idb` is the title-
screen/cutscene executable.** `set_file_prefix("GATE")` and the whole
`show_intro`/`current_section` dispatch (already-named going in, and
genuinely used throughout a large cluster of functions in the
`0x1F000`-`0x23000` range per its ~30 xrefs) are consistent with a
scripted intro/cutscene sequencer, not gameplay.

**The gate→gatemain handoff is a real DOS `EXEC` call — architecturally
different from `ultima1`'s custom overlay loader.** `_main` builds 8
numeric command-line arguments via `_itoa` (`xmouse`, `videoMode`,
`soundMode`, plus 5 more globals — `word_2A256`/`58`/`5A`/`5C`/`5E`,
roles not yet identified) and calls:
```
_execl("GATEMAIN.EXE", "GATEMAIN.EXE", arg1..arg8, NULL)
```
via the genuine Microsoft C runtime `_execl`/`_execve`/`__doexec` CRT
cluster (confirmed present in `gate.asm`, not a hand-rolled loader like
`ultima1`'s `chainToExecutable`/`execProgram`). The code immediately
following the `call _execl` (restore video mode, print `"GATEMAIN.EXE
could not be started..."`, return 1) is a **failure-only path**: real
DOS `EXEC` semantics for the C runtime's `exec*()` family mean the
calling process is torn down when the child successfully starts, so
this fallback is only reached if `EXEC` itself fails to launch the
child — it is not a "gatemain returned control to gate" path. Worth
double-checking this assumption once `gatemain.idb`'s own `_main` is
read, but it matches the standard DOS CRT `exec()` emulation model.

**Implication for the ScummVM reimplementation**: unlike `ultima1` (5
executables stayed resident in one continuously-running process, jumping
between each other via a custom loader — a real design decision to
avoid DOS's `EXEC` memory/reentrancy costs), Gateway's 2 executables use
plain DOS `EXEC`, meaning `gate.exe` and `gatemain.exe` really are
separate OS-level processes/loads with no implied shared-memory state
across the handoff — only whatever's passed via the 8 command-line
arguments and/or a file on disk. Modeling this in ScummVM should be
simpler than `ultima1`'s case (no in-engine "mode switch" trick needed,
just launching into `gatemain`'s engine state fresh, informed by the 8
passed arguments) — but worth confirming no additional state is shared
via a file (savegame-like) the way `ultima1` used `inuse.u1`.

**Open follow-ups, not resolved this pass**:
- The 5 unidentified globals passed as `arg4`-`arg8` to `GATEMAIN.EXE`
  (`word_2A256`/`58`/`5A`/`5C`/`5E`) — likely more hardware/config state
  (joystick? additional video/sound detail?) alongside the confirmed
  `xmouse`/`videoMode`/`soundMode`.
- `current_section`'s exact value meanings (0/1/2/3 seen so far) and what
  each actually shows — `show_intro`'s cluster is large (dozens of
  functions in the `0x1F000`-`0x23000` range) and wasn't traced function-
  by-function this pass, just confirmed as real and load-bearing via its
  ~30 `current_section` xrefs.
- `sub_1BED2` (called from `_main`'s failure path to print the
  "could not be started" message, and also from `Font_writeString`) is
  **not** a simple `fputs`-equivalent — it calls `get_message` (resolving
  a message ID to a string, same mechanism named in `gate.idb`'s
  `MESSAGE`/`get_message` already-named cluster) and issues a raw
  `_int86` `INT 10h` call reading BIOS cursor position before drawing,
  suggesting a real "draw message text at the current cursor position"
  primitive, not a trivial wrapper. Worth a real trace before naming it
  confidently — flagged rather than guessed.

### `gate.idb` color cluster, decoded

Second pass, same session. Ran `rank_unnamed_functions.py` for the first
time against this IDB (325 unnamed functions) and picked the two
functions `_main` calls directly right after `show_intro`/the
`current_section` dispatch (`sub_1C082`, `sub_1C0C4`, both 13-caller
targets) as the starting point, since they're both reachable from
already-understood code.

Traced a 5-function/2-global cluster shared between `_main`'s startup
screen-clear and the already-named `Font_writeChar` glyph renderer:

- **`max_color_index`** (`word_29B80`) — set inside the already-named
  `init_graphics` per detected video mode: `1` (2-color), `0Fh`
  (16-color EGA), `0FFh` (256-color VGA), confirmed by direct read of
  the 3-way mode dispatch. Used everywhere else in this cluster purely
  as a clamp ceiling for color values — i.e. this is genuinely
  `numColors - 1`, not a guess.
- **`current_draw_color`** (`word_2F0B0`) — initialized to
  `max_color_index` in `init_graphics`, then read/written by
  `setDrawColor`. Confirmed **distinct** from the already-named
  `Font_fgColor`/`Font_bgColor` — this is the color used by the
  non-text box/fill primitives (`sub_1C0C4` and its callees), not glyph
  rendering, even though both clusters share the same clamp ceiling and
  get initialized together in `init_graphics`.
- **`Font_setColors`** (`sub_1B300`, ex-8-caller target) — trivial
  2-line setter directly writing the already-named `Font_fgColor`/
  `Font_bgColor` globals, no clamping. The "root" color setter that
  everything else in the font-color half of this cluster funnels
  through.
- **`Font_setColorsClamped`** (`sub_1BEA4`) — stores its first arg raw
  and its second (background) arg clamped to `max_color_index`, then
  calls `Font_setColors` with the clamped pair. Called from `_main`'s
  DOS-EXEC-failure path immediately before the "`GATEMAIN.EXE` could not
  be started" message print (`sub_1BED2`) — i.e. this sets that error
  message's text colors.
- **`setDrawColor`** (`sub_1C082`) — clamps its argument to
  `max_color_index`, returns the previous `current_draw_color` value,
  updates it, then calls `sub_24F42` (not yet named — mirrors the new
  color into offset `0x22`/`0xC` of what looks like a `SCREEN`-typed
  struct instance living at a fixed `dseg+0x2CEE`, confirmed by a second
  writer at the same base, `sub_24E26`, itself called from both
  `sub_1C0C4` and `Font_writeChar`). Called directly from `_main` with
  `0Fh` (white) immediately before the box-fill call now understood to
  be `sub_1C0C4`.

**Not renamed yet, flagged instead of guessed**: `word_2F0AE`/
`word_2F0AC` (the raw/clamped shadow copies `Font_setColorsClamped`
stores alongside the real `Font_fgColor`/`Font_bgColor` write — their
purpose beyond "shadow copy" isn't confirmed); `sub_1C0C4` itself (the
actual box-fill/draw-rect primitive `_main` calls with `(mode=2, x=0,
y=0, w=word_29B6A, h=word_29B6E)` — traced enough to know it dispatches
through `sub_24E26`/`sub_25D7A`, both **also** called directly from
`Font_writeChar`, meaning this is a shared box/glyph-cell rasterizer,
not two independent things — but the exact draw semantics per mode bit
`0x80` of its first argument weren't fully pinned down); the `SCREEN`
struct itself (all 21 words/`0x2C` bytes are still unnamed
`field_0`..`field_2A` placeholders — a real target for a future
`apply_structs_gate.py` pass, now that at least 2 of its fields
(`0xC`, `0x22`, `0x24`) have confirmed roles from this pass).

5 renames applied via `ida_scripts/apply_renames_gate.py` (3 functions:
`Font_setColors`, `Font_setColorsClamped`, `setDrawColor`; 2 globals:
`max_color_index`, `current_draw_color`). 180/502 functions now named.

## `gatemain.idb` — findings log

Started 2026-08-21, same session, pivoting here from `gate.idb` since
tracing the box-fill primitive further was turning into a deep
font-rendering rabbit hole for modest confidence gain. First full
export+save round-trip for this IDB (`gatemain.asm`/`gatemain.idc` —
15MB/394k lines, 4.5MB `.idc`, both committed alongside this writeup).

### `main`'s top-level flow confirms the Early-engine text parser

Read `main` directly (`gatemain.asm:960`, `proc near` — note the
different name/calling-convention style from `gate.idb`'s `_main proc
far`, consistent with these being two separately-compiled programs, not
two halves of one build). The prior tentative work already covering this
function turned out to be extensive and internally consistent — a good
sign, though still independently spot-checked rather than assumed:

- Calls `j_gatemain_start`, then `_setjmp` against `main_jump_regs` —
  the classic C `setjmp`/`longjmp` restart point. `restartType`
  (the `setjmp` return value) is compared against `LOAD_UNDO`/
  `LOAD_SAVE` enum constants (already defined) to decide what to restore
  before entering `game_loop`, meaning save/load/undo are implemented as
  a `longjmp` back to this exact point in `main`, not a separate code
  path.
- `game_loop` reads a line via `j_InputWindow_getLine`, lowercases it
  (`_strlwr`), and special-cases three parser meta-words *before*
  general parsing: `PARSER_OOPS` (corrects the previous misunderstood
  word via `j_Parser_oops`), `PARSER_UNDO` (`Parser_performUndo`), and
  `PARSER_AGAIN` (repeats the last command) — each resolved through
  `vocab_list` (a `VocabEntry` array) and its `_flags`/`_altVocabId`
  fields. This is a textbook Infocom-style parser meta-command layer,
  confirming the Early-engine parser hypothesis directly (see the
  engine-lineage note at the top of this file) — not just implied by
  the struct list as before.
- `nothing_entered` prints `"I beg your pardon?\n"` — the classic
  parser did-not-understand response — when the line is empty.
- General parsing goes through `j_Parser_parseWord`, then dispatches
  into the room/logic system via `_roomLogicNum` and further calls not
  yet traced past this first read.

**Not traced yet**: the actual verb/object dispatch after parsing
succeeds, the room/logic section format itself (`Room`,
`LogicSection2`-`8`, `LogicIndexEntry`), and what `logic238`-style
already-named functions (seen as callers throughout the codebase, e.g.
in the RTLink-thunk survey below) actually represent structurally —
almost certainly one compiled logic script per room/number, echoing
AGI's `LOGIC.n` convention, but not confirmed by reading one directly
yet.

### RTLink overlay architecture, decoded — and a major function-count correction

Ran `rank_unnamed_functions.py` for the first time against this IDB
(2481 unnamed). The single highest-ranked target, `sub_11635` (196
distinct callers), turned out to be a real interpreter-internal function
(a recursive walk through `METHOD_SECTION_INFO`-driven "prehandler"
stages via the already-named `Logics_getPrehandlerMode`) — traced enough
to understand its mechanics but not confidently named this pass (left
for a future session rather than guessing on a 196-xref function).

**Far more consequential finding**: a huge fraction of the *next* tier
of "high caller count" entries were all exactly 8 bytes and shared one
body shape:
```
call near ptr rtlink_thunk
jmp  <target, in a different overlay segment>
```
This is the commercial **RTLink** DOS overlay linker's call-gate
mechanism (Polytron/Blinker-era; already hinted at by the pre-existing
`RTLinkSeg` struct and `rtlink_check_filenames2` function seen in
`main`) — a linker-generated trampoline emitted at every cross-overlay
call site, not independent game logic. **This is a third distinct
code-loading architecture in this project**, next to `ultima1`'s custom
overlay loader and `gate.idb`'s real DOS `EXEC` handoff: gatemain uses a
proper commercial overlay manager with per-call-site thunks rather than
whole-program chaining.

Wrote `ida_scripts/find_rtlink_thunks.py` (read-only survey) to size
this up before touching anything. First pass found 955 candidates by
shape alone; a second look caught a real false-positive class — some far
`jmp`s land in a tail chunk IDA attributes back to the *same* function
(a legitimately split/relocated function body, not a call to a different
function) — `get_func_name` on the jump target then just returns the
thunk's own name, which would have produced nonsense self-referencing
renames (`sub_312DB` → `thunk_sub_312DB`) if not caught. Fixed by
comparing the jump target's owning-function start address against the
thunk's own address; **712 genuine cross-function thunks** remained
after excluding 243 same-function tail chunks.

Batch-renamed all 712 via the new `ida_scripts/apply_rtlink_thunks_gatemain.py`
(a one-off structural script generating `thunk_<target-name>` names
programmatically, not a curated list — see its docstring) — DRY_RUN
verified first, then applied for real with full export+save. **This
alone moved `gatemain.idb` from 807/3288 (25%) to 1519/3288 (46%)
functions named** — the single highest-value action taken on either IDB
so far, and it was pure bookkeeping/pattern-recognition, not case-by-case
tracing.

**Follow-on fix applied to the shared tooling**: `rank_unnamed_functions.py`
now auto-detects an IDB's `rtlink_thunk` symbol (present in `gatemain.idb`,
absent in `gate.idb`/all of `ultima1`'s IDBs, so this is a no-op
everywhere else) and excludes thunk-shaped functions from its ranking, so
future passes see genuine unranked logic first instead of overlay
boilerplate.

**Maintenance note for later sessions**: many of the 712 thunks' targets
are themselves still-unnamed (`thunk_sub_674A7`-style), so their names
will go stale-but-harmless once those targets get real names later.
`apply_rtlink_thunks_gatemain.py` is idempotent and safe to re-run
periodically to refresh them — not done on a schedule, just noted here.

### The `_decoded` executables are Paul's own RTLink-flattening tool's output, not IDA-native

Per Paul: `gate_decoded.exe`/`gatemain_decoded.exe` weren't produced by
pointing IDA straight at the original RTLink-linked executable plus its
overlay files. Paul wrote a **custom tool** that flattens the original
RTLink binary + overlays into a single static executable suitable for
ordinary disassembly — adjusting the far jumps following each
`rtlink_thunk` call site to point at the right place in the new flat
layout, and moving the data segment to the end of the image. This
explains the `_decoded` suffix on both IDBs' input filenames, and why
gatemain's overlay-call mechanism shows up as a flat, disassemblable
`call rtlink_thunk; jmp <target>` pattern at all rather than needing
actual overlay-swap emulation to trace through.

**A known rare bug in that tool**: when a far call/jmp targets another
routine *within the same original RTLink segment* (i.e. doesn't go
through `rtlink_thunk` at all, since that's only for cross-segment
calls), the tool has occasionally failed to correctly patch that far
pointer's **segment** word during flattening. Per Paul, the **offset**
word of such a pointer should still be trustworthy even when broken this
way — recoverable by finding which segment makes the offset land on
something sensible, rather than trusting the encoded segment as-is.

**Checked for this specifically**: the 243 "same owning function" cases
excluded from the RTLink-thunk batch rename above (a thunk-shaped stub
whose far `jmp` IDA attributes back to its *own* function as a tail
chunk, rather than to an independently-named target function) were the
natural place this bug could have been hiding, so they got a full
programmatic recheck (`ida_scripts/diagnose_thunk_chunks.py`) rather than
being accepted on the strength of the sample already spot-checked when
they were first found: decoded each far `jmp`'s raw bytes directly,
checked the encoded segment word wasn't implausibly small/stale, and
confirmed the target actually decodes as real code. **All 243 came back
clean** — large, plausible segment values (matching the general
magnitude of other legitimately-named segments elsewhere in this IDB),
landing on genuine decoded instructions. These are real split
(multi-chunk) functions — a short stub chunk whose body lives
elsewhere with no other caller establishing it as an independent named
function — not manifestations of the flattening bug. No fix needed for
this class; kept `diagnose_thunk_chunks.py` in the repo as a reusable
sanity-checker in case a genuine instance of the bug turns up elsewhere
during future passes (any far call/jmp resolving to something
implausible is worth re-running this style of check against before
assuming it's a real code oddity).

### Fixed a pipeline blind spot: collapsed functions were invisible in every `.asm` export

Discovered while trying to read `vocab_load` (wanted to cross-check
`VOCAB.DAT`'s on-disk format against real game data — Paul pointed at
his installed copies at `c:\games\gw`/`c:\games\gw2`): the exported
`.asm` showed only a one-line placeholder,
`[00000210 BYTES: COLLAPSED FUNCTION vocab_load. PRESS NUMPAD+ TO EXPAND]`,
instead of any real instructions. This is `ida_funcs.FUNC_HIDDEN` — a
purely cosmetic GUI "collapse" state (Numpad-) that some earlier manual
session left set on a fair number of functions in both IDBs, and
`ida_loader.gen_file(OFILE_ASM)` respects it, silently hiding that
function's body from every grep/Read of the `.asm` this whole pipeline
relies on. `Logics_getPrehandlerMode` (referenced but not readable
during the `sub_11635` trace in the previous session) was hit by this
exact same issue and is almost certainly why it couldn't be inspected
directly at the time.

Wrote `ida_scripts/unfold_functions.py` — clears `FUNC_HIDDEN` on every
collapsed function in the current IDB, no curation needed (purely
mechanical, always safe to run). Found **171 collapsed functions in
`gatemain.idb`** (including `vocab_load`, `objects_load`,
`Logics_getPrehandlerMode` and several other `Logics_get*` interpreter
internals, `LogicStrings36`/`43`/`44`, `start`) and **108 in `gate.idb`**
(mostly CRT/library internals, including `_execl`/`_execve`/`__doexec`
from the earlier gate→gatemain EXEC-handoff finding — that finding
itself wasn't affected since it only ever needed to observe the *call
site*, not read inside those functions' own bodies). Ran against both
IDBs and re-exported; both `.asm` files are substantially larger now
with real disassembly where placeholders used to be
(`gatemain.asm` +36k lines, `gate.asm` +10k lines).

**Practical implication for all prior and future greps of these `.asm`
files**: anything that came back empty or was skipped as "not defined
as a normal proc" earlier in this project might just have been
collapsed, not actually absent — worth a second look with the current
export before concluding something isn't there.

### `VOCAB.DAT` decoded — the first real on-disk format, and it's Huffman-compressed

With `vocab_load` finally readable, traced it directly against Paul's
real installed copy of the game (`c:\games\gw\VOCAB.DAT`, 22,081 bytes).
Full record-level format now in [file-formats.md](file-formats.md) — the
short version: a small Huffman tree header, a compressed bitstream that
decodes into a flat text pool, then a word table (text offset + flags)
and a synonym/link table (surface-word → canonical vocab id, plus an
optional per-vocab-id `_logicNum` hook) built from the same 4-byte
`VocabFileRec` shape reused for two different meanings depending on
which table it appears in.

**Worth flagging as a methodology note**: an initial raw hex dump of
`VOCAB.DAT`'s first 0x140 bytes (before `vocab_load` was readable) looked
like it might be a plain offset/index table — small 16-bit values,
several suspiciously negative-looking. That would have been the wrong
conclusion entirely: those bytes are the Huffman node table followed
immediately by compressed bitstream data, which just happens to look
like plausible-but-meaningless small integers at a glance. Tracing the
actual loader code first (once the collapsed-function blindspot above
was fixed) avoided sinking time into reverse-engineering compressed
bytes as if they were a literal structure.

`huffman_decompress` itself (`gatemain.asm:5690`) is shared with
`get_message` (`GATESTR.DAT`'s loader, traced next — see below), so
this is a general engine-level compression primitive, not something
`VOCAB.DAT`-specific — relevant if this project ever extends to another
Early-engine Legend title (see the engine-lineage note at the top of
this file).

### `GATESTR.DAT` decoded — sectioned, per-string compression with an LRU cache

Same session, per Paul's direction. Traced `gatestr_load` and
`get_message` fully; complete record-level format now in
[file-formats.md](file-formats.md#gatestrdat--compressed-messagestring-resource-file).
Confirmed against the real file (`c:\games\gw\GATESTR.DAT`, 349,805
bytes) — its header decodes exactly as predicted (`0x0038` = 56
sections, followed by immediately-plausible `{stringsCount, streamSize}`
pairs like `{61, 1063}`, `{142, 3119}`, `{11, 256}`).

Considerably more sophisticated than `VOCAB.DAT`: strings are grouped
into sections (matching the `(sectionId << 10) | index` bit-packed
message-id scheme `main`'s parser and presumably every logic script
use), each string **individually** Huffman-compressed against one
**global** shared tree (not a per-file tree like `VOCAB.DAT`'s), plus an
extended "common strings" dictionary — Huffman symbols `0x80`+ each
expand to a whole cached word/phrase rather than one raw byte, a neat
two-level scheme for compressing English prose specifically. A 32-entry
LRU cache (`_textCache`) of already-decompressed strings avoids
re-decompressing anything requested twice, backed by a shared working
buffer (`gatestr_buffer`) that grows from a soft target down to a hard
floor depending on what memory is actually available at runtime — a
very DOS-memory-constrained-era design choice.

**Not yet traced**: `makeRoomInTextCache`'s eviction policy (called
when the cache or its buffer is full), and the base 128-symbol alphabet
shared with `VOCAB.DAT`'s decoder (same mechanics, independent trees per
file).

### The room/logic "format" — and why there isn't one: it's compiled native code, not data

Per Paul's direction, next target after `GATESTR.DAT`. Traced
`Logic_call` (`gatemain.asm:9995`) and `Logic_getMethodIndex`
(`gatemain.asm:4938`) — the core verb/method dispatch mechanism the
parser's `Parser_perform` calls into — expecting to find an external
resource file (a `LOGIC.n`-style bytecode file, per the AGI-inspired
hypothesis in earlier sessions). **That hypothesis was wrong**: there is
no such file. This is the single most important architectural finding
for scoping the eventual ScummVM reimplementation.

**`proc_table`** (`gatemain.asm:95668`) is the table `LogicIndexEntry`
was defined for: a flat array of 6-byte `{u8 type, u8 pad, far ptr
tableP}` records, one per "logic-bearing entity" (rooms, NPCs, items,
global handlers — `METHODS_COUNT` of them, `734` in the real build),
indexed 1-based (index 0 is a reserved/invalid sentinel — `type=0x40`,
`tableP` a non-pointer garbage value that's never actually
dereferenced, since both `Logic_call` and `Logic_getMethodIndex` reject
`index == 0` before ever reading `proc_table[0]`). **Confirmed via
direct read of the data segment**: `proc_table` is `db`/`dd`
**static initialized data linked directly into `gatemain.exe`/
`gatemain.ovl`**, not read from any file at runtime — e.g.
`proc_table_001` (the first real entity, `_methodIndex = 1`) sits right
there in the `.asm` as literal byte/word values, not populated by a
loader function.

**`type` (1-8) is a tag selecting which of 8 differently-shaped
metadata records `tableP` points to** — confirmed by
`Logic_getMethodIndex`'s `type-1`-indexed jump table
(`off_12529`), one case per type, each reading a `_methodIndex` field
from a *different* offset appropriate to that type's own struct:
`Room` (type 1, `_methodIndex` at `+0x10`), `LogicSection2` (type 2,
`+0x22`), `LogicSection3` (type 3, `+0xA`), `LogicSection4` (type 4,
`+0x12`), `LogicSection5` (type 5, `+0x14`), `LogicSection6`/
`LogicSection8` (types 6 and 8 — sharing one code path, `section68`,
because both structs happen to place `_methodIndex` at the identical
offset `+0x16`; a compiler/layout coincidence, not a semantic merging
of the two types), `LogicSection7` (type 7, `+0x1E`). **`Room` is not a
separate concept from the `LogicSectionN` structs — it's literally
variant type 1 of the same tagged-union table.** Spot-checked
`proc_table_001`/`002` against the `Room` struct's field layout
byte-for-byte (26 bytes, matches exactly including the `_vocabArrIndex`/
`_val4`/`_val3`/`_methodIndex`/etc. field positions already in the
struct definition).

**`Logic_call(index, param)`** resolves `index` through this table to a
final `methodIndex` (0-695, i.e. 0-based and capped, confirmed via the
`> 696` bounds check), splits it into `section = methodIndex >> 9`
(0 or 1, given the range) and a 9-bit offset within that section, and
calls one of two flat far-pointer arrays — `METHODS0`/`METHODS1`, 512
slots each — passing `param` and returning whatever that function
returns. **These 1024 far pointers are the actual compiled x86 code for
every room/object/handler's logic** — real native functions, linked in
via the RTLink mechanism already documented above, not bytecode for a
VM to interpret.

**Why this matters for the ScummVM reimplementation — significantly
more than any other finding so far**: Sierra AGI-style engines store
room/object logic as data (bytecode `LOGIC.n` files) precisely so a
reimplementation can just write a new interpreter for that bytecode and
immediately support all the original content. **Gateway cannot work that
way.** Every room and object's actual behavior is compiled 8086 machine
code baked into `gatemain.exe`/`gatemain.ovl` at Legend's own build time
— there is no separate data format to parse and reinterpret. A faithful
reimplementation has no shortcut around reading each of these ~734
entities' compiled logic (via `tableP`, dispatched into `METHODS0`/
`METHODS1`) and manually reimplementing its behavior in C++, entity by
entity — closer in scope to `ultima1`'s per-command tracing work than to
writing a generic bytecode VM. Worth flagging prominently in any future
scoping discussion of this project's remaining size.

**Not yet traced**: the `MethodSectionMap` array sitting immediately
before `proc_table` in the same data segment (pairs like `<34, 2>`,
`<7, 1>`, `<11, 64>`) — plausibly related to, but not confirmed to be,
the `METHOD_SECTION_INFO` table `sub_11635`/`Logics_getPrehandlerMode`
walk through (from the earlier `gatemain.idb` session) — that whole
"prehandler chain" layer sits on top of this core dispatch mechanism and
still isn't fully connected to it. Also not traced: what the 8 types
actually represent semantically (room vs. NPC vs. item vs. global
handler, etc. — inferred only from `Room` being type 1 so far) and the
still-unnamed fields (`_val1`-`_val4`, `_unkHandlerId`, `_prehandlerId`)
shared across the 8 variant structs.
