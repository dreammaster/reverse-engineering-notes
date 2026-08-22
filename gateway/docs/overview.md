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

### `load_and_scale_pic` named

Moved to `sub_265B0` (11 callers). Resolved via its two helper calls'
own caller lists: both `sub_25B90` and `sub_25BCE` are also called
directly by the already-named `scale_pic` (itself reached from
`Image_load`), so `sub_265B0` is running the exact same load-and-
scale-to-fit sequence `scale_pic` uses, just starting from a picture
number (via the already-named `load_picture`) instead of an
already-loaded picture. Named **`load_and_scale_pic`**, matching the
existing lowercase-underscore convention of its immediate neighbors
rather than inventing a new style for this small, closely-related
cluster. Many unrelated callers (`Commset_show`, `sub_7179E`,
`sub_74149`, and 8 more) confirm it's a generic "load and display
picture N, scaled" entry point, not owned by one subsystem.
`sub_25B90`/`sub_25BCE` themselves left unrenamed.

Applied via `apply_renames_gatemain.py`'s twenty-fifth batch.

### `AnimPics_registerSlot` named — the last piece of the `AnimPics` cluster

Moved to `sub_26D7E` (10 callers) — already fully characterized (but
left unrenamed) back when `AnimPics_freeAll`/`AnimPics_resetForRoom`
were named. Finalizing it now closes out the `AnimPics_*` cluster.

**`AnimPics_registerSlot(picNumber, frameCount, loopDirection,
duration)`**: registers a new active animated-picture slot (capped at
5 via `_animPicsSlotCount`, `frameCount` capped at 20) — allocates a
handle (`new_handle`), clears `frameCount` `Image` records via the new
**`Image_clearFrames`** (was `sub_26C88` — the zero/init counterpart to
the already-named `Image_freeFrames`), loads each frame via the
already-named `Image_load` (bailing out and freeing everything if any
frame fails to load), then stores `frameCount`/`loopDirection`/
`duration` into the same per-slot table `AnimPics_resetForRoom`/
`sub_26F74` maintain, and increments `_animPicsSlotCount`.

With this, the `AnimPics_*` cluster — register, free-all, reset-for-
room, resync-slots, and now clear-frames — is fully named. Only
`sub_26F74` (the per-slot timing/draw loop) and the still-unconfirmed
`sub_26F2A` remain open in this subsystem.

Applied via `apply_renames_gatemain.py`'s twenty-sixth batch.

### `Window_destroy` named

Moved to `sub_28BB7` (10 callers), confirmed directly by its body: a
full window-teardown function, distinct from the already-named
(lighter-weight) `Window_close`. `Window_destroy(windowNum)` validates
`windowNum` in `[0,6)`, then calls `Window_close`, releases any
reserved regions (`Windows_ReserveRegions(windowNum, 0)`), zeroes
`Windows_x2[windowNum]` (the per-slot "in use" flag), rescans all 6
slots to recompute `Wndows_numWindows` (highest active index + 1), and
clears `Windows_activeWindow` to `-1` if this was the active window —
a strict superset of `Window_close`, not a synonym for it.

Applied via `apply_renames_gatemain.py`'s twenty-seventh batch.

### `Game_restartAfterDeath` named — the player-death handler

Skipped two more candidates that turned out to be more of the same
trouble already flagged: `sub_474F8` (10 callers — unresolved far
calls to segment `0x802` and a jump into the middle of a local label)
and `sub_4A722` (9 callers — a bare mid-function fragment with no
prologue, in the same `0x4A6xx`/`0x4A7xx` neighborhood as the
already-flagged `sub_4A69F` cluster). Neither renamed.

Moved to `sub_9E8DF` (9 callers, 1007 bytes, reached via a real thunk
— a substantial function). Confirmed conclusively via **two separate
real call sites**, each printing a decoded `GATESTR.DAT` death message
immediately before calling it:

- `msgId 0x41E`: *"Possessed by some crazed notion, you leap from the
  cliff walkway into the abyss."* — a `JUMP`-off-a-cliff easter egg.
- `msgId 0x4008`: *"...he slices off a bit more of your neck than you
  can afford to lose."* — killed by an enemy's axe.

Both lead directly into this same function — it's the **player-death
handler**. `Game_restartAfterDeath()`: calls the still-unconfirmed
`sub_26F2A` and the already-named `AnimPics_freeAll`, then — gated on
whether this is the player's first death or a repeat one
(**`_deathCount`**, was `word_CE8A8`) — either shows a "you have died"
picture directly, or pauses (`TextWindow_showMorePrompt`) and routes
through `sub_15674` (a major hub function seen repeatedly throughout
this project but still not renamed) with a death picture/message pair.
Afterward it resets `_roomLogicNum`'s handler, object `0x28`'s
handler, a large swath of `Persisted_valNNN` globals (95 through at
least 124), and roughly a dozen individual objects' handlers
(logicNums `0x21`-`0x40`) back to their initial values — effectively
restarting the game's state in place after the player dies, with no
separate confirmation prompt visible in this function itself (any
"play again?" framing likely happens in whatever calls it, or isn't
offered at all for these particular sudden-death scenarios).

This sits alongside the already-documented `Game_showEndingMessage`/
`Game_endGameMenu` (the "official" win/lose ending) as a second,
simpler death-handling path — used for the game's many scattered
instant-death scenarios (traps, monsters, misadventures) rather than
the deliberate ending sequence.

Applied via `apply_renames_gatemain.py`'s twenty-eighth batch.

### `invoke_callback` named

Moved to `sub_1019C` (8 callers), a trivial one-liner: `push bp; call
[far ptr arg]; pop bp; retf` — calls the far function pointer passed
as its only argument, no further arguments forwarded. A minimal
indirect-call trampoline. 8 callers across otherwise-unrelated
functions confirm it's generic plumbing rather than anything tied to
one specific callback's role. Named **`invoke_callback`**.

Applied via `apply_renames_gatemain.py`'s twenty-ninth batch.

### `Midi_bufferByte` named — a MIDI status-byte state machine sighted

Moved to `sub_1DD41` (8 callers), a trivial "append byte to buffer"
primitive — writes `AL` to `*_midiBufferPos` (was `word_C8445`) and
advances the pointer. Mechanically nothing more than that, but its
*caller context* is a significant new clue for the still-open `.MUS`/
MIDI unification thread flagged several passes ago.

All of its callers sit inside one un-named data-driven state machine
(visible only as bare `seg024:XXXX` locations — the same "compiled
logic, not a clean function" shape already established elsewhere in
this codebase, not itself renamed). That state machine branches on
incoming byte values that match **Standard MIDI status bytes almost
exactly**: `0xF0` (SysEx start), `0xF2` (Song Position Pointer), `0xF3`
(Song Select), a checked range `0xFA`-`0xFD` (System Realtime
Start/Continue/Stop), and `0xFF` (Meta-event/Reset). This strongly
suggests the state machine *is* the `.MUS` format's MIDI event-stream
parser — buffering bytes via `Midi_bufferByte` (reset to a fixed
offset `0x527` elsewhere in the same code) before eventual dispatch,
presumably down to the already-named `Midi_sendByte` cluster. Not
fully unified yet — the state machine itself is large and un-named —
but a concrete, promising lead for that follow-up pass.

Applied via `apply_renames_gatemain.py`'s thirtieth batch.

### `Midi_readVarLengthValue` named — a MIDI VLQ decoder confirmed

Moved to `sub_1ECDE` (8 callers, called from `sub_1EE70` — a name
already familiar from the `Midi_sendByte` pass). Confirmed as a
textbook **Standard MIDI File variable-length quantity (VLQ)
decoder**: reads a byte, accumulates its low 7 bits into a growing
32-bit value (shifting the accumulator left 7 bits each iteration),
and continues while the byte's high bit (`0x80`) is set — exactly the
well-known MIDI delta-time/VLQ encoding. Named
**`Midi_readVarLengthValue(trackIndex)`**.

This also finally confirms and names its helper, **`sub_1ECB6`** →
**`Midi_peekTrackByte(trackIndex, byteOffset)`** — already
characterized (but left unrenamed) in the `Midi_sendByte` writeup
above as "a generic per-track stream-offset byte reader." Callers pass
`byteOffset=0` to read at the track's current position (advanced
separately by `Midi_readVarLengthValue`'s per-track position counter)
or a small positive offset to peek ahead at a just-identified
fixed-format payload without advancing — e.g. `sub_1EE70`'s 3-byte
tempo meta-event read at offsets 2-4, from the earlier pass. The
per-track base-offset table itself wasn't renamed — it's referenced
via two different segment-relative names in different callers
(`-0x5E5Ch` here vs. `0xA1A4` in `Midi_readVarLengthValue`) that may or
may not be the same physical array; not confirmed either way.

Applied via `apply_renames_gatemain.py`'s thirty-first batch.

### `Surface_getPixelOffset` named

Skipped `sub_2609A` (8 callers) — a graphics-mode-only per-slot
color/position setter (gated on `_videoIndex != 0`, storing a word and
two bytes into a 2-entry table) with no message/string confirmation
available to pin down its exact purpose; the plausible guesses (cursor
color pair, hotspot color/position) weren't confident enough to commit
to. Not renamed.

Moved to `sub_2A933` (8 callers), called from the already-named
`Surface_draw`/`Surface_draw2`. Confirmed directly by its body as the
bounds-checked pixel/byte-address primitive underlying those drawing
routines: returns a small negative error code if the surface is a
special `0xCA00`-sentinel surface, or if the given `x`/`y` are out of
bounds against the surface's width/height; otherwise computes a byte
offset from the surface's bytes-per-line-shaped fields and adds it to
the surface's base far pointer (`_image`). Named
**`Surface_getPixelOffset(surface, x, y)`**.

Applied via `apply_renames_gatemain.py`'s thirty-second batch.

### `Midi_sendCommand` named — the MPU-401 command/acknowledge protocol

Moved to `sub_1D84A` (7 callers, called from `sub_1EE70` — familiar
from the `Midi_sendByte`/`Midi_readVarLengthValue` passes) and its
implicit-register-argument helper `sub_1D808`. Together these resolved
into another textbook MPU-401 confirmation: the standard UART-mode
**command-and-acknowledge protocol**.

**`Midi_sendCommand_raw`** (was `sub_1D808`, an implicit-`AH`-argument
primitive): polls `_midiStatusPort` for "not busy" with a timeout;
once ready, disables interrupts, writes the command byte to
**`_midiCommandPort`** (was `word_C83AE` — confirmed the *same physical
port* as `_midiStatusPort`, set alongside it in `sub_1D966`: MPU-401's
status register doubles as the command register on write, a classic
quirk of the chip), then polls for a response. If the response byte is
`0xFE` — MPU-401's standard command-acknowledge byte — returns success.
If it's anything else (real incoming MIDI data arriving mid-command,
since the UART is bidirectional), it's dispatched through
**`_midiDataCallback`** (was `off_C83BD`, a callback pointer configured
by `sub_1D953`) and the function keeps waiting for the real `0xFE`.
Returns failure if the ACK never arrives before timeout. Interrupts are
restored via `pushf`/`popf` bracketing regardless of outcome.

**`Midi_sendCommand`** (was `sub_1D84A`) is simply the normal-
calling-convention wrapper: loads its `command` argument into `AH` and
calls `Midi_sendCommand_raw`.

Another clean, mechanically-certain piece of the growing MPU-401/MIDI
picture — now covering byte output (`Midi_sendByte`), command
handshaking (`Midi_sendCommand`/`_raw`), VLQ decoding
(`Midi_readVarLengthValue`), track-byte peeking (`Midi_peekTrackByte`),
and output buffering (`Midi_bufferByte`), still short of a single
unifying writeup but steadily converging.

Applied via `apply_renames_gatemain.py`'s thirty-third batch.

### `Logics_describeCorridorOnce` named

Moved to `sub_AB180` (7 callers). Confirmed by decoding its real
`GATESTR.DAT` message (`msgId 0x3C00`): *"This is one of the many
corridors that wind their way through the occupied portions of
Gateway. These man-made hallways are nested inside the original
tunnels that were dug by the Heechee..."* — a generic Gateway-station
corridor description. `Logics_describeCorridorOnce()`: if `byte_CF114`
is set, clears it and prints that message; otherwise does nothing — a
one-time-description gate. Confirmed shared across multiple different
rooms' compiled logic (7 callers, including `sub_ABA21` via the
already-named `j_method233`), consistent with several physically
distinct maze-like corridor rooms in the game reusing identical
flavor text and one shared first-visit flag.

Applied via `apply_renames_gatemain.py`'s thirty-fourth batch.

### `Opl2_writeRhythmRegister` named — the OPL2 subsystem grows

Investigated but skipped `sub_14A5F` (6 callers, called from
`Logics_checkMoveRestriction`) — a generic-looking "print an object's
header (name + location preposition), then invoke its own logic for a
given action" dispatcher, but several pieces stayed unclear
(`j_scene_update?` and `thunk_sub_669E3` weren't traced, and the exact
verb/action semantics behind its `param` argument remained ambiguous).
Not renamed. Also skipped `sub_1A0FC` (6 callers) — another
`sp-analysis failed` mid-function fragment, a separate instance of the
same disassembly-boundary problem seen elsewhere in this binary, not
part of the already-flagged `sub_4A69F` cluster specifically.

Moved to `sub_1D732`, which resolved cleanly into a continuation of
the OPL2/AdLib subsystem sighted a few passes ago. It writes the
OPL2's real hardware register `0xBD` via the already-named
`Opl2_writeRegister`, building the byte from four flag globals:

- bit `0x80` — **`_opl2TremoloDepth`** (was `byte_D1C52`)
- bit `0x40` — **`_opl2VibratoDepth`** (was `byte_D1C58`)
- bit `0x20` — **`_opl2RhythmEnabled`** (was `byte_D1C53`)
- bits `4-0` — **`_opl2RhythmInstruments`** (was `byte_D1C59`)

This is an *exact* match for OPL2's documented register `0xBD`: bit 7
tremolo (AM) depth, bit 6 vibrato depth, bit 5 rhythm-mode enable, and
bits 4-0 individual rhythm-instrument enables (bass drum, snare,
tom-tom, cymbal, hi-hat) — confirming real OPL2 hardware programming
rather than a coincidental register number. Named
**`Opl2_writeRhythmRegister`**.

Applied via `apply_renames_gatemain.py`'s thirty-fifth batch.

### `Vocab_matchesAbbreviation` named — a parser abbreviation matcher

Skipped `sub_1E2E3` (a single `nop` byte — a tail-merge artifact, not
a real function) and `sub_1E315` (another `sp-analysis failed`
chunked fragment in the same `seg098` neighborhood). Moved to
`sub_255A8` (7 callers), which resolved cleanly.

`Vocab_matchesAbbreviation(word, abbrev)`: walks `abbrev` while it's
non-null, comparing each character (case-normalized) against the
corresponding character of `word`, advancing both pointers on a match
and stopping on a mismatch or once `abbrev` is exhausted. Returns 1
only if every character of `abbrev` matched — i.e. `word` starts with
`abbrev` — 0 otherwise. This is the classic parser "does the player's
typed abbreviation match this longer vocabulary word" check (e.g.
`n` matching `north`).

This also confirms its character-normalization helper, **`sub_1AECE`**
→ **`Char_toLower`**: looks up a per-character-code `ctype`-style flag
table checking the "is uppercase" bit, adding `0x20` to fold it to
lowercase (standard ASCII case-fold) if set. A generic utility, also
used directly by `sub_204CE` (not renamed).

Applied via `apply_renames_gatemain.py`'s thirty-sixth batch.

### `Icon_drawButton` named — the mouse-driven icon toolbar

Moved to `sub_27C31` (7 callers, 756 bytes). Confirmed via the
already-named global `button_strings` array: this is the on-screen
**clickable-icon-button drawing function**.

`Icon_drawButton(buttonIndex, stateArray)`: looks up `buttonIndex`'s
current state byte from `stateArray` (a per-icon status/highlight
value), uses it to index a 4-parallel-word rectangle-bounds table for
this button's screen position, loads and draws the button's icon
image (`Image_Init`/`Image_drawAt`), then — in graphics mode — draws a
3D-beveled border around it (light-gray/dark-gray `fillRect` and
`Screen_drawLine` calls, the classic raised-button look). Returns the
button's associated string from `button_strings[stateByte]` — the
vocabulary word or label corresponding to this icon/state, handed back
to the caller (presumably fed into the parser as if the player had
typed it).

Confirmed generic — not owned by one dialog — via its callers:
`Dialog_prompt`, `get_mouse_input`, `prompt_for_filename`,
`Commnet_proc1`. All part of the mouse-driven icon toolbar (most
plausibly a compass rose for movement) that these Legend "Early
engine" games offered alongside their primary text parser.

Applied via `apply_renames_gatemain.py`'s thirty-seventh batch.

### `thunk_sub_5D9F3`/`thunk_sub_5D9F3_2` named — stragglers from the RTLink-thunk batch pass caught

`sub_30D4F` and `sub_3119B` (7 callers each) both turned out to be
genuine RTLink call-site thunks (`call rtlink_thunk; jmp <target>`)
that the earlier batch rename (`apply_rtlink_thunks_gatemain.py`)
missed. Both target the same unnamed function, `sub_5D9F3` (reached
from `get_mouse_input`), from two different overlay segments — the
usual one-thunk-per-caller-segment pattern already documented for this
codebase's RTLink thunks.

They were missed because IDA had merged each thunk's `jmp` target back
in as a "FUNCTION CHUNK" of the thunk itself, which made
`find_rtlink_thunks.py`/`rank_unnamed_functions.py`'s same-function-
start check (`get_func_attr(target, FUNCATTR_START) != ea`) treat them
as split-body false negatives rather than real thunks — a new, subtler
variant of the "real function with a split body" exclusion case those
scripts were already built to handle. Renamed by hand following the
established `thunk_<target>` convention (with a `_2` suffix on the
second, since IDA requires unique names) rather than re-running the
whole batch script for two stragglers.

Applied via `apply_renames_gatemain.py`'s thirty-eighth and
thirty-ninth batches.

### `Surface_advanceSegmentOnCarry` named

Moved to `sub_2A90E` (6 callers). An unusual, low-level function with
no formal stack arguments at all — it takes an implicit `ax:dx` far
pointer and reads the CPU carry flag exactly as the *caller* left it
after some preceding arithmetic on the offset. This is the classic
8086 far-pointer-offset-overflow fixup: if the caller's addition
carried (offset wrapped past `0xFFFF`), add `0x1000` to `ES`;
otherwise add `0x1000` to `DS` — the standard "advance to the next
64KB bank" segment adjustment needed after adding to a raw video-
memory/picture-buffer offset that might cross a 64KB boundary. Its
callers (`sub_17D6A`/`sub_17E31`/`sub_17FB8`/`sub_2AA24`/`sub_2B6D4`/
`sub_2C42A`, none renamed) sit in the same graphics/`Surface`-drawing
neighborhood as the already-named `Surface_getPixelOffset`. Named
**`Surface_advanceSegmentOnCarry`**.

Applied via `apply_renames_gatemain.py`'s fortieth batch.

### `Game_handleWeaponDischarge` named — the consequences of firing a gun

Moved to `sub_A8577` (6 callers, reached via a real thunk). Confirmed
conclusively by decoding all six of its real `GATESTR.DAT` messages: on
Gateway station, weapons are illegal, and this is the **"consequences
of firing a gun"** handler.

`Game_handleWeaponDischarge(outcomeType)` dispatches on `outcomeType`
(0-4), each branch loading a distinct `.RS` sound effect
(`Stream_loadFile`) and printing a distinct message:

- **1** — a fatal hit: *"...you are riddled with energy bolts and
  bullets."* — straight into the already-named `Game_showEndingMessage`.
- **3** / **4** — a miss, the bolt ricocheting dangerously vs. safely.
- **0** / **2** — a miss witnessed by soldiers, leading to arrest. If
  the player can't afford the 1000-credit fine (checked against a
  32-bit player-money field), they're executed by expulsion into
  vacuum (another `Game_showEndingMessage` call) — *"I'm going to have
  to sentence you to expulsion from Gateway - without a vac suit."*
  Otherwise they pay the fine, have their weapon confiscated (a
  handler update on a specific object, `logicNum 0x14C`), and a
  follow-up event is queued via the already-named
  `Queue_exists`/`Queue_find`/`Queue_add`.

This also confirms the 32-bit **player-credits/money field** it reads
and writes: `Persisted_val213`/`word_CF34C` (confirmed adjacent —
`0xCF34A`/`0xCF34C` — and saved together as one 4-byte `SaveField`),
renamed **`_playerCreditsLo`**/**`_playerCreditsHi`**. Seen added to
and subtracted from at several other points in the game (earning and
spending money), and checked here against the 1000-credit weapons
fine.

Applied via `apply_renames_gatemain.py`'s forty-first batch.

### `Queue_processTurn` named — the turn/WAIT event-queue loop sighted

Moved to `sub_12FC3` (5 callers, called directly from `main()`'s main
game loop right before `_turnCount` is incremented). Its body shares
function chunks with `sub_130D6` (itself `sp-analysis failed`), and
together they interleave with a cluster of already-named symbols that
pin down the architecture even though the two functions' exact
individual roles aren't fully separated yet:

- The already-named `_queueCount`/`Queue_add`/`Queue_remove`/
  `Queue_find` — the shared chunk walks the exact same 4-byte-entry
  scheduled-event table those functions operate on.
- The already-named `waitMsg` (`"Do you want to continue waiting?"`)
  and `j_continue_waiting` — confirming this cluster is reused as the
  **WAIT command's inner loop**, not just a once-per-turn callback.

`Queue_processTurn(prevValue, currentValue)`: gates on a one-shot skip
flag (`byte_CB7F2`, not renamed), then either fires action `0x1B` on
the current room or falls through into the shared queue-walking chunk.
Named for its confirmed role — the per-turn (and per-WAIT-iteration)
scheduled-event-queue processing entry point.

Not renamed this pass: `sub_130D6` itself, and `word_CB7F6`/
`word_CB808` (which look like a queue-walk index and a countdown value
respectively — the latter plausibly related to the weapon-confiscation
countdown from the `Game_handleWeaponDischarge` pass, but that
relationship wasn't confirmed). Left for a future pass rather than
guessed under this much interlocking complexity.

Applied via `apply_renames_gatemain.py`'s forty-second batch.

### `Sb_detectDsp` named — a fourth sound backend: Sound Blaster

Moved to `sub_186F0` (5 callers), which — together with three closely
related helpers it's called alongside — resolved into a complete,
textbook-exact **Sound Blaster DSP detection sequence**. A fourth
sound-hardware backend, alongside the already-documented PC-speaker
tone/sample engine, MPU-401/MIDI cluster, and OPL2/AdLib cluster, all
reached from the already-named `Stream_selectHandler`.

- **`Sb_writeByte(AL=byte)`** (was `sub_186F0`): polls the DSP
  write-status port (`base+0xC`) for bit 7 clear (not busy), does the
  standard short I/O delay (four `jmp $+2`), then writes the byte to
  that same port — the write-status and write-command registers share
  one address, exactly as on real SB hardware.
- **`Sb_readByte`** (was `sub_186D4`): polls the DSP read-buffer-status
  port (`base+0xE`) for bit 7 set (data available), then reads the
  byte from the read-data port (`base+0xA`).
- **`Sb_resetDsp`** (was `sub_186B2`): the exact standard SB reset
  sequence — write `1` to the reset port (`base+6`), busy-wait ~256
  iterations, write `0` to release reset, then poll (via `Sb_readByte`,
  up to 32 tries) for the DSP to respond with the magic byte `0xAA`
  confirming it's alive.
- **`Sb_detectDsp`** (was `sub_18682`): calls `Sb_resetDsp`, then runs
  the standard SB "DSP Identification" compatibility test — writes
  command `0xE0` then test byte `0xC6`, reads back, and checks the
  result equals `0x39`, the exact bitwise complement of `0xC6`. On
  success calls `sub_18963(1)` (not renamed, plausibly "mark Sound
  Blaster detected").

The base port, **`_sbBasePort`** (was `word_C84F3`, typically `0x220`
on real hardware), anchors all of these fixed offsets. Every piece of
this matches the well-documented Sound Blaster DSP protocol exactly —
conclusive proof this is real SB (or 100%-compatible) hardware
detection, not a coincidental register shape.

Applied via `apply_renames_gatemain.py`'s forty-third batch.

### `Sound_stopTrack` named — the `startGame?` mislabeling finally corrected

Moved to `sub_1D8CB` (5 callers, called from `sub_1D966` and from
`startGame?`). It resolved into the **MPU-401 shutdown/uninstall
function** — the exact reverse of `sub_1D966`'s IRQ-driven install:
restores the original 8259 IRQ mask, sends MPU-401's standard `0xFF`
reset command via the already-named `Midi_sendCommand_raw`, and
restores the original DOS interrupt vector `sub_1D966` had saved
before installing its own handler. Named **`Midi_shutdown`**. This
also finally justifies naming **`sub_1D966`** itself —
**`Midi_initDevice`** — already characterized (but left unrenamed) in
the `Midi_sendByte` pass; its shutdown counterpart makes the whole
picture unambiguous.

Tracing `Midi_shutdown`'s caller `startGame?` all the way through
finally let the long-standing flag from several passes ago (see "a
mislabeled tentative name" in the sound-track-selection-subsystem
section above) be corrected for real, instead of just flagged. Its
full body confirms: it only proceeds if `word_C8582`'s streaming-
active bits are set **and** the given `trackId` matches the currently-
playing track or is `0xFFFF` ("stop whatever's playing"). It waits for
the active buffer to drain, dispatches a backend-specific stop based on
`word_C8582`'s mode bits (MIDI via the new `Midi_shutdown`, or one of
two other not-yet-renamed backend stop routines), clears the backend-
selector bits, resets the current-track globals, then falls into an
**unconditional tail** (reached even when the initial guards fail)
that frees buffer handles and resets every `Stream`-buffer bookkeeping
global to its idle state — the *exact same* reset sequence already
observed at the tail of `sub_15DB2`. Renamed **`Sound_stopTrack`**:
this is "stop the currently playing sound/music track," nothing to do
with starting a new game.

That exact-match tail also finally justifies renaming `sub_15DB2` →
**`Sound_selectTrack`** (the name floated but not applied several
passes ago): it calls `Sound_stopTrack(0xFFFF)` — an unconditional stop
— before loading and starting a new track. `sub_15F35` (the
two-resource-variant lookup helper) remains unrenamed.

Applied via `apply_renames_gatemain.py`'s forty-fourth batch.

### `Sound_selectDevice` named — the sound-mode dispatcher confirmed

Moved to `sub_1FDB8`, called directly from `gatemain_start` — matching
the `soundMode` command-line argument identified in the earlier
cross-IDB argv-mapping finding. Confirmed via its two callees:

- **`Opl2_detectAndInit`** (was `sub_1CD54`): sets `_opl2BasePort` to
  the real `0x388` before probing for the chip's presence.
- **`Midi_detectDevice`** (was `sub_1FA5E`): calls the already-named
  `Midi_initDevice(basePort, irqLine)` then immediately
  `Midi_shutdown()` again — a detect-only probe: initialize just long
  enough to see if the hardware responds, capture the result, then
  tear it back down rather than leaving it running.

**`Sound_selectDevice(mode, midiBasePort, midiIrq)`**: stores
`midiBasePort`/`midiIrq` into **`_midiBasePortConfig`**/
**`_midiIrqConfig`** (distinct from the already-named `_midiDataPort`,
which is the port actually in use once initialized), then dispatches
on `mode`: `1`/`2` probes for OPL2/AdLib via `Opl2_detectAndInit`;
`4` probes for MPU-401/MIDI via `Midi_detectDevice`, and on success
also prints the game's title (`"       Gateway      "`) and runs two
more setup routines. Sets `word_C8582` flag bits recording which
backend probe succeeded — value `4` for MIDI (matching
`Sound_stopTrack`'s own test of that same bit) and value `2` for
OPL2/AdLib — tying this cleanly into the already-confirmed
`Sound_stopTrack`/`Sound_selectTrack` architecture.

Applied via `apply_renames_gatemain.py`'s forty-fifth batch.

### `InputWindow_redrawPromptLine` named

Moved to `sub_5C91C` (5 callers). Confirmed via the already-named
`aaInputPrompt`/`get_input_line_ptr`/`Commset_winContent`: writes the
prompt string followed by the current input buffer's text into the
content window — redrawing the "prompt-plus-typed-so-far" line — then
hides the mouse cursor if mouse input mode is off, or shows it
(waiting for button release first if needed) otherwise.

Applied via `apply_renames_gatemain.py`'s forty-sixth batch.

### `String_matchesPrefixCI` named

Moved to `sub_204CE` (5 callers). Byte-for-byte the same algorithm as
the already-named `Vocab_matchesAbbreviation` (same case-insensitive
prefix check via `Char_toLower`, same `0`/`1` sentinel-byte return
convention), but a separate compiled copy used in the startup/sound-
config-parsing area — right next to `Sound_selectDevice`/
`Opl2_detectAndInit`/`Midi_detectDevice`, and immediately preceded by
`sub_204CE`'s neighbor `sub_20448` (a hex-digit-string parser, not
renamed) — consistent with parsing a `BLASTER`-style environment/
command-line config string (sound card base address/IRQ/DMA settings)
rather than in-game parser vocabulary. Named generically
(`String_matchesPrefixCI`) rather than reusing the game-vocabulary-
specific `Vocab_` prefix.

Applied via `apply_renames_gatemain.py`'s forty-seventh batch.

### `Mouse_shutdown` named

Moved to `sub_249FF` (5 callers). Confirmed directly by its body,
including IDA's own inline comment ("Reset mouse driver") on the
`INT 33h AX=0` call: if `mouseState` bit `0x38` is set, hides the mouse
(`Mouse_Hide`), resets the mouse driver, and restores the cursor range
(`set_mouse_range`); if bit `0xC` is set, frees mouse resources
(`Mouse_free`) and calls `sub_24A42` (not renamed — also called from
the already-named `Mouse_init`, a shared init/shutdown helper).

Applied via `apply_renames_gatemain.py`'s forty-eighth batch.

### `load_and_draw_pic` named

Moved to `sub_2661C` (5 callers). A one-shot "load a picture frame,
draw it, then free it" utility, confirmed directly by its body:
`Image_load`s the given picture/frame into a temporary local `Image`
(returning `0` immediately on failure), calls `sub_2666E(x, y, img)`
(not renamed — its exact coordinate/offset handling wasn't fully
unpicked, but it eventually calls a drawing/blit routine,
`sub_2B6D4`) to draw it at the given position, frees the temporary
`Image`, and returns `1`. Named **`load_and_draw_pic`** to match the
established `load_and_scale_pic` convention for this small cluster of
one-shot picture-loading entry points.

Applied via `apply_renames_gatemain.py`'s forty-ninth batch.

### `Logics_describePondView` named — a single room's environmental description generator

Moved to `sub_791E2` (5 callers). Confirmed via its real decoded
`GATESTR.DAT` message and the already-recognized lowercase direction-
name string constants (`aNorth`/`aSouth`/`aEast`/`aWest`/
`aNortheast`/etc., in the same `seg086`) as a single room's — the
pond's — detailed environmental description generator, not a generic
utility.

`Logics_describePondView(directionIndex)`: looks up `directionIndex`
(0-4) in a 5-entry table to find matching shore/direction data, then
prints message `0x4824`: *"You're standing on the %sern shore of the
pond. %slight gently reflects off the calm %s surface of the pond. A
leaf occasionally falls into the pond to the %s and causes a small
ripple, distorting the %s reflection."* — filling its five `%s`
placeholders from a direction name (confirming the table holds
direction-string references) and, gated on `Persisted_val183` (a
day/night flag), either sun- or moon-flavored text (including the
literal string `aSunS` = `"sun's"` for the final possessive reflection
phrase).

Applied via `apply_renames_gatemain.py`'s fiftieth batch.

### `Logics_describeBeastApproach` named — a crystal-shard beast-deterrence puzzle

Moved to `sub_80894` (5 callers). Confirmed via **five** real decoded
`GATESTR.DAT` messages as the turn-by-turn handler for a hostile-
creature encounter shared across a cluster of 4 related rooms
(`_roomLogicNum` `0x98`-`0x9B`) — a full crystal-shard deterrence
puzzle.

Every call prints a room-specific "beast notices you" message. Then,
gated on `Persisted_val177`:

- **Set** — *"He immediately becomes transfixed by the light
  reflecting off the...crystal shard and stands motionless."* Sets
  `Persisted_val178 = 1`, clears a bit on object `0x9D`, and removes
  queue item 3 — the shard (clean and displayed) is holding the beast
  at bay.
- **Clear** — *"You freeze..."* and (except in room `0x98`) *"a glint
  from the...shard...crosses his face. He stops dead...then resumes
  walking toward you,"* followed by *"He grabs you...and slams you
  head first into the %s wall"* — the beast attacks, and a queue item
  is scheduled (plausibly a damage/consequence follow-up). **Or**,
  specifically in room `0x98`, the successful-escape resolution
  instead fires: *"...he becomes momentarily motionless...lets out a
  deafening shriek...quickly exits the clearing"* — the shard (this
  time apparently displayed at just the right moment, or with a clean
  spot catching the light) fully dazzles and routs the beast.

Confirms a genuine multi-outcome puzzle: held/clean-shard vs.
mud-covered-shard vs. no-shard-shown, each producing a distinct
narrative resolution.

Applied via `apply_renames_gatemain.py`'s fifty-first batch.

### `Queue_tickCountdowns` named — closing the loop on `Queue_processTurn`

Moved to `sub_130D6` — the companion function explicitly flagged for a
future pass when `Queue_processTurn` was named. Multi-chunk and
`sp-analysis failed`, but its mechanism is now clear from reading both
of its chunks: it's the actual countdown-queue tick that
`Queue_processTurn` falls into.

`Queue_tickCountdowns()`: walks the same `_queueCount`-bounded,
4-byte-entry scheduled-event table `Queue_add`/`Queue_remove`/
`Queue_find`/`Queue_processTurn` operate on, decrementing each entry's
countdown value; when an entry's countdown reaches zero, it
memmove-compacts the entry out of the table (the same removal shape as
`Queue_remove`, just triggered by expiry rather than an explicit call)
and applies its associated handler update. After a full pass, if a
caller-supplied flag indicates this is running as part of the WAIT
command, it checks `byte_CC530` and — if set — calls the already-named
`j_scene_update?` then `j_continue_waiting(waitMsg)` to ask *"Do you
want to continue waiting?"*.

This confirms and closes the loop on the mechanism flagged (but not
fully named) back when `Queue_processTurn` was named: the turn-advance
and WAIT-command loops share this exact countdown-tick code.

Applied via `apply_renames_gatemain.py`'s fifty-second batch.

### `Logics_lookAtCurrentRoom` named

Moved to `sub_15470` (4 callers). A thin wrapper: calls
`sub_14A5F(_roomLogicNum, action=8)` (`sub_14A5F` itself not renamed —
a generic "print object header, invoke its logic for an action"
dispatcher whose exact verb semantics vary per call site) and always
returns 1. Called directly from `main()` and the already-named
`show_startup` — consistent with action `8` being "describe/look at
this room," invoked once at game startup and once per the main loop's
LOOK-equivalent point. Named **`Logics_lookAtCurrentRoom`**.

Applied via `apply_renames_gatemain.py`'s fifty-third batch.

### `Logics_saveOrRestoreHandler` named

Moved to `sub_15AD8` (4 callers, all from a single caller `sub_9478A`
via `j_method074`, not itself investigated). Confirmed mechanically by
direct read as a small push/pop-style save-and-restore mechanism for
one specific object's handler state, keyed by a caller-supplied
context value against a 4-slot table: with `mode==0` it reads object
`0x71`'s handler-index-1 value and stores it into the matching slot;
with `mode!=0` it reads the previously-stored value back out and
writes it back. The specific significance of `logicNum 0x71`/handler
index `1` wasn't determined — no message-string anchor was available
for this one. Named **`Logics_saveOrRestoreHandler`** for the
confirmed mechanism.

Applied via `apply_renames_gatemain.py`'s fifty-fourth batch.

### `Sound_selectTrackForRoom` named

Moved to `sub_15BDA` (4 callers, called directly from `main()` and the
already-named `show_startup`). Confirmed as the **room-to-background-
music mapping entry point** via a real call to the already-named
`Sound_stopTrack` and a body that otherwise duplicates
`Sound_selectTrack`'s own tail logic (the same `word_C856E`/
`word_C8580`/`word_C857A`/`word_C857E` globals, the same `+0x3D8E`
table, the same `sub_15F35` resource-variant-lookup calls).

`Sound_selectTrackForRoom(roomNum)`: looks up `roomNum` in a 106-entry
per-room table to find its music config (section id, track id, a
duration/volume-ish byte, and a flag byte); if the found section/track
already match what's currently playing, does nothing. Otherwise stops
the current track and runs the same track-loading sequence
`Sound_selectTrack` uses to start the new room's music.

Applied via `apply_renames_gatemain.py`'s fifty-fifth batch.

### `Windows_setCurrentWindow` named

Moved to `sub_16978` (4 callers). Confirmed directly by its body,
using the already-named `Windows_currentWindow`/`Listbox_draw`: saves
the current window to return later; if the requested window is valid
and different, redraws the current listbox as deselected and switches
to it; always returns the *previous* current window, letting callers
temporarily switch windows and restore the old one afterward.

Applied via `apply_renames_gatemain.py`'s fifty-sixth batch.

### `Listbox_resetStateStack`/`Listbox_pushState` named — a nested-listbox stack

Moved to `sub_17A12` (4 callers), a one-line "reset a counter to 0"
function. Confirmed via the immediately-following function's body,
which pushes the current listbox's items/divider-index onto a 20-deep
stack (indexed by that same counter, `word_D0766`) before calling the
already-named `Listbox_reset` with new items — a **nested-listbox
state stack**, used to open a listbox/menu on top of the current one
and later restore it.

- **`Listbox_resetStateStack`** (was `sub_17A12`): sets the stack's
  depth counter to 0. Called from several already-named entry points
  (`Events_waitForPress`, `InputWindow_getLine`, `Scene_draw`) —
  consistent with clearing any nested-dialog stack state at the start
  of a fresh top-level input/display session.
- **`Listbox_pushState`** (was `sub_17A19`): saves a window's current
  items, divider index, and a third value onto the stack, increments
  the depth, then calls `Listbox_reset` with the new items — pushing
  the current listbox state before replacing it. No-ops if the stack
  is already full (20 deep).

Applied via `apply_renames_gatemain.py`'s fifty-seventh batch.

### `Speaker_sampleIsr` named — the digitized-sample ISR body traced

Moved to `sub_18842` (4 callers). Confirmed directly by its body — a
real hardware interrupt service routine (PIC end-of-interrupt `out
20h,20h`, full register save/restore, `iret`) — continuing the
"digitized PC-speaker sound-effect engine" thread from several passes
ago, whose self-modifying ISR body was explicitly flagged then as "not
traced further."

`Speaker_sampleIsr()`: initializes several playback-state globals
(buffer length/position/end-marker-shaped values, not renamed), then
dispatches to one of two continuation routines (`sub_18905` or
`sub_18883`, not renamed — plausibly double-buffered "next sample
byte" handlers) before signaling end-of-interrupt and returning. Not
fully unpicked — the two continuation routines and the exact 1-bit
sample-decoding scheme remain open — but this confirms the ISR is a
real, complete interrupt handler, closing a small piece of that
earlier "not traced further" gap.

Applied via `apply_renames_gatemain.py`'s fifty-eighth batch.

### `Opl2_noteOn`/`Opl2_noteOff` named

Moved to `sub_1D05A`/`sub_1D1A4` (4 callers each — `sub_1E21B`/
`sub_1E45C`/`sub_1E4C4`, not renamed, plausibly the MIDI-to-OPL2
translation layer that converts MIDI note events into FM-synth
register writes). Confirmed via the already-named
`_opl2RhythmEnabled`/`Opl2_writeRegister`/`Opl2_writeRhythmRegister` as
the OPL2 FM-synth's fundamental note-on/note-off primitives.

**`Opl2_noteOn(channel, velocity)`**: clamps `velocity` to `0x7F` (the
MIDI velocity range) and stores it per-channel; looks up that
channel's operator-register offsets from one of two tables depending
on `_opl2RhythmEnabled` (melodic vs. rhythm/percussion channel-to-
operator mapping, since OPL2's 2-operator FM voices are wired
differently for the 5 fixed rhythm instruments), then applies the
volume to each of up to 2 operators (carrier + modulator).

**`Opl2_noteOff(channel)`**: for melodic channels, clears the key-on
bit in OPL2 register `0xB0+channel` — the standard OPL2 note-off. For
rhythm-mode channels, instead clears the corresponding bit in
`_opl2RhythmInstruments` via `Opl2_writeRhythmRegister`.

Applied via `apply_renames_gatemain.py`'s fifty-ninth batch.

### `Midi_resetDevice` named

Moved to `sub_1D85B` (4 callers). Confirmed via the already-named
`Midi_sendCommand_raw`/`_midiDataPort`: sends MPU-401's standard reset
command (`0xFF`), then reads and discards one byte directly from the
data port — flushing any stray leftover byte right after a reset.
Called from the already-named `Midi_initDevice` during setup, plus
three other MIDI-adjacent call sites not renamed.

Applied via `apply_renames_gatemain.py`'s sixtieth batch.

### `Midi_peekByte`/`Midi_readVarLengthValue2` named

Moved to `sub_1E168` (4 callers) and its helper `sub_1E148`. Both turn
out to be a **second compiled copy** of the already-named
`Midi_peekTrackByte`/`Midi_readVarLengthValue` pair — the same
duplicate-copy pattern already seen with `Vocab_matchesAbbreviation`/
`String_matchesPrefixCI` — operating on a different base pointer
(`_tmpSub._sub._set1`) and position counter (`word_D20A0`) for what's
presumably a single implicit stream rather than an array of tracks
indexed by `trackIndex`.

**`Midi_peekByte(byteOffset)`** (was `sub_1E148`): reads a byte at
`*(_tmpSub._sub._set1) + byteOffset + word_D20A0`.

**`Midi_readVarLengthValue2`** (was `sub_1E168`): byte-for-byte the
same Standard MIDI File VLQ decode loop as `Midi_readVarLengthValue`,
just using `Midi_peekByte` and incrementing `word_D20A0` instead.

Applied via `apply_renames_gatemain.py`'s sixty-first batch.

### `Sound_shutdown` named

Moved to `sub_1FE30` (4 callers, called from the already-named
`Sound_selectDevice`, `finish`, and `shutdown`). Confirmed as the
sound subsystem's own full teardown function — the counterpart to
`Sound_selectDevice`'s device-selection init: unconditionally stops
the current track (`Sound_stopTrack(0xFFFF)`); if `word_C8582`'s
MIDI-active bit is set (the same bit `Sound_selectDevice` sets on a
successful MPU-401 probe), calls `sub_1FCAA` (still unnamed) and what
was guessed here as clearing "an on-screen device indicator"; then
masks `word_C8582` down to just bit `8`, clearing all backend-selection
state. Called from the game's top-level exit routines.

**Correction (see the `Midi_sendDisplayText` entry below):** that
guess about `sub_1FB56` was wrong — it doesn't touch the screen at all.
It sends the passed string as a Roland-style MIDI SysEx "Display Data"
message, so `Sound_shutdown` calling it here is actually clearing the
*device's own LCD text display* (on an MT-32/Sound Canvas-compatible
module), not anything on the game's screen.

Applied via `apply_renames_gatemain.py`'s sixty-second batch.

### `Screen_waitForVerticalRetrace` named

Moved to `sub_22954` (4 callers). Confirmed via the already-named
`video_status_reg` as the classic "wait for vertical retrace"
synchronization primitive: busy-waits for the EGA/VGA vertical-retrace
status bit to clear, then busy-waits again for it to become set — the
standard technique to synchronize to the very start of a new retrace
period. Called from `Screen_setEGAPalette` (avoiding palette-change
tearing/snow) and `Mouse_Hide`/`Mouse_show` (avoiding cursor-draw
flicker).

Applied via `apply_renames_gatemain.py`'s sixty-third batch.

### `getUppercaseKeypress` named

Moved to `sub_2384F` (4 callers). Confirmed directly by its body: calls
the already-named `get_keypress()`, and if the result is a lowercase
letter, converts it to uppercase. A single-key menu-choice reader,
named to match the existing lowercase-underscore
`get_keypress`/`get_input_line_ptr` convention.

Applied via `apply_renames_gatemain.py`'s sixty-fourth batch.

### `Screen_fadeIn` named

Moved to `sub_26228` (4 callers). Confirmed directly by its body as
the fade-IN counterpart to the already-named `Screen_fadeOut`: in text
mode it does nothing; in basic EGA mode it just sets a fixed reference
palette directly; otherwise (VGA/other) it builds a scaled-down copy
of the reference palette in a local buffer — each color component
multiplied by a growing fraction stepping from 0 up to full — calling
`Screen_setEGAPalette` after each step, ramping the screen up from
black to full brightness.

Applied via `apply_renames_gatemain.py`'s sixty-fifth batch.

### `AnimPics_tick` named — the AnimPics cluster's last piece

Moved to `sub_26F74` (4 callers) — already fully characterized (but
left unrenamed) back when `AnimPics_freeAll` was named, several passes
ago. Finalizing the name now closes out the `AnimPics_*` cluster
entirely.

`AnimPics_tick(mode)`: if `_animPicsSlotCount` is 0, returns
immediately. Otherwise walks every active animated-picture slot,
comparing the current `_clock()` against a per-slot randomized
deadline (computed from `_rand()` scaled by the slot's registered
duration parameters); when a slot's deadline passes, advances its
frame index (forward or backward per the slot's loop-direction byte)
and draws the new frame via the already-named `Image_draw`, then
computes the next randomized deadline. Called from input-polling loops
(`Events_waitForPress`, `get_input_character`, `get_mouse_input`) so
animations keep advancing while the game waits for the player.

With this, `AnimPics_*` — register, free-all, reset-for-room,
resync-slots, clear-frames, and now tick — is completely named.

Applied via `apply_renames_gatemain.py`'s sixty-sixth batch.

### `Windows_setContentWindow` named

Moved to `sub_28231` (4 callers). A trivial two-global setter,
confirmed mechanically by direct read: sets the global `winNumber`
(already named, but otherwise only ever written here) and
`word_D2A96` (not renamed, but already known from the
`Icon_drawButton`-adjacent code to be read as a `TextWindow_addDirect`
target window number) from its two arguments. Called from the
already-named `room_load` plus three other room/UI-adjacent functions
— consistent with selecting which window receives a room's text
output, but not confirmed beyond the mechanical read/write shape.

Applied via `apply_renames_gatemain.py`'s sixty-seventh batch.

### `LogFile_close` named

Moved to `sub_2881D` (4 callers). Confirmed directly by its body,
using the already-named `LogFile_windowNum`/`LogFile_handle`/
`LogFile_disabled` globals: if a transcript log file is currently
open, closes it, resets the window-number tracking to "none", and
clears the disabled flag. Called from the already-named `finish`/
`shutdown` exit routines plus two other not-renamed functions — the
transcript/log-file close counterpart to whatever opens it.

Applied via `apply_renames_gatemain.py`'s sixty-eighth batch.

### `Video_getValidIndex` named

Moved to `sub_2A597` (4 callers). Confirmed via the already-named
`videoIndex`/`video_set_videoIndex` (distinct from the similarly-named
but separate `_videoIndex` global used elsewhere) as a validated
video-mode-index getter: returns `videoIndex` if in range `0`-`7`, or
the sentinel `-6` otherwise. Called from `Surface_draw`/
`Surface_draw2`/`Video_ClearScreen` at the start of drawing operations.

Applied via `apply_renames_gatemain.py`'s sixty-ninth batch.

### `format_long_decimal` named

Moved to `sub_1063F` (3 callers, called from the already-named
`Commset_show`). Confirmed via repeated calls to the MS Quick C/MSC
runtime's 32-bit long-division helper (`unknown_libname_5`, dividing
by 1000) as a signed 32-bit integer-to-decimal-string formatter:
special-cases exactly zero, negates and prefixes `-` for negative
values, then repeatedly divides by progressively smaller powers of ten
to extract each decimal digit into a local buffer. A generic number-
formatting utility, plausibly used for score/turn-count/credits
display. Named to match the project's lowercase-underscore convention
for custom C-runtime-adjacent utilities.

Applied via `apply_renames_gatemain.py`'s seventieth batch.

### `Dialog_showFormattedPrompt` named

Moved to `sub_2A163` (4 callers, called directly from the already-named
`Dialog_prompt`). Confirmed via a real call to the C runtime's
`_vsprintf` as the core "format a message and show it in an auto-sized
dialog box" implementation `Dialog_prompt` wraps: formats its
printf-style arguments into a local buffer, then walks the resulting
text measuring line count (capped at 24) and max line width (capped at
79, i.e. an 80x25 text screen) to auto-size the dialog. The remainder
of the function (actually creating/positioning the window and
displaying the text) wasn't traced in full this pass.

Applied via `apply_renames_gatemain.py`'s seventy-first batch.

### `Dialog_restorePrevious` named

Moved to `sub_2A41D` (4 callers, including the just-named
`Dialog_showFormattedPrompt`, which calls it conditionally right at
the top). Confirmed via direct read as the **pop/restore half of a
nested-dialog stack**: no-ops if a nesting-depth counter is 0;
otherwise decrements it, destroys the just-closed dialog's window,
restores the saved image offset, redraws the previous dialog's
background image and saved text, optionally shows the text cursor,
and shows the mouse again. Mirrors the same nested-state-stack pattern
already seen in `Listbox_pushState` — the corresponding "push" half
for dialogs isn't identified yet.

Applied via `apply_renames_gatemain.py`'s seventy-second batch.

### `Parser_askForClarification` named

Moved to `sub_13CC7` (3 callers, including the already-named
`GatewayParser_speakHandler`/`Parser_proc6`). Confirmed via its
literal strings, already recognized by IDA: `"[Please be more
specific."`, `" I'm not sure wh[at]"`, `" you mean by "`. The parser's
**ambiguous-preposition clarification-request handler**: if a flag in
the parse-result struct is set, does nothing; otherwise, if the
struct's vocab-ID field matches one of 8 specific values (plausibly a
set of ambiguous prepositions), builds and prints a "please be more
specific, I'm not sure what you mean by..." prompt naming the
referenced word.

Applied via `apply_renames_gatemain.py`'s seventy-third batch.

### `Logics_checkIsHolding` named

Moved to `sub_1452B` (3 callers, called directly from `main()`).
Confirmed via a real decoded `GATESTR.DAT` message (`msgId 0xC407`:
`"[%sn't holding%s.]"`) as an implicit **"is `<subject>` holding
`<object>`?"** precondition check: if `logicNum` is one of two
special-cased values (`0xE3`/`0xE4`), the check passes unconditionally
(returns 0); otherwise it prints the subject's name, the failure
message, and the referenced object's name (`Logics_logicNum211`), and
returns 1 (check failed — not holding). The two special-cased values
presumably bypass the check for specific NPCs/contexts where it
doesn't apply.

Applied via `apply_renames_gatemain.py`'s seventy-fourth batch.

### `Game_showIllustration` named — the cutscene/illustration display sequence

Moved to `sub_15674` (3 direct callers, but referenced *in passing* by
name throughout many earlier passes as "a major hub function" —
`AnimPics_freeAll`, `Sound_stopTrack`, `Screen_fadeIn`, and
`Game_restartAfterDeath` all mentioned it without tracing it). Finally
traced and named directly.

`Game_showIllustration(picNumber, arg2, arg4, arg6, arg8)`: if a
"picture currently shown" flag is set, tears down any active animated-
picture overlays (`AnimPics_freeAll`). Checks whether graphics display
is available; if not, falls back to a text-only path (`sub_158C3`, not
renamed). Otherwise, if `picNumber != 0`, loads and draws the picture
(stopping the current sound track first if a specific mode bit is set,
to avoid audio glitching during picture display), or fills the screen
black if the picture fails to load; either way calls the already-named
`Screen_fadeIn`, delays 3 seconds, then calls `sub_157A9` (not renamed
— plausibly the caption-text display) with its message arguments, and
frees the loaded image.

This is the game's full-screen illustration/cutscene display sequence
— confirmed as the hub `Game_restartAfterDeath` and others route
through for "show a picture with fade-in, delay, and caption text,"
e.g. death/ending scenes and other major story beats.

Applied via `apply_renames_gatemain.py`'s seventy-fifth batch.

### `TextWindow_addMessageList` named

Moved to `sub_158C3` (3 callers) — `Game_showIllustration`'s own
text-only fallback path, traced directly. Walks a far-pointer array of
dword message-string pointers, grouped by null separator entries: for
each group, prints a tab character, then each non-null message in the
group back-to-back, then a newline; stops entirely at the first group
that starts with a null entry. `Game_showIllustration` calls this when
graphics display isn't available, to print the same caption content a
picture-mode call would otherwise show alongside the image.

Applied via `apply_renames_gatemain.py`'s seventy-sixth batch.

### `AnimPics_finishPlayback` named

Moved to `sub_26F2A` (19 callers) — the busiest still-unnamed function
at the time, sitting physically right between the named
`AnimPics_resyncSlots` and `AnimPics_tick` inside the `sg1692` overlay
segment. If any AnimPics slots are currently registered, it clears a
private scratch buffer (`unk_D2302`) and latches each per-slot "shown"
byte in a private 6-byte array (`byte_D22EE`) to 1 — these arrays
belong only to this function, distinct from the slot handle/frame
tables `AnimPics_registerSlot` itself writes. It always finishes by
tail-calling `Events_checkKeypress`, which consumes a pending Space or
Enter keypress (returning it to the caller) while requeuing any other
pending key into `injectCharacter` for later.

At 6+ call sites (e.g. inside `sub_9B5F9`, `sub_B1730`, `sub_BA9A5`,
and the death-restart sequence) it is called immediately before
`AnimPics_freeAll`. An anonymous inline loop physically inside `sg1692`
(sitting between the `AnimPics_resyncSlots` and `AnimPics_tick`
definitions, entered via a computed/indirect call IDA never attributed
a function boundary to) also calls it exactly once, right after its own
`AnimPics_tick` / wait-for-keypress loop exits. Both patterns match the
same role: settle whatever frames are currently displayed and swallow
any pending skip keypress, as the standard finishing step immediately
before an anim-pics playback sequence tears its slots back down.

Applied via `apply_renames_gatemain.py`'s seventy-seventh batch.

### `GameDate_format` named

Moved to `sub_13629` (4 callers) — a clean, self-contained date
formatter, traced directly from its data tables rather than its
callers. Takes a single `dayCount` argument, adds 16 to it, then walks
a 12-entry table (at `seg067+0x1FE`) holding the standard non-leap
Gregorian month lengths (31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30,
31), starting from table index 4 (May, 0-based) and subtracting whole
months off `dayCount` while cycling the index mod 12, until what's left
fits within the current month. If the table index wrapped back below 4
(past December into Jan-Apr), the year-suffix digit is 3, otherwise 2;
it then calls the real C runtime's `_sprintf` with the format string at
`seg067+0x216` (`"%02d-%02d-21%02d"`) to render
`"<month>-<day>-21<yearSuffix>"` into a static buffer (`unk_D2E3A`),
returning a far pointer to it.

Working the math backward confirms the game's internal day-counter
epoch — `dayCount == 0` — formats to **May 17, 2102**, an in-universe
date. Called from four room/logic-adjacent functions (`sub_72260`,
`sub_A60CE`, `sub_AA9E3`, `sub_B8235`, none renamed yet), consistent
with a journal/diary-entry or event-log timestamp formatter reading an
elapsed-day counter somewhere in game state.

Applied via `apply_renames_gatemain.py`'s seventy-eighth batch.

### `RawFile_write` named

Moved to `sub_21B15` (4 callers) — a thin wrapper directly around DOS
`INT 21h`/`AH=40h` ("write to file with handle"): `bx`=handle,
`cx`=count, `ds:dx`=buffer, returning 0 on carry-set (error) or DOS's
own returned byte count otherwise. Initially mistaken for the C
runtime's own `_write` (already named at a different address, a much
larger function with real FILE-handle-table bookkeeping and
append-mode handling) — the rename script's real-apply run caught this
itself (`ok=False`, since `_write` was already taken), which is what
surfaced the distinction.

This is a separate, much more primitive raw-handle writer, sitting in
the small `sg12EE` segment directly alongside the already-named
`fseek`, `fsetpos`, and `set_filename_prefix` — a distinct, lightweight
custom file-I/O layer the game uses directly, bypassing the C
runtime's buffered stdio entirely. `fsetpos`'s presence in the same
small group makes the save-game system the most plausible consumer.

Applied via `apply_renames_gatemain.py`'s seventy-ninth batch.

### `_tzset` and the timezone globals named

A full, confidently-confirmed cluster, traced directly from its data
rather than its callers. `sub_1B81C` is the standard MSC runtime
`_tzset()`: it calls `_getenv("TZ")` (the literal string `"TZ"`
verified directly at its argument address) and, if the environment
variable is set and non-empty, parses the classic POSIX `TZ` format
(e.g. `"PST8PDT"`) — `_strncpy`s the leading 3-letter standard-timezone
abbreviation into `_tzname`, `atoi`s the numeric UTC-offset digits and
multiplies by 3600 (`__aFlmul`) into a 32-bit `_timezoneLo`/
`_timezoneHi` pair (seconds west of UTC), then `_strncpy`s any trailing
DST abbreviation into `_tznameDst`, setting `_daylight` to 1 if that
name is non-empty or 0 otherwise. The default data — before any `TZ`
value is parsed — is `"PST"`/`"PDT"`/28800 seconds/`_daylight=1`, the
standard MSC runtime default.

`sub_1B80C` is a one-time-init guard around `_tzset`, checking/
incrementing `word_D2E00` so the environment variable only gets parsed
once; it's called from the already-named `_ftime` and `__dtoxtime`
before they read the timezone globals. `_ftime` in particular divides
`_timezoneLo`/`_timezoneHi` by 60 to fill in a `struct timeb`'s
timezone-in-minutes field, which is what confirmed the pair as a
32-bit seconds value rather than something else. The 32-bit global was
split Lo/Hi to match this project's existing convention for dword
globals IDA represents as two `word_` symbols (see
`_playerCreditsLo`/`_playerCreditsHi`).

Applied via `apply_renames_gatemain.py`'s eightieth batch.

### `_isindst` named

Moved to `sub_1B8F0` (3 callers, another sibling of the `_tzset`
cluster, called from the already-named `_ftime` and `__dtoxtime`).
Reads a 0-based month field at `tm+8`: month < 3 (before April) or
month > 9 (after October) returns 0 immediately (never DST); month
strictly between 3 and 9 (May through September inclusive) returns 1
immediately (always DST) — exactly the pre-2007 US DST rule (Daylight
Saving Time ran from the first Sunday of April through the last Sunday
of October before the 2007 rule change). For the two boundary months
(3 = April, 9 = October) it computes the weekday of a reference date
via a classic day-of-week formula (a year field at `tm+0Ah`, times 365
plus a leap-day correction, divided by 7) to find that month's first
(April) or last (October) Sunday, then compares the date's day-of-month
(`tm+0Eh`) and what's plausibly an hour field (`tm+4`, checked against
the 2am DST-transition hour) against it to decide which side of the
transition the given date/time falls on. This is the standard MSC
runtime `_isindst()`, closing out the `_tzset`/timezone-globals cluster
from the previous pass.

Applied via `apply_renames_gatemain.py`'s eighty-first batch.

### `Dos_setErrnoFromCode` and `errno` named

Moved to `sub_18F54` (3 callers) — the real worker behind the
already-named `__maperror`, which just zeroes `ah` and tail-calls this.
If `ah` is already non-zero on entry, the function uses it directly as
the result (its other two callers, `_close` and `_dos_findfirst`, call
it directly rather than through `__maperror`, presumably passing a
pre-known errno value straight through via `ah`). Otherwise it clamps
`al` to a max index (`byte_CAE19`, with a special case forcing index 5
for `al` in 0x20-0x21), looks it up via `xlat` against a translation
table at segment offset `0x2F3A` (a DOS extended-error-code → errno
mapping table), and stores the sign-extended result into the global
now named `errno`.

That global (`word_CAE11`) was confirmed via the already-named `fread`
reading it, plus two direct `0x16`/`EINVAL` literal stores elsewhere in
the code — standard C runtime `errno` usage throughout.

Applied via `apply_renames_gatemain.py`'s eighty-second batch.

### `Picture_checkFormatMatch` named

Moved to `sub_25B90` (3 callers) — reads the global `pic_header._flags`
byte and masks it to the low nibble, a format-code sub-field distinct
from the individual flag bits tested elsewhere in the same struct
(e.g. the already-named `PICFLAG_HAS_PALETTE`, and separately-tested
`0x10`/`0x40` bits). Uses that format code to index a per-format table
(`byte_C96B0`), and compares the result against a per-video-mode table
(`byte_C96B6`) indexed by the current `_videoIndex`; if they match,
returns the format-table value (the picture's required video mode),
otherwise returns 0.

Called from `Picture_Load`, `load_and_scale_pic`, and `scale_pic` — the
latter prints the literal string `" scale_pic : EGA -> VGA disabled "`
(sitting right after these two tables in memory) when a picture's
format doesn't match the active video mode, consistent with this being
the format/video-mode compatibility check that decision is based on.

Applied via `apply_renames_gatemain.py`'s eighty-third batch.

### `Image_allocateSurface` named

Moved to `sub_2BCA5` (3 callers) — calls `sub_2A9C7(height, width,
videoIndex)` to compute a required buffer size; if that reports an
error, returns the error code `0xFFE6` (-26). Otherwise allocates a
handle of that size via the already-named `new_handle`; if allocation
fails (a null handle), also returns `0xFFE6`. Otherwise calls
`sub_2BBBA(height, width, videoIndex, handle, surface, arg_A)` to
build/decode the actual image data into the new buffer, and returns 0
on success.

Called from `sub_24A42`, itself called from the already-named
`Image_load` — the size-computation-plus-allocation-plus-build step of
loading an image into a surface. `sub_2A9C7` and `sub_2BBBA` remain
unnamed.

Applied via `apply_renames_gatemain.py`'s eighty-fourth batch.

### `InputWindow_setDisplayMode` named

Moved to `sub_5CD81` (3 direct callers, plus several more via
`thunk_sub_5CD81` from other overlays) — remaps mode 2 to 5, then
compares against a cached current mode (`word_CBCFE`); no-ops if
unchanged. Otherwise resets `word_CBD94` to 0, caches the new mode, and
checks a second mode-like global (`word_CBCFC`) against 2: if it's 2,
calls the already-named `Scene_draw(0)` then
`InputWindow_redrawPromptLine` (a full scene-plus-prompt-line redraw).
Otherwise hides the mouse, clears the already-named `Input_window_mb`
via `WindowText_clear`, calls the tentatively-named `scene_update?`,
then shows the mouse again.

Called with small literal mode values (1, 3, 4, and 5 observed at
different call sites) from `InputWindow_getLine`, `get_mouse_input`,
and several room-logic overlays reached through the thunk — a shared
display/input-mode switch that only runs its transition side effects
when the mode actually changes.

Applied via `apply_renames_gatemain.py`'s eighty-fifth batch.

### The undo-snapshot cluster named

A full, tightly-coupled cluster traced together. `sub_62AB0` →
`Undo_resetSnapshotBuffer`: if the global memory handle (now
`_undoSnapshotHandle`) is non-null, frees it via the already-named
`kill_handle` and zeroes it; always resets a size accumulator (now
`_undoSnapshotSize`) and two flags (now `Parser_undoSnapshotValid` and
`Parser_undoBufferAllocated`) to 0.

`sub_62AE2` → `Undo_allocateSnapshotBuffer`: calls
`Undo_resetSnapshotBuffer`, then computes a required buffer size by
summing per-entry contributions across the object method table
(indexed over the already-named `METHODS_COUNT`) and the save-field
table (indexed over the already-named `SAVE_FIELDS_COUNT`) into
`_undoSnapshotSize`, plus a constant `0x42`. Compares that (rounded up)
against `get_buffer_size()`'s available space; if it fits, allocates a
new handle of that size via `new_handle` into `_undoSnapshotHandle`,
and if allocation succeeded, sets `Parser_undoBufferAllocated` to 1.

Both are called from the already-named `save_game`'s mode-3
("quicksave") path: it clears `Parser_undoSnapshotValid` up front,
(re)allocates the buffer if needed, then locks the handle and writes
the current game state into it via `synchronize_save` — treating the
handle's memory as a virtual file — setting
`Parser_undoSnapshotValid` to 1 only if that write succeeds.

The already-named `Parser_performUndo` requires both flags before
actually loading the undo slot (`load_game(3)`): `Parser_undoBufferAllocated`
alone decides which of two messages it prints ("undone" vs "nothing to
undo" — the underlying message strings, at `off_CB926`/`off_CB92A`,
weren't decoded this pass), while `Parser_undoSnapshotValid` gates
whether the load actually happens at all — capturing the difference
between "a buffer exists" and "a valid snapshot was taken this turn."

Applied via `apply_renames_gatemain.py`'s eighty-sixth batch.

### `Opl2_setOperatorVolume` named

Moved to `sub_1D492` (3 callers, called twice per note from the
already-named `Opl2_noteOn` — once per operator, carrier and
modulator, closing out the "not renamed" reference left in that
writeup). Reads a cached per-channel velocity value (stored earlier by
`Opl2_noteOn` at offset `0x1CA` for rhythm channels or `0x1B8` for
melodic ones), then reads the operator's current output-level register
byte (masked to the low 6 bits, the OPL2 level field) and inverts it
(`0x3F` − level) so higher means louder.

For operators flagged to track velocity (gated by a per-operator byte
at `+0x1A6` and a table byte at `+0xC`), it scales that inverted level
by the velocity through a lookup table and rescales back down. It then
re-inverts to attenuation scale, ORs in the operator's key-scale-level
bits (shifted into bits 6-7), and writes the result to OPL2 register
`0x40+operatorRegisterOffset` — the standard OPL2 Level/KSL register —
via the already-named `Opl2_writeRegister`.

Applied via `apply_renames_gatemain.py`'s eighty-seventh batch.

### `Opl2_setNoteSelect` named

Moved to `sub_1D570` (3 callers) — writes OPL2 register 8 (the
chip-wide CSM-select/Note-Select register) with `0x40` if the global
`byte_D1C54` is non-zero, or 0 otherwise. This is the standard OPL2 NTS
(bit 6) keyboard-split-mode bit, which changes how key-scale frequency
splits are computed across the whole chip. `byte_D1C54` is set by this
function's only caller, `sub_1CF90`, immediately before calling this to
commit the setting to hardware.

Applied via `apply_renames_gatemain.py`'s eighty-eighth batch.

### `Sound_initPlaybackTiming` named

Moved to `sub_20390` (3 callers) — no-ops unless bit `0x10` of the
sound-engine state word (`word_C8582`, a widely-shared bitmask used
throughout the sound backend selection/dispatch code — which exact
backend bit `0x10` marks wasn't pinned down this pass) is set. When
set, computes `arg_2 * 100000` (`0x186A0`, via `__aFlmul`) into a
32-bit pair (`word_C858E:word_C8590` — plausibly a duration or
sample-count scaled into a fixed-point/microsecond unit), stores
`arg_4` into `word_C858C`, copies an existing 32-bit value
(`word_C8596:word_C8598`) into `word_C8592:word_C8594` (plausibly
resetting an elapsed/position counter from a total-duration value),
and stores `arg_0` into `word_C858A`.

Called from `sub_1E7D4` and `sub_1F1DE` (both unnamed, themselves
called from the already-named `Sound_stopTrack` area, and matching the
"other Sound_stopTrack backend routines" flagged as open in earlier
passes) — consistent with initializing per-track playback-timing state
for one specific sound backend before starting or resuming a track.
Which backend, and the exact units of the stored values, remain open.

Applied via `apply_renames_gatemain.py`'s eighty-ninth batch.

### `Sound_getElapsedPlaybackTime` named

Moved to `sub_203D6` (3 callers) — computes `word_C8596:word_C8598`
minus a snapshot pair (`_tmpSub._val7/_val8`) via unsigned 32-bit
subtract, then unsigned-divides that by 1000 (`__aFuldiv`) and returns
the result. `word_C8596:word_C8598` is a running clock/tick counter;
`_tmpSub._val7/_val8` is a snapshot of that same counter taken
elsewhere (in `sub_1EB9E`, unnamed) at track-start time — so this
computes elapsed playback time since that snapshot, scaled down by
1000 (plausibly milliseconds from an underlying microsecond-ish tick,
matching the previous pass's `Sound_initPlaybackTiming` and its
`*100000` scaling).

Called from `sub_1E7D4` and `sub_1F1DE`, the same two unnamed callers
as `Sound_initPlaybackTiming` — the elapsed-time query half of that
same per-track timing mechanism.

Applied via `apply_renames_gatemain.py`'s ninetieth batch.

### `Game_showCaptionText` named

Moved to `sub_157A9` (2 callers) — the long-flagged "`Game_showIllustration`'s
caption-text helper" from several passes ago, finally traced directly.
Takes a far-pointer array of dword message pointers (`msgArray`,
grouped by null `0:0` separator entries — the same shape
`TextWindow_addMessageList` walks for its text-only fallback), an
(x, y) starting position, and a `redrawPicture` flag.

For each group: resets the y position, then for every message in the
group draws it *twice* via `Font_setColor`/`Font_setPosition`/
`Font_writeString` — once in black at `(x, y)`, then again in white at
`(x-1, y-1)` — a drop-shadow/embossed caption style, advancing y by 12
pixels per line. After a group finishes, it delays roughly 15
(ticks/seconds); if that delay is interrupted by a skip keypress, it
returns immediately. Otherwise, if `redrawPicture` is set, it redraws
the already-loaded illustration before continuing to the next caption
group over the same picture; if not set, it fills the screen black
instead — supporting both "captions layered over a static picture" and
"sequential black-background caption pages" (e.g. ending-sequence text
crawls) as two different display modes of the same routine.

Applied via `apply_renames_gatemain.py`'s ninety-first batch.

### `Sound_lookupTrackVariant` named

Moved to `sub_15F35` (2 callers) — the long-flagged "sound resource-
variant lookup" from several passes ago, finally traced. Walks a
6-bytes-per-entry table (up to 37 entries) whose first word field is a
key; if the given track ID matches an entry's key, returns either that
entry's second word field when the MIDI-active bit (bit 4) of
`word_C8582` is set, or its third word field otherwise. Returns 0 if
the track ID isn't found.

Called twice from the already-named `Sound_selectTrackForRoom` — a
per-room/track table mapping a logical track ID to the specific
sound-resource number to use for whichever backend (MIDI vs. other) is
currently active.

Applied via `apply_renames_gatemain.py`'s ninety-second batch.

### `Game_refuseRestart` named

Moved to `sub_1057E` (2 callers) — calls the already-named
`LogFile_close`, frees the currently-loaded illustration, invokes a
pre-hook to the current room's logic (`Logic_call(_roomLogicNum,
action=24)`), reloads game state via `load_game(1)`, redraws, invokes
a matching post-hook (`action=25`), then prints the decoded literal
message `"[Sorry, you can't use \"restart\" right now.]"` and returns
-1.

Despite the `load_game`/`Logic_call` bracketing looking restart-shaped,
the printed message is unambiguous: this is the handler for a restart
request the engine declines — called from the already-named
`Game_endGameMenu` and from `sub_69EDA` — most plausibly reloading or
resuming the current session rather than actually restarting, distinct
from the already-named `Game_restartAfterDeath`, which performs a real
restart.

Applied via `apply_renames_gatemain.py`'s ninety-third batch.

### Correction: the "digitized PC-speaker" ISR is actually Sound Blaster DMA playback

Tracing `Speaker_sampleIsr`'s two dispatch targets (`sub_18883` and
`sub_18905`) revealed that this whole cluster was mischaracterized
several sessions ago as a "digitized PC-speaker sound-effect engine."
It is actually the **Sound Blaster DSP's auto-init-DMA sample-playback
interrupt handler**. PC-speaker playback never touches the ISA DMA
controller or the Sound Blaster DSP base port — but both dispatch
targets do so directly and unambiguously:

- `sub_18883` → **`SoundBlaster_startNextDmaBlock`**: reprograms ISA
  DMA channel 1 (mask, clear flip-flop, auto-init/write/increment/
  single mode `0x49`, base address, page register, count, re-enable),
  advances the internal block index, then sends Sound Blaster DSP
  command `0x14` (8-bit single-cycle DMA DAC output) plus the 16-bit
  sample count via the DSP command port. The "more data queued, start
  the next DMA block" dispatch path.
- `sub_18905` → **`SoundBlaster_uninstallDmaIsr`**: masks DMA channel
  1, restores the original interrupt vector for the configured SB IRQ
  line (the vector this ISR's installer had saved), clears two state
  flags, and reads the DSP's IRQ-acknowledge port to clear the pending
  hardware interrupt. The "playback finished, uninstall and acknowledge"
  dispatch path.

The installer code sitting just above the ISR in the same segment
confirms this further: it saves the current interrupt vector for a
configurable IRQ line (`byte_C84F5`, defaulting to 3 — a classic Sound
Blaster IRQ choice) before installing this ISR, using the already-named
`Sb_writeByte` to program the card first.

The renamed ISR itself is now **`SoundBlaster_dmaIsr`**. The buffer-
position/length globals it sets up on entry
(`byte_C84F6`/`word_C84F7`/`word_C84FC`/`word_C84FE`/`word_C8500`/
`byte_C84FB`) are unaffected by this correction — they're real
per-block DMA state, just belonging to the Sound Blaster backend
instead of a PC-speaker one. Whether a genuinely separate PC-speaker
digitized-sample path exists elsewhere in the sound engine wasn't
re-checked this pass -- only that this specific ISR and its cluster
belong to Sound Blaster, not PC-speaker.

Applied via `apply_renames_gatemain.py`'s ninety-fourth batch.

### `SoundBlaster_writeByteFromIsr` named

Moved to `sub_18950` (2 callers) — byte-for-byte the same DSP-write
handshake as the already-named `Sb_writeByte` (poll the status port
until bit 7 clears, a few I/O-delay jumps, then write the byte) —
another instance of this project's duplicate-compiled-copy pattern
(previously seen with `Vocab_matchesAbbreviation`/
`String_matchesPrefixCI` and the MIDI VLQ-decode pair). The one
difference: this copy polls unboundedly, with no retry limit and no
carry-flag timeout signal, fitting its use inside
`SoundBlaster_startNextDmaBlock`, itself called from interrupt context
(`SoundBlaster_dmaIsr`) where a private, self-contained near-call
duplicate avoids relying on the far-callable `Sb_writeByte`'s calling
convention. This completes the Sound Blaster DMA cluster corrected in
the previous pass.

Applied via `apply_renames_gatemain.py`'s ninety-fifth batch.

### `Opl2_stopTrack` named

Moved to `sub_1E974` (2 callers) — one of the long-flagged "other
Sound_stopTrack backend routines," now confirmed as the OPL2/AdLib
backend's stop-track handler, called directly from the already-named
`Sound_stopTrack`. Resets a state byte, calls an unnamed helper
(`sub_1CC34(0)`, plausibly resetting some MIDI-file-position parse
state shared with the MIDI backend), and conditionally calls another
unnamed cleanup routine if a track-loaded flag is set.

Then loops over all 11 OPL2 logical channels (0-10, matching 9 melodic
voices or 6 melodic + 5 rhythm-mode percussion voices), zeroing a
per-channel state word and calling the already-named
`Opl2_noteOn(channel, velocity=0)` then `Opl2_noteOff(channel)` to
silence each one. Finally, if a specific bit of the shared
`word_C8582` sound-state word is set, clears a related bit range and
resets the state byte again — tearing down whatever per-track state
that bit range represents for this backend.

Applied via `apply_renames_gatemain.py`'s ninety-sixth batch.

### `Parser_printBeMoreSpecific` named

Moved to `sub_13CB1` (2 callers) — prints the literal message
`"[Please be more specific.%s]\n"` via `TextWindow_add`. Called from
the already-named `GatewayParser_speakHandler` and `Parser_proc6` — a
simpler, generic sibling of the already-named
`Parser_askForClarification` (which fills in a specific ambiguous
word), used when the parser needs to ask for clarification without a
particular word to reference.

Applied via `apply_renames_gatemain.py`'s ninety-seventh batch.

### `Parser_printTalkingIsStrange` named

Moved to `sub_13F85` (2 callers) — if given logic ID `0xD3` (a
special-cased ID, plausibly the player character), uses the literal
string `" yourself"` as the referenced name; otherwise calls
`j_printObj(logicNum, 3)` to get the target's descriptive name/pronoun.
Either way, prints decoded `GATESTR.DAT` message `0xC404` — *"Doesn't
it strike you that talking to%s is just a little strange?"* — filling
in the name. Then sets `Persisted_val6` to `'e'` and queues logic/event
`0x2B` via `Queue_add(0x2B, 1)` (plausibly scheduling a delayed
follow-up reaction).

Called from the already-named `GatewayParser_speakHandler` — the
parser's response to trying to talk to a non-conversational target (an
object, or oneself).

Applied via `apply_renames_gatemain.py`'s ninety-eighth batch.

### `Parser_callActionHandler` named

Moved to `sub_147A6` (2 callers) — bounds-checks its `actionId`
argument to 1-195 (returning 0 if 0 or out of range), then indexes a
6-bytes-per-entry function-pointer table (`off_3C978`) by
`actionId*6` and calls that far function pointer directly, returning
its result. Called from the already-named `Parser_perform` — the core
"dispatch to the handler for this action/verb ID" primitive of the
parser execution engine, similar in shape to the object-method-table
dispatch pattern seen elsewhere (`sub_1234F`/`sub_123A1`, left unnamed
due to corrupted disassembly) but using its own separate table.

Applied via `apply_renames_gatemain.py`'s ninety-ninth batch.

### `Windows_switchListboxWindow` named

Moved to `sub_169A6` (2 callers) — no-ops if `Windows_currentWindow`
is negative. Otherwise redraws the current listbox deselected, then
steps `Windows_currentWindow` forward or backward (wrapping through
the 6 window slots 0-5, direction chosen by the sign of its argument)
until landing on one whose `Windows_listboxIndex[]` entry indicates it
actually has a listbox, then redraws that window's listbox selected.

Called from the already-named `get_mouse_input` and from `sub_1796D`
— the "switch focus to the next/previous listbox window" navigation
primitive, e.g. Tab/Shift-Tab-style cycling between listbox windows.

Applied via `apply_renames_gatemain.py`'s hundredth batch.

### `Listbox_getSelectedIndexForWindow` named

Moved to `sub_16A89` (2 callers) — calls the already-named
`Windows_getListboxIndex(winNumber)`; if that's negative (the window
has no listbox), returns -1. Otherwise returns `Listbox_selectedIndex[]`
at that listbox index — the currently-selected item index for the
listbox in the given window. Called twice from the already-named
`Listbox_mouseButtonDown`.

Applied via `apply_renames_gatemain.py`'s hundred-and-first batch.

### `Pit_setReloadCount` and `Sound_setTimerRate` named

A tightly-coupled pair traced together. `Pit_setReloadCount`
(`sub_1CC15`) programs the 8253/8254 PIT's channel 0 (the system
timer, normally driving IRQ0) via `out 0x43` with command byte `0x36`
(channel 0, mode 3 square wave, 16-bit binary, LSB-then-MSB access),
then writes the given count's low byte then high byte to port `0x40`
— the standard sequence to reprogram the system timer to a custom
tick rate.

`Sound_setTimerRate` (`sub_1CC34`) is the higher-level caller: it
stores the given rate into a code-segment-resident word (consistent
with being read back by a timer ISR living in the same segment), sets
a "stopped" flag if the rate is less than 1, then calls
`Pit_setReloadCount` to actually reprogram the hardware. It runs with
interrupts disabled around the whole sequence. Called from the
already-named `Opl2_stopTrack` (with rate 0, resetting/stopping the
timer) and from `sub_1E329` (unnamed, presumably setting a real tempo-
derived rate) — the shared master timer-rate control underlying the
sound engine's custom tick clock. `Pit_setReloadCount` itself has a
second, direct caller (`sub_1CC58`, unnamed) that bypasses
`Sound_setTimerRate`'s bookkeeping.

Applied via `apply_renames_gatemain.py`'s hundred-and-second batch.

### `Logics_collectPlayerItemLists` named

Moved to `sub_15932` (2 callers) — walks two separate contained-items
linked lists off logic ID `0xD3` (the player, per this session's
earlier finding in `Parser_printTalkingIsStrange`) using the already-
documented `Logics_getUnkHandler(0xD3, handlerIndex)`/`Logics_getVal1`
traversal pattern (the same shape `Logics_describeContents` uses for
its container-contents walk): first with `handlerIndex=1`, snapshotting
each visited logic ID into a flat array, null-terminated; then again
with `handlerIndex=0` into a second flat array, also null-terminated.

Called from `sub_A2D8D` and `sub_3141B` (both unnamed) — plausibly
separating the player's worn vs. carried items (or two similarly-split
inventory categories) into two ready-to-iterate arrays, though which
handler index maps to which category wasn't independently confirmed
this pass.

Applied via `apply_renames_gatemain.py`'s hundred-and-third batch.

### `Listbox_getSelectedItemText` named

Moved to `sub_16B53` (2 callers) — returns a far pointer to a static
buffer holding the text of the current window's currently-selected
listbox item, or an empty string if there's no listbox or no items.
Two source formats are handled: if the listbox's item data is a
raw-text blob (a sentinel word at its start), the selected line is
copied out directly and trailing spaces trimmed. Otherwise the listbox
stores each line as a list of vocab-word indices, so each word's text
is looked up in the vocabulary table and concatenated together with
spaces. Finally, if a per-listbox flag has a specific bit set, the
first character of the result is capitalized.

Called from `prompt_for_filename` and `sub_5D9F3` (the RTLink-thunked
function behind `thunk_sub_5D9F3`, seen many times this session as a
caller of other listbox/UI primitives) — the general "read the
highlighted listbox entry as a string" primitive, e.g. for reading a
selected filename out of a file-picker listbox.

Applied via `apply_renames_gatemain.py`'s hundred-and-fourth batch.

### `Listbox_handleNavigationKey` named

Moved to `sub_1796D` (2 callers) — the listbox keyboard-navigation
dispatcher. Maps an extended key code to: Home / End / Page Up / Page
Down (via the already-named `Listbox_getNumLines`) / Up / Down (via
the already-named `Listbox_deltaChange`) / Left / Right (switch
listbox window, via the already-named `Windows_switchListboxWindow`).
For any other character, gates on an unnamed helper, and if it passes
and the character is printable/alphabetic, calls the already-named
`Listbox_findLineStartingWith` — a type-ahead jump-to-item feature.
Returns 1 if the keypress was consumed by the listbox, 0 otherwise
(letting the caller process it as a normal character).

Called from `get_mouse_input` and `prompt_for_filename`.

Applied via `apply_renames_gatemain.py`'s hundred-and-fifth batch.

### `Opl2_setChannelFeedback` named

Moved to `sub_1D58C` (2 callers) — no-ops if the per-channel flag byte
at `+0x1A6` is set (the same flag `Opl2_setOperatorVolume` gates its
velocity-tracking on), i.e. this only applies to normal (non-rhythm-
only) channels. Otherwise indexes the same 7-byte-stride per-channel
table `Opl2_setOperatorVolume` uses to read a feedback-amount byte
(doubled into bits 1-3) and an algorithm/connection-type byte (set as
bit 0), ORs them together, and writes the result to OPL2 register
`0xC0+channelRegisterOffset` — the standard OPL2 Feedback/Connection-
Type register — via the already-named `Opl2_writeRegister`, using the
same per-channel register-offset field as `Opl2_setOperatorVolume`.

Called from `sub_1D3C4` and `sub_1D448` (both unnamed, plausibly the
note-on/instrument-setup routines for this backend).

Applied via `apply_renames_gatemain.py`'s hundred-and-sixth batch.

### `Opl2_setOperatorAttackDecay` and `Opl2_setOperatorSustainRelease` named

Two more per-operator OPL2 register setters, traced together.
`Opl2_setOperatorAttackDecay` (`sub_1D5E8`) indexes the same 7-byte-
stride table the OPL2 cluster's other per-operator setters use, reads
a byte shifted into the high nibble and another masked into the low
nibble, ORs them together, and writes the result to OPL2 register
`0x60+operatorRegisterOffset` — the standard OPL2 Attack-Rate/Decay-
Rate register.

`Opl2_setOperatorSustainRelease` (`sub_1D63E`) is byte-for-byte the
same shape, reading two different table fields and writing to register
`0x80+operatorRegisterOffset` — the standard OPL2 Sustain-Level/
Release-Rate register.

Both are called from `sub_1D3C4` and `sub_1D448` (both unnamed,
plausibly the note-on/instrument-setup routines for this backend),
completing the four standard per-operator OPL2 envelope/level
registers this cluster now covers: `0x40` level/KSL
(`Opl2_setOperatorVolume`), `0x60` attack/decay, `0x80` sustain/
release, plus the per-channel `0xC0` feedback/connection
(`Opl2_setChannelFeedback`).

Applied via `apply_renames_gatemain.py`'s hundred-and-seventh batch.

### `Opl2_setOperatorModulationFlags` named

Moved to `sub_1D694` (2 callers) — indexes the same 7-byte-stride
table as the rest of this cluster and builds a byte from four boolean
flag fields (bits 4-7) plus a 4-bit value (bits 0-3), writing the
result to OPL2 register `0x20+operatorRegisterOffset` via the
already-named `Opl2_writeRegister`. Register `0x20-0x35` is the
standard OPL2 AM/Vibrato/Envelope-Type/KSR/Multiple register (Tremolo,
Vibrato, sustain EG type, key-scale rate, and frequency multiplier).

Called from `sub_1D3C4` and `sub_1D448`, completing all 5 standard
OPL2 per-operator/channel registers this cluster now covers: `0x20`
AM/VIB/EG/KSR/Mult, `0x40` Level/KSL (`Opl2_setOperatorVolume`), `0x60`
Attack/Decay, `0x80` Sustain/Release, and `0xC0` Feedback/Connection
(`Opl2_setChannelFeedback`).

Applied via `apply_renames_gatemain.py`'s hundred-and-eighth batch.

### The OPL2 per-operator register cluster closed out

Three more functions traced together, completing the whole cluster
this session built up piece by piece. `Opl2_setOperatorWaveform`
(`sub_1D786`) writes OPL2 register `0xE0+operatorRegisterOffset` (the
Waveform-Select register) from either a 2-bit value in the per-
operator table or a forced 0 (sine) when a "waveform select enabled"
global flag is clear — the last of the standard OPL2 per-operator
registers (`0x20`/`0x40`/`0x60`/`0x80`/`0xC0`/`0xE0`) left to identify.

`Opl2_setOperatorProperty` (`sub_1D3C4`) is a property-ID dispatcher:
given an operator index and a property ID (0-0x11), it calls exactly
one of this session's OPL2 register setters — letting a caller update
a single named instrument/operator parameter without touching the
others.

`Opl2_applyOperatorSettings` (`sub_1D448`) is the "commit everything"
counterpart: it unconditionally calls every one of those same register
setters in sequence for one operator, the "load/commit a full
instrument definition" role, called from `sub_1D2FC` when first
setting up an operator's complete OPL2 register state.

Together, `Opl2_setOperatorProperty` and `Opl2_applyOperatorSettings`
tie together the entire OPL2 register-writing cluster traced across
several passes this session: `Opl2_setOperatorVolume` (`0x40`),
`Opl2_setNoteSelect` (register 8), `Opl2_setChannelFeedback` (`0xC0`),
`Opl2_setOperatorAttackDecay` (`0x60`),
`Opl2_setOperatorSustainRelease` (`0x80`),
`Opl2_setOperatorModulationFlags` (`0x20`), and
`Opl2_setOperatorWaveform` (`0xE0`) — a complete instrument-parameter
interface for the OPL2/AdLib backend.

Applied via `apply_renames_gatemain.py`'s hundred-and-ninth batch.

### `Opl2_loadOperatorPatch` named

Moved to `sub_1D2FC` (2 callers) — copies 13 bytes from a source patch
structure (read every other byte — a 26-byte source, matching a
duplicated-field or word-sized-field-per-byte encoding) into the
7-byte-stride per-operator table this whole OPL2 cluster shares, then
writes a `waveform` argument (masked to 2 bits) into the field the
already-named `Opl2_setOperatorWaveform` reads. Finally calls the
already-named `Opl2_applyOperatorSettings` to push the whole loaded
patch out to OPL2 hardware in one go.

This is the "load a MIDI-instrument-patch definition into an OPL2
operator" entry point, called from `sub_1CFB0` — the natural top of
the whole OPL2 register cluster traced across this session.

Applied via `apply_renames_gatemain.py`'s hundred-and-tenth batch.

### `Opl2_setRhythmMode` and `_opl2ChannelCount` named

Moved to `sub_1CEC0` (2 callers) — the master OPL2 rhythm-mode toggle.
When enabling, configures two extra rhythm-mode channels (OPL2's hi-
hat/cymbal-style extra voices) via a per-channel setter in this same
cluster with specific hardcoded parameters. Always sets the already-
named `_opl2RhythmEnabled` global from its argument, sets the channel
count (now `_opl2ChannelCount`) to 11 (6 melodic + 5 rhythm-mode
percussion voices) if enabling or 9 (melodic-only) otherwise —
matching the channel-loop bounds already seen in the already-named
`Opl2_stopTrack` — clears the already-named `_opl2RhythmInstruments`
bitmask, then calls an unnamed helper and the already-named
`Opl2_writeRhythmRegister` to commit the mode switch to hardware.

`_opl2ChannelCount` (`word_D1C82`) is the OPL2 backend's active
logical-channel count, read as a channel-loop upper bound elsewhere in
the backend.

Applied via `apply_renames_gatemain.py`'s hundred-and-eleventh batch.

### `Opl2_setMasterVolume` and `_opl2MasterVolume` named

Moved to `sub_1CF6E` (2 callers) — clamps its argument to the range
1-12 and stores it into a global (now `_opl2MasterVolume`). That
global's only reader is `sub_1CB32` (unnamed), called from `sub_1D7DA`
— part of this same OPL2 per-channel cluster, itself called from the
already-named `Opl2_setRhythmMode` — consistent with a 1-12 master-
volume-style scaling factor feeding into per-channel volume/instrument
calculations.

Applied via `apply_renames_gatemain.py`'s hundred-and-twelfth batch.

### `Opl2_writeDetectRegister` named

Moved to `sub_1861F` (3 callers) — byte-for-byte the same OPL2
register-write sequence as the already-named `Opl2_writeRegister`
(write the register number to port `0x388`, delay via 4 `IN` reads —
the calibrated ~3.3μs address-write delay from the Adlib Programming
Guide — write the value to port `0x389`, then delay via 23 `IN` reads
of port `0x388`) but as a private, near-callable duplicate using raw
port I/O directly instead of calling `Opl2_writeRegister`.

Called repeatedly from `sub_18432` (itself called from the already-
named `Stream_selectHandler`), consistent with the classic AdLib/OPL2
hardware-presence detection sequence (reset/mask the timers, start
timer 1, check status) rather than the main note-playing engine —
another instance of this project's duplicate-compiled-copy pattern.

Applied via `apply_renames_gatemain.py`'s hundred-and-thirteenth batch.

### `Sound_takeTrackFlag` named

Moved to `sub_1DD8E` (2 callers) — with interrupts disabled,
atomically reads then zeroes a per-track word in a small table — a
classic "take and clear a pending-event flag shared with an ISR"
pattern (the `cli`/`sti` bracketing implies a timer ISR writes this
same table). Called from `sub_1F1DE` and `sub_1F93E`, both already
seen as callers within the `Sound_initPlaybackTiming`/
`Sound_getElapsedPlaybackTime` timing cluster from earlier this
session — plausibly consuming a per-track "segment/loop completed"
notification set by the sound engine's timer ISR, though the exact
event this flag represents wasn't independently confirmed.

Applied via `apply_renames_gatemain.py`'s hundred-and-fourteenth batch.

### `Midi_setDataCallback` named

Moved to `sub_1D953` (2 callers) — with interrupts disabled,
atomically sets the already-named far-pointer global
`_midiDataCallback` from its two word arguments. Called twice from
`sub_1F552` — a simple atomic setter for the MIDI data callback
pointer, matching the `cli`/`sti` pattern used elsewhere in this
session's sound-timing cluster for state shared with an ISR.

Applied via `apply_renames_gatemain.py`'s hundred-and-fifteenth batch.

### `Midi_stopTrack` named

Moved to `sub_1F910` (2 callers) — another of the long-flagged "other
Sound_stopTrack backend routines," called directly from the already-
named `Sound_stopTrack`. Resets a scratch value, sets a flag word to
1, then busy-loops calling an unnamed helper (tied to the just-named
`Midi_setDataCallback`'s caller's neighborhood) until that flag word is
cleared back to 0 — a blocking "drain the pending MIDI queue until
finished" loop. Afterward clears a second flag if set, and returns 1.

The MIDI/MPU-401 backend's stop-track handler, paralleling the
already-named `Opl2_stopTrack` for the OPL2 backend — with this,
both of the game's synthesized-music backends now have a named
stop-track handler.

Applied via `apply_renames_gatemain.py`'s hundred-and-sixteenth batch.

### `Midi_stopTrackStep` named

Moved to `sub_1F93E` (2 callers) — the per-call state-machine step
`Midi_stopTrack`'s busy-loop repeatedly invokes, advancing a shared
scratch state counter (reused here purely as a 0-19 step index) by one
on every call via a 20-entry jump table:

- **Step 1**: if a flush-needed flag is set, calls the already-named
  `Sound_takeTrackFlag` per track and sends MIDI byte `0xFC` for any
  flagged one, then resets the MIDI device.
- **Step 2**: resets the device, installs a fixed completion routine
  via the just-named `Midi_setDataCallback`, then spins sending a
  device command until it succeeds.
- **Steps 3-18** (16 steps, one per MIDI channel): send Control Change
  123 ("All Notes Off") and 121 ("Reset All Controllers") on that
  channel.
- **Step 19**: clears the flag `Midi_stopTrack`'s busy-loop is waiting
  on, signaling the whole shutdown sequence is complete.

This one function makes `Midi_stopTrack`'s entire multi-call drain
loop concrete: a proper MIDI "all channels silent, device reset"
teardown sequence, spread one step per call so it doesn't block for
too long in any single call.

Applied via `apply_renames_gatemain.py`'s hundred-and-seventeenth batch.

### `Midi_sendDisplayText` named (corrects a much earlier guess)

Moved to `sub_1FB56` (2 callers, called from the already-named
`Sound_selectDevice` and `Sound_shutdown`). Sends a Roland-style MIDI
SysEx "Display Data" message: three header bytes (`0x20, 0, 0` —
matching Roland's MT-32/Sound-Canvas Display-Data SysEx address `0x20
0x00 0x00`), then 20 bytes read from the given string, accumulating a
running byte sum, then a standard Roland 7-bit two's-complement
checksum byte — all sent one byte at a time via `Midi_sendByte`, with
unnamed helpers presumably framing the SysEx start/end.

This directly corrects the guess made in `Sound_shutdown`'s own
writeup much earlier this session, that this call "clears an on-screen
device indicator" — it never touches the screen. `Sound_selectDevice`
calls it on successful MPU-401 detection (presumably to show a device/
game identifier), and `Sound_shutdown` calls it presumably to clear/
blank the display on exit — writing text to a General-MIDI-compatible
module's own onboard LCD, not the game's screen.

Applied via `apply_renames_gatemain.py`'s hundred-and-eighteenth batch.

### The Roland SysEx framing trio named

Three functions traced together, confirming and completing the
prediction made in `Midi_sendDisplayText`'s own writeup. `sub_1FA8E`
→ **`Midi_beginRolandSysEx`**: calls the already-named `Midi_initDevice`;
on success, resets the device, sends MPU-401 UART-mode command `0x3F`
(spinning until it succeeds), then sends the exact standard Roland
SysEx header — `0xF0` (SysEx start), `0x41` (Roland manufacturer ID),
`0x10` (device ID), `0x16` (Roland MT-32/Sound Canvas model ID), `0x12`
(DT1 "Data Set 1" command) — preceding an address+data+checksum+
terminator message.

`sub_1FAFE` → **`Midi_endSysEx`**: sends `0xF7` (MIDI SysEx "End of
Exclusive"), then calls the already-named `Midi_shutdown` — the
closing half of the pair.

`sub_1FC4E` → **`Midi_busyWaitDelay`**: a calibrated busy-wait loop
(counter squared each iteration, result unused) purely to burn CPU
cycles, presumably giving the MIDI device time to process a SysEx
message before the caller continues.

All three are called from `Midi_sendDisplayText` and `sub_1FB10` —
`Midi_beginRolandSysEx`/`Midi_endSysEx` bracket the Roland Data Set
SysEx payload, with `Midi_busyWaitDelay` settling afterward.

Applied via `apply_renames_gatemain.py`'s hundred-and-nineteenth batch.

### `Sound_loadAndStartTrack` named

Moved to `sub_1FE5C` (868 bytes, 2 callers — the largest function
named this session) — the shared track-loading worker called from
both the already-named `Sound_selectTrack` and
`Sound_selectTrackForRoom`, after they've picked a track/room number.
No-ops unless a relevant sound-state bit is set and a track/room
selector global is nonzero.

Opens the selected `.MUS` resource via `open_file2`; when coming from
the room-based path, additionally walks up to 4 header-described
sub-chunks, allocating a handle and reading each one in (skipping
chunks below a size threshold). Either way it ends up with the main
track payload in a shared pointer global, plus two header words copied
out.

Then it dispatches on backend: if the MIDI bit is set, it parses the
loaded MIDI data (via a helper traced earlier this session) and, on
success, loops up to 256 times priming the MIDI event queue, snapshots
the clock into the same fields the already-named
`Sound_getElapsedPlaybackTime` reads, sets the "playing" bits, and
optionally spins on a ready-handshake pair. Otherwise, it calls a
presumed OPL2/other-backend equivalent preparation step.

Several inner helpers remain unnamed, but this function's own role —
load a `.MUS` track's data and kick off playback on whichever backend
is active — is clear from its structure and already-named callers.
This ties together nearly every sound-engine thread traced this
session: file I/O, the MIDI event-queue/timing cluster, and the
backend-selection bits in the shared sound-state word.

Applied via `apply_renames_gatemain.py`'s hundred-and-twentieth batch.

### `Midi_prepareTrackData` named

Moved to `sub_1F63A` (2 callers) — stores the passed data pointer into
a shared scratch field, returning immediately if it's null. Otherwise
records the two header words the just-named `Sound_loadAndStartTrack`
read from the `.MUS` file (via an unnamed helper), validates the
loaded data (another unnamed helper), and if that succeeds calls two
more unnamed helpers before returning success.

Called from `Sound_loadAndStartTrack` (with the just-loaded MIDI
track's data pointer and the two header words read from the MUS file)
and from `sub_1F7D6` — the MIDI backend's "parse/validate the loaded
track data and prepare it for playback" step.

Applied via `apply_renames_gatemain.py`'s hundred-and-twenty-first batch.

### `Midi_serviceTick` named

Moved to `sub_1F692` (2 callers) — the MIDI backend's per-tick service
routine. If the already-named `Midi_stopTrack`'s busy-loop flag is
set, delegates straight to the already-named `Midi_stopTrackStep` and
returns. Otherwise, if a "playing" flag is clear, no-ops. Otherwise
services each active track via an unnamed helper, then checks another
unnamed condition; if a further flag is set, reconfigures the
MPU-401's active-track set (sending device commands with a computed
track-count bitmask, swapping a pair of per-track arrays, and sending
further commands — the tail of this ~320-byte function wasn't traced
instruction-by-instruction).

Called up to 256 times per call from the already-named
`Sound_loadAndStartTrack` to prime the MIDI event queue, and
repeatedly from `sub_201C0` — consistent with this being the regular
per-frame/per-tick MIDI service routine, not a one-shot setup step.

Applied via `apply_renames_gatemain.py`'s hundred-and-twenty-second batch.

### `Sound_serviceTick` named

Moved to `sub_201C0` (2 callers, one via a data-driven call site) —
the top-level sound-engine tick dispatcher, guarded against
re-entrancy (returns 1 immediately without doing any work if already
running). Otherwise, if a "sound active" bit is clear, skips straight
to cleanup. If set, dispatches by backend: MIDI calls the just-named
`Midi_serviceTick`; OPL2/other calls an unnamed helper (itself calling
the already-named `Opl2_stopTrack` elsewhere, plausibly the OPL2
backend's own per-tick service routine); otherwise defaults to no
activity. Based on that result, sets or clears a bit in the shared
sound-state word (matching the same bit patterns `Opl2_stopTrack`
manipulates at its own end), then releases the re-entrancy guard and
returns the result.

Called from the already-named `room_load` — the shared per-tick
service point for whichever sound backend is currently active,
completing the top-level tick-dispatch picture alongside
`Midi_serviceTick`.

Applied via `apply_renames_gatemain.py`'s hundred-and-twenty-third batch.

### `Mouse_initCursorSurfaces` named

Moved to `sub_24A42` (2 callers) — allocates the two mouse-cursor
image surfaces (`mouse_surface2` and `mouse_surface`, both 24x16) via
the already-named `Image_allocateSurface`, storing each one's
resulting image handle into a scratch global. Then computes the
initial mouse position: x is hardcoded to 319 in video mode 3, or
otherwise centered on screen; y is always centered.

Called twice from the already-named `Mouse_init` — the mouse cursor's
own surface-allocation-plus-initial-centering setup step.

Applied via `apply_renames_gatemain.py`'s hundred-and-twenty-fourth batch.

### `Mouse_pollDriverState` named

Moved to `sub_24FAE` (2 callers) — if the DOS mouse driver is present
(per the already-named `mouseState`), calls `INT 33h AH=3` (the
standard "get mouse position and button status" mouse-driver call),
then halves the returned x position if in video mode 3 (that mode's
coordinate space is twice as wide as the internal one), and passes the
resulting (x, y) to the already-named `Commset_btn_setMouse`.

Called from the already-named `Mouse_pollPosition` and
`get_mouse_buttons` — the shared low-level "read the real DOS mouse
driver and feed its position into the game's own mouse state"
primitive.

Applied via `apply_renames_gatemain.py`'s hundred-and-twenty-fifth batch.

### `Region_setValueAndStyle` named

Moved to `sub_27582` (2 callers) — no-ops if either the window number
or region index is negative. Otherwise looks up the actual region slot
via the already-named `Windows_regionIndexes`, sets the already-named
`Regions_val1` and `Regions_style` arrays at that slot, then calls the
already-named `Region_fill` to redraw it.

Called from the already-named `Listbox_add` and `Listbox_reset` —
sets a listbox item's region value/style (plausibly selected vs.
unselected appearance) and immediately refills it.

Applied via `apply_renames_gatemain.py`'s hundred-and-twenty-sixth batch.

### `ScalePic_scaleCoordinate` named

Moved to `sub_25C52` (2 callers) — if `direction` is -1, returns the
value scaled by 3/4; if `direction` is 1, returns the value scaled by
4/3 (the reciprocal); any other direction returns the value unchanged.

Called from the already-named `scale_pic` (which prints
`" scale_pic : EGA -> VGA disabled "` when refusing to scale) — the
coordinate/dimension scaling primitive behind its EGA↔VGA
picture-scaling conversion.

Applied via `apply_renames_gatemain.py`'s hundred-and-twenty-seventh batch.

### `ScalePic_selectPalette` named

Moved to `sub_25BCE` (2 callers) — gated on a `pic_header._flags` bit.
If `direction` is -1 (the same "scale down 3/4" convention as the
just-named `ScalePic_scaleCoordinate`), fetches a pointer via an
unnamed helper and copies 16 words from it into a fixed buffer. If
`direction` is 1 ("scale up 4/3"), fetches from a different unnamed
helper and copies 48 words instead. Otherwise no-ops.

Called from the already-named `scale_pic` and `load_and_scale_pic` —
plausibly swapping in the color-palette/lookup table appropriate for
the scale direction (e.g. distinct EGA vs. VGA color-mapping data)
before an EGA↔VGA picture scale.

Applied via `apply_renames_gatemain.py`'s hundred-and-twenty-eighth batch.

### `Surface_beginOverlay` named

Moved to `sub_26892` (2 callers) — returns 0 immediately if the given
state struct's active flag is already set. Otherwise allocates a
surface sized to the given rectangle into the struct's embedded image
sub-structure; returns 0 if that fails. On success: saves the struct's
current image pointer into a backup slot so it can be restored later,
stores the new rect, clears some flag bytes and sets the active flag,
hides the mouse, swaps the freshly-allocated surface's own image data
into the struct's current-image slot, draws it over the screen rect,
and shows the mouse again, returning 1.

Called from the already-named `Icon_drawButton` and
`Dialog_showFormattedPrompt` — a reusable "allocate a temporary
drawing surface over a rectangle, saving whatever image was there
before" overlay primitive, presumably paired with a not-yet-identified
restore/end counterpart.

Applied via `apply_renames_gatemain.py`'s hundred-and-twenty-ninth batch.

### `Screen_setDrawMode` named

Moved to `sub_2BE7A` (2 callers) — validates its fill-mode argument is
in range 0-11 (12 raster-op-style modes); if so, stores it and the
given color/flag arguments into the screen struct and returns 0. If
out of range, leaves the screen struct untouched and returns the error
code `0xF05F`.

Called from the already-named `Font_writeChar` and `fillRect` — a
shared "configure the screen's current draw mode/color before writing
pixels" validator+setter.

Applied via `apply_renames_gatemain.py`'s hundred-and-thirtieth batch.

### `Screen_dispatchSpanFill` named

Moved to `sub_2C6D1` (2 callers) — the shared low-level pixel-span
dispatcher behind both the already-named `Screen_drawLine` and
`Screen_fillRect`. Ensures the already-named `Screen_setVTable` has run
once; if a screen-state flag is set, first clips/adjusts the
coordinates via an unnamed helper, bailing out early on error.

Then, based on several screen-state fields (pen color, fill mode, a
line-width-derived value, and a video-mode-indexed function-pointer
table), it tries to jump directly to a specialized fast pixel-writing
routine for the current state/video-mode combination; if no
specialized routine matches, it falls back to a large, presumably
generic, span-filling routine. This mirrors a classic "select the
fastest matching rasterizer, else use the slow generic one"
optimization pattern for horizontal pixel-span writing shared by line
and rectangle drawing.

Applied via `apply_renames_gatemain.py`'s hundred-and-thirty-first batch.

### `Memory_fillBytes` named

Moved to `sub_2137A` (2 callers) — a near-callable "fill a buffer with
a repeated byte" helper: word-aligns the destination (storing one byte
first if the pointer is odd), fills whole words via `rep stosw`, then
any trailing odd byte via `rep stosb`. Similar in spirit to the
already-named `_memset` but not the same implementation (no segment-
wraparound handling for buffers crossing a 64K boundary, and a simpler
alignment check) — a distinct, private near-call helper rather than a
duplicate-compiled copy.

Called from `sub_20D4F` and `sub_2139D`.

Applied via `apply_renames_gatemain.py`'s hundred-and-thirty-second batch.

### `String_copyPadded` named

Moved to `sub_632B9` (2 callers) — copies up to `width` characters
from a source string into a destination buffer; once the source's
null terminator is hit, pads the remainder of the destination with
spaces instead. Always null-terminates the destination after exactly
`width` characters.

Called from the already-named `prompt_for_filename` and from
`sub_632F5` — a fixed-width, space-padded string copy, consistent with
formatting a filename into a fixed-width field (e.g. an 8.3-style
padded name) for display or storage.

Applied via `apply_renames_gatemain.py`'s hundred-and-thirty-third batch.

### `Font_getTabStopDistance` named

Moved to `sub_70F07` (2 callers) — computes the pixel width of a
4-space tab stop via the already-named `Font_stringWidth`, then finds
the given x-position's remainder into that width and subtracts it to
get the distance to the next tab stop. If that distance is smaller
than a single space character's width, adds a full extra tab width
instead, avoiding a visually-too-small jump.

Called from the already-named `Commset_printText` — the tab-expansion
distance calculation for its proportional-width text layout.

Applied via `apply_renames_gatemain.py`'s hundred-and-thirty-fourth batch.

### `prompt_for_line` named

Moved to `sub_2983A` (2 callers) — the generic line-input prompt loop,
called twice from the already-named `prompt_for_filename`. Sets the
initial input line via the already-named `InputArea_setLine`
(optionally seeding it from an existing string), hides the mouse,
flushes pending text, disables the log file, shows the text cursor,
and prints a prompt (either the given text or a default). Then loops
on `get_keypress()`, dispatching Ctrl-C/Escape (cancel), Backspace,
Enter (accept), and printable characters — a full line editor.

This is the shared line-input primitive `prompt_for_filename`
specializes for filenames specifically.

Applied via `apply_renames_gatemain.py`'s hundred-and-thirty-fifth batch.

### `Parser_clearResultStruct` named

Moved to `sub_608AA` (2 callers) — given a 12-byte struct holding two
embedded (flag byte, far pointer) sub-fields, for each sub-field: if
its pointer is non-null and its flag byte is greater than 1, calls the
already-named `kill_pointer_` to release it. Then zeroes the entire
struct via `_memset`.

Called twice from the already-named `Parser_proc4` — a "release any
owned pointers, then clear" cleanup step for a parser-result-shaped
structure with two ref-counted pointer fields.

Applied via `apply_renames_gatemain.py`'s hundred-and-thirty-sixth batch.

### `Screen_fillSpanGeneric` named

Moved to `sub_2C83C` (3 callers) — the generic slow-path rasterizer
the already-named `Screen_dispatchSpanFill` falls back to (via a tail
`jmp`) when no video-mode-specific fast pixel routine matches for the
current state/mode combination.

Confirmed by direct algorithmic reading rather than message decoding
this time: if its first and third args (x1, x2) are equal, it's a
degenerate vertical span — walks y from `min(y1,y2)` to `max(y1,y2)`,
rotating a dash/pattern bitmask one bit each step and calling a
mode-specific plot callback only when the rotated-out bit is set. That
callback is a far function pointer at offset+4 of the same per-video-
mode, 20-byte-stride table entry `Screen_dispatchSpanFill` indexes
into by `videoIndex` — confirming the two functions share that table's
layout.

Otherwise it runs a textbook two-accumulator Bresenham line algorithm
between `(x1,y1)` and `(x2,y2)` — computing `abs(dx)`/`abs(dy)` and
their signs, then stepping the larger-magnitude axis one unit per
iteration while accumulating the smaller axis's error term, again
gating each plotted pixel on the same rotating dash-pattern bit. This
is a genuine, general-purpose Bresenham implementation, not a
special-cased horizontal/vertical-only fill.

Shared by both the already-named `Screen_drawLine` and `Screen_fillRect`
(both call through `Screen_dispatchSpanFill`) as their common
generic-case pixel plotter — a line is drawn directly between its two
endpoints, and a rectangle's edges/fill presumably reduce to the same
primitive when the fast per-mode path doesn't apply.

Applied via `apply_renames_gatemain.py`'s hundred-and-thirty-seventh batch.

### `Opl2_setChannelFrequency`/`Opl2_updateChannelFrequency` named

`sub_1D7DA` (3 callers in the ranked list — actually 2 real callers,
both from the already-named `Opl2_setRhythmMode`) had been sitting in
the ranked list for a while, previously described only as "a per-
channel setter in this same [OPL2] cluster, not yet renamed." Traced
it and its one callee fully this pass.

**`Opl2_setChannelFrequency`** (was `sub_1CB32`) writes OPL2 register
`0xA0+channel` (F-Number low byte) and `0xB0+channel` (KeyOn | Block |
F-Number high 2 bits) via the already-named `Opl2_writeRegister` — the
standard OPL2 per-channel frequency/key-on register pair, unambiguous
from the register offsets alone. It computes the 10-bit F-Number+block
pair from a `note` argument via two lookup tables at fixed data-segment
offsets (not independently decoded), after first scaling `note - 0x2000`
by a "jump into a chain of `add`s" technique (a classic multiply-by-
repeated-addition trick) whose iteration count is driven by
`_opl2MasterVolume` (the already-named, clamped 1-12 global).

That last part is a genuine curiosity worth flagging rather than
quietly acting on: every *other* use of `_opl2MasterVolume` found so
far is a true amplitude/loudness scale, but here it scales a pitch-
related delta feeding into the F-Number lookup, before any volume
register is touched. This doesn't necessarily mean the global is
misnamed — `Opl2_setChannelFrequency` is only ever reached (via
`Opl2_updateChannelFrequency`) for OPL2's two auxiliary rhythm-mode
channels (7 and 8), so it's plausible this is some rhythm-specific
pitch-compensation tied to the volume knob rather than evidence the
global's core meaning is wrong elsewhere. Left as a flagged anomaly,
not a correction, since there isn't enough evidence either way yet.

Also worth noting: the call from `Opl2_updateChannelFrequency` to
`Opl2_setChannelFrequency` is a plain near call with no `push cs`, yet
`Opl2_setChannelFrequency` is declared `proc far` and ends in `retf` —
the same intra-segment far-call mismatch already known from this
project's custom RTLink-flattening tool (see its own section earlier
in this doc), not a real bug in the original code.

**`Opl2_updateChannelFrequency`** (was `sub_1D7DA`) is the actual
`Opl2_setRhythmMode` caller: given a channel number, it reads three
per-channel state tables at fixed offsets (`-0x629C`, `-0x62B0` as a
word array, `-0x62BA`) and passes them to `Opl2_setChannelFrequency`,
then caches the resulting F-Number low byte back into a fourth
per-channel table at `-0x6292`. `Opl2_setRhythmMode` calls it twice
(channels 8 then 7 — OPL2's two auxiliary rhythm voices), each call
preceded by directly poking that channel's `-0x629C`/`-0x62B0` table
entries — which alias to the globals `byte_D1C6E`/`word_D1C80` for
channel 8 and `byte_D1C6D`/`word_D1C7E` for channel 7 — with fixed
values immediately before the call. In other words: this is the
"commit a channel's pending note/frequency settings to OPL2 hardware"
step, not a volume setter as its position in the ranked list (right
next to the OPL2 volume-register cluster) might have suggested.

Applied via `apply_renames_gatemain.py`'s hundred-and-thirty-eighth batch.

### `Logic_heecheetownSpecial` and its patron-departure trio named — a hidden get-drunk minigame

Found by following up `sub_B466E` (3 callers in the ranked list), which
turned out to lead somewhere much more interesting than its own
structure suggested. Its own shape — a couple of hardcoded-logicNum
`j_Logics_updateHandler(logicNum, 0, 0)` detach calls, one conditional
on `Logics_IsPrehandler1`, then a `Queue_remove` — didn't say much on
its own, but its actual caller (found via direct xref, not the ranked
list, since it's reached through 3 near-identical siblings called
together) turned out to be a single, fully-traceable object logic
handler with its own `GATESTR.DAT` messages.

**`Logic_heecheetownSpecial`** (was `sub_B28A8`) is the per-object
logic handler for logic ID `0x13D` (317) — reached via its own thunk
from a data-driven object-dispatch table entry, not an ordinary code
xref, the same shape as object-specific handlers elsewhere in this
codebase. Decoding its own message references nailed the whole scene
down conclusively: this object is a drink called the **"Heecheetown
Special,"** served by a robot bartender at one of the game's bars.

- **EXAMINE** (verb `0x3A`) prints *"The glass contains a locally
  brewed concoction that is known as a 'Heecheetown Special.' It looks
  lethal."*
- Considering throwing it away: *"You think about the mess that you
  would make and decide against it."*
- Taking it from the bartender walks through a small vignette — *"The
  robot bartender emits a happy 'cheep' as you take the drink,"* *"You
  put your finger in the drink. It's very wet,"* *"The distinctive
  reek of alcohol is almost overpowering"* — via `Logics_autoTakeObject`.
- **Drinking it** (verbs `0x30`/`0x36`) prints the full "gulp it down"
  description (*"Your eyes bulge out, the little hairs on your neck
  stand up, and your stomach is coated with a wave of cold fire..."*),
  detaches the glass's own handler, and — if logic `0x100` (confirmed
  as the NPC **Thom**, checked via
  `Logics_prehandlerChainReaches(0x100, _roomLogicNum)`, i.e. "is Thom
  in the current room") — prints Thom's own approving line: *"Thom
  grins at you. 'Excellent drink, hey?'"*

Then it starts an intoxication countdown (`Queue_add`), and on each
subsequent call increments `Persisted_val209`, printing escalating
drunkenness symptoms straight out of `GATESTR.DAT`: *"You feel a
little woozy"* → *"You are having trouble concentrating"* → *"The room
seems to be spinning, and you are having trouble remaining vertical"*
→ *"You decide to lay on the floor and take a nap."* At that final
stage — the player passing out drunk — it calls the three
`Logic_heecheetownSpecial_patronLeavesN` helpers, sets
`Persisted_val216 = 4`, and calls `thunk_sub_A60CE(0)`; `sub_A60CE` is
one of the already-named `Logics_checkMoveRestriction`'s own callers,
consistent with installing a "passed out, can't move" restriction on
the player. A genuine hidden get-drunk minigame.

**`Logic_heecheetownSpecial_patronLeaves1`/`2`/`3`** (were `sub_B466E`/
`sub_B49DE`/`sub_B4DA7`) are three near-identical helpers fired
together at that final stage. Each unconditionally detaches one
specific patron logic (`0x100` confirmed as Thom; `0x10B`/`0x10A` for
the other two, presumably other bar patrons by parallel structure, not
independently confirmed) and a shared logic `0x137` from their handler
chains; each conditionally also detaches logic `0x13E` if its
prehandler chain currently reaches that patron (i.e. `0x13E` happens
to be attached to/carried by them at the time); each finishes by
cancelling that patron's own queued per-turn effect via `Queue_remove`
(`0x1C`/`0x1D`/`0x1E` respectively). Read together: the other patrons
react to the player passing out by leaving/dispersing, and whatever
turn-based behavior they had running stops. `0x137`/`0x13E` and the
exact identity of `0x10A`/`0x10B` weren't independently confirmed
beyond this structural role.

Applied via `apply_renames_gatemain.py`'s hundred-and-thirty-ninth batch.

### `Commset_run`/`Commset_drawScreen`/`Commset_redrawChangedIcons`/`Commset_drawKeycapIndicator` named

Following up `sub_749C9`/`sub_755AF` (3 callers each in the ranked
list, sharing the exact same 3 callers — `sub_73E5A`, `sub_74149`,
`sub_74D38`) led to a small, cleanly-traceable UI subsystem rather than
anything narrative this time.

**`Commset_drawScreen`** (was `sub_73E5A`) creates or reuses (tracked
via `word_CCCE2`) a full content window sized/positioned per video
mode, loads the matching background region (`0xF00`/`0xF01`/`0x3F00`),
draws a 3D-beveled border, sets colors/font — then stores the new
window handle into the **already-named `Commset_winContent`** global,
conclusively tying this whole cluster to the existing `Commset_show`/
`Commset_winContent`/`Commnet_proc1` group (this project already had
two spellings, "Commset" and "Commnet," for the same in-universe
communicator-device UI from earlier sessions — `Commnet_proc1` is
called directly from `Commset_show`, confirming they're the same
subsystem). After drawing the background picture, it calls
`Commset_redrawChangedIcons` and `Commset_drawKeycapIndicator` for
indices 0-3, then a still-untraced `sub_7450E`.

**`Commset_redrawChangedIcons`** (was `sub_749C9`) loops icon index
1-15, comparing a per-icon current-state byte against a per-icon
last-drawn-state table; on a change, redraws that icon via the
already-named `Image_display` (picture number = a fixed base plus the
icon index) — i.e. up to 15 independently-toggleable status icons,
each its own picture resource, redrawn only when changed. Distinct
from the already-named `AnimPics_*` cluster (different tables, no
slot-count/duration bookkeeping — a much simpler "did this one icon's
state change" check).

**`Commset_drawKeycapIndicator`** (was `sub_755AF`) draws one
drop-shadowed hotkey-style character at a fixed, per-index position
(a lookup table gives x/y, adjusted per video mode): a black 'X' first
(plausibly font 6's placeholder/box glyph, used purely to blank the
cell) then the index's actual character on top in a separate
foreground color. The 4 actual characters (from a small per-index
table) weren't independently read out this pass, so which 4 Commset
options these correspond to remains unconfirmed.

**`Commset_run`** (was `sub_74149`) is the actual top-level entry
point — reached only via a data-driven thunk, not an ordinary call
site. It's the modal "bring up one Commset screen" session: stops any
playing track, fades the screen out, clears windows, frees a cached
image, calls `Commset_drawScreen`, shows the mouse, then enters a
`Mouse_pollPosition`-driven loop testing the polled position against
on-screen regions — the standard "modal screen, wait for a click on
one of its regions" pattern seen elsewhere in this UI layer. Its own
callee `sub_74D38` (also one of the 3 shared callers) wasn't traced
this pass.

Applied via `apply_renames_gatemain.py`'s hundred-and-fortieth batch.

### `Sound_reportMusicToggle`/`Sound_reportSoundToggle` named

`sub_6D7A3`/`sub_6D836` (3 callers each, sharing the same
two-per-function pattern of "one thunk-reached RTLink command chunk,
one ordinary near-call sibling"). Quick, clean pair — both just print
a one-line confirmation message via `TextWindow_add` after a sound
setting is toggled.

**`Sound_reportMusicToggle`** (was `sub_6D7A3`) tests `word_C8582`
bits 1-2 and prints `"[Music is on.]"` or `"[Music is off.]"`
(`aMusicIsOS_`, `"[Music is o%s.]\n"`). Called from `sub_6D7CD` right
after `Sound_selectDevice` (a "turn music on and pick a device"
command), and from the RTLink-thunk-reached chunk behind `sub_2ECC5`
right after `Sound_shutdown` (a "turn music off" command).

**`Sound_reportSoundToggle`** (was `sub_6D836`) is the sibling for
sound *effects*: tests `word_C8582` bit 3 and prints `"[Sound is
on.]"`/`"[Sound is off.]"` (`aSoundIsOS_`). Called from the
RTLink-thunk-reached chunks behind `sub_2ECB1` (sets the bit,
reconfigures the device and stream — "sound effects on") and
`sub_2ECA7` (clears it — "sound effects off") — a separate on/off
toggle from the music one, confirming this engine has independent
music and sound-effects mute switches.

Applied via `apply_renames_gatemain.py`'s hundred-and-forty-first batch.

### `Logics_dropIntoPond` named

`sub_7D203` (3 callers). Confirmed conclusively via its own `GATESTR.DAT`
message: `0x4C78`, `"You drop%s and%s to the pond floor.\n"`. The two
`%s` slots are filled by two `j_printObj(logicNum, valType)` calls
(`valType` `0x44` then `2` — two different printed forms of the same
object). After printing, it detaches `logicNum`'s handler and
reassigns it to handler `0xA9` (presumably "the pond floor" acting as
a location) via the already-named `j_Logics_updateHandler`.

A nice bonus tie-in: if `logicNum == 0xA2` specifically, it *also*
detaches/reassigns a companion object `0xBF` to handler `0x94`. `0xA2`
is one of the mount/vehicle-related logic numbers
`Logics_checkMoveRestriction` flagged as unidentified several sessions
ago (alongside `0xA8`/`0x9D`) — this is the first independent
confirmation that `0xA2` really is some kind of vehicle/harness object,
since dropping it into the pond drags a companion accessory down with
it. Called from `sub_7AE5A` and `sub_7D26B` — presumably part of a
swim/dive/fall-in-water puzzle where carried items sink to the pond
floor.

Applied via `apply_renames_gatemain.py`'s hundred-and-forty-second batch.

### `Logics_travelViaTransitDisk` named — Gateway's Heechee teleportation mechanic

`sub_907ED` (3 callers, and itself one of `sub_14A5F`'s callers).
Confirmed via its own `GATESTR.DAT` message (`0x5421`) as the core
function behind Gateway's signature **Heechee transit-disk
teleportation network**: *"...the world around you fades to black,
leaving you in a dark empty space. A moment later, the world fades
back in and you find yourself on a metal disk similar to the one you
just stood on at%s."* — the `%s` filled in via `printObjLower` from
the *departing* room, which the function saves into `Persisted_val1`
right before switching rooms.

`Logics_travelViaTransitDisk(newRoomOrId, printAltMsgFlag)`:

1. Fires the leaving-room hook (`Logic_call(_roomLogicNum,
   action=0xF)`) and snapshots the departing room into
   `Persisted_val1`.
2. Sets `_roomLogicNum` to `newRoomOrId` — or to `unk_C8152` if
   `newRoomOrId` is the special value `0x230`.
3. Reassigns the already-confirmed player logic (`0xD3`) to handler
   `0x235` or `0x236` (two container variants, chosen by whether the
   destination is `0x230`) via the already-named
   `j_Logics_updateHandler`.
4. Optionally prints message `0x61F0` if `printAltMsgFlag` is set —
   not independently decoded this pass.
5. Prints the fade-to-black/transit-disk arrival narration above.
6. Picks a room-description variant (`9`-`0xC`, from `Persisted_val2`
   and a room bit via the already-named `Logics_getBit`) and calls
   `sub_14A5F(_roomLogicNum, variant)` — the same shared room-look
   helper the already-named `Logics_lookAtCurrentRoom` calls.
7. Fires the entering-room hook (`Logic_call(_roomLogicNum,
   action=0xE)`).
8. On first arrival at room `0x232` specifically, awards a one-time
   score bonus via the already-established
   `Logics_getTakeScore`/`Score_add`/`Logics_setTakeScore` pattern — a
   "reached this location" discovery bonus.

A genuinely satisfying confirmation of one of Gateway's most
recognizable mechanics, found purely by following up an ordinary
3-caller ranked-list entry.

Applied via `apply_renames_gatemain.py`'s hundred-and-forty-third batch.

### `Logics_printObjectDescription` named — revisiting a previously-skipped function with better context

`sub_14A5F` was explicitly skipped several sessions ago ("a
generic-looking 'print an object's header, then invoke its own logic
for a given action' dispatcher, but several pieces stayed unclear...
Not renamed"), back when it only had `Logics_checkMoveRestriction` as
a confirmed caller. This session's work on `Logics_lookAtCurrentRoom`
and — just now — `Logics_travelViaTransitDisk` gave it two more
already-named callers, which was enough to resolve it properly.

`Logics_printObjectDescription(index, lookMode)`: prints the object/
room's own name (`j_printObj(index, 145)`); if the player's (the
already-confirmed logic `211`/`0xD3`) current location
(`Logics_getPrehandler(211)`) differs from the room/object being
described, also prints `", on/in <that location's name>"` (the same
on/in preposition convention, via `Logics_getBit(loc, 0x1C)`, as the
already-named `Logics_describeContents`), then always a newline.
Optionally refreshes the displayed picture. Unless `lookMode` is `0xA`
or `0xC`, prints a tab before invoking the object/room's own compiled
logic for this look mode via `Logic_call(index, lookMode)`; if that
returns nothing and `lookMode != 0xC`, falls back to a shared generic
description (`thunk_sub_669E3`, still not traced).

Confirmed as the shared "describe this room/object" backend for all
three known callers: `Logics_checkMoveRestriction` (`lookMode` `9` or
`0xA`, after a normal room-to-room move), `Logics_lookAtCurrentRoom`
(the explicit LOOK command), and `Logics_travelViaTransitDisk`
(`lookMode` `9`-`0xC` after a transit-disk teleport) — the same `9`-
`0xC` "look mode" range showing up in all three confirms this is one
shared brief/full/first-visit-style description dispatcher, not three
independent implementations. A good reminder that a function worth
skipping today may become nameable once its neighbors get named.

Applied via `apply_renames_gatemain.py`'s hundred-and-forty-fourth batch.

### `Logics_stripPlayerItems`/`Logics_restorePlayerItems` named — the save/strip/restore-inventory trio

`sub_15A7A` (3 callers). Detaches every item the player (logic `0xD3`)
currently has: repeatedly queries `Logics_getUnkHandler(0xD3, 1)` (the
"worn" chain, per the already-named `Logics_collectPlayerItemLists`)
and detaches whatever it returns via `j_Logics_updateHandler(logicNum,
0, 0)` until the chain is empty, then does the same for handler index
`0` (the "carried" chain). Named **`Logics_stripPlayerItems`**.

Its caller `sub_159D5` turned out to be the direct **restore**
counterpart, closing out a three-function cluster with the already-
named `Logics_collectPlayerItemLists`. **`Logics_restorePlayerItems`**
is gated on `Persisted_val19`/`Persisted_val20` (both zero = nothing
to restore, no-op — the code that actually *sets* these two flags
nonzero wasn't found this pass, so what triggers a pending restore
isn't independently confirmed). When triggered, it calls
`Logics_stripPlayerItems` to clear whatever the player is currently
holding, then reattaches every entry from two flat, null-terminated
snapshot arrays — at the *exact same offsets* (`-0x7376`, `-0x73B2`)
`Logics_collectPlayerItemLists` populates — as worn (`handlerId=1`)
and carried (`handlerId=0`) items respectively, before zeroing both
flags.

Following `Logics_collectPlayerItemLists`'s own only caller
(`sub_A2D8D`, a TOUCH-verb handler) confirmed the whole trio's
purpose via its own messages: an NPC dresses the player in a fresh
outfit (*"'I'm afraid I must ask that you be suitably dressed.' She
slips you into a blue coverall, exactly like the one you left
behind."*) and leads them away by the hand (*"you are too shocked and
mystified to even think of resisting"*) — right after snapshotting the
player's current items. The three functions together implement
"temporarily take away and re-clothe the player for a scripted scene,
then give everything back afterward" — presumably for a medical-exam-
or quarantine-style sequence.

Applied via `apply_renames_gatemain.py`'s hundred-and-forty-fifth batch.

### `Logics_printRaftStatus` named — a raft-boarding puzzle

`sub_8D517` (3 callers). Confirmed via its own two `GATESTR.DAT`
messages as a raft-boarding status check. Tests four conditions via
the already-named `Logics_IsPrehandler1`: the player (`0xD3`) attached
to logic `0x1FB`, logic `0x1CC` attached to `0x1FB`, logic `0x204`
attached to the player, and logic `0x1FC` attached to `0x1CC`. If all
four hold, it prints *"'All we need to do is push off from the shore,
ensign.'"*; otherwise it prints *"'You and I both need to be on the
raft together. I'll hold the tiller and steer. You'll hold the pail
and bail like crazy.'"*

This nails down `0x1FB` as **the raft** itself, `0x1CC` as an NPC
companion (addressed as "ensign") who steers, and `0x204`/`0x1FC` as
per-person raft gear — plausibly the pail (player's) and tiller
(companion's), though not individually confirmed. None of these
`logicNum`s are independently named yet. Called from `sub_8CB20` and
the RTLink-thunk-reached `sub_8D38C` — the "are both of us aboard the
raft, ready to shove off" announcement.

Applied via `apply_renames_gatemain.py`'s hundred-and-forty-sixth batch.

### `Logics_examinePrayerFan` named — the crystalline Heechee prayer fans

`sub_BF003` (3 callers). A per-object logic handler (`actionType==6`,
the same convention as `Logic_heecheetownSpecial`), shared by three
color-variant objects (`0x285`/`0x286`/`0x287`). Confirmed via its own
`GATESTR.DAT` messages as the artifact behind one of Gateway's classic
collectible puzzles — the **crystalline Heechee "prayer fans."**

For **EXAMINE** (verb `0x3A`): checks whether any of three related
logics (`0x28A`/`0x289`/`0x28B` — plausibly the other unique fans, or
sockets they fit into) is currently attached to this fan; if so,
prints *"The `<color>` prayer fan is in/on `<that item>`."* Otherwise
prints the fan's own appearance: *"The object looks like a delicate
crystalline prayer fan that has been folded shut. It glows with a
pure `<color>` light that seems to come from inside the translucent
crystal."* If the fan isn't yet attached to the player and the current
room is `0x272` (presumably the dome where they're first found) and a
per-fan bit isn't set, it additionally prints a first-discovery-only
line: *"The prayer fan is floating about two meters off the floor of
the dome, rotating slowly in the air."* It finishes by clearing a
shared bit on all three fan objects together.

The three per-color message IDs selecting each fan's actual color word
(`0x8119`/`0x811D`/`0x8122`) didn't decode via `dump_gatestr_messages.py`
— not independently confirmed this pass. Verb `0x90`'s handling (gated
behind two untraced thunks) also wasn't fully resolved. Still, the
core artifact and its floating/glowing presentation are now solidly
confirmed.

Applied via `apply_renames_gatemain.py`'s hundred-and-forty-seventh batch.

### `Logics_insertPrayerFan` named — solving the three-prayer-fan puzzle

`sub_BF410` (3 callers, 1295 bytes — the largest function named this
session). The companion handler to `Logics_examinePrayerFan`, for
logic IDs `0x289`/`0x28A`/`0x28B` this time. Confirmed via its own
messages as **the resolution mechanic for Gateway's classic
three-prayer-fan puzzle**, and a genuinely satisfying one to nail down
end to end.

The device itself is described (shared message `0x282D`): *"There are
three colored slots on top of the Heechee device, and below each of
them is an intense light. Under the red slot is a purple light, under
the blue slot is a green light, and under the yellow slot is an
orange light."* — a deliberately **cross-matched** color puzzle (the
fan's own color doesn't match the slot it belongs in).

On a correct insertion, it prints *"You insert `<fan>` into `<slot>`.
The `<color>` light flares brilliantly. You hear a mechanical click,
followed by a whining sound, and then the fan slowly descends into the
machine..."*, plays two loaded animation resources, and awards 5
points. It then checks whether **all three** fan/slot pairs are
correctly filled — confirmed as `0x289`↔`0x286`, `0x28B`↔`0x285`,
`0x28A`↔`0x287`, the actual (non-obvious) correct cross-pairing. If
so, it prints the puzzle's payoff: *"The machine suddenly detaches
itself from the platform, levitates a few centimeters in the air, and
begins spinning around all of its axes. You pluck the machine out of
the air."* — reassigns the whole device (logic `0x288`) to the player,
awards a 25-point bonus, and plays a victory music track via
`Sound_selectTrack`.

Not every branch was traced instruction-by-instruction (a
`thunk_sub_67867` gate taking several vocab-ID arguments wasn't
independently decoded), but the puzzle's shape — insert three fans
into their cross-matched slots, watch the light show, then get
rewarded with a portable device and a real score bonus — is now fully
confirmed by direct message evidence.

Applied via `apply_renames_gatemain.py`'s hundred-and-forty-eighth batch.

### `Logics_walkCatwalk`/`Logics_relocateCatwalkContents` named — closing out an earlier deferred function

`sub_9E7EF` was read in full earlier this session but left unrenamed —
its structure (evacuating/restoring a linked-list chain off logic
`0x28` via scratch-indexed slots) was clear, but the *why* wasn't.
Following up its caller `sub_9C347` (3 callers) resolved both at once.

**`Logics_walkCatwalk`** (was `sub_9C347`) is confirmed via its own
messages as the movement handler for a **perilous cliffside catwalk
leading to a glowing portal**. For `actionOrCode==0xF` (the same
"leaving this location" hook seen in `Logics_checkMoveRestriction`/
`Logics_travelViaTransitDisk`), it checks `Parser_val21` (the
confirmed "direction just attempted" global) and `Persisted_val96`
(the player's position index along the catwalk, `0`-`5`): at position
`5`, stepping further either steps off the catwalk into the portal
(*"You step off the perilous catwalk and into the glowing portal"*) or
finds it closed (*"You cannot step through portal. It is closed."*),
depending on a bit on logic `0x32`. For other action/code values, at
either boundary it instead returns a sentinel char (`'!'`/`'#'`) —
matching the "exit blocked" return convention already documented for
`Logics_tryMoveDirection`'s other exit-type handlers. In the middle of
the catwalk, it steps the player one position via
`Logics_relocateCatwalkContents`, printing *"You take a cautious step,
inching along the cliff walkway"* going forward or *"You turn around,
and take a hesitant step back the way you came"* going back, then
describes newly-visible scenery and contents.

**`Logics_relocateCatwalkContents`** (was `sub_9E7EF`) is the
primitive underneath: given an old and new position along the
catwalk, it detaches the player from logic `0x28` (the catwalk
itself), caches whatever chain of items was left at the old position
into a scratch table (keyed by position), then restores whatever chain
was previously cached at the new position — before reattaching the
player. Net effect: **each spot along the catwalk remembers what you
dropped there**, restored exactly when you walk back to it — a neat
position-based inventory-caching mechanic for a linear, one-step-at-a-
time location.

Applied via `apply_renames_gatemain.py`'s hundred-and-forty-ninth batch.

### `Logics_printBeckerJudgment` named — a whole-playthrough morality callback

`sub_85100` (3 callers, 614 bytes). Confirmed via an unusually rich
set of `GATESTR.DAT` messages as a beloved Gateway mechanic: the NPC
**Becker's cumulative judgment of the player**, built from many small
choices made throughout the entire game.

Opens with *"'Ok. Let's see,' Becker muses, scratching his head."*
(or a second variant, or just a continuing quote mark depending on
`arg_0`), then always: *"'You can tell me all you want about yourself
but I've formed my own opinion of you.'"* It then walks roughly nine
past-choice categories, printing either the "good" or "bad" version of
each depending on tracked game state, and incrementing or decrementing
a running tally per clause:

- Greeted Becker courteously, vs. didn't shake his hand
- Avoided killing "the harmless `<creature>`", vs. unnecessarily
  killed it
- Protected another creature's life, vs. unthinkingly slaughtered it
- Didn't hurt his pet "Mr. Pookie", vs. chased him away or murdered him
- Cleverly disassembled his ship's actuator, vs. needlessly dug up a
  grave
- Got his lens without harming a tree, vs. cut the tree down (its
  extinction!)
- Helped him get his cane/walking stick, vs. refused
- Let him read the player's magazine, vs. refused — paired with an
  inverse clause about reading Becker's own private journal

Individual clauses weren't each traced down to their exact triggering
bit/prehandler check this pass (there are roughly nine of them, and
the function also touches the already-flagged `sub_4A722` — part of
the previously-declined `sub_4A69F` cluster). The function finishes by
picking a closing remark from a small table indexed by the tally,
storing the tally into `word_CD600`, and — specifically when called
with `arg_0 == -1` while in room `0x1BE` — delivering a final verdict
("did well" vs. "did poorly") based on the tally's sign.

A genuinely charming confirmation of just how much of Gateway's small
"be a decent person to the environment and its inhabitants" choices
get remembered and recited back at the player later.

Applied via `apply_renames_gatemain.py`'s hundred-and-fiftieth batch.

### `Opl2_serviceTick`/`Opl2_updateGlideStep` named — closing out the OPL2 tick-service side

`sub_1E950` (2 callers) is confirmed directly from its only caller,
the already-named `Sound_serviceTick`, as the OPL2 backend's per-tick
service routine — the exact counterpart to the already-named
`Midi_serviceTick` on the MIDI side. If `byte_D20A2` ("OPL2 currently
playing") is clear, it calls the already-named `Opl2_stopTrack` and
returns 0; otherwise, if a glide is in progress, it calls
**`Opl2_updateGlideStep`** (was `sub_1E9F4`) and returns 1.

`Opl2_updateGlideStep` is a **software pitch-glide/portamento
stepper** — OPL2 hardware has no native glide support, so this
interpolates a value (`word_C857C`) toward a target (`word_C858C`) one
step at a time in software, timed via 32-bit tick math (a duration
computed once via a 32-bit division, `__aFuldiv`, of the total delta
by a fixed constant). Each tick it checks whether enough real time has
elapsed since the glide started; if so, it nudges the value one unit
closer to the target (incrementing or decrementing depending on
direction), clearing the glide state entirely once the target is
reached. Called once per tick from `Opl2_serviceTick`, and from
`sub_1E7D4` (unnamed, reached via a data table entry — presumably
where a glide first gets kicked off).

With this, the OPL2 backend's tick-service side lines up cleanly with
the already-documented MIDI tick-service side from earlier this
session.

Applied via `apply_renames_gatemain.py`'s hundred-and-fifty-first batch.
