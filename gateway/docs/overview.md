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
- ~~The 5 unidentified globals passed as `arg4`-`arg8` to
  `GATEMAIN.EXE`~~ — resolved in a later session, see
  [below](#word_2a256-58-5a-5c-5e-named-to-match-gatemainidbs-argv-parsing).
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

### `word_2A256`/`58`/`5A`/`5C`/`5E` named to match `gatemain.idb`'s `argv` parsing

Much later session, closing out the open item from the very first pass
above. `gatemain.idb`'s own session (tracing its `word_C84D0` decoder
callback back to `gatemain_start`'s `argv` parsing) confirmed the exact
positional mapping: `argv[1]`→`Mouse_enablement`, `argv[2]`→`videoMode`
(both matching this IDB's already-named `xmouse`/`videoMode`, confirming
the two sides really do line up), `argv[3]`→`soundMode` (already named
here too), and `argv[4]`-`[8]`→`gatemain.idb`'s `cmdline_param4`-`8`.

**`argv[6]` (`word_2A25A`) → `streamMode`** — confirmed with real
corroboration on *this* side, not just positional inference:
`gatemain.idb` traced it as `Stream_selectHandler`/`Stream_configure`'s
mode-selector argument (0/1/2/4), and this exact global is set to the
literal value `4` in one code path here (`gate.asm` line ~22358),
matching `Stream_configure`'s special-cased `mode==4` branch precisely —
independent confirmation from both sides of the same cross-process
handoff.

**`argv[4]`/`[5]`/`[7]`/`[8]` (`word_2A256`/`58`/`5C`/`5E`) →
`gatemainArg4`/`5`/`7`/`8`** — renamed for the confirmed cross-IDB
*position* only; neither session decoded what these four individually
control (`gatemain.idb`'s own `cmdline_param4`/`5`/`7`/`8` are likewise
still unrenamed placeholders). Named this way rather than left as
`word_XXXXX` so a reader on either side can immediately see which
`cmdline_paramN` global on the other side is the same value, without
implying more understanding than actually exists.

Applied via `ida_scripts/apply_renames_gate.py`'s second batch.

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

### `OBJECT.DAT` decoded — plain string blob, offsets baked into `proc_table`

Same session, immediate follow-on from the room/logic finding above.
Traced `objects_load`/`Logics_getObjectString`; full format in
[file-formats.md](file-formats.md#objectdat--objectroomnpc-name-strings).
Confirmed against the real file (`c:\games\gw\OBJECT.DAT`, 8,031 bytes
— header `0x1F5D` = 8029 = file size minus 2, exactly).

The simplest of the three real external formats found this session: a
`u16` byte-length followed by that many raw bytes, loaded verbatim into
a fixed buffer — the bytes are just concatenated NUL-terminated ASCII
strings (room/object/NPC names; the real file starts `"Blast Zone\0Gray
Plain\0Plateau\0..."`). **No index table in the file at all** — the
byte offset into this blob for any given entity's name is a field
common to *all 8* `LogicIndexEntry` variant structs (`Room`/
`LogicSection2`-`8`), sitting at offset 0, one word before
`_vocabArrIndex` — previously present in every struct definition but
unlabeled/unexplained until this pass. This offset is itself part of
`proc_table`'s static compiled data (per the finding above), so the
*mapping* of object → name is compiled in, while only the name *text*
is externally editable — a sensible split for content that gets tweaked
far more often than program logic during development.

**Bonus find along the way**: a small **dynamic name-override layer** —
`Logics_getObjectString` checks a fixed 44-entry `LOGIC_STRINGS` table
(`FunctionEntry` structs, `{id, fnPtr}`) via `LogicStrings_call` before
ever touching the static string blob; a match calls that entry's own
compiled function instead, letting a handful of specific objects (44 out
of 734) have a computed rather than fixed name/description. Explains the
oddly-named `LogicStrings36`/`43`/`44` functions surfaced by last
session's collapsed-function fix. Not traced further — each handler
would need individual reading, and this is a narrow, self-contained
mechanism rather than a blocking architectural question.

### `GATE_XXX.RGN`/`GATE_XXX.PIC` decoded — regions and multi-frame pictures

Same session, per Paul's direction to trace the picture/region formats
next. Full record-level layouts in
[file-formats.md](file-formats.md#gate_xxxrgn--clickable-region-files)
(regions) and
[file-formats.md](file-formats.md#gate_xxxpic--pictureimage-files)
(pictures) — narrative highlights below.

**Shared groundwork**: every numbered resource file (`GATE_XXX.<ext>`)
goes through one helper, `open_file2`, which is literally
`sprintf("%s_%03d.%s", filename_prefix, fileNumber, FILE_TYPES[fileType])`
— confirms the naming convention visible in the real install directly
from code, not just pattern-matched from file names.

**`.RGN` (traced via `load_regions`)**: a direct-seek (no count prefix)
array of 6-byte `RegionIndex` records, each pointing to its own array of
6-byte `RegionEntry` hit-rects (`itemId` + 4 byte-sized coordinates).
Confirms and extends the pre-existing struct definitions exactly.
**Genuinely interesting mechanic surfaced along the way**: stored
coordinates get rescaled at load time based on the active video mode —
`x` is simply doubled, but `y` is scaled by `96/224` or `168/224`
depending on whether the hardware is a shorter-than-224-line mode (EGA/
Tandy-class) or left unscaled for modes that can show the full 224-line
design resolution (VGA-class) — concrete evidence Gateway's region/hit-
test coordinates were authored once at a fixed logical resolution and
adapted per video mode at runtime, not re-authored per mode.

**`.PIC` (traced via `load_picture`/`Image_load`)**: considerably richer
— the picture-numbering scheme derived directly from the code
(`bank = picNumber>>12`, forced to 1 instead of 0 on non-default video
hardware; `fileNumber = bank*100 + ((picNumber>>8)&0xF)`) reproduces
**exactly** the five `GATE_0xx`-`4xx.PIC` file groupings seen in the
real install — strong independent confirmation that the numbering
scheme was decoded correctly, not just guessed from the file names.
Within one physical file, pictures are indexed by a **12-byte
`PicIndexEntry`** at a direct seek (`lowByte * 12`, no count prefix, same
addressing style as `.RGN`'s index), giving a file offset, flags
(embedded palette bit, bit-depth bits, one still-unexplained bit),
frame count, and dimensions. **Pictures can hold multiple frames**, each
with its own `(x, y)` draw-position entry in a small table starting
right at the picture's file offset — a lightweight sprite-sheet-like
mechanism, not just single static images. The real pixel payload
(handed to `PictureDecoder_load`) comes right after that offset table
(and an optional palette block) — its actual encoding wasn't traced this
pass.

**Not yet decoded**: whether the 4 non-zero picture banks really
correspond to the game's four story acts, and a couple of still-unnamed
header/flag bits in each format. `RegionIndex.field_2`'s meaning is also
still open.

### Picture pixel compression decoded — a real LZ77+Huffman hybrid

Same session, immediate follow-on per Paul's direction — the one piece
of the picture pipeline left open above. Traced
`PictureDecoder_load2`/`_unpack`/`_fetch`
(`gatemain.asm:39883` onward); full mechanical breakdown in
[file-formats.md](file-formats.md#picture-pixel-compression--an-lz77huffman-hybrid-plus-per-video-mode-blit).
More sophisticated than a simple RLE scheme — genuine LZ77-style
sliding-window compression with **Huffman-coded tokens** (literal bytes
and match lengths both decoded through canonical-Huffman lookup tables
built from small static constant tables at setup, not fixed-width
codes), similar in spirit to a simplified precursor of Deflate/LZH.
Confirmed: a 4096-byte sliding window, an 8KB double-buffered output
(flushed to the screen in two 4KB halves so the whole decompressed
picture never needs to fit in memory at once), and a terminator token
ending the stream.

**Genuinely satisfying confirmation**: the two video-mode-specific
final "blit" callbacks (`PicFile_copy_nonEga`/`PicFile_copy_ega`,
dispatched from `PictureDecoder_load` by `_videoIndex`) turned out to be
exactly the two pixel-format strategies you'd expect for this era's
hardware — a straight linear byte copy for chunky/packed modes (VGA
256-color), versus the classic 4-plane EGA/Tandy **planar bit-unpacking**
technique (each decompressed byte is a 4-bit "which bitplane(s) does
this pixel belong to" mask, OR'd into up to 4 separate framebuffer
planes at a rotating bit position) for EGA/Tandy-class modes. This
confirms the LZ77 stream itself carries **EGA-native plane data**
directly for those modes, not a device-independent format reinterpreted
per hardware — a real, concrete implementation detail for the ScummVM
renderer to replicate faithfully (it can't just decompress to a generic
RGB/indexed buffer and blit — for EGA modes it needs to reproduce this
exact plane-serial write behavior, or reimplement equivalent semantics).

**Not yet decoded**: `PictureDecoder_getBlockOffset`'s exact bit-packing
for match distances, and the precise per-table semantics of
`_array1`-`_array13` beyond confirming the overall architecture (enough
is understood to describe the compression scheme accurately, not to
reimplement every table from this write-up alone yet).

### `GATE_XXX.FNT`/`GATE_XXX.MUS` traced — one clean win, one honestly murky one

Same session, per Paul's direction to close out the remaining resource
types. Full record-level layouts in
[file-formats.md](file-formats.md#gate_xxxfnt--bitmap-font-files) and
[file-formats.md](file-formats.md#gate_xxxmus--background-music-files-partial-low-confidence).

**`.FNT` (traced via `Font_LoadFont`)**: a clean, simple format —
10-byte header (glyph height/width-in-bytes, printable-char range,
optional-table flags), optional 128-byte variable-width and
variable-spacing tables, then a flat packed 1-bit-per-pixel glyph
bitmap covering only the font's declared printable range. Same
`_videoIndex*100 + number` banking convention as `.PIC`. `fontNumber ==
0` is special — no file at all, a hardcoded default description whose
zero `bytesPerLine` suggests it defers to some other built-in rendering
path rather than an actual loaded bitmap (plausible, not confirmed).

**`.MUS` (traced via `sub_1FE5C`, the periodic background-music-channel
refresh routine)**: considerably murkier, and documented as such rather
than forced into false precision. No struct was pre-defined for this
format going in — the first resource type in this whole session where
that was true. Confirmed the file-numbering convention (same
packed-byte-pair style as pictures/regions) and a **general
architecture**: a small per-song directory record read first to get a
byte-length, checked against an available-memory budget, and only if it
fits, the full track loaded wholesale in a second read — a
lazy/memory-gated full-residency model, philosophically different from
`GATESTR.DAT`'s LRU-cache-of-partial-data approach. The exact 12-byte
record's field-by-field layout wasn't nailed down — flagged as needing a
different approach next time (starting from the sound-hardware-output
side rather than the memory-management side that this pass approached
it from).

**Worth noting for future sessions**: this is the first resource format
in the whole `file-formats.md` set where the investigation didn't reach
full confidence in one pass. Consistent with the project's discipline
throughout — better to document the real uncertainty than to backfill a
clean-looking but unverified record layout.

**Update**: the recommended next angle — hardware-output side — was
picked up several passes later and paid off; see
[`Midi_sendByte` named](#midi_sendbyte-named--the-mus-engines-hardware-output-side-finally-traced)
below.

### Prehandler-chain primitives named

Same session, per Paul's direction to re-run `rank_unnamed_functions.py`
(now that the RTLink-thunk noise is filtered out) and resume general
naming. `sub_11635` (196 callers) was still the single highest-ranked
unnamed target — the same function flagged but left unnamed several
sessions ago, when `Logics_getPrehandlerMode` was still hidden behind
the collapsed-function issue fixed since then. With that fixed, both it
and its sibling `Logics_getPrehandler` are now directly readable, and
between them they fully explain the "prehandler chain" mechanism first
guessed at during the room/logic architecture session:

- **`Logics_getPrehandlerMode(logicNum, stageIndex)`** (already named) —
  for type-6 objects specifically, indexes an **array** of prehandler
  IDs starting at the `LogicSection6._prehandlerId` struct field
  (confirming `_prehandlerId`/`_prehandlerId2`, previously listed as two
  separate struct fields, are actually **one 2-element array** — type-6
  objects support exactly 2 prehandler stages). For every other type, it
  falls through to `Logics_getPrehandler(logicNum)`, which returns a
  single prehandler ID for types `2`/`6`/`7`/`8` only (confirmed via the
  already-named `LOGICTYPE_2`/`_6`/`_7`/`_8` constants) — types `1`
  (`Room`), `3`, `4`, `5` have no prehandler concept at all. Both
  functions return `0` for "no prehandler at this stage."
- **`sub_11635` → `Logics_prehandlerChainReaches`** — walks
  `logicNum`'s prehandler chain stage by stage (bounded per-type via
  `METHOD_SECTION_INFO`, the same table `Logics_getPrehandlerMode`
  consults), and whenever a stage's mode is nonzero, **recurses**,
  treating that mode value as *another* `logicNum` to check against the
  *same* target — i.e. "does `logicNum`, or anything it delegates to
  through its prehandler chain, eventually reach `targetLogicNum`."
  Confirmed via a real call site in `main()`:
  `sub_11635(vocab_list_0._logicNum, Logics_logicNum211)` — both
  arguments are `logicNum`-shaped `proc_table` indices, not "a vocab id
  and a logicNum" as an earlier session's hedge had guessed.
- **`sub_115CE` → `Logics_prehandlerHasMode`** — a sibling doing
  **exact-match** checking instead of recursive delegation: walks the
  same stage-bounded loop, returning `1` as soon as some stage's
  `Logics_getPrehandlerMode` result equals a given `mode` value exactly
  (no recursion), with an optional `Logics_getVal2_2` precondition gate.

Applied both renames via the new `ida_scripts/apply_renames_gatemain.py`
(gatemain.idb's first accumulating rename script, mirroring
`apply_renames_gate.py`'s convention) — DRY_RUN verified, then applied
for real with full export+save.

**Not renamed yet**: `sub_14A37` (80 callers, tiny) was checked next but
its callees (`sub_28E2C`/`sub_1DDF6`/`sub_1E052`/`sub_1E0E8`) are all
themselves unnamed, so a confident name wasn't reachable without a
deeper multi-function trace — left for a future pass rather than
guessed from a generic-looking `filename`-typed parameter alone.

### The scoring subsystem, confirmed by actually decoding real game text

Same session, continuing down the ranked list. `sub_1535E` (92 callers)
turned out to hinge on the exact wording of two `GATESTR.DAT` messages
it prints (IDs `0x803`/`0x804`) — rather than guess from the message-id
numbers alone, wrote `ida_scripts/dump_gatestr_messages.py`: a
**standalone Python reimplementation of `huffman_decompress`** (not an
IDA script — no `.idb` needed, just the file-formats.md write-up's
confirmed algorithm applied directly to the file bytes) and ran it
against Paul's real `c:\games\gw\GATESTR.DAT`. This is the same
technique that unlocked `MONDAIN.EXE`'s naming in the sibling `ultima1`
project (`dump_msg_strings.py` there) — decoding the actual text turns
guesswork into certainty. Kept in the repo as a reusable tool, same as
that precedent, for any future message-id ambiguity:
`python dump_gatestr_messages.py <path-to-GATESTR.DAT> <msgId> [...]`.

The decoded strings settled everything definitively:
- `0x803` → `"[Your score has just gone up by %d."`
- `0x804` → `" NOTE: You can activate and deactivate score-change
  notification using the NOTIFY command."`
- `0x2A` → `"You have achieved a score of %d out of 1600, in %d
  turns."` (the two `%d` args at that call site are `_turnCount` then
  `_score`, matching the message's order exactly)
- `0x29` → `"It is Dorman day %d."` (computed as `_gameTicks/480 + 1`
  right before printing — a real-time game clock, 480 ticks per
  in-universe "Dorman day")

Renamed 7 symbols via `apply_renames_gatemain.py`'s second batch:
`sub_1535E` → **`Score_add`** (adds its argument to `_score`, prints the
`0x803` notification if `_scoreNotifyEnabled`, with the `0x804`
explanatory note shown only once via `_scoreNotifyTipShown`), plus the
five underlying globals (`Persisted_val128`/`_3`/`_11`/`_12`/`_175` →
`_score`/`_turnCount`/`_scoreNotifyEnabled`/`_scoreNotifyTipShown`/
`_gameTicks`) — all of which came from an earlier session's
`SaveField`-table enumeration and had never been given real names.
`Score_add`'s naming follows this codebase's established subsystem-
prefix convention (`Font_`, `Screen_`, `Windows_`, `Regions_`) rather
than the `Logics_` prefix reserved for the core interpreter/dispatch
layer, since this is genuinely a self-contained scoring subsystem, not
part of the method-dispatch mechanism itself.

Worth reaching for `dump_gatestr_messages.py` again any time a
message-id argument to `TextWindow_add`/`get_message`-family calls needs
disambiguating — decoding the real string directly beats inferring
intent from surrounding code shape alone.

### `Stream_*` subsystem named — a generic chunked file loader

Same session, per Paul's direction to trace `sub_14A37` (80 callers)
and its four then-unnamed callees. Turned out to be a self-contained,
reusable subsystem, not anything specific to whatever file happens to
be passed in:

- **`sub_14A37` → `Stream_loadFile(filename)`** — the top-level entry:
  flush the active window's pending text, then read, process, and free
  a whole file's worth of chunked buffers.
- **`sub_1DDF6` → `Stream_readChunks(filename)`** — opens the file,
  `fseek`/`ftell`s to find its total size, then reads it into **up to 8
  separately-allocated RAM buffers** (each up to ~64KB, since a single
  DOS memory allocation can't span an arbitrary-sized file) — a
  32-byte header plus payload per buffer. Abortable at any point via
  `Events_isKeyPending` (a keypress cancels the whole load) or two
  `word_C8582` config-flag bits.
- **`sub_1E052` → `Stream_processChunks()`** — walks the same buffer
  array, dispatching each filled slot to `Stream_processChunk`,
  abortable the same way between chunks.
- **`sub_180E3` → `Stream_processChunk`** — the actual per-chunk
  handler: pulls a couple of header bytes out of the buffer, then calls
  the real processing logic through a **configurable callback function
  pointer** (`word_C84D0`) with the payload — meaning this whole
  mechanism is a generic pluggable streaming framework, not
  hardcoded to one resource type. The callback's actual target wasn't
  identified this pass.
- **`sub_1E0E8` → `Stream_freeChunks()`** — frees all 8 buffer slots
  (`kill_pointer_`), the cleanup step.
- **`sub_28E2C` → `TextWindow_flushPendingText()`** — a smaller, mostly
  independent finding picked up along the way: flushes
  `Windows_pendingText` for the active window via `TextWindow_addDirect`
  if there's anything queued, distinct from the already-named
  `TextWindow_flushText`.

**Confidence note**: the mechanical architecture (chunked buffered
read, player-interruptible via keypress, callback-dispatched
processing, then cleanup) is confirmed with high confidence by direct
read. The *purpose* — era-typical for hiding disk latency behind
something else the player is doing, most plausibly reading on-screen
text given the abort-on-keypress behavior — is a reasonable inference
from that mechanism, not independently confirmed by finding what the
callback actually does with each chunk. `Stream_` is a new subsystem
prefix (this codebase already groups functions by subsystem —
`Font_`/`Screen_`/`Windows_`/`Regions_`/`Score_`/etc. — and nothing
existing fit this one).

Applied all 6 renames via `apply_renames_gatemain.py`'s third batch.
Note for future re-runs of this script: entries referencing an
already-renamed symbol by its *old* name break once that rename has
been applied for real (`idc.get_name_ea_simple` can't find a name that
no longer exists) — every entry in this script now keys by hex address
instead, once confirmed, to avoid exactly that trap.

### `word_C84D0` traced — a shared decoder continuation, not 4 functions

Same session, pulling on the `Stream_processChunk` callback thread per
Paul's direction. `word_C84D0` isn't 4 independent handler functions —
resolving its 4 possible values (`0x220`/`0x238`/`0x493`/`0x6EE`) against
their enclosing segment (`sg09a4`, base `0x1802A`) showed 3 of the 4
land exactly on `loc_` labels **inside** other already-defined
functions, not at function starts (e.g. the `0x238` target sits exactly
32 bytes into what IDA calls `sub_18242`, skipping a precondition check
the `0x220` entry point doesn't skip). This is the same "no clean chunk
boundary" pattern already seen with the RTLink thunks two sessions ago —
several real entry points into **one shared decoder continuation body**,
not separate functions, which is exactly why they'd never gotten real
names: nothing about their shape looked like an independent function to
begin with.

Traced the selector back through `Stream_selectHandler` (was
`sub_18042`, dispatches on mode 0/1/2/4 to wire up
`word_C84D0`/`word_C84D2`/`word_C84D4`) to its only caller,
`Stream_configure` (was `sub_1DDC0`), called exactly once from
`gatemain_start` — and from there straight into `gatemain_start`'s own
`argv` parsing.

**This directly resolves a loose end from the very first `gate.idb`
session**: `gatemain_start` parses `argv[1]`→`Mouse_enablement`,
`argv[2]`→`videoMode` (both already named, confirming the mapping),
`argv[3]`→`cmdline_param3` (now `_soundMode`), and `argv[4]`-`[8]`→
`cmdline_param4`-`8`. That's **exactly** the shape of `gate.idb`'s
`_execl("GATEMAIN.EXE", "GATEMAIN.EXE", xmouse, videoMode, soundMode,
word_2A256, word_2A258, word_2A25A, word_2A25C, word_2A25E, NULL)` call
from three sessions ago — meaning `gate.idb`'s 5 then-unidentified
globals (`word_2A256`/`58`/`5A`/`5C`/`5E`) are precisely
`cmdline_param4`/`5`/`6`/`7`/`8` on this side. `cmdline_param6` (→
`_streamMode`) is confirmed as `Stream_selectHandler`'s mode selector —
**a pure launch-time configuration value**, fixed for the whole game
session, not anything computed during play.

**Not resolved this pass**: what resource type this decoder continuation
actually processes (would need tracing `sub_18148`/`sub_18182`/
`sub_181D8`/`sub_18432`/`sub_18682`/`sub_186F0`, several more unnamed
functions this cluster touches), and what `cmdline_param4`/`5`/`7`/`8`
individually control (only `cmdline_param6`'s role as the mode selector
was pinned down this pass, since it's the one that traces cleanly to
`Stream_selectHandler`).

**`gate.idb` follow-up applied in a later session**: that IDB's
`word_2A256`/`58`/`5A`/`5C`/`5E` (`argv[4]`/`[5]`/`[6]`/`[7]`/`[8]`
respectively) were renamed using this confirmed mapping —
`streamMode`/`gatemainArg4`/`5`/`7`/`8` — closing out that IDB's own
long-standing "5 unidentified globals passed to `GATEMAIN.EXE`" open
item from its very first session. See that IDB's own findings-log
section above.

### CPU speed calibration decoded — refines the `Stream_*` mode dispatch

Same session, kept pulling on the `word_C84D0` thread past the point
above. Turns out `Stream_selectHandler`'s mode branches aren't gated by
resource type at all (as hedged in the previous section) — they're
gated by a **CPU speed rating**, measured fresh via a complete,
self-contained calibration routine using a classic DOS-era technique:

- **`Cpu_beginSpeedTest`** (was `sub_18182`, called unconditionally at
  the top of `Stream_selectHandler`) — saves the real `INT 70h`/`72h`
  vectors and the current DOS date/time, installs a trivial temporary
  ISR (just decrements a counter and `iret`s), and reprograms the
  8253/8254 PIT (ports `43h`/`40h`) to a known frequency.
- **`Cpu_measureSpeed`** (was `sub_18148`) — runs a fixed `imul`-based
  busy loop until the temporary ISR's tick counter hits zero; however
  many loop iterations completed (scaled down) becomes
  **`cpuSpeedRating`** (was `word_C84DA`) — faster CPUs simply get more
  busy-work done per fixed-length timer tick.
- **`Cpu_endSpeedTest`** (was `sub_181D8`) — restores the original `INT
  70h`/`72h` vectors and PIT divisor, then **re-sets DOS's date/time**
  from the values saved at the start — correcting for the clock drift
  that running the PIT at a different frequency would otherwise cause.
  A properly bracketed calibrate/restore pair, not a permanent hook.

`Stream_selectHandler`'s `cmp cpuSpeedRating, 0x160` checks gating each
mode branch are this rating being compared against a fixed threshold —
i.e. genuinely a **performance-tier selector** ("is this CPU fast enough
for the nicer code path"), not a resource-type dispatch as the previous
section's lower-confidence note speculated. This refines rather than
invalidates that earlier naming: `Stream_selectHandler` does select a
handler, `Stream_configure` does configure it — just gated on CPU speed,
now confirmed, rather than resource type, which was never actually
confirmed, only guessed.

**Still open**: what operation is actually being performance-tiered
(still most plausibly something in the chunked file-read/process
pipeline `Stream_readChunks`/`Stream_processChunk` already cover, given
how the mode dispatch and the buffer mechanism share so much state, but
not independently confirmed) — would need `sub_18242`/`sub_18432`/
`sub_18682`/`sub_186F0` and the `word_182FB`/`word_1832F`/`word_1833B`-
family locals traced to close that out.

### It's a digitized PC-speaker sound-effect engine — and the `.RS` files are its samples

Same session, kept reading past the calibration routine into the actual
mode-1 continuation body. This resolves the "still open" question right
above. Full record-level file format in
[file-formats.md](file-formats.md#rs--digitized-pc-speaker-sound-effect-files).

Right after the calibration and mode dispatch, the continuation:
installs a **self-modifying timer-interrupt handler** (writes a source
segment/offset and a couple of parameter bytes directly into immediate
operands of a pre-existing `INT 8` handler template sitting inline in
the same code — classic tight-ISR self-modifying-code technique, not
reading its parameters from memory each interrupt for speed), computes
a PIT divisor from a fixed base-clock constant divided by a rate value,
and then executes the textbook two-instruction PC speaker enable
sequence: `in al, 61h` / `or al, 3` / `out al, 61h` (gates Timer 2 to
the speaker and turns the speaker driver on). This is a **digitized
audio playback engine**, toggling the PC speaker one bit at a time under
interrupt control — precisely the kind of thing 1990s DOS games used for
speech/sound-effect playback without a sound card (the well-known
"RealSound"-style single-bit PC-speaker DAC technique).

**This is almost certainly the playback engine for the real install's
`.RS` files** (`AIR.RS`, `ALARM.RS`, `BIRD.RS`, `BOTTLES.RS`, `BUSY.RS`,
`CAMERA.RS`, etc. — short sound-effect samples, distinct from every
numbered/banked resource type documented above). Checked the header of
5 real `.RS` files directly: all start with a `"STEVE"` ASCII magic,
followed by a version byte (`0x02`) and a second constant byte (`0x48`)
identical across all 5 — and a 16-bit length field that matches
**exactly** `fileSize - 32` in every single case checked, confirming a
fixed 32-byte header. The `0x48` constant is intriguing: the *same*
literal value gets loaded into the 8253/8254 PIT's count register by a
function in this cluster — a real numeric match, worth remembering even
though it wasn't confirmed as a direct "read this header byte into the
PIT" relationship (that function hardcodes the value in its own code).

**This also gives the CPU-speed-tiering its obvious motivation**:
single-bit PC-speaker digitized audio is notoriously sensitive to the
ISR's own execution time — the interrupt handler's code path has to
fit within a precisely known number of cycles to keep sample timing
correct, so a faster or slower CPU genuinely needs a different-shaped
handler (or different timing constants) to play back the same sample
stream correctly. The mystery from the previous section is resolved:
CPU speed gates *which digitized-audio ISR variant* gets installed, not
resource type or file format.

**Not traced further this pass**: the self-modifying ISR's individual
patched fields, `sub_18432`'s mode-2-specific setup (calls `sub_1861F`/
`sub_1863B`, itself doing more PIT/port work not fully read), and the
exact 1-bit sample encoding/decoding scheme. No further renames applied
this pass — the remaining pieces live inside the same unlabeled shared
continuation body as before (no clean function boundaries to rename)
or in functions whose exact role wasn't confirmed confidently enough
yet (`sub_18415`/`sub_18432`/`sub_1861F`/`sub_1863B`).

### `Queue_remove` and `Logics_checkMoveRestriction` named

Same session, per Paul's direction to continue down the (freshly
re-ranked) unnamed-function list. Two clean wins:

- **`sub_12ED2` → `Queue_remove`** — the sibling to the already-named
  `Queue_add`: searches `_queueTable` (confirmed as the same array via
  an identical `seg126_93`/`-0x73FCh` offset access) for a matching
  `_id`, then `memmove`-compacts every later entry down one slot if
  found. A textbook remove-and-compact on a flat array, high confidence
  purely from the shared data access with an already-named sibling.
- **`sub_14B64` → `Logics_checkMoveRestriction`** — confirmed by
  decoding its own printed text via `dump_gatestr_messages.py`:
  `0x800` = `"You can't move while you're wearing the collar."`,
  `0x801` = `"[You get o%sf%s first.]"` (a dismount/disembark-first
  message, `%s` placeholders presumably filling in a vehicle/mount
  name). A shared movement-precondition gate, checked before letting the
  player move, that tests several hardcoded plot-item `logicNum`s
  (`0xD3`/211 confirmed as **the collar**; a couple more — `0xA2`,
  `0xA8`, `0x9D` — not individually identified, plausibly mount/vehicle
  related given the dismount message) through
  `Logics_prehandlerChainReaches`/`Logics_IsPrehandler1`/
  `Logics_prehandlerHasMode`, plus a room-override hook via
  `Logic_call(_roomLogicNum, action=0xF)`. Returns nonzero (with the
  blocking message already printed) if movement is currently
  disallowed — genuinely useful narrative content confirmation: Gateway
  has an in-story restraint item (a collar) that can prevent the player
  from moving, and at least one mount/vehicle situation with the same
  kind of gate.

Applied both via `apply_renames_gatemain.py`'s sixth batch. Also fixed
several more instances of the "stale old-name lookup" fragility flagged
two sessions ago — every entry added since then that had *already* been
applied for real in an earlier turn needed converting to its hex
address before this script would run again; worth double-checking for
this whenever re-running the script after a gap.

### `Logics_tryMoveDirection` named — the room-exit resolution function

Same session, immediate follow-on — `Logics_checkMoveRestriction`'s
only caller in this list, `sub_14ED6` (44 callers), turned out to be the
core room-exit resolution function itself. Confirmed thoroughly by
direct read, not just message decoding this time:

- Its one parameter is the **parsed direction character** (`'n'`/`'s'`/
  `'e'`/`'w'`/etc.), confirmed two ways: compared directly against a
  per-room exit-table entry field, and copied into `Parser_val21` on a
  match (a parser-state global recording the direction just used).
- The current room's exit table (entry count from `sub_12445
  (_roomLogicNum)`) holds one small variant record per defined
  direction — a direction-char byte plus a **1-5 type tag** selecting
  between 5 different exit-resolution shapes: a direct room link, a
  `Logics_getBit`-gated door (bits `0xC`/`0x10`, printing a locally
  embedded `"%sn't open.\n"` string when closed — a literal constant in
  this executable's own data segment, not a `GATESTR.DAT` message), a
  fixed blocked-message table lookup, a `sub_14742`-computed dynamic
  destination, and one more table-based lookup not yet distinguished
  from the others.
- Calls the already-named `Logics_checkMoveRestriction` before actually
  committing to the move, and falls back to the classic `"You can't go
  that way.\n"` when no exit table entry matches the direction at all.

Applied via `apply_renames_gatemain.py`'s seventh batch. **Not traced
further**: `sub_123F3`/`sub_12445`/`sub_14742`'s individual roles, and
the precise distinction between the 5 exit-type branches (only the
door-gated one was read in enough detail to describe confidently).

### `Logics_getRoomMoveEnabled`/`Logics_getRoomExitCount`/`Logics_callSpecialExit` named

Same session, per Paul's direction to trace `Logics_tryMoveDirection`'s
three helpers directly. All three confirmed cleanly:

- **`sub_123F3` → `Logics_getRoomMoveEnabled`** and **`sub_12445` →
  `Logics_getRoomExitCount`** — identical shape: validate the argument
  as a `logicNum`, confirm `proc_table.type == 1` (must be a `Room`, not
  any of the other `LogicSectionN` variants), then return one `Room`
  field — the low byte of `field_16` for the first, the full word at
  `field_18` for the second. `Logics_tryMoveDirection` uses the first as
  an **overall room-level movement gate** (independent of, and checked
  before, `Logics_checkMoveRestriction`'s item-based gating — e.g. the
  collar) and the second directly as the exit-table's entry count.
  Neither `Room.field_16`'s nor `field_18`'s deeper semantics were
  confirmed beyond this mechanical usage — struct field renames left for
  a future `apply_structs_gatemain.py` pass rather than guessed here.
- **`sub_14742` → `Logics_callSpecialExit`** — a function-pointer
  dispatch table (`off_3C862`, bounds-checked to 44 entries), calling
  whichever handler is selected with a single action-code argument.
  This is `Logics_tryMoveDirection`'s exit-type-4 branch — a per-exit
  "special" handler distinct from a plain room link or a
  `Logics_getBit`-gated door, called as
  `Logics_callSpecialExit(exitTableEntryId, 0xF)`, matching the
  action-code convention already established elsewhere (`Logic_call`,
  `room_load`'s action arguments). The individual handler routines in
  the dispatch table weren't traced.

Applied via `apply_renames_gatemain.py`'s eighth batch. Between this
and the two previous passes, the room-exit/movement cluster
(`Logics_tryMoveDirection` and everything it calls) is now reasonably
well documented end to end, short of the 44 individual special-exit
handlers and the two remaining untraced exit-type branches.

### `Game_showEndingMessage`/`Game_endGameMenu` named — the actual win/lose ending

Same session, back to the top of a freshly re-ranked list. `sub_312D1`
(38 callers) turned out to be the game's real win/lose ending, confirmed
definitively by decoding its own message text:

- `0x7816` → `"You have failed.\n"`
- `0x7817` → `"You have won the game, scoring %d out of 1500 points.\n"`
  (the `%d` is `_score`)

`Game_showEndingMessage` branches on `_hasWonGame` (was `byte_CBB6E`),
prints whichever message applies, resets the flag, then calls
**`Game_endGameMenu`** (was `sub_C4AE6`) — the textbook Infocom-style
post-ending prompt: repeatedly asks for a choice and dispatches
`1`=restart, `2`=restore a save (`j_load_game(2)`), `3`=undo
(`Parser_performUndo`), `4`=quit (`j_shutdown`). Called from 38
different places throughout the compiled logic — presumably one call
site per way the player can actually win or lose the game.

Minor curiosity worth a note rather than a rename: this ending message's
`1500`-point max doesn't match the earlier-decoded status message's
`1600`-point max (`"You have achieved a score of %d out of 1600, in %d
turns."`, from the `Score_add`/`_score` session) — plausibly a real
distinction (points needed *to win* vs. the absolute ceiling including
bonus points) rather than an inconsistency, but not independently
confirmed.

Applied via `apply_renames_gatemain.py`'s ninth batch. **Not traced**:
`sub_C48E4`, the actual prompt/choice-reading function
`Game_endGameMenu` calls.

### `Logics_describeContents`/`Logics_countVisibleContents`/`Logics_listContents` named

Same session, back to the top of a freshly re-ranked list again.
`sub_149D8` (28 callers) turned out to be the classic "On/In the X you
see: ..." container/surface description sentence, confirmed by its own
message text: `"\t%cn%s you see"`, where `%c` is `'O'` (`"On"`, when
`prepositionType == 1`) or `'I'` (`"In"`, otherwise), and `%s` is the
far-pointer string `j_printObj` returns for the container/surface
object's own name.

**`Logics_describeContents`** first calls **`Logics_countVisibleContents`**
(was `sub_66DFD`) purely as a gate — prints nothing at all if it returns
`0` — then prints `"On/In <object> you see"`, calls
**`Logics_listContents`** (was `sub_667D0`) to print the actual
comma-separated content list, and closes with `".\n"`. Both helpers walk
the *same* linked list of contained items (`Logics_getUnkHandler` for
the first entry, then `Logics_getVal1` repeatedly for each next one,
terminated at `0`), filtering on `Logics_getBit(item, 8)` — consistently
used across this cluster as a visibility/hidden flag, though not
independently confirmed beyond this usage. `Logics_countVisibleContents`
just counts matches; `Logics_listContents` prints each one's name,
inserting `","` between entries via `TextWindow_addChar`.

Applied via `apply_renames_gatemain.py`'s tenth batch.

**Tooling bug found and fixed while refreshing the two thunks these
renames made stale** (`thunk_sub_66DFD`/`thunk_sub_667D0`, now pointing
to newly-named targets): `apply_rtlink_thunks_gatemain.py`'s discovery
filter only accepted function names still starting with `sub_`, which
worked for the very first pass but meant **every one of the 712 already-
renamed `thunk_*` functions became invisible to the script on any later
re-run** — the "safe to re-run this script" claim in its own maintenance
note was actually broken the whole time, silently reporting "0 thunks
renamed" instead of refreshing anything. Fixed by also accepting names
already starting with `thunk_` as scan candidates, in both
`apply_rtlink_thunks_gatemain.py` and `find_rtlink_thunks.py` (for
consistency). Confirmed working: the fixed re-run found and updated
exactly the 2 stale thunks from this session, leaving the other 710
correctly untouched.

### `Game_updateStatusLine` named — the in-game clock/status-bar builder

Same session, top of the list again. `sub_136AF` (27 callers) builds and
displays the game's status-line text combining the current room's name
with an in-game date/time — confirmed by reading its two literal format
strings directly (embedded in `gatemain.exe`'s own data segment at
`seg067+0x227`/`+0x238`, **not** `GATESTR.DAT` messages — resolved with a
small throwaway IDA script rather than `dump_gatestr_messages.py`, since
the far pointer's segment wasn't the `0xF000` sentinel):

- `"%s %d, %02d:%02d"` — 24-hour format
- `"%s %d, %d:%02d%c"` — 12-hour format, `%c` is `'a'`/`'p'` for am/pm

Computes hour-of-day and minute from **`_gameMinutes`** (was
`Persisted_val4`: `/60` then `%24` for the hour, `%60` for the minute)
and a day number from **`_gameDayNumber`** (was `Persisted_val5`, plus a
fixed `+0x10` offset — plausibly a calendar-epoch shift). This is a
**second, more granular clock** distinct from the already-named
`_gameTicks` (480 ticks/day, used only for the simpler "It is Dorman day
%d." narrative message) — both track the same underlying real-time
progression, just at different resolutions for different displays.
Entirely blanked (spaces instead of a real time) when **`_statusTimeHidden`**
(was `Persisted_val7`) is set.

**Not renamed**: `sub_160E1` (the function `Game_updateStatusLine` hands
its finished string to) turned out to be a generic bounded string-copy
utility on inspection, not confidently a "set title/status bar" function
in its own right — left alone rather than mis-attributing a role to it.
Also left open: the 12-entry name lookup table `Game_updateStatusLine`
consults (`seg067_0+0x1FEh`, not decoded), and `Persisted_val194`'s exact
role in selecting the 24-hour vs. 12-hour format (only confirmed as
"compared against the literal value 8," not what that value represents).

Applied via `apply_renames_gatemain.py`'s eleventh batch.

### `TextWindow_showMorePrompt` named — the classic "-- MORE --" pagination prompt

Re-ran `rank_unnamed_functions.py` again; new top target `sub_1496B` (24
callers, 109 bytes). Direct read confirms the well-known "-- MORE --"
screen-pagination idiom:

1. Flushes an (always-empty, in this binary's data) buffer via the
   already-named `TextWindow_addDirect` — `unk_CBB7B` is a single `db 0`
   immediately after the already-named `_scoreNotifyTipShown` global.
2. Saves the current text cursor position (`get_text_cursorPos`).
3. Writes the literal string `"- MORE -"` (`aMore_1`, at `0xCBB7C` —
   confirmed to be the very next byte after `unk_CBB7B`) via
   `Text_writeString`.
4. Polls `j_Events_waitForPress`/`Events_checkKeypress` in a tight loop
   until a key is actually pressed.
5. Restores the saved cursor position.
6. Overwrites the prompt with 8 literal space characters (the string
   right after `aMore_1` in the data segment, at `0xCBB85`), blanking it.
7. Calls `TextWindow_resetFontLinesRemaining` to reset the page's
   line-count budget so another screenful can accumulate before the next
   prompt.

24 callers scattered across many unrelated logic/method routines —
confirms this is generic pagination plumbing invoked wherever paginated
text output fills the active window, not something owned by one
subsystem. `Text_writeString`'s calling convention was also clarified in
passing: it takes a real `ds:offset` far pointer to a string (confirmed
against an existing call site passing `offset aCriticalErrorU`), not a
`GATESTR.DAT` message ID — the earlier `Game_updateStatusLine` finding's
"literal format strings" pattern is the same mechanism, just via a
different code path (`seg067` there vs. the default `ds` segment
`sg4d43` here).

Applied via `apply_renames_gatemain.py`'s twelfth batch. Hit the
recurring stale-old-name gotcha again: the batch's `Persisted_val4/5/7`
entries (added last pass as string lookups, before their rename had
landed) failed with `name not found` once the twelfth-batch dry-run ran
against an IDB where they'd already been renamed to
`_gameMinutes`/`_gameDayNumber`/`_statusTimeHidden` — fixed by switching
those three entries to their confirmed hex EAs (`0xcb800`, `0xcb802`,
`0xcbb6f`), same fix pattern as every prior occurrence of this bug.

### `Logics_takeObject` named — the TAKE-command mechanics

Re-ran `rank_unnamed_functions.py` again; new top target `sub_153B6`
(22 callers). Confirmed directly and conclusively: its **one real call
site** (`sub_6AD19`, the TAKE verb's top-level dispatcher) prints the
literal message `"You take%s"` (`aYouTakeS`) immediately before calling
it — this is the actual TAKE command mechanics.

`Logics_takeObject(logicNum)`:

1. Bails out (returns 0) if `thunk_sub_67662(logicNum,
   Logics_logicNum211, 0)` returns 2 — object can't be taken (e.g.
   fixed scenery).
2. Otherwise reassigns the object's handler to `Logics_logicNum211`
   (via the already-named `Logics_updateHandler`) and clears bit 8 —
   the same "hidden/visible" bit already established in the
   `Logics_*Contents` cluster.
3. Awards a one-time pickup score bonus: reads a per-object score value
   via the new **`Logics_getTakeScore`** (was `sub_12109`), adds it to
   `_score` via `Score_add` if nonzero, then zeroes it via
   **`Logics_setTakeScore`** (was `sub_12179`) so the same item can't be
   scored twice.
4. Sets bit 2 ("taken"), clears bit 0xA, sets bit 0x1D, clears bit 8
   again, and returns 1.

`Logics_getTakeScore`/`Logics_setTakeScore` are a bounds-checked
(against `METHODS_COUNT`) getter/setter pair over the same `proc_table`
type-tagged-struct field (`Room.field_E` / `LogicSection2.field_20` /
`LogicSection8.field_14` — one field slot, three struct shapes, same
dispatch idiom as the already-named `Logics_getVal2_2` family).
Confirmed generic (not take-specific plumbing) since every call site —
both here and in the similarly-shaped take logic inside `sub_143F3`
(itself not renamed this pass, a plausible "take from container/other
room" generalization worth tracing next) — immediately `Score_add()`s a
nonzero result and zeroes it right after, the identical one-time-bonus
pattern.

`sub_6AD19` itself (the TAKE verb dispatcher) wasn't renamed this pass:
alongside the plain single-object TAKE path just described, it has two
other branches whose exact semantics aren't yet confirmed — a
`vocab_list_0._logicNum == 0xD8` / `Parser_number == '*'` special case
that loops calling `thunk_sub_71F59` up to 4 times (plausibly a
multi-object or "take all" form, not confirmed), and a
`vocab_list_0._textP+2 == 1` branch that swaps `_roomLogicNum` and
fires `Logic_call` actions `0xF`/`0xD`/`0xE` (plausibly taking a
room-logic-driven "exit" object rather than a normal item, not
confirmed). Left open for a future pass.

Applied via `apply_renames_gatemain.py`'s thirteenth batch.

### `Mouse_pollPosition` named — and an animated-picture-overlay subsystem sighted but left unnamed

Re-ran `rank_unnamed_functions.py` again. New top target was actually
`sub_26F2A` (19 callers) — investigated first, but ultimately **left
unrenamed**. It zeroes a `word_C9658`-sized array (`unk_D2302`, 2 words
per entry) and clamps another parallel array (`byte_D22EE`) to 0/1 for
the same count, then calls `Events_checkKeypress`. Tracing `word_C9658`
led into a real subsystem — `Image_Init`/`Image_Free`/`Image_load`/
`Image_draw`, plus three still-unnamed neighbors (`sub_26D7E`
registers a new slot with up to 20 frames into a fixed 5-slot table at
`0xA3BA`, capped by `word_C9658`; `sub_26F74` is a real per-slot
animation-timing/draw loop using `_clock`/`_rand` and `Image_draw`;
`sub_27134` is the "already initialized" per-tick driver `sub_26D50`
calls once `word_C9658 != 0`) — clearly an **animated picture-overlay
engine** (randomly-timed multi-frame sprite animations layered on room
scenes, up to 5 concurrent, up to 20 frames each). Confidently
characterized at the architecture level, but `byte_D22EE`/`unk_D2302`
specifically (`sub_26F2A`'s own two arrays, at addresses far from the
`0xA3BA` per-slot table the other three functions share) have no
downstream reader visible in this IDB — their consumer is presumably
inline in the data-driven compiled room/object logic itself, per the
established "room/logic is compiled machine code" architecture finding.
Not confident enough to name `sub_26F2A` or its two arrays yet — left
open for a future pass that traces actual compiled room logic bytes,
not just engine-side plumbing.

Moved down the list to `sub_24FFB` (16 callers) instead, which resolved
cleanly. Confirmed via its one real caller — the already-named
`get_mouse_input`, which passes `&x`/`&y` straight through at a real
call site. **`Mouse_pollPosition(xPtr, yPtr)`**: if both pointers are
null, just forwards to the already-named `get_mouse_buttons()`.
Otherwise, if `mouseState`'s keyboard-cursor-emulation bits are set,
reads one key (`sub_25216`, a private single-caller helper, not
renamed) to move a keyboard-driven pointer and feeds Enter/Space
through `addCharacter` as a click; else reads the real mouse position/
buttons via `INT 33h` (`AH=3`), first waiting out an already-held
button via `sub_24FAE` (also private/single-caller, not renamed).
Writes the resulting x/y through the two far-pointer arguments and
returns a button/click code. In passing, confirmed `sub_26F74` (the
animation-timing loop above) is invoked directly from
`get_mouse_input`'s own body — the engine advances room animations
while polling for input, not on a separate timer/thread.

Applied via `apply_renames_gatemain.py`'s fourteenth batch.

### `Logics_printKeyedMessage` named — a generic keyed-message-table lookup+print utility

Re-ran `rank_unnamed_functions.py` again; `sub_26F2A` (19 callers)
still tops the list but remains left unnamed for the same reason as
last pass. Moved down to `sub_148E8` (17 callers), which resolved
cleanly.

`Logics_printKeyedMessage(key, table, count)`: `table` is a far
pointer to `count` 6-byte records — `(word key, dword
far-pointer-to-message-string)`. Scans for a record whose key equals
`key` **or is `0`** (a wildcard/default entry); if that record's
message pointer is non-null, prints it (`TextWindow_add`) and returns
1. If the matched record's message is null, it keeps scanning
**forward** through subsequent records — regardless of their own key —
for the first one with a non-null message, and prints that instead:
an empty-message match is effectively a placeholder pointing at a
shared fallback message stored a little further down the same table.
Returns 0 if nothing ever matches.

Confirmed generic (the `key` argument isn't tied to one particular
source) via two different call sites:

- `sub_76A79` (a compiled logic method, reached through
  `j_method158`) passes `vocab_list_0._altVocabId` against a 6-entry
  table.
- `sub_87302` is a thin one-argument wrapper — `sub_87302(x)` just
  calls `Logics_printKeyedMessage(x, seg182:0, 0x37)` — passing its own
  argument straight through against a completely different, 55-entry
  table.

This is the same "per-object special-response override table, falling
back to shared generic text" pattern already familiar from the
compiled room/object logic architecture — just factored out as a
single reusable engine primitive rather than duplicated inline in every
method.

Applied via `apply_renames_gatemain.py`'s fifteenth batch.

### `AnimPics_freeAll` named — correcting last pass's guess about `sub_27134`

Re-ran `rank_unnamed_functions.py` again; `sub_26F2A` still tops the
list, still left unnamed. Moved down to `sub_27134` (17 callers) — the
same function last pass's writeup mentioned in passing (and
mischaracterized) while sighting the animated-picture-overlay
subsystem.

Direct read shows `sub_27134` is **not** a per-tick driver — it's the
subsystem's **teardown/free-all** routine: for each active slot (`0..
_animPicsSlotCount`, was `word_C9658`), it frees the slot's 20 `Image`
records via the new **`Image_freeFrames`** (was `sub_26D08`, confirmed
as a trivial "`Image_Free` on N consecutive `Image`-sized records"
helper) and then `kill_handle()`s the slot's own memory handle
(**`animPicsHandles[i]`**, was `byte_D22DA` — confirmed as a 5-slot
array of dword handles), zeroing it. Finally resets
`_animPicsSlotCount` to 0. Renamed as **`AnimPics_freeAll`**.

Its caller, `sub_26D50`, is now **`AnimPics_resetForRoom`**: if
`_animPicsSlotCount` is already nonzero, it tears everything down via
`AnimPics_freeAll`; otherwise (first-ever call) it just zeroes the
10-word per-slot frame-duration/timing table at `0xA3BA` that the
still-unnamed `sub_26D7E`/`sub_26F74` (slot-register and per-slot
timing/draw loop, respectively — both already characterized in the
previous pass's investigation) maintain. Called from `graphics_init` —
the room-transition reset point for the whole subsystem.

**Correction to last pass's overview.md entry**: it described
`sub_27134` as "the 'already initialized' per-tick driver `sub_26D50`
calls once `word_C9658 != 0`" — mechanically accurate about *when*
it's called, but wrong about *what it does*: it tears down, it doesn't
drive per-tick updates. That per-tick timing/draw role belongs to
`sub_26F74` instead (still unnamed).

Applied via `apply_renames_gatemain.py`'s sixteenth batch.

### Sound-track-selection subsystem sighted — `get_buffer_size` confirmed, `startGame?` flagged as mislabeled

Re-ran `rank_unnamed_functions.py` again; `sub_26F2A` still tops the
list, still left unnamed. Moved down to `sub_15DB2` (15 callers, called
from many different compiled logic routines — consistent with "change
the current room/scene's music or sound" being invoked from many room
scripts).

Tracing it led straight back into the already-documented `Stream_*`/
digitized-sound-engine subsystem — it reads and writes the very same
`word_C8582` config-flags global already established there. Mechanics
(not renamed, see below): compares two requested track/section IDs
against the currently-playing ones (`word_C856E`/`word_C8580`),
no-ops if either already matches; otherwise calls the already-flagged
`startGame?(0xFFFF)` (see below), looks up up to 4 associated
sub-resource IDs per section via a table at `+0x3D8E` and
**`sub_15F35`** (itself a small lookup: given a cue ID, scans a
37-entry table by key and returns one of two associated resource
variants depending on `word_C8582` bit 4 — plausibly a streamed-vs-
preloaded or quality-tier choice), then resets several buffer-
bookkeeping globals and calls `get_buffer_size`/`sub_1FE5C`.

Two concrete findings, one confirmed and applied, one flagged for a
future pass:

- **`get_buffer_size`** (was `get_buff_size?`): confirmed exactly as
  its tentative name already said — finds the largest free memory
  block, reserves a fixed amount depending on video mode (halved if
  an image is currently active), returns whatever's left. Simply
  dropped the uncertainty-marking `?` now that it's directly verified.
  Applied.
- **`startGame?` is almost certainly mislabeled** — a leftover guess
  from an earlier, less-confident pass, per the standing "nothing
  should be presumed accurate" caution. Its real body (called from
  `sub_15DB2` with the literal argument `-1`) has nothing to do with
  starting a new game: it tests the same `word_C8582` stream-config
  bits, compares its argument against the currently-playing track ID
  `word_C8580` (proceeding only on an exact match or `-1` = "any"),
  and — under interrupts disabled (`cli`/`sti`) — calls `sub_20390`
  and `sub_1F910` and busy-waits on buffer-drain-looking conditions.
  This reads far more like **"stop the currently playing sound/music
  stream (or force-stop unconditionally if `-1`)"** than anything
  game-start-related. **Not renamed** this pass — the exact semantics
  of `sub_20390`/`sub_1F910` and the two resource-variant meanings in
  `sub_15F35` aren't confirmed yet, and this deserves a dedicated pass
  rather than a rushed guess replacing one wrong tentative name with
  another.

`sub_15DB2` itself also left unrenamed pending that same follow-up
pass — its own role (something like `Sound_selectTrack`) is reasonably
clear at the architecture level, but not yet nailed down precisely
enough for a confident name.

Applied via `apply_renames_gatemain.py`'s seventeenth batch (one
rename: `get_buffer_size`).

### A suspected new RTLink-flattening-bug instance sighted, and skipped

Re-ran `rank_unnamed_functions.py` again. `sub_26F2A` still tops the
list (still left unnamed). Next was `sub_4A69F` (15 callers) — but its
disassembly is unreliable: it and its immediate neighbors `sub_4A65C`/
`sub_4A663` all show `; sp-analysis failed`, share jumbled/overlapping
labels, and contain far calls to suspicious literal targets (`call far
ptr 0:1A4h`, `call far ptr 2E3h:114h`) that don't resolve to any real
symbol — exactly the shape the project's known RTLink-flattening-tool
bug would produce (an intra-segment far call whose segment word didn't
get patched, leaving the offset reliable but the segment wrong/blank).
Not confirmed further and not renamed — the function boundaries
themselves are suspect here, so any name would rest on unreliable
disassembly. Flagged for whoever eventually audits the flattening
tool's output more broadly; moved on to a different target.

### `Midi_sendByte` named — the `.MUS` engine's hardware-output side, finally traced

Moved down to `sub_1D896` (15 callers), which resolved cleanly into a
genuine breakthrough. This is the exact angle the earlier `.MUS`
investigation explicitly flagged as needed next ("starting from the
sound-hardware-output side rather than the memory-management side" —
see the "one clean win, one honestly murky one" section above).

`Midi_sendByte(byte)`: polls the status port (**`_midiStatusPort`**,
was `word_C83AC`) for bit `0x40` clear — MPU-401's "output not ready/
busy" bit — with a `0xFFFF`-iteration timeout; once clear, writes
`byte` to the data port (**`_midiDataPort`**, was `word_C83AA`) and
returns 1, or 0 on timeout.

Confirmed as genuine MPU-401 UART-mode MIDI hardware output (ruling out
an earlier passing hypothesis of a printer, which the port-polling
shape alone couldn't distinguish) via the surrounding cluster:

- **`sub_1D966`** (not renamed) configures `_midiDataPort`/
  `_midiStatusPort` from a caller-supplied base port, then installs a
  real DOS interrupt vector at `IRQ+8` and unmasks it in the 8259 PIC —
  textbook MPU-401 IRQ-driven setup.
- **`sub_1EE70`** (not renamed) reads a 3-byte big-endian value via
  **`sub_1ECB6`** (a generic "read byte at offset N from track #X's
  current stream position" accessor) at offsets 2/3/4 — exactly the
  shape of a Standard MIDI File tempo meta-event (`FF 51 03 tt tt
  tt`) — and its subsequent 32-bit arithmetic involves the literal
  constant `500000`, which is MIDI's standard default
  microseconds-per-quarter-note unit, before calling `Midi_sendByte` to
  actually transmit the result.

Not fully unified into one complete confirmed picture yet (the exact
tempo-to-port-byte-sequence formula in `sub_1EE70` wasn't nailed down
precisely, and `sub_1D966`/`sub_1ECB6`/`sub_1EE70` weren't renamed this
pass), but this is real, concrete progress on the piece the earlier
`.MUS` writeup explicitly called murky — worth a dedicated follow-up
pass to finish unifying with `sub_1FE5C` (the already-flagged periodic
background-music-channel refresh routine) into one confirmed `.MUS`/
MIDI playback engine writeup.

Applied via `apply_renames_gatemain.py`'s eighteenth batch (three
renames: `Midi_sendByte`, `_midiDataPort`, `_midiStatusPort`).

### `Clock_delayTicks` named — a simple busy-wait delay primitive

Re-ran `rank_unnamed_functions.py` again; `sub_26F2A`/`sub_15DB2`/
`sub_4A69F` remain at the top for the reasons already documented.
Moved to `sub_288F4` (14 callers), which resolved cleanly and simply.

`Clock_delayTicks(loTicks, hiTicks)`: records the current `_clock()`
value, then busy-loops calling `_clock()` again until the elapsed
32-bit tick count (`current - start`) is `>=` the `(loTicks, hiTicks)`
32-bit target. A generic timing/delay primitive — used by the
already-named `Screen_fadeOut` to pace fade steps, and by `sub_26C0C`
(part of the `Image_Init`/`AnimPics`-adjacent cluster, not renamed)
for similar pacing.

Applied via `apply_renames_gatemain.py`'s nineteenth batch.

### `Speaker_playErrorBeep` named — PC-speaker tone generation confirmed

Moved to `sub_2899D` (14 callers), which resolved into another clean
win: a real, distinct **PC-speaker square-wave tone generator**,
separate from the digitized-sample PC-speaker playback engine
documented earlier.

**`Speaker_playTone(freqLo, freqHi, durationLo, durationHi)`** (was
`sub_28920`) is the classic sequence: enable the speaker (`in al,61h;
or al,3; out al,61h` — the exact idiom already flagged in project
memory), program PIT counter 2 for square-wave mode (`out 43h, 0B6h`),
write the 16-bit frequency divisor as two sequential bytes to the
counter's data port (`out 42h`), hold for the given duration via the
just-named `Clock_delayTicks`, then disable the speaker again (`in
al,61h; and al,0FCh; out al,61h`).

**`Speaker_playErrorBeep`** (was `sub_2899D`) calls it twice — a short
~4004Hz tone (divisor 298) for 50 ticks, a 50-tick gap, then the same
tone again — a double-beep. Confirmed as specifically an **"invalid
selection" error sound** via a real call site: `sub_28595` (itself
reached from the already-named `get_mouse_input`) plays it and returns
0 exactly when a clicked region index is out of range or maps to an
empty/invalid table entry, versus its normal path (a valid click),
which inserts the selected character into the input line and returns
1.

Applied via `apply_renames_gatemain.py`'s twentieth batch.

### `Opl2_writeRegister` named — an AdLib/OPL2 FM-synthesis subsystem sighted

Moved to `sub_1CAF6` (13 callers), which uncovered a genuinely new
sound-hardware subsystem in one shot: **AdLib/OPL2 FM synthesis**,
distinct from both the digitized-sample PC-speaker engine and the
`Midi_sendByte`/MPU-401 cluster documented earlier.

**`Opl2_writeRegister(reg, value)`** (was `sub_1CAF6`): writes `reg`
to the OPL2 address port, does several dummy port reads (the chip's
required inter-write settling delay), writes `value` to the data port
(address port + 1), then several more dummy reads — a longer delay,
matching the OPL2's well-documented longer settling time after a data
write than after an address write. The port variable it uses
(**`_opl2BasePort`**, was `word_D3BD0`) is confirmed assigned the
literal `0x388` elsewhere in the same overlay — the standard AdLib/
OPL2 base I/O address. Notably, **IDA's own inline comment on this
exact `out dx,al` instruction ("DMA controller, 8237A-5, channel 0
base address and word count") is a stale auto-annotation** — it
matches the literal port value `0` in isolation (IDA's hardware-port
database default guess for a bare `out 0,al`), but the actual runtime
port comes from `_opl2BasePort`, confirmed `0x388`, not `0`. A good
reminder that IDA's automatic port/register comments describe what an
immediate operand *could* mean in isolation, not necessarily what a
variable-driven I/O actually targets at runtime.

Its only caller, `sub_1CB32` (not renamed), computes and writes OPL2
registers `0xA0+channel`/`0xB0+channel` — the chip's real per-channel
frequency-LSB and octave/key-on/frequency-MSB registers — strongly
suggesting an `Opl2_setChannelFrequency`-shaped function. Left
unrenamed this pass (the exact frequency/octave-table math wasn't
worked out), a natural next step alongside unifying the whole
music-engine picture with the still-open `Midi_sendByte` cluster.

Applied via `apply_renames_gatemain.py`'s twenty-first batch.

### `Logics_autoTakeObject` named — the "Taking the key first" mechanic

Moved to `sub_143F3` (12 callers, called directly from `main()`) — this
was already flagged as a suspected relative of `Logics_takeObject` when
that pass named `Logics_getTakeScore`/`Logics_setTakeScore`. Confirmed
conclusively by decoding the real `GATESTR.DAT` message it prints:
`msgId 0xC406` → **`"[Taking%s first.]\n"`** — the classic
parser-adventure idiom where, say, `UNLOCK DOOR` with the key lying on
the floor first auto-picks it up and announces `"[Taking the key
first.]"` before the real command proceeds.

`Logics_autoTakeObject(logicNum)` only proceeds if all of: `logicNum`
matches the current parser subject (`Logics_logicNum211 ==
Parser_val2`), the object's prehandler type is `7`, bit `0x1D` is set
(a flag `Logics_takeObject` itself sets on a normal take — plausibly
"portable"/"auto-takeable"), bit `0xA` is clear,
`Logics_prehandlerChainReaches(logicNum, Logics_logicNum211)` is
**false**, and `thunk_sub_67662(logicNum, Logics_logicNum211, 0)`
returns 0. If every condition holds, it prints the object's name plus
the `"[Taking%s first.]"` message, then runs the **exact same
take-mechanics tail as `Logics_takeObject`** — handler reassignment,
clearing bit 8, the one-time `Logics_getTakeScore`/`Score_add`/
`Logics_setTakeScore` bonus gated on bit 2, then setting bit 2.

Not every precondition's exact meaning was pinned down this pass —
prehandler type `7` and bit `0xA` specifically are still open — flagged
rather than guessed.

Applied via `apply_renames_gatemain.py`'s twenty-second batch.

### `AnimPics_resyncSlots` named — another piece of the animated-picture-overlay subsystem

Moved to `sub_26EDC` (12 callers), which slotted straight into the
already-documented `AnimPics_*` cluster: it uses the already-named
`_animPicsSlotCount` and the same `0xA3BA`-ish per-slot table
`AnimPics_resetForRoom`/`sub_26D7E`/`sub_26F74` maintain.

`AnimPics_resyncSlots()`: for each active slot, resets the per-slot
frame-index byte (the same field `sub_26F74` reads as its frame
position) to either `0`, or `frameCount-1` if the slot's loop-direction
byte reads `0xFF` (playing in reverse) — then adds the current
`_clock()` value into a per-slot 32-bit accumulator, re-basing each
slot's animation-timing clock to *now*. Confirmed called from the
already-named `room_load`: this is the "resync all active animation
timers to the current clock" step run after a room transition,
preventing animations from jumping forward using stale elapsed time
accumulated while the game was paused or loading.

Applied via `apply_renames_gatemain.py`'s twenty-third batch.

### `Queue_find` named — a companion to `Queue_remove`

Skipped `sub_4A616` (12 callers) — another `sp-analysis failed` tiny
stub in the same suspicious `0x4A6xx` neighborhood as the `sub_4A69F`
cluster flagged two passes ago, reached both by a normal `call` and by
a cross-segment `jmp` from a different overlay (`seg101:0029`)
straight into its body. It just sets a return value of 0 or 1 with no
arguments, so its semantic role is entirely caller-dependent; not
pursued further.

Moved to `sub_12F81` (11 callers), which resolved immediately via a
direct structural comparison: it's the read-only companion to the
already-named `Queue_remove` — the exact same `_queueCount`-bounded
scan over the exact same 4-byte-entry table at the exact same
`seg126_93`-relative offsets, matching on the same key byte. Returns
the matching entry's stored word if found, or the sentinel `0x7FFF` if
not — a find/peek operation where `Queue_remove` additionally deletes
the match. Named **`Queue_find`**.

Applied via `apply_renames_gatemain.py`'s twenty-fourth batch.
