# AGS Archaeology: Rob Blanc 1 Reconstruction

## Goal

Reconstruct usable C source for **Rob Blanc 1**, an early-2000s commercial
Adventure Game Studio (AGS) title, starting from an IDA Pro disassembly and
using the earliest *publicly available* AGS engine source as a reference.
The ultimate target is a **ScummVM engine reimplementation** — this shapes
scope: game-specific/AGS-engine logic (opcodes, structs, script VM, room/
GUI/character behavior) is the real prize, since ScummVM needs an accurate
reimplementation of *that*. Third-party library code (Allegro, JGMOD,
ALMP3, etc.) is a different matter — see "Third-party library scope" below.

Two things live side by side in this repo and must not be confused:

- **`Engine/`, `Common/`** — the reference source: AGS engine **3.2.1.1115**,
  released 2011 (see `Engine/version.rc`). This is the earliest AGS source
  Chris Jones ever published. It is **~9 years newer** than the Rob Blanc 1
  binary and will have diverged significantly (renamed functions, new/removed
  features, restructured opcodes, etc.). Treat it as a strong reference for
  *style, structure, and probable intent* — never assume 1:1 identical
  implementation without checking the disassembly.
- **`rob_blanc_1.asm`** — an IDA-exported disassembly listing of the actual
  Rob Blanc 1 Windows executable (`C:\games\ags\robblanc1_win\rb.exe`,
  linked 2002-07-21, PE32, links DDRAW/DINPUT/DSOUND/WINMM — an
  Allegro-on-Windows build of the old AGS Windows engine). **917k lines,
  ~28MB.** Never `Read` this file in full — use `Grep`/the scripts in
  `reversing/scripts/` instead.

A fourth, narrower reference also lives here: `Engine/libsrc/allegro-4.2.2/`
is the **complete upstream Allegro 4.2.2 source** (added for third-party-
library identification — see below), distinct from the pre-existing
`Engine/libsrc/allegro-4.2.2-agspatch/`, which only ever contained AGS's
*patched* Windows-specific files. The two trees have overlapping filenames
(`win/wwnd.c`, `win/wmouse.c`, etc.) with **different content** — always
check which one a lead is actually pointing at, and be aware the 2002
binary almost certainly linked a genuinely older Allegro version (4.2.2
wasn't released until ~2005) than either of these trees, so treat this the
same way as the `Engine`/`Common` 2011 reference: strong structural guide,
verify against the disassembly, don't assume 1:1.

A fifth reference, `ags-archives/`, holds the OFFICIAL AGS release
archives for versions 2.00 through 3.30+ (docs-only through ~2.7x: each
version's `CHANGES.TXT`/`TECHINFO.TXT`, no source). Unlike the 2011
`Engine`/`Common` reference, this is CONTEMPORARY with Rob Blanc 1's own
era, and has let this project pin the binary's actual engine version
tightly: **AGS 2.4b, July 2002** (see "Rob Blanc 1's AGS version" below
and `reversing/notes/ags-archives-cross-reference.md` for the complete
evidence and usage guidance). Check this archive's `CHANGES.TXT` files
before assuming a "confirmed absent" feature needs fresh disassembly
work to date -- it will often say, in plain English, exactly which
version added/removed it.

### Rob Blanc 1's AGS version: 2.4b, July 2002

Cross-referencing this project's own already-confirmed findings against
`ags-archives/*/docs/CHANGES.TXT`'s dated version entries pins Rob Blanc
1 tightly: `>= 2.4b` (its `IsMusicVoxAvailable` function, added in that
exact version) and `< 2.5` (three independent findings -- `MYOGG`/
`MYSTATICOGG` confirmed absent, `MAXTOPICOPTIONS=15` not 2011's 30, and
`DCMD_SETGLOBALINT` confirmed absent from the dialog-script interpreter
-- all match features 2.5, released September 2002, introduces). AGS
2.4b was released in the same month as the binary's own 2002-07-21 link
date. Treat this as the working ground-truth version for "was this
feature present" questions -- see the dedicated notes file for the
complete evidence table and how to extend it.

### Important correction (do not assume PDB-level ground truth)

The disassembly is **not** annotated from a recovered PDB. Of its ~2582
functions, 535 already carry real names; those names come from two sources
only:

1. **FLIRT signature matches** for statically-linked library code (Allegro,
   the CRT, alfont/FreeType, etc.) — generally reliable for what it claims,
   but it only identifies *library* internals, not game-specific/AGS
   engine code.
2. **Prior manual matching work** (done by the project owner, in earlier
   sessions, before this repo's AI-assisted phase) — a reasonable starting
   point, but not infallible. Verify, don't just trust, when it matters.

The other ~2047 functions are unnamed (`sub_XXXXXXXX`) and are the real
target of the identification work.

## The three aims (in progress, ongoing across sessions)

1. **Identify commonalities** between `rob_blanc_1.asm` and the reference
   source (`Common/`, `Engine/`) — matching global strings, function names,
   structure layouts, opcode tables, etc. — and use that to name/annotate
   more of the disassembly.
2. **Keep the IDB in sync** via re-runnable IDAPython scripts under
   `reversing/scripts/` as more matches are confirmed, rather than doing
   one-off manual renames that get lost.
3. **Prepare for later C reconstruction.** Anything applied to the IDB now
   (function prototypes matching the reference signatures, struct layouts,
   comments citing exact `source_file`/evidence) should make the eventual
   Hex-Rays/manual decompilation pass read as close to the original C as
   possible. When adding matches, prefer also recording the reference
   function's signature/struct layout, not just its name. The eventual
   consumer of this reconstruction is a **ScummVM engine reimplementation**
   — see "Third-party library scope" immediately below for how that
   narrows aim #1's effort allocation.

## Third-party library scope (read before investigating any library function)

Once a function (or global) is confirmed to belong to a statically-linked
third-party library (Allegro, JGMOD, ALMP3, libcda, etc. — see
`reversing/notes/third-party-library-identification.md`), **do not chase
that library's own internal helper functions** — the ones called only by
other functions *within the same library*, never directly by AGS/game
code. Reason: a ScummVM reimplementation replaces the ENTIRE library with
ScummVM's own equivalent (or a modern port), wholesale — the original
library's internal implementation details (how `load_mod` parses JGMOD's
on-disk format internally, how `almp3`'s decoder buffers frames, Allegro's
own internal blitting helpers, etc.) are simply discarded, not ported.
They cost real investigation effort (large, string-poor, no local source
to check names against) for zero eventual payoff.

What's still worth identifying, once a call site is confirmed to be a
third-party API:
- The **public API surface** actually called FROM AGS/game code (e.g.
  `load_mod`, `play_mod`, `stop_midi`) — this tells us what the
  ScummVM-side replacement needs to provide/emulate, and confirms which
  library is linked at all (useful for Task #10's own inventory).
- **Struct fields on the CALLING side** (AGS's own globals/structs that
  hold the library's return values, handles, or parameters) — e.g.
  `GameState.music_repeat`, `RoomStruct.ebscene[]` — these are AGS's own
  data, not the library's, and matter for the reconstruction regardless.

What to stop doing the moment a function is confirmed library-internal:
walking further into its own callees, decoding its internal control flow,
or trying to name it against upstream source purely for IDB completeness.
Leave it unnamed (or leave IDA's own FLIRT-assigned name alone) and move
on — record in `matches.json`/notes only the boundary fact ("this and
everything it calls is `library X`'s own internals, not chased further"),
not a blow-by-blow account of what's inside.

## Repo layout for this project

```
rob_blanc_1.asm                  - the disassembly (huge, script-only access)
Common/, Engine/                 - reference source, AGS 3.2.1.1115
ags-archives/                    - official AGS release archives, versions 2.00
                                    through 3.30+ (docs-only through ~2.7x: each
                                    version's CHANGES.TXT/TECHINFO.TXT, no source
                                    code). Added mid-project by the owner.
                                    CONTEMPORARY with Rob Blanc 1's own era, unlike
                                    the 2011 Engine/Common reference -- used to pin
                                    the binary's actual engine version to AGS 2.4b
                                    (July 2002). See "Rob Blanc 1's AGS version"
                                    above and reversing/notes/ags-archives-cross-
                                    reference.md for the complete evidence and
                                    how to use CHANGES.TXT to date a feature.
Engine/acwin___Win32_DebugWorking/acwin.map
                                  - linker map from a LOCAL build of the
                                    reference source (built by the project
                                    owner in modern VS, 2024). NOT Rob Blanc 1
                                    itself -- but gives exact symbol name ->
                                    source .obj/file for the reference build,
                                    which is a much stronger cross-reference
                                    signal than string grepping. Keep this
                                    file; reversing/scripts/parse_refmap.py
                                    depends on it. (Everything else under
                                    Engine/.vs, Engine/Backup,
                                    Engine/UpgradeLog.htm, acwin.vcxproj* is
                                    unrelated VS-upgrade-wizard clutter from
                                    getting the reference solution to build
                                    locally -- safe to ignore.)
reversing/
  scripts/
    extract_asm.py                - parses rob_blanc_1.asm -> functions.json,
                                     strings.json. Re-run after re-exporting
                                     the .asm from IDA (e.g. after applying
                                     new names).
    parse_refmap.py                - parses acwin.map -> refmap_symbols.json
                                     (symbol name -> {objfile, addr}) for the
                                     reference build.
    cross_reference.py             - matches extracted string literals against
                                     literal C strings in Common/+Engine/ source
                                     -> string_matches.json. Good for finding
                                     which sub_XXXXXXXX are worth investigating
                                     next (a string used by that function that
                                     only appears in one source file is a
                                     strong lead).
    build_matches.py               - auto-generates matches.json by cross-
                                     referencing already-named asm functions
                                     against refmap_symbols.json (exact linker-
                                     symbol name match => confirmed source
                                     file/obj). High confidence, fully
                                     mechanical.
    apply_matches.py               - IDAPython script. Run INSIDE IDA (Alt-F7)
                                     with the Rob Blanc 1 IDB open. Reads
                                     matches.json and renames/comments the live
                                     IDB accordingly. Idempotent -- safe to
                                     re-run after matches.json grows. Also
                                     auto-resolves the recurring name-collision
                                     pattern where a script-export string
                                     literal already owns the name a function
                                     needs (bumps the string to s_<name>).
    apply_all_and_export.py        - IDAPython driver script. Chains
                                     apply_matches.py + apply_structs.py, saves
                                     the database, then re-exports both
                                     rob_blanc_1.asm and rob_blanc_1.idc --
                                     runnable headlessly via IDA's own batch
                                     mode (idat.exe -A -S, no GUI/Alt-F7
                                     needed), from the repo root:
                                       idat.exe -A
                                         -S"reversing/scripts/apply_all_and_export.py"
                                         -L"reversing/scripts/logs/apply_all.log"
                                         rob_blanc_1.idb
                                     Idempotent; safe to re-run any time
                                     matches.json/apply_structs.py grow. NOTE:
                                     apply_matches.py's own name lookup only
                                     matches the LITERAL current asm_name (or
                                     new_name as a fallback) -- if the IDB
                                     already carries a DIFFERENT stale name at
                                     that address (e.g. from a since-corrected
                                     match, before this script existed to keep
                                     things in sync), the entry is silently
                                     skipped rather than fixed. Check the log's
                                     "SKIP (name not found in IDB)" lines after
                                     each run; a genuine stale-name mismatch
                                     needs a one-off `idc.set_name(0x<addr>,
                                     "<name>", ...)` fix (the sub_XXXXXXXX
                                     asm_name conveniently IS the address in
                                     hex) -- a couple of long-standing cases
                                     from earlier sessions (run_dialog_request
                                     ->run_dialog_script, stop_fast_forwarding
                                     ->remove_screen_overlay) were fixed this
                                     way the first time this script ran.
    extract_prototypes.py          - aim #3 tooling. Pulls C/C++ signatures out
                                     of the reference source for every high-
                                     confidence matched function ->
                                     prototypes.json. Handles multi-line
                                     signatures, strips default-value args
                                     (IDA doesn't want them), and adds the
                                     implicit `this` + __thiscall for the
                                     project's flat C++ member-function names
                                     (see FLAT_CPP_NAMES in the script -- an
                                     explicit map, not a generic "__" heuristic,
                                     since real identifiers like
                                     __GetLocationType/__find_route also
                                     contain "__" without being ours).
    apply_prototypes.py            - IDAPython script, run AFTER apply_matches.py.
                                     Applies prototypes.json via idc.SetType().
                                     Most AGS-specific pointer types
                                     (CharacterInfo*, ccInstance*, block, ...)
                                     will fail to resolve until the
                                     corresponding structs exist in the IDB's
                                     local type library -- that's expected, not
                                     a bug. Failures get deferred into a
                                     comment (tagged [reversing-proto]) instead
                                     of silently dropped, and re-running later
                                     (once structs are defined) picks them up.
                                     Struct definition is a separate,
                                     not-yet-built phase of this project.
    find_struct_accesses.py        - struct-offset recovery helper. Given a
                                     global pointer name (e.g. "playerchar"),
                                     scans rob_blanc_1.asm for every
                                     `mov reg, <global>` followed by a
                                     `[reg+NNh]` dereference, groups hits by
                                     offset, and lists which (already-matched)
                                     functions touch each one -- so the
                                     busiest offsets can be tackled first.
                                     Does not guess field names; that's a
                                     human/Claude judgment call reading each
                                     access site's context against the
                                     matching source function's behavior.
    apply_structs.py               - IDAPython script. Applies ONLY struct/
                                     type declarations independently verified
                                     safe against this specific 2002 binary --
                                     see reversing/notes/struct-layout-drift.md.
                                     Struct layouts drift between 2002 and 2011
                                     just as much as function boundaries do
                                     (confirmed for CharacterInfo, SpriteCache,
                                     ccInstance, GameSetupStructBase -- sizes
                                     are drastically different). Never generate
                                     an IDA struct definition straight from the
                                     2011 source without checking it against an
                                     existing IDB struct size or a malloc/`push
                                     <size>` allocation site in the disassembly
                                     first.
    count_data_offsets.py          - mechanical byte-offset calculator for a
                                     slice of rob_blanc_1.asm's .data section
                                     (parses db/dw/dd, dup() counts, align
                                     directives; extract the target range
                                     first with e.g. `sed -n '<start>,<end>p'
                                     rob_blanc_1.asm > region.txt`, too big to
                                     load whole). Used to prove
                                     GameSetupStructBase's global instance is
                                     actually OriGameSetupStruct
                                     (Common/acroom.h:2769) -- computed offsets
                                     landed exactly on every previously-
                                     confirmed field anchor with zero
                                     deviation before being trusted for new
                                     fields. Reports any unparsed line
                                     explicitly (should be zero for a clean
                                     .data region) rather than silently
                                     miscounting -- don't trust offsets past
                                     an unhandled line.
  analysis/                        - generated JSON artifacts (regeneratable,
                                     but keep committed since they're
                                     expensive to rebuild and are the working
                                     dataset for cross-session continuity):
      functions.json, strings.json, refmap_symbols.json,
      string_matches.json, matches.json
  notes/                           - free-form per-subsystem investigation
                                     notes (dialog system, script VM, room
                                     format, etc.) as they're written up.
```

## Workflow for extending the identification work

1. If `rob_blanc_1.asm` was re-exported from IDA (new names applied since
   last time), re-run `extract_asm.py` first.
2. Run `cross_reference.py` to refresh string-based leads.
3. Investigate leads: for a `sub_XXXXXXXX` of interest, find strings/global
   data it references in the `.asm` (grep `CODE XREF:` / `DATA XREF:` near
   its `proc near`/`endp` bounds), check `string_matches.json` for those
   strings, and read the candidate source function(s) to confirm behavior
   actually matches (arg count, control flow shape, order of calls).
4. Record confirmed matches as entries in `reversing/analysis/matches.json`
   (append manually, `"kind": "manual"` for hand-found ones, with
   `asm_name`/`new_name`/`source_file`/`confidence`/`evidence` filled in) —
   or re-run `build_matches.py` to regenerate the mechanical subset without
   clobbering hand-added entries (it currently only emits `"kind":
   "function"` mechanical matches; keep manual entries in a separate pass or
   merge carefully — check the script before assuming it's non-destructive
   if you extend it).
5. Open the IDB in IDA, run `apply_matches.py` to push renames/comments in
   (and `apply_structs.py` for struct/type declarations) — or run
   `apply_all_and_export.py` via `idat.exe -A -S` for a headless one-shot
   that does both AND re-exports steps 6 below, no GUI needed.
6. Re-export `rob_blanc_1.asm`/`rob_blanc_1.idc` from IDA when it's useful
   to snapshot progress (done automatically by `apply_all_and_export.py`),
   and go back to step 1.

## Current snapshot (as of this writing — regenerate via the scripts above
rather than trusting these numbers as they age)

- 2587 functions total (grew from 2582 after several libcda functions
  received IDA function boundaries mid-session): 690 named, 1897 unnamed
  (`sub_*`) as of the last IDB re-export (started this project at 535
  named). `matches.json` has since grown to 511 entries — fully applied/
  in sync with the last re-export as of this writing.
- 2727 string literals extracted from `.data`/`.rdata`; 1096 of those
  matched verbatim into source (819 to a single file) after the full
  Allegro 4.2.2 tree was added under `Engine/libsrc/allegro-4.2.2/` (was
  944/776 before that addition) — this pool was "largely exhausted" for
  Engine/Common code specifically; a productive third-party-library round
  (Task #10, now paused — see below) pushed it further before wrapping up.
- `reversing/analysis/matches.json` has 589 entries (function + struct-field
  matches combined)
- 25 struct definitions built entirely from disassembly evidence (not
  borrowed from the 2011 source — see `reversing/notes/struct-layout-drift.md`):
  `MouseCursor` (`game.mcurs[]`, found incidentally while investigating
  `GameSetupStructBase`'s `hotdot`/`hotdotouter` — a rare full match to
  2011's layout with zero drift in every field: `pic`, `hotx`, `hoty`,
  `view`, `flags` all independently confirmed via already-matched
  functions, only `name[10]` unconfirmed by direct evidence though boxed
  in with zero slack; stride confirmed at 0x18/24 bytes), `ExecutingScript`
  (the `scripts[]` call-stack array — now FULLY mapped, zero unaccounted
  bytes from `+0x00` to `+0x6C`/108 bytes total, vs. 2011's ~725-byte
  struct: `inst`/`forked` confirmed as the first/last fields via
  `post_script_cleanup`'s `rep movsd` bulk-copy stride; the middle section
  cracked by decoding IDA's own pre-existing local-variable names for the
  bulk-copy buffer (`newnum`, `ooo`, `dlgnum` — genuine hints, not generic
  `var_NN` names) and confirming each against usage — `newnum`@+0x04
  (pending new-room number) → `new_room()`, an unnamed flag@+0x08 →
  `__actual_invscreen()`, `ooo`@+0x0C (pending restore-game slot, with
  sentinel 1000 meaning "show dialog") → `restore_game_data()`/
  `RestoreGameDialog`, `dlgnum`@+0x10 → `do_conversation()`, plus the
  already-confirmed `run_another` chain (`script_run_another[2][30]`,
  `run_another_p1[2]`, `run_another_p2[2]`, `numanother` — DRIFT:
  capacity 2 here vs. 2011's declared `MAX_QUEUED_SCRIPTS=4`) and a final
  unnamed flag@+0x64 → `RestartGame()`; cross-confirmed end to end by a
  newly-identified `ExecutingScript::init()` constructor-equivalent that
  zero/(-1)-initializes exactly these 8 offsets in this exact order.
  Architectural finding: 2002 gives 5 of 2011's 9 `PostScriptAction` enum
  cases their own dedicated field, where 2011 later unified all 9 into one
  generic `postScriptActions[]` queue array — a later addition, not a
  reduced version of something already present), plus
  `GUIMain`, `CharacterInfo`, `ccInstance`, `ccScript`, `GUIButton`,
  `GUITextBox`, `GUILabel`, `GUIListBox`, `GUIInv`, `GUISlider`,
  `SpriteCache` (this one was a pleasant surprise — already fully
  field-recovered directly in the live IDB from before this project's own
  tracking began, just never pulled into `apply_structs.py`; now
  formalized), and `GameSetupStructBase` — **FULLY MAPPED**: every
  byte from `+0x00` through `+0xBF84` (49028 bytes total) is now
  accounted for, the largest and longest-running single-struct effort
  in this project (2011's own `GameSetupStructBase` is only ~3900
  bytes; this build's version, over 12x bigger, matches a much older,
  flatter ancestor layout). 37 fields confirmed: `gamename`, `options`,
  `paluses`, `defpal[256]`, `iface[10]`, `numiface`, `numviews`,
  `mcurs[10]`, `globalscript`, `numcharacters`, `chars`,
  `__charcond[50]`, `__invcond[100]`, `compiled_script`,
  `playercharacter`, `totalscore`, `numinvitems`, `invinfo[100]`,
  `numdialog`, `numdlgmessage`, `numfonts`, `color_depth`,
  `target_win`, `dialog_bullet`, `hotdot`, `hotdotouter`, `uniqueid`,
  `reserved[2]`, `numlang`, `langcodes[5][3]`, `messages[500]`,
  `fontflags[10]`, `fontoutline[10]`, `numgui`, `dict`, `reserved2[8]`,
  `spriteflags[6000]` — plus 4 fields independently CONFIRMED ABSENT
  (`numcursors`, `default_lipsync_frame`, `invhotdotsprite`,
  `default_resolution` — genuine later AGS additions this 2002 build
  predates, not merely unfound).
  MAJOR FINDING: this global's true identity is
  `OriGameSetupStruct` (`Common/acroom.h:2769`) — AGS's own OLDEST
  ancestor struct in its save-compatibility evolution chain
  (`OriGameSetupStruct` → `OriGameSetupStruct2` → `OldGameSetupStruct` →
  ... → `GameSetupStructBase`), preserved read-only in the 2011 header
  via `ConvertOldGameStruct` (`acroom.h:3017`) purely for old-save
  upgrading — this retroactively explains nearly all of the "drastic
  drift" found in earlier rounds (byte-sized `options[20]`,
  `gamename[30]`, the field-order divergence) as one single fact rather
  than scattered coincidences.
  `spriteflags[6000]` (the LAST remaining gap, resolved this round) is
  the one field that belongs to the further-derived 2011
  `GameSetupStruct` rather than `OriGameSetupStruct`/
  `OriGameSetupStruct2` — confirmed via a direct, literal `cmp
  index,1770h` (6000) sanity-clamp bounds check in
  `prepare_characters_for_drawing` (already matched), landing EXACTLY
  on this struct's own confirmed total size with zero slack (`MAX_
  SPRITES=6000` here, not 2011's declared 30000 — a 5x reduction). The
  32-byte gap right before it turned out to be `OriGameSetupStruct2`'s
  own declared `reserved2[8]`, completing that struct's ENTIRE field
  list with zero remaining gaps.
  See `reversing/notes/struct-layout-drift.md` for the complete
  round-by-round history of how this struct was cracked open across
  dozens of rounds — a genuine mid-investigation retraction (`defpal`)
  and a real tooling bug found and fixed (the `align`-rounding OFFSET
  CORRECTION). Nothing further remains to recover in this struct's own
  layout.
  Along the way, this struct's investigation also turned up
  `DialogTopic` (tied to `numdialog`, which lives here) — a genuinely
  NEW struct with NO 2011 ancestor declaration to lean on at
  all, unlike everything else in this project. EVERY field eventually
  confirmed via independent access sites — `optionflags[15]` (the
  element count 15, not 2011's declared 30, confirmed via a LITERAL
  `0x0F` constant appearing in both `SaveGameSlot`'s save-game writer
  and `restore_game_data`'s matching reader), `optionscripts`,
  `entrypoints[15]`, `startupentrypoint`, `codesize`, `numoptions`
  (ending EXACTLY at the struct's own confirmed 1156-byte total,
  proving 2011's `topicFlags` field is ABSENT — no room left for it),
  and finally `optionnames[15][0x46]` — the one field left open at the
  end of the previous round, resolved by following `do_conversation`'s
  own option-display code directly (a name-based search for
  `get_translation` had come up empty — its distinctive error string
  isn't in this binary at all) and finding `"imul eax,46h; ...call
  GetTranslation"`, an ALREADY-correctly-named function matching 2011's
  `get_translation` almost exactly. `0x46`(70) is the per-option text
  length — `15*0x46=0x41A` lands with a 2-byte alignment pad exactly on
  the already-confirmed `optionflags`, zero slack. `MAXTOPICOPTIONS=15`
  is now confirmed three independent ways for this struct. A struct
  that started this round with zero reference material ended up as
  complete as `MouseCursor`/`InventoryItemInfo`'s best cases, just via
  real access-site evidence for every field instead of a still-existing
  2011 declaration to check against.
  **`MoveList` (`mls[]`) recovered next**, after `InterfaceElement` (its
  ~13 unconfirmed fields) hit a wall: no already-matched function
  touches any of them, the whole legacy "interface" subsystem is barely
  referenced in the compiled game logic traced so far. `MoveList` paid
  off immediately instead — the live IDB already had a PARTIAL type
  applied to `mls[]` (only `pos`/`numstage` named, from before this
  project's own tracking began); every other field was still raw-offset
  arithmetic. `find_route` (already matched) confirmed 9 of its 12
  fields at once — `pos[40]`/`numstage` (via a direct `memcpy` sized to
  the route's stage count) plus a "start of move" reset block
  (`fromx`/`fromy`/`onstage`/`onpart`/`lastx`/`lasty`/`doneflag`), every
  one at its EXACT 2011-declared offset, zero drift — and
  `walk_character` independently confirmed the array's `0x200`(512)
  -byte stride via a `shl eax,9` scaling. `MoveList` joins `MouseCursor`
  /`InventoryItemInfo` as one of the cleanest zero-drift matches in this
  project.
  **`ViewStruct272` (the `views[]` animation data) recovered next** —
  `dword_52313C` confirmed as the `views` global (a genuine standalone
  pointer, not embedded in `GameSetupStructBase`, mirroring the
  `dialog`/`DialogTopic` pattern) via `load_ac2game_dta`'s
  `malloc`/`fread` sized to `numviews*0x8D4`(2260 bytes/view). Reading
  60+ usage sites of that global turned up an "over-determined fit"
  across three independent strides that all reconcile with zero slack
  at once: total view size `0x8D4`, per-loop block size `0x118`(280
  bytes, found in three separate places — an unmatched helper
  `sub_40C3E0` first, then corroborated twice more inside the
  already-matched `update_stuff`), and per-frame size `0x1C`(28 bytes,
  matching 2011's `ViewFrame` exactly) — `0x118/0x1C=10` frames/loop
  and `0x14+8*0x118=0x8D4` both land exactly, with no other loop/frame
  count closing all three simultaneously. `update_stuff`'s frame-advance
  logic confirmed `numframes[8]`@`+0x02`(2-byte stride) directly via a
  bounds check, and its "loop has no frames, mirror the previous loop"
  fallback path (classic AGS behavior) independently confirmed the
  `0x118` stride a second way AND that each frame's first field (`pic`)
  uses `-1` as an "unused slot" sentinel, matching 2011's own
  convention. DRIFT: 8 loops×10 frames (80 slots) here vs. 2011's
  16×20(320) — and ARCHITECTURAL FINDING: the header ends immediately
  after `numframes[8]` with zero slack, meaning 2011's separate `int
  loopflags[16]` array is CONFIRMED ABSENT from this build's per-view
  data entirely, not merely unfound. See
  reversing/notes/struct-layout-drift.md for the complete writeup,
  including a promising unresolved lead surfaced along the way: a
  32-byte-stride runtime array, `dword_4E45C8`, initially misread as a
  "per-character animation-state array" (an unverified assumption
  based only on IDA's own generic `chat`/`chaa` loop-variable naming).
  **`RoomObject` recovered next** — pulling that thread immediately
  overturned the guess: `load_new_room` (already matched) sets
  `dword_523128 = &roomstats[newnum]` (matching 2011's confirmed
  `croom=&roomstats[newnum]`) then `dword_4E45C8 = dword_523128+8` —
  meaning `dword_4E45C8` is `croom->obj`, AGS's room-`Object` array,
  entirely unrelated to `CharacterInfo`. `RoomStatus.beenhere`@`+0x00`
  and `.numobj`@`+0x04` are confirmed in the same block (2011's
  declared leading fields, zero drift); `RoomStatus` itself isn't
  formalized as its own struct yet (capacity unconfirmed), just these
  two fields plus `obj[]`'s start address. `RoomObject` itself closed
  out completely in already-matched script-API functions: `x`/`y`
  (`GetObjectAt`), `num`/`baseline` (`GetObjectAt`, the latter an exact
  match to 2011's `get_baseline()` logic), `view`/`loop`/`frame`
  (`SetObjectView`/`SetObjectFrame` — `SetObjectView` also reads `loop`
  back and compares it directly against `ViewStruct272.numloops`,
  independently upgrading that field from MEDIUM to HIGH confidence),
  `wait`/`moving` (`update_stuff`, with `moving` nailed down via a
  `do_movelist_move(&obj.moving,...)` call matching 2011's exact call
  shape), `cycling` (cleared on every view/frame change), and `on`/
  `flags` (`GetObjectAt`'s hit-test gate). DRIFT: 2011's tint/zoom/
  last-width/last-height/blocking-box fields are all CONFIRMED ABSENT
  from this build's `RoomObject` — the header runs straight from `y` to
  `transparent` to `num` with zero gap, and the struct ends immediately
  after `flags`, matching this project's repeated "later AGS feature,
  confirmed absent" pattern. See reversing/notes/struct-layout-drift.md
  for the complete writeup and the methodology note it prompted: don't
  trust an IDA-assigned variable name as evidence, always trace the
  pointer to its real origin.
  **A follow-up round closed `RoomObject`'s last two MEDIUM fields**:
  `SetObjectTransparency` confirms `transparent` with an exact
  instruction-for-instruction match to 2011's implementation, and
  `AnimateObject` confirms `overall_speed` directly and, as a bonus,
  supplies the first independent evidence for `ViewFrame272.speed`
  (`wait = spdd + views[view].loops[loopn].frames[0].speed`) — every
  `RoomObject` field is now HIGH confidence, joining `MouseCursor`/
  `InventoryItemInfo`/`MoveList` as a fully zero-guesswork struct.
  `RoomObject.flags` also picked up two confirmed bit values
  (`OBJF_NOINTERACT`=1 via `GetObjectAt`, `OBJF_NOWALKBEHINDS`=2 via
  `prepare_characters_for_drawing`, which turns out to ALSO draw room
  objects despite its character-focused name), and `ViewFrame272.speed`
  got a third independent confirmation from the mouse cursor's own idle-
  animation code inside `GetLocationType` — proving `ViewStruct272` is a
  shared animation format used by cursors, objects, and characters
  alike. `ViewFrame272`'s `xoffs`/`yoffs`/`flags`(mirroring)/`sound`
  remain unreached by any already-matched caller after this search and
  are shelved at the same status as `InterfaceElement`'s remaining
  fields.
  **Pivoted to characterizing `sub_40C3E0`/`sub_40C75E`** (the
  `ViewStruct272`-adjacent unmatched functions flagged in earlier
  rounds) and found something new for this project: a command-list
  dispatcher pair with NO 2011 source at all to anchor to, living or
  dead-commented (`run_event_block`'s own EventBlock-era subsystem
  "was replaced" by 2011). `sub_40C75E` loops over a 24-byte-stride
  command list calling `sub_40C3E0` per entry, which dispatches on a
  `type` byte to `SetObjectView`/`SetCharacterView`,
  `AnimateObject`/an inline character-animate equivalent, and (not yet
  individually read) `move_object`/`walk_character` — closely
  resembling 2011's later `NewInteractionCommand`/
  `run_interaction_commandlist` in the SET of actions covered, but with
  a much simpler flat POD layout (no vtable, unlike 2011's
  `NewInteractionAction`-derived version) — plausibly a genuine
  pre-refactor ancestor. Formalized as **`EventBlockCmd`**, explicitly
  labeled project-invented rather than source-derived. Left both
  functions unnamed (`new_name: None`) rather than force an invented
  name onto them. Hypothesized but unconfirmed: `sub_40C75E` fires for
  `EventBlock.respond[i]==4`, the one value `run_event_block`'s own
  evidence doesn't already account for.
  **A follow-up round read the remaining `type` branches (3/4/5) and
  closed out `EventBlockCmd` completely** — the full 0-5 enum is now
  known (3/4 = move object/character with/without wall-avoidance, 5 =
  set position directly with no movement, anything else = a second
  distinct "unknown animation encountered" error proving the switch is
  exhaustive), resolving the previously-mysterious `+0x00` field (the
  move/set-position target X coordinate) and clarifying that the whole
  struct is really 4 generic reusable argument slots whose meaning is
  entirely `type`-defined — mirroring 2011's `data[]`/`IPARAM1`-`5`
  convention exactly, just without the later `NewInteractionValue`
  wrapper or vtable. Every byte through `+0x15` is now positively
  identified.
  **A final round confirmed the `respond[i]==4` hypothesis directly**
  by reading `run_event_block`'s own disassembly at the call site, and
  it uncovered a whole undocumented game resource in the process:
  `respond[i]==4` ("Run Animation") indexes a 10-slot global table
  (`unk_52024C`, bounds-checked against a literal `0Ah`) of reusable
  `EventBlockCmd` command lists — this build's entirely-undocumented-
  in-2011 "Animations" resource system, matching the old AGS Editor's
  distinct "Animations" project-tree resource type (long gone by 2011,
  same fate as the rest of the EventBlock/interaction-scripts
  subsystem). Formalized as **`GameAnimation`** (`EventBlockCmd
  command[10]; int numCommands;`, `0xF4`/244 bytes total), confirmed by
  two independent pieces of evidence — the caller's `data[i]*0xF4`
  index scaling and the callee's own `numCommands` loop bound — landing
  on the same total with zero slack. A parallel gate table,
  `dword_52033C` (same `0xF4` stride, checked for "is this slot
  populated"), remains unexplored beyond that single check.
  **A quick follow-up resolved `dword_52033C` completely**: its address
  is exactly `unk_52024C+0xF0` — i.e. it was never a second table at
  all, just `&unk_52024C[0].numCommands` reached via a differently-
  computed address IDA didn't recognize as overlapping. The "empty
  animation" check is simply `GameAnimation[data[i]].numCommands != 0`,
  giving that field a second, independent confirmation. Nothing left
  open in this thread.
  **`RoomStatus` recovered next** (its `beenhere`/`numobj` leading
  fields were already known from the `RoomObject` round) —
  `SaveGameSlot`/`restore_game_data` (already matched) do a single raw
  `fwrite`/`fread` of the whole `roomstats` array with a literal
  `ElementSize=0x1390`, independently reconfirming the struct's total
  size and, via the restore loop's own bound, `MAX_ROOMS=300` with
  ZERO drift from 2011 — unusual for this project. `tsdatasize`@`+0x168`
  and `tsdata`@`+0x16C` get individual confirmation from both functions
  (a heap pointer can't survive a raw blob copy, so both size-prefix it
  separately, matching 2011's "free old, reallocate, refill" pattern on
  restore exactly). `obj[10]`/`flagstates[15]` were closed via clean
  zero-slack arithmetic between the confirmed `obj[]` start and
  `tsdatasize`'s position, anchored by `flagstates` matching 2011's
  `MAX_FLAGS=15` exactly (MEDIUM confidence — arithmetic fit, no direct
  access site). DRIFT: 10 room objects here vs. 2011's `MAX_INIT_SPR=40`
  — the usual 4x-reduction pattern. Everything from `+0x170` onward
  (4640 bytes) is left as an honestly-labeled unexplored tail — this
  build predates 2011's `NewInteraction`-based `intrHotspot`/
  `intrObject`/`intrRegion`/`intrRoom` fields entirely (already proven
  via `EventBlockCmd`/`GameAnimation`), so 2011's declared layout for
  this region cannot be assumed to apply.
  **A follow-up round cracked part of that tail**: `DisableHotspot`/
  `EnableHotspot`/`get_hotspot_at` (all already matched) all touch the
  identical offset, `+0x135C`, confirming `hotspot_enabled[20]` —
  and the same access pattern turns out to be one already read once
  before, in `load_new_room`'s room-entry init loop, just not
  recognized at the time. Capacity 20 matches 2011's own documented
  ORIGINAL `MAX_HOTSPOTS` value (`Common/acroom.h:65`'s comment: "v2.62
  increased from 20 to 30; v2.8 to 50") — this build genuinely
  predates both later increases, a rare case of drift lining up with
  an explicit version-history comment rather than an inferred pattern.
  A parallel search for `DisableRegion`/`EnableRegion` came up
  completely empty (neither name nor error string exists anywhere in
  this binary, unlike the hotspot pair) — logged as an open lead, not
  confirmed absent.
  **An immediate follow-up closed the loop**: `SetWalkBehindBase`
  (already matched) writes `RoomStatus.walkbehind_base[15]` starting at
  `+0x1370` — landing EXACTLY where `hotspot_enabled` ends, with ZERO
  gap. That gap is 2011's declared position for `region_enabled`,
  so its total absence is now CONFIRMED, not just circumstantial — no
  room exists for it at all. `walkbehind_base[15]` plus a natural
  2-byte pad also lands exactly on the struct's confirmed `0x1390`
  total, which similarly proves 2011's trailing
  `interactionVariableValues[100]` is CONFIRMED ABSENT too. Only one
  real gap remains in `RoomStatus`: `+0x170`–`+0x135C` (4588 bytes,
  where 2011's `NewInteraction`-based fields would sit, already proven
  irrelevant here).
  **That last gap closed immediately** — 2011's own source has it
  commented out right next to the `NewInteraction` declarations:
  `/* EventBlock hscond[MAX_HOTSPOTS]; EventBlock objcond[
  MAX_INIT_SPR]; EventBlock misccond; */`, this build's EventBlock-era
  ancestor layout. `RunHotspotInteraction`/`RunObjectInteraction`
  (already matched) and a newly-characterized helper called only from
  `new_room` confirm all three directly: `hscond[20]`@`+0x170`,
  `objcond[10]`@`+0xD00`, `misccond`@`+0x12C8`. The arithmetic converges
  from three independent directions at once — `hscond`'s capacity (20)
  matches `hotspot_enabled`'s independently-confirmed capacity,
  `objcond`'s capacity (10) matches `RoomObject.obj[]`'s independently-
  confirmed capacity, and `hscond+objcond`'s combined size lands EXACTLY
  on `misccond`'s confirmed start, which itself ends EXACTLY on
  `hotspot_enabled`'s confirmed start. **`RoomStatus` is now FULLY
  MAPPED** — every byte accounted for, turning out to be almost a
  direct, still-live implementation of what 2011 keeps only as a
  commented-out historical footnote.
  **A third attempt at `InterfaceElement`'s remaining fields found
  nothing new** — a direct address search across the ENTIRE
  disassembly for every one of its unconfirmed fields' computed
  absolute addresses turned up zero references anywhere, stronger
  negative evidence than the previous two shelvings. Rob Blanc 1 most
  likely just doesn't use the old icon-bar interface system at all —
  treated as a genuine dead end now, not a not-yet-found lead.
  **Pivoted to `WordsDictionary` instead** (the `dict` field's
  internal layout, worked out in an earlier round but never formalized)
  and picked up three clean function matches along the way:
  `read_dictionary`, `read_string_decrypt`, and `decrypt_text` (the
  last confirmed beyond doubt by the disassembly literally referencing
  AGS's famous `"Avis Durgan"` text-encryption key). Formalized the
  struct itself — this build flattens 2011's dynamic `char**word`/
  `short*wordnum` double-allocation into ONE fixed 1500-word blob
  behind a single presence-flag pointer (the same idiom as
  `compiled_script`), with capacity confirmed via two independent
  zero-remainder divisions landing on the same number from both ends.
  **`CharacterInfo`'s remaining gaps closed the same way `RoomStatus`'s
  did** — via 2011's OTHER save-compat ancestor, `OldCharacterInfo`
  (`Common/acroom.h:2599`). Every already-confirmed field already
  landed exactly where `OldCharacterInfo`'s declared order predicts,
  including the struct's own total size (`0x140`); reading
  `FollowCharacterEx`/`SetCharacterIdle`/`SetCharacterTransparency`/
  `SetCharacterBaseline` (all already matched) confirmed the two small
  remaining gaps directly (`following`/`followinfo`/`idleview`/
  `transparency`/`baseline`), and also RESOLVED a long-standing caution
  note on the `inv` field: `main`'s already-known startup loop writes
  it as a 2-byte value, not 4 — it's `OldCharacterInfo`'s declared
  `short inv[100]`, not a mis-sized attempt at 2011's modern 301-entry
  array as previously worried. The trailing tail (`actx`/`acty`/
  `name[30]`/`scrname[16]`/`on`) is filled in at MEDIUM confidence,
  positional-only — no caller found yet, a candidate for a future round.
  **A follow-up round found leads for two of those**: `GetLocationName`
  (already matched) passes `name`@`+0x110` straight to `GetTranslation`
  for the "what's under the cursor" hover text, and `GetCharacterAt`
  (already matched) delegates to a previously-undocumented internal
  helper whose per-character hit-test filter checks `on`@`+0x13E`
  directly. Both upgraded from MEDIUM to HIGH confidence. `actx`/
  `acty`/`scrname` remain open.
  **A quick follow-up resolved `scrname` completely**: `compile_room_script`
  (already matched) passes `scrname`@`+0x12E` straight to `strcat`
  while building `"#define cEgo 0\r\n"`-style character-name-to-index
  macros for room-script compilation — upgraded to HIGH confidence.
  `actx`/`acty` were checked and shelved: 2011's only usage site for
  either sits deep inside hardware-accelerated drawing code
  (`gfxDriver`/`actspsbmp`/`SetTint`) this build has already been
  shown, repeatedly, to predate entirely — plausibly a later addition,
  not just unfound. `CharacterInfo` is now HIGH confidence everywhere
  except those two fields.
  **Fresh survey target: `GUIMain`** — unlike most other structs in
  this project, its already-confirmed fields (`x`/`y`/`numobjs`/
  `mouseover`/`mousedownon`/`on`/`objs`/`objrefptr`) turned out to
  match 2011's CURRENT/live `GUIMain` declaration (not an old
  save-compat ancestor) with zero drift — a rare case, alongside
  `MouseCursor`/`InventoryItemInfo`. Laying 2011's declared fields
  into the five previously-opaque padding gaps closes ALL of them
  simultaneously with zero slack (`vtext`/`name`/`clickEventHandler`
  → `+0x00..+0x28`; `wid`/`hit`/`focus` → `+0x30..+0x3C`; `popup`/
  `popupyp`/`bgcol`/`bgpic`/`fgcol` → `+0x40..+0x54`; `mousewasx`/
  `mousewasy` → `+0x58..+0x60`; `highlightobj`/`flags`/`transparency`/
  `zorder`/`guiId`/`reserved[6]` → `+0x64..+0x90`) — a strong
  over-determined fit even before individual confirmation.
  `SetGUIBackgroundPic` (already matched) confirms `bgpic`@`+0x4C`
  directly, anchoring the hypothesis with real evidence. Checked and
  NOT found: `SetGUIClickable`/`SetGUITransparency`/`SetGUIZOrder` —
  none of their names or error strings exist in this binary, so
  `flags`/`transparency`/`zorder` stay MEDIUM confidence.
  **An immediate follow-up upgraded four more fields at once**:
  `GetGUIAt` (already matched) touches `on`/`flags`/`x`/`y`/`wid`/`hit`
  all in one hit-test loop, confirming `flags`@`+0x68` bit 0
  (`GUIF_NOCLICK`, matching 2011 exactly) and `wid`@`+0x30`/`hit`
  @`+0x34` (used to compute the bounding box's right/bottom edge)
  directly, plus reconfirming `x`/`y`/`on`. `transparency`/`zorder`/
  the color fields remain MEDIUM — no `GUIMain::draw` equivalent is
  matched yet, the most likely place to find them.
  **Checked and NOT found: `GUIMain::draw`/`draw_at`** — turns out 2011's
  own version doesn't touch `transparency` at all (delegated to a
  later hardware-acceleration abstraction), a dead end. `zorder` got a
  stronger negative result instead: `SetGUIZOrder`/`GUI_SetZOrder`/
  `update_gui_zorder` are all absent by name, AND `GetGUIAt`'s own loop
  iterates GUIs by raw index directly with NO `play.gui_draw_order[]`
  indirection at all (unlike 2011) — real evidence, not just an absent
  string, that this build predates GUI z-order sorting as a feature.
  **`popup`@`+0x40` and `popupyp`@`+0x44` confirmed next**, via
  `check_controls` (already matched): a literal `cmp X,1` gates the
  "should this GUI auto-show" branch, matching 2011's `POPUP_MOUSEY=1`
  exactly, and the mouseY-vs-`popupyp` trigger check confirms the
  latter — `remove_popup_interface`'s own evidence had already named
  `+0x44` as "popupyp-related" in an earlier round, just not yet
  propagated into the struct file. 14 of `GUIMain`'s ~24 fields are
  now HIGH confidence.
  **Pivoted to `ccInstance`'s own long-parked 2400-byte unexplored
  region** (`+0x1C..+0x97C`) and DISPROVED a standing hypothesis rather
  than confirming it: the region was guessed to "almost certainly" hold
  2011's `callStackLineNumber`/`callStackAddr`/`callStackCodeInst`/
  `callStackSize` arrays. Reading the interpreter's SCMD_CALL handler
  (`sub_42B394`, the self-recursive function already flagged in
  `reversing/notes/csrun-interpreter-evolution.md`) in full shows this
  build handles nested script calls via NATIVE C RECURSION — `pc` gets
  saved into a plain local stack variable before the interpreter calls
  itself, and restored from that same variable on return. No array, no
  counter, nothing that could overflow/underflow in 2011's sense —
  consistent with 2011's `"Call stack overflow (recursive call
  error?)"`/`"Call stack underflow"` strings being entirely absent from
  this binary, while the `"stack overflow"` strings that DO exist guard
  a completely different thing (the VM's data-stack pointer). The
  2400-byte region is back to genuinely unknown — this round only rules
  out the previous guess, doesn't supply a replacement.
  **A "wrong turn, right destination" follow-up**: reading
  `ccCreateInstanceEx` in full while chasing `ccInstance`'s gap didn't
  find that, but turned up a clean win on `ccScript` instead —
  `fixuptypes`@`+0x18`/`fixups`@`+0x1C`/`numfixups`@`+0x20`, previously
  one single TENTATIVE positional guess, now confirmed directly via
  the relocation loop that reads them (and whose switch on fixup type
  matches 2011's `FIXUP_GLOBALDATA`/`FIXUP_STRING`/`FIXUP_IMPORT`
  constants exactly, targeting `ccInstance.globaldata`/`strings`, both
  already confirmed).
  **`ccInstance.line_number` closes the small pad next to the big gap,
  and cascades into four global-variable IDs**: grepping the
  interpreter's (`sub_42B394`) full offset list instead of reading it
  start-to-finish showed it touches exactly one offset outside the
  still-unresolved 2400-byte gap — `+0x9A0`, in the SCMD_LINENUM
  (opcode 36) handler, `[ecx+9A0h] = edx; dword_5347F4 = edx`, matching
  2011's `inst->line_number = arg1; currentline = arg1;`
  (`CSRUN.CPP:1334-1336`) exactly. Closes the previously-unexplored
  `_pad_9A0` field as `line_number` and IDs `dword_5347F4` as
  `currentline` in one stroke. Following that global to its other
  readers picked up three more: `cc_error` (already matched) sets
  `ccError`/`ccErrorLine` right next to it (`dword_5347F8`/
  `dword_5347FC`, `cscommon.cpp:59-60`), and a previously-untouched
  3-instruction getter (`sub_42AAA1`, called from `quit`) turned out to
  be `ccGetCurrentInstance()`, returning a fourth adjacent global
  (`dword_534800`/`current_instance`, `CSRUN.CPP:770`) that
  `sub_42B394` itself also saves/restores around nested calls and that
  `ccCallInstance` resets at top-level entry/exit — matching 2011's
  save/restore pattern in shape, just always resetting to 0 instead of
  restoring a saved prior value, consistent with this build's
  native-recursion call design needing no explicit re-entrancy
  bookkeeping. All four globals happen to sit in four consecutive
  dwords in `.data`, most likely link-order coincidence rather than a
  real struct. The big 2400-byte gap itself is still completely
  unexplored.
  **`ccInstance.exportaddr[600]` closes that gap completely, on the very
  next round** — a third read of `ccCreateInstanceEx` found what the
  2400 bytes actually are: 2011's export-address resolution step
  (`CSRUN.CPP:933-948`) has a disassembly counterpart that writes
  computed addresses directly into `[cinst+idx*4+0x1C]` rather than
  mallocing a separate `char **exportaddr` array — an array EMBEDDED
  inline in the struct starting exactly where the gap starts. Three
  independent confirmations converge with zero slack: the loop bound is
  the already-confirmed `ccScript.numexports`@`+0x1C48`; the per-entry
  computation matches source exactly for both export types
  (`EXPORT_FUNCTION`→`&cinst->code[eaddr]`, `EXPORT_DATA`→`cinst->
  globaldata+eaddr`, `CSRUN.CPP:940`/`942`); and the auto-import loop
  right after reads the same array back as the 3rd argument to
  `SystemImports::add`, matching `CSRUN.CPP:959` exactly. Capacity is
  600 entries (matching `ccScript.export_addr[600]`'s own capacity) x 4
  bytes = 2400 = `0x960`, landing exactly on `stack`@`+0x97C` with zero
  remainder — also retroactively explaining why the interpreter never
  touches this region (`exportaddr` is populated once at instance
  creation, never read by the bytecode loop). **`ccInstance` is now
  FULLY MAPPED** — every byte from `+0x00` through its confirmed
  `+0x9A8` total size accounted for, joining `RoomStatus`/
  `GameSetupStructBase` as one of this project's completely-closed
  structs.
  **Fresh survey target: `GameState` (`play`), the biggest remaining
  prize** — 2011's runtime-state struct (150+ fields across every
  subsystem) is, unlike most structs here, a plain global rather than a
  malloc'd object, so there's no allocation-size anchor. `score`
  through `inv_numorder` (11 fields, `+0x00`-`+0xEC`) turned out to
  already be named directly in the live IDB from prior manual work —
  the same "already recovered, just needs formalizing" situation as
  `SpriteCache` — reinforced by `play_globalvars`'s pre-existing
  50-entry declaration landing exactly on `MAXGLOBALVARS=50`
  (`acruntim.h:21`) with zero drift. New matches this round:
  `replace_macro_tokens` (`AC.CPP:7104`, the GUI label "@SCORE@"/etc.
  substitution routine) confirms `score`@`+0x00` via two separate reads
  matching source exactly; `SetGlobalInt` (already matched) confirms
  `globalscriptvars[300]` (DRIFT: 300 here vs. 2011's `MAXGSVALUES=500`)
  via an exact bounds-check/error-string match; `SetMouseBounds`
  (already matched) confirms `wait_counter`/`mboundx1-y2` as five
  zero-drift consecutive fields matching 2011's exact declared
  adjacency. **A genuine structural puzzle surfaced and was left
  open rather than papered over**: naively treating everything as one
  contiguous struct would place these newly-confirmed fields at
  computed offsets that require the confirmed-UNRELATED global `ifnum`
  (predecessor of a default function argument, `AC.CPP:12533`) to sit
  INSIDE the same struct object — structurally impossible for a real C
  struct. Likeliest explanation: `in_cutscene`/`wait_counter`/
  `mboundx1-y2`/`globalscriptvars` are independent standalone 2002
  globals that 2011 later consolidated into one `GameState` struct,
  rather than already being members of the same object as `score` in
  this build — not yet conclusively proven. `apply_structs.py`'s new
  `GameState` struct deliberately stops at `inv_numorder` rather than
  asserting a size past the ambiguity. See `reversing/notes/
  struct-layout-drift.md` for the full reasoning.
  **A GameState follow-up closed the `inv_numinline`/`inv_item_wid`/
  `inv_item_hit` lead and settled the `inv_numorder` question**: reading
  `sub_40D80C` in full showed it's an instruction-for-instruction twin
  of 2011's `offset_over_inv(GUIInv*)` (`AC.CPP:5394-5409`), confirming
  `inv_numinline`@`+0xF0` (`itemsPerLine`), `inv_item_wid`@`+0x100`/
  `inv_item_hit`@`+0x104` (matching 2011's exact declared pair), and
  upgrading `inv_top`/`inv_numdisp` (previously "?"-flagged/medium) to
  HIGH confidence. Separately, `update_invorder` (already matched)
  turned out to be a much simpler single-character predecessor of
  2011's per-character-generalized version — no per-character loop, no
  `OPT_DUPLICATEINV` handling, no `MAX_INVORDER` bounds check — and its
  body directly PROVES `inv_numorder`@`+0xEC` is NOT obsolete here: it's
  the one true live counter, where 2011 keeps `obsolete_inv_numorder`
  only as a backwards-compatibility mirror of a per-character count.
  Surfaced a new global along the way, `play_invorder` (a `short[]`,
  this build's predecessor of 2011's per-character `charextra[].
  invorder[]`) — its address is far from `GameState`'s own confirmed
  range, joining the "independent 2002 global, later folded into
  GameState" side of the still-open structural question from the
  previous round rather than resolving it.
  **`EndSkippingUntilCharStops` turns out to secretly be
  `unload_old_room` too** — cracking the round's last open thread
  (`dword_4EEB54`/`58`/`5C`/`6C`/`70`, flagged "inconclusive"
  previously). Reading the whole ~220-line function (already matched
  via callgraph, but far bigger than 2011's 3-line source counterpart)
  showed it does everything 2011 later split into a separate
  `unload_old_room()` — a 3-way screen-transition dispatch matching
  `current_fade_out_effect()` (`AC.CPP:3538-3567`) instruction for
  instruction identifies `dword_4EEB6C` as `fade_effect` and
  reconfirms `play_scren_tint` as `screen_tint`; a final trio of
  zero-writes matching source's `play.bg_frame=0;
  play.bg_frame_locked=0; play.offsets_locked=0;` identifies
  `dword_4EEB70`=`bg_frame_locked` (doubly confirmed via its exact
  2011-matching adjacency to `fade_effect` too), `dword_4EEB58`=
  `bg_frame`, and a third standalone global, `word_4EF236`=
  `offsets_locked`. Joins `sub_42B394`/`cc_run_code` and
  `offset_over_inv`/`GetInvAt` as another "one big pre-refactor 2002
  function, later split into several 2011 ones" case — `unload_old_room`
  didn't exist as its own function yet. Formalized as a documented
  "GameState island 2" comment block in `apply_structs.py` rather than
  struct members, consistent with the still-open contiguity question.
  **A self-correction**: the previous round's guess that an unmatched
  helper (`sub_409A9C`) was plausibly `save_room_data_segment` turned
  out wrong once actually read — it's `cancel_all_scripts`
  (`AC.CPP:3078-3091`), an exact algorithmic match confirmed via the
  already-known `ExecutingScript` layout (`scripts[]`'s base address,
  `0x4CC848`, pinned down for the first time) and two small dispatched
  helpers that turned out to be `ccAbortInstance`/
  `ccAbortAndDestroyInstance` (`CSRUN.CPP:1991-2003`, both new matches).
  Also identifies `num_scripts` (a new global) and reconfirms
  `ExecutingScript.forked`/`.numanother` a third independent way. Note
  for next time: `sub_409A9C`'s behavior had already been described
  accurately, in passing, inside `post_script_cleanup`'s own evidence
  from an earlier round — grepping `matches.json` for a function's
  address before guessing its identity from callsite position alone
  would have caught this immediately.
  **GameState "island 2" (`in_cutscene` through `globalscriptvars`) is
  now FULLY MAPPED**, closing the last two genuinely-unidentified
  fields from the prior two rounds: `dword_4EEB54`=`fast_forward` (via
  `FadeOut`'s early-bailout gate, matching AGS's extremely common `if
  (play.fast_forward) return;` idiom — 41 occurrences in `AC.CPP` —
  and zero-gap positional adjacency to `in_cutscene` matching 2011's
  exact declared order) and `dword_4EEB5C`=`bg_anim_delay` (via
  `mainloop`'s background-frame-advance gate, an instruction-for-
  instruction match to source). The `bg_anim_delay` find also upgrades
  `bg_frame` to HIGH confidence (a second independent confirmation),
  reconfirms `bg_frame_locked` a third way, and picks up two bonus
  standalone globals: `anim_background_speed` and (a `roomstruct`
  field, not GameState) `num_bscenes`. The only field left open in
  GameState's confirmed territory is now the small positional-only
  `text_speed`/`sierra_inv_color`/`talkanim_speed` gap between
  `inv_numinline` and `inv_item_wid` — plus the still-unresolved
  question of whether island 1 and island 2 are the same contiguous
  object.
  **MAJOR CORRECTION: GameState is one contiguous 2404-byte struct
  after all — the `ifnum` puzzle resolved, and the previous three
  rounds' "independent standalone globals" conclusion overturned.**
  `SaveGameSlot` (already matched) writes `play` to the save file with
  a literal size constant — `fwrite(&play, 0x964, 1, Stream)` (2404
  bytes) — landing with zero slack exactly where an unrelated global,
  `String1`, begins. This proves `GameState` really is one object end
  to end. That forced a second look at `ifnum` (address `0x4EEB28`,
  sitting squarely inside this proven range) — its actual usage (inside
  `main`, already matched: `ifnum = game_options[OPT_TWCUSTOM]; if
  (ifnum==0) ifnum=-1;`) matches 2011's `play.speech_textwindow_gui`
  assignment (`AC.CPP:26389-26391`) exactly. The pre-existing `ifnum`
  label was simply a naming mistake, not a genuine separate global.
  Two more fields dismissed earlier as "standalone" also turn out to
  fall inside the proven 2404-byte range once actually checked against
  it — `play_invorder`@`+0x614` and `offsets_locked`@`+0x81E` — while
  `screen_tint` was believed at this point to compute to `+0x10C6C`,
  outside it (later found to be WRONG — a line-number/address mixup,
  corrected several rounds below). `apply_structs.py`'s `GameState`
  struct is rebuilt as one
  unified definition, padded to its now-proven total size with two
  explicitly-labeled unexplored regions. Process lesson: the original
  "proves discontiguity" call was built on an address looking
  inconvenient, never checked against a hard size anchor — worth
  actively hunting for one (a `fwrite`/`malloc` literal) before
  concluding two regions are separate objects.
  **Three more fields close in the newly-reopened +0x114..+0x138 gap**:
  `totalscore`@`+0x118` (via `replace_macro_tokens`'s two macro
  branches, matching 2011's `#define MAXSCORE play.totalscore`
  exactly), `max_dialogoption_width`@`+0x130` (via `do_conversation`'s
  dialog-text-window width computation, exact match), and
  `no_hicolor_fadein`@`+0x134` (via an unmatched helper called from
  `FadeIn`/`process_event`, a close but not line-for-line role match,
  medium-high confidence). Five dwords in this stretch remain
  genuinely unidentified — `apply_structs.py` reflects this precisely
  with two small pad blocks bracketing the three confirmed fields
  rather than one undifferentiated gap.
  **A follow-up round closed two more of those five**: `post_script_cleanup`
  (already matched) confirms `roomscript_finished`@`+0x124` via its
  `runnext[0]=='$'` branch, matching 2011's `run_text_script_iparam(...);
  play.roomscript_finished=1;` exactly (bonus: identifies the global
  `roominst`). `check_controls` (already matched) confirms
  `used_inv_on`@`+0x128` via its `GOBJ_INVENTORY` click branch, matching
  2011's `offset_over_inv(...)` call and `play.used_inv_on=iit;` exactly
  — a third independent confirmation that `sub_40D80C` is this build's
  `offset_over_inv` equivalent, and a bonus identification of the two
  mystery mouse-offset globals from two rounds ago as
  `mouse_ifacebut_xoffs`/`mouse_ifacebut_yoffs`. A third field,
  `dword_4EEB34`, gets a MEDIUM-confidence `skip_display` candidacy
  (a message-box wait loop checking it against 0/2/3), retracting a
  weaker same-role guess for a different field from the prior round.
  Three dwords in this stretch remain genuinely unidentified.
  **An important nuance surfaced while confirming `play_invorder`'s
  capacity**: its own capacity closed cleanly (a zero-interruption
  200-byte span = 100 shorts, matching `MAX_INV=100` with zero drift),
  but the global immediately after it turned out to be part of THREE
  50-entry per-character render-time caches (`prepare_characters_for_
  drawing`, already matched — percent-scaled X/Y plus zoom, recomputed
  every frame) that are unambiguously NOT save-worthy state, yet still
  sit inside `SaveGameSlot`'s proven 2404-byte fwrite span. This means
  that span sweeps in more than just true `GameState` fields — it also
  captures adjacent-but-distinct `AC.CPP` globals the linker happened
  to place contiguously after `play`. Every field already confirmed by
  BEHAVIOR (not just position) is unaffected, but `play_invorder`@
  `+0x614` and `offsets_locked`@`+0x81E` — both previously assumed
  corrected into GameState purely on falling inside the span — are
  reopened as unsettled rather than treated as confirmed members.
  **Immediate correction**: chasing `CharacterExtras` as a fresh target
  walked straight back into that same render-cache trio, and the
  earlier "definitely not save-worthy scratch" call was too hasty.
  `word_4EF1BC` reads with a defaulting-to-100-when-zero fallback
  matching 2011's `zoom_level = charextra[aa].zoom; if
  (zoom_level==0) zoom_level=100;` exactly, and that value scales two
  base sprite dimensions into `word_4EF0F4`/`word_4EF158`, matching
  `scale_sprite_size(...); charextra[aa].width=newwidth;
  charextra[aa].height=newheight;` exactly. These are real, meaningful
  **`CharacterExtras.width`/`.height`/`.zoom`** fields — 2011 still
  maintains them today — just laid out as three parallel `short[50]`
  arrays (structure-of-arrays) instead of 2011's single array-of-
  structs. This doesn't undo the prior caution (the fwrite span still
  isn't sufficient evidence alone) — it reinforces it: the span sweeps
  in a whole separate real struct, not just scratch memory.
  **`walkable_areas_on` closes a real gap, and settles `offsets_locked`
  again**: `sub_40AAE3`'s `memset(&byte_4EF224,1,0x10)` matches 2011's
  `memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);` exactly
  (`MAX_WALK_AREAS=15`, zero drift). Its confirmed end lands exactly 2
  bytes before `offsets_locked`'s already-known address — matching
  2011's declared adjacency `walkable_areas_on; short screen_flipped;
  short offsets_locked;` with zero slack for the one intervening field.
  This gives `offsets_locked` a SECOND independent confirmation (on top
  of its original zero-write evidence), settling the question reopened
  two rounds ago. Bonus: identifies `dword_523204` as `raw_saved_screen`.
  **GameState's tail closes completely**: `RawSetColor` (already
  matched) confirms `raw_color`@`+0x938` exactly, sitting with zero gap
  before the already-IDA-named `play_filenumbers` — proving 2011's
  `raw_modified[MAX_BSCENE]` is absent. `ListBoxSaveGameList` (already
  matched) confirms `filenumbers[20]`'s capacity via its own sort-loop
  bound (`MAXSAVEGAMES=20`, `Engine/acdialog.h:870`), and that field
  **lands exactly on GameState's own proven total size with zero bytes
  left over — this is the struct's last field.** Working backward from
  there closed three more: `script_timers[21]`@`+0x838` (via
  `update_stuff`'s own opening lines, an instruction-for-instruction
  match to 2011's `MAX_TIMERS` loop), and `sound_volume`/
  `speech_volume` (via two as-yet-unnamed helper functions computing
  `vol*sound_volume/255` and passing `speech_volume` into WAV/MP3
  speech-file loaders — both exact matches to source, each sitting with
  zero gap against its confirmed neighbor). GameState is now mapped
  end to end from `+0x00` to `+0x964` — every remaining gap is an
  explicitly-sized, explicitly-labeled pad rather than unaccounted
  territory.
  **Six more fields close out both remaining pads**: `load_new_room`'s
  room-entry-edge logic confirms `entered_edge`@`+0x828` (exact match
  to 2011's descending-threshold assignment) and, via its second edge-
  detection block, CONFIRMS `entered_at_x`/`entered_at_y` are ABSENT
  (this build reuses the shared `tox`/`toy` scratch globals instead of
  persisting them). Two already-IDA-named globals sitting right next to
  it turn out to be exactly where they should be: `want_speech`@`+0x82C`
  (via `SetVoiceMode`, exact match) and a plausible-but-position-
  contradicting `stop_dialog_at_end`@`+0x834` (2011 places it much
  earlier, near `reserved[10]`, not next to `want_speech` — flagged as
  a genuine unexplained difference). `SetNormalFont` confirms
  `normal_font`@`+0x894` exactly; the already-named `fontid` right
  after it is a medium-high-confidence `speech_font` candidate (kept
  cautious given the `ifnum` mislabeling precedent); and
  `play_key_skip_wait` — already IDA-named but never behaviorally
  verified — now has an exact match via `check_controls`. GameState's
  struct is now broken into small, precisely-sized pads with no
  undifferentiated multi-hundred-byte gaps left anywhere.
  **MAJOR SELF-CAUGHT ERROR: `screen_tint`'s address was a line-number/
  address mixup, not actually outside GameState.** Its address had
  originally come from reading a `grep -n` LINE NUMBER (`500204`) as if
  it were the hex memory address `0x500204` — never cross-checked
  against a neighboring self-encoding label the way every other custom
  global name in this project was grounded. The TRUE address (verified
  via three independent self-encoding neighbors) is `+0x8AC`, well
  inside the struct, and lines up exactly with 2011's declared order —
  closing FOUR fields at once with zero gaps between them:
  `swap_portrait_lastchar`@`+0x8A0` and `swap_portrait_side`@`+0x10C`
  (a much-earlier field, upgraded from a many-rounds-old tentative
  guess) via the same `_displayspeech` evidence; `seperate_music_lib`@
  `+0x8A4` — a THIRD mislabeled pre-existing global found in this
  project (`play_want_music` is misleading; 2011 has no "want_music"
  field at all, `IsMusicVoxAvailable` proves it's really
  `seperate_music_lib`); and `in_conversation`@`+0x8A8` via
  `do_conversation`'s opening increment. Process lesson: a grep line
  number and a hex address look similar and are easy to conflate —
  every custom-named global needs grounding against a neighboring
  auto-named label before its address is trusted.
  **`bad_parsed_word` closes almost the last remaining pad**:
  `SaidUnknownWord` (already matched) confirms `bad_parsed_word[100]`
  exactly, landing with the EXPECTED 2-byte alignment gap before the
  already-confirmed `raw_color` — strong positional reinforcement on
  top of the role match. This also confirms 2011's `num_parsed_words`/
  `parsed_words[MAX_PARSED_WORDS]` (declared immediately before
  `bad_parsed_word`) are ABSENT — the 34 bytes where they'd sit belong
  to unrelated already-named parser globals (`comparetonum`/
  `compareto`) instead. The only sizeable unmapped GameState territory
  left is the `+0x60C..+0x80C` pad (containing `CharacterExtras`) and
  three still-unidentified dwords in the small `+0x114..+0x138` stretch.
  **The `+0x60C..+0x80C` pad turns out to be fully byte-accounted for**,
  even though not every piece is named: 8 bytes of unconfirmed video/
  music-parameter globals, `play_invorder` (200 bytes, role/capacity
  known, GameState membership still genuinely unresolved — no
  positional evidence either way, since neither neighbor is itself a
  confirmed member), `CharacterExtras` (300 bytes, confirmed separate),
  and 4 bytes of an unidentified countdown timer. The three small-gap
  dwords resisted a fourth round of attempts and are left as genuinely
  hard cases rather than forced. `apply_structs.py` now represents all
  of this with precisely-sized pads instead of one undifferentiated
  512-byte unknown — a clarity improvement, not new content.
  **`text_speed`/`sierra_inv_color`/`talkanim_speed` close** — the last
  three purely-positional guesses in GameState's early section, sitting
  unconfirmed since the very first fresh-survey round. `sierra_inv_color`
  closed via `__actual_invscreen`'s exact `wsetcolor`/`wbar`-equivalent
  call sequence. `text_speed` and `talkanim_speed` both closed the same
  way: an exact match to 2011's own init literal value (`15` and `5`
  respectively) alongside an independent role-matching use site.
  `talkanim_speed`'s use site is notable — 2011's own source only ever
  assigns it once and never reads it again, but this build actively
  uses it for talk-animation timing, another case (like `inv_numorder`)
  of a field 2011 kept declared but stopped using. Every field from
  `+0x00` through `+0x104` now has real behavioral evidence.
  **The init block: one ~65-instruction block confirms or closes nearly
  everything left in GameState.** A massive sequential init sequence
  inside `main` (this build's inlined `init_game_settings()`
  predecessor) sets ~40 fields to literal values matching 2011's own
  init sequence almost line for line. This closes THREE fields that had
  resisted five separate rounds — `follow_change_room_timer`@`+0x114`
  (init value 150, exact), `no_multiloop_repeat`@`+0x120`, and
  `no_textbg_when_voice`@`+0x12C` (both via matching sequential init-
  code position plus value) — and every remaining tentative field:
  `speech_text_shadow`, `screen_flipped`, `speech_font` (resolving the
  standing `ifnum`-style mislabeling caution — it's genuine), and
  `skip_display` (upgraded to high). ~25 more already-confirmed fields
  get exact-value bonus reconfirmations in the same block. GameState's
  field-level identification work is now, for all practical purposes,
  essentially complete — remaining open items are `play_invorder`'s
  membership question and a handful of still-unidentified globals in
  the smaller pads.
  **`sub_40A6D8` read in full: a genuinely different fade-out
  technique, not just a refactor.** This build's high-color-depth
  screen-fade helper (already tied to the confirmed `no_hicolor_fadein`
  flag) does the same job as 2011's `highcolor_fade_out()` — but via a
  manual pixel-by-pixel darkening loop rather than Allegro's alpha-
  blending API, predating that API entirely. Deliberately left unnamed
  rather than renamed, matching this project's established convention
  (`sub_42B394`/`cc_run_code`) for role-matches whose implementation
  diverges too much to claim 1:1 correspondence.
  **Fresh survey target: `ScreenOverlay`** — closed completely in a
  single round via `add_screen_overlay`'s own construction sequence
  (already matched; only its error string had been read before).
  Confirms a genuine array-of-structs, `pic`/`type`/`x`/`y`/`timeout`
  (5 fields, `0x14`-byte stride), matching 2011's `OVER_TEXTMSG`/
  `OVER_COMPLETE`/`OVER_CUSTOM` constants and custom-ID search range
  exactly. DRIFT: capacity 10 vs. 2011's `MAX_SCREEN_OVERLAYS=20` — the
  familiar 2x-reduction pattern. CONFIRMED ABSENT: `bmp`
  (`IDriverDependantBitmap*`, a later hardware-acceleration
  abstraction, same pattern as `CharacterInfo.actx`/`.acty`),
  `bgSpeechForChar`, `associatedOverlayHandle`, `hasAlphaChannel`,
  `positionRelativeToScreen` — this build has exactly 2011's first 5
  fields. Bonus: a new function match (`find_overlay_of_type`, exact
  instruction-for-instruction match) and three related globals
  (`numscreenover`, `is_complete_overlay`, `is_text_overlay` — the last
  a second independent confirmation of a global first seen incidentally
  during an earlier GameState round).
  **`CreateGraphicOverlay` read in full reconfirms `ScreenOverlay`
  and closes `spritewidth`/`spriteheight`**: matches 2011's version
  (`AC.CPP:13125-13138`) line for line — `create_bitmap_ex(...,
  spritewidth[slott], spriteheight[slott])` upgrades `dword_4CD2E8`/
  `dword_4E787C` to HIGH confidence via a second independent usage
  context, and `return screenover[nse].type;` is a second independent
  confirmation of `ScreenOverlay.type`@`+0x04`. Bonus: two new function
  matches, `wputblock` (exact instruction-for-instruction match) and,
  incidentally, `draw_sprite` (a well-known third-party Allegro API).
  **CORRECTION: `sub_409FD4` is `remove_screen_overlay`, not
  `stop_fast_forwarding`** — the earlier match (several rounds prior, on
  callgraph-position evidence alone, body never read) is wrong. Reading it
  in full while investigating `RemoveOverlay`'s callees shows an exact,
  complete algorithmic match to `remove_screen_overlay`/inlined
  `remove_screen_overlay_index` (`AC.CPP:3404-3441`), including a `rep
  movsd`/`ecx=5` shift-down loop matching `ScreenOverlay`'s own confirmed
  20-byte size exactly. This means `EndSkippingUntilCharStops`
  (`sub_40AAE3`)'s final call is actually `remove_screen_overlay(-1)`,
  matching 2011's `unload_old_room()` calling `remove_screen_overlay(-1)`
  at `AC.CPP:3627` — not `stop_fast_forwarding()` as previously recorded.
  The real `stop_fast_forwarding()` (`AC.CPP:24132`) has **not** been
  located in this binary and is an open lead for a future round.
  `matches.json` entries for both functions corrected (old evidence kept
  for the record); `apply_structs.py` needed no change (zero mentions).
  **The `stop_fast_forwarding` lead is now resolved — it doesn't exist as
  a function here.** Reading `StartCutscene`/`EndCutscene` (already
  matched via linker symbol, but never read body-first) in full:
  `EndCutscene` inlines `play.fast_forward=0` directly and covers the rest
  of `stop_fast_forwarding()`'s job with a call to the already-matched
  `UpdatePalette` (whose own 2011 body already does the equivalent
  `setpal()`/`invalidate_screen()` work); `StartCutscene` has no call to
  `EndSkippingUntilCharStops()` where source has one. CONFIRMED ABSENT:
  the entire `SkipUntilCharacterStops`/`EndSkippingUntilCharStops`/
  `stop_fast_forwarding`/`initialize_skippable_cutscene` subsystem, not
  just one function — consistent with the earlier finding that no
  `SkipUntilCharacterStops`-related strings exist anywhere in the binary.
  Bonus: third/fourth independent confirmations of `dword_4EEB50`=
  `in_cutscene`/`dword_4EEB54`=`fast_forward`.
  **`CharacterExtras.xwas`/`.ywas`/`invorder[]`/`invorder_count` confirmed
  absent** (reading `EndSkippingUntilCharStops`/`unload_old_room`-combined
  in full end to end for the first time): no `charcache`/`xwas`-wipe loop
  exists in this function at all, and `char_zoom` (`word_4EF1BC`, the one
  field a `wantMoveNow`-equivalent function would have to read) has
  exactly TWO xrefs in the whole binary, both already accounted for inside
  `prepare_characters_for_drawing` — no other function reads it, ruling
  out a separate scaled-movement-smoothing function existing anywhere.
  Converges with an earlier round's dead-end `INVALID_X`-sentinel search.
  `invorder[]`/`invorder_count` (2011's per-character inventory-order
  pair) are absent for the same reason `play_invorder`/`update_invorder`
  were found game-wide-not-per-character several rounds ago — the feature
  they belong to doesn't exist yet. Two bonus drift findings fell out of
  the same full read: ambient-sound-stop is a single hardcoded
  `StopAmbientSound(1)`, not a loop over channels 1–7; and room-script
  cleanup frees only `roominst`, not a separate `roominstFork` — no such
  global is touched anywhere in the function. Also a second independent
  confirmation of `RoomObject.moving`@`+0x18` via this function's own
  `objs[ff].moving=0` reset loop.
  **`animwait`/`walkwait` fold into `CharacterInfo.wait`@`+0x1C`, closing
  the last big open lead**: reading `update_stuff`'s per-character
  walking/animation section in full shows THREE separate 2011 fields
  (lip-sync wait, `walkwait`, `charextra[].animwait`) are all the SAME
  field here — the TURNING_AROUND-branch `walkwait--` decrement, the
  walking<1 `animwait=0` reset, the `animwait>0`/`animwait--` decrement,
  and the final `animwait=frames[frame].speed+chi->animspeed` computation
  all read/write `[chi+0x1C]`. Matches `OldCharacterInfo`'s single
  declared `wait` field exactly (no separate `walkwait` in that ancestor
  at all) — `animwait` is CONFIRMED ABSENT as its own field, not a gap.
  Bonus: upgrades `CharacterInfo.animspeed`@`+0x42` to high confidence
  (read directly as the computation's second addend).
  `process_idle_this_time` resolved as a genuine identity but NOT a
  per-character array: `dword_52320C` (all 3 xrefs inside `update_stuff`)
  matches the role exactly, set/reset as a single shared flag rather than
  a 50-entry array — works because this build's flatter, single-pass-
  per-character loop shape only ever consumes it within the same
  character's own iteration. Bonus: identifies `dword_523120` as
  `loopcounter`. `slow_move_counter` shelved rather than forced (even
  2011's own source writes it once and never reads it — unfalsifiable
  either way).
  **`tint_r`/`tint_g`/`tint_b`/`tint_level`/`tint_light` upgraded to
  CONFIRMED ABSENT** (follow-up round): tracing `prepare_characters_for_
  drawing`'s actual character scale-then-blit control flow end to end —
  from the confirmed zoom-scaling code through bitmap creation, the
  mirroring check, sprite fetch, to the final blit — shows no tint-related
  step anywhere in the sequence: no 8-argument call shaped like
  `get_local_tint(...)`, no `apply_tint_or_light(...)` call, and neither
  function is matched (or even flagged as a lead) anywhere else in the
  binary. Not just the `CHF_HASTINT` branch is missing — the entire tint
  computation-and-application subsystem has no counterpart here. This
  also explains why GameState's `rtint_red`/`rtint_green`/`rtint_blue`/
  `rtint_level`/`rtint_light` (`get_local_tint`'s own room-tint-override
  source) have never turned up in any GameState round. With this,
  `CharacterExtras`'s field-level investigation is essentially complete —
  only `slow_move_counter` remains genuinely open.
  **`music_master_volume` closes the `dword_4EF220` lead, `play_speech`
  gets named**: `sub_418E82` turns out to be `update_music_volume` fused
  with 2011's separate `calculate_max_volume()` — its volume formula
  (`thisroom.options[ST_VOLUME]*30 + dword_4EF220`, clamped [0,255])
  matches source exactly and identifies `dword_4EF220` as
  `GameState.music_master_volume`, confirmed a second way via
  `play_speech`'s (`sub_4141B8`, upgraded from unnamed medium-confidence
  to a named high-confidence match) own `-60` speech-ducking decrement
  right before calling it — a hardcoded stand-in for 2011's configurable
  `speech_music_drop` field. Zero-slack bonus: the field ends exactly 4
  bytes before the already-confirmed `walkable_areas_on`, proving 2011's
  `digital_master_volume`/`cur_music_number`/`music_repeat` are confirmed
  absent here too. `play_speech`'s tail also resolves the two-round-old
  "plausibly lipsync-related" mystery cluster — wrong guess, the real
  answer is Sierra-speech-mode switching (`byte_513340`=
  `game.options[OPT_SPEECHTYPE]`), giving `no_textbg_when_voice` a second
  independent confirmation. Two drift points recorded: this build's
  speech-load failure is fatal (`quit()`) where 2011 fails gracefully, and
  no OGG attempt exists between WAV and MP3.
  **Closed the loose end from last round: `my_load_wave`/`my_load_mp3`/
  `load_sample`/`play_sample` all matched.** `sub_408623`=`my_load_mp3`
  is an overwhelming instruction-for-instruction match to source,
  including a `malloc(0x186A0)` matching `MP3CHUNKSIZE`'s OLD,
  commented-out value of 100000 still sitting in the source itself — this
  independently upgrades `pack_fopen`/`pack_fread`/`pack_fclose` from
  medium to high confidence via a second, unrelated caller. `sub_408556`=
  `my_load_wave` matches similarly cleanly, and following its two callees
  down turned up two more well-known Allegro APIs: `sub_4444C0`=
  `load_sample` (its real `"wav"`/`"voc"` extension-dispatch logic,
  calling the already-matched `load_wav`/`load_voc`) and `sub_444AF0`=
  `play_sample` (opens with the already-matched `allocate_voice`, and its
  frequency-scaling branch contains an unmistakable MSVC divide-by-1000
  magic-constant fingerprint, `0x10624DD3`). Bonus: identifies 3 new
  globals (`thiswave`/`mp3in`/`thistune`) and 5 candidate Allegro
  voice-control functions for a future round. Also closed: `rtint_red`/
  `rtint_green`/`rtint_blue`/`rtint_level`/`rtint_light` confirmed
  absent outright — their only writer, `SetAmbientTint`, has zero string
  occurrences anywhere in the binary, same as their reader.
  **The full `play_sample` call chain closes**: all 5 candidate
  voice-control functions from the previous round confirmed on an
  immediate follow-up — `voice_set_volume`/`voice_set_pan` (each keyed by
  an unmistakable MSVC reciprocal-multiplication magic constant, divide-
  by-255 and divide-by-1000 respectively), `voice_set_playmode` (the
  `PLAYMODE_BACKWARD`=2 bit test matches exactly), `voice_start`
  (identifies `retrace_count`), and `release_voice` (a one-line body
  whose `0xFFFFFFFF` write only makes sense once you know Allegro's own
  `TRUE` is `#define`d as `-1`, not `1`). All 7 calls in `play_sample`'s
  real chain (`allocate_voice`→`voice_set_volume`→`voice_set_pan`→
  `voice_set_frequency`→`voice_set_playmode`→`voice_start`→
  `release_voice`) are now fully identified with zero remaining unmatched
  links, and the `VOICE` struct's first 3 members (`sample`/`num`/
  `autokill`) are cross-confirmed across three different functions.
  **`cur_music_number`/`music_repeat` close the last `GameState` pad —
  with a self-caught correction.** `dword_4EF024`=`cur_music_number`,
  confirmed across 6 functions (`GetCurrentMusic`/`PlayMusic`/
  `scr_StopMusic`/`main`, plus a decisive literal match: `restore_game_data`
  writes `2000`, matching source's own `"play.cur_music_number=2000; //
  make sure it gets played"` comment word for word). `dword_4EF028`=
  `music_repeat`, via `SetMusicRepeat`'s entire one-line body matching
  2011's entire function body verbatim, sitting with zero gap right after
  `cur_music_number` exactly as source declares them adjacent. **This
  REQUIRED correcting a mistake from two rounds ago**: `music_master_volume`'s
  own field entry had claimed the same zero-gap argument that proves
  `digital_master_volume` absent (sits AFTER `music_master_volume`) ALSO
  proved `cur_music_number`/`music_repeat` absent (2011 declares them
  BEFORE `music_master_volume`) — that doesn't follow; a zero-gap proof
  about what comes after says nothing about what comes before. Both fields
  turn out to exist after all, just as a standalone pair at `+0x60C`, far
  from `music_master_volume`'s own `+0x808` — 2011 later consolidated two
  previously-independent globals into one contiguous run, the same pattern
  seen repeatedly elsewhere in this project. `digital_master_volume`
  remaining confirmed absent is unaffected — only the over-broad extension
  of that argument was wrong, corrected in place with a visible note.
  **GameState's very last pad turns out to already be solved**: the
  remaining `+0x820..+0x828` span (2 dwords, between `offsets_locked` and
  `entered_edge`) is exactly where 2011's `entered_at_x`/`entered_at_y`
  would sit — already confirmed absent several rounds ago (`load_new_room`
  writes the equivalent assignment into shared `tox`/`toy` scratch globals
  instead). An exhaustive grep of the entire disassembly for every address
  in that 8-byte range turns up zero xrefs anywhere, independently and
  more forcefully reconfirming the same conclusion. With this, every byte
  of `GameState` from `+0x00` through its proven `+0x964` total is now
  either a confirmed field or an explicitly-evidenced pad — no remaining
  unexplored territory anywhere in the struct.
  — this
  **completes the full `GUIObject` class hierarchy** (all six derived
  classes' vtables identified and structs recovered). Struct work has
  repeatedly found genuine 2002-vs-2011 divergence (smaller fixed-capacity
  arrays, missing later-added fields/methods, different field order,
  version-gated fields entirely absent pre-2002) — never assume a 2011
  layout applies without independent verification via a known IDB size or
  an allocation-size site in the disassembly. Also resolved a generalizable
  `GUIObject` base-class fact applicable to every derived class: `x@+0x08,
  y@+0x0C, wid@+0x10, hit@+0x14, activated@+0x1C`, own fields starting at
  `+0x20`. One important caution from this round: don't trust a vtable-to-
  class mapping on slot-shape resemblance alone (empty/non-empty pattern)
  — a table was briefly misidentified as `GUISlider` this way before
  reading the actual method body proved it was `GUIListBox`; the real
  `GUISlider` table turned out to be a different one, sitting unnoticed
  between two already-pinned tables. Always read at least one
  distinguishing method's body before committing a vtable-to-class match.
- Confirmed early win: the string table at `aMov`/`aMemwritelit`/`aRet`/...
  in `.data` is `sccmdnames[]` from `Common/CSCOMP.H` (script bytecode
  mnemonic table, used by `Common/CSRUN.CPP`'s bytecode dumper) — a good
  example of the string-matching technique paying off for a whole array at
  once, not just one function.
- Current productive avenue: callgraph-following from already-matched
  functions, and mining vtables/field-access patterns for struct recovery
  (which has repeatedly turned up *new* function matches as a side effect —
  see `reversing/notes/struct-layout-drift.md`'s `ccInstance`/`GUIButton`
  rounds). Some threads have gone cold (old CRT file-search APIs with no
  2011 counterpart, a few ambiguous shared vtable stubs) — see
  `reversing/notes/` for specifics before re-attempting those.
- **Fresh survey target (in progress): `RoomStruct`** (`thisroom`/
  `rstruc`, 2011's `roomstruct`, `Common/acroom.h:806`) — the current-
  room data format, never formalized before despite incidental references
  in many already-matched functions. The global instance and 4 leading
  pointer fields (`walls`/`object`/`lookat`/`regions`) turned out to
  already be IDA-named from work predating this project, matching 2011's
  declared order with zero drift. **Caution surfaced immediately**: IDA's
  Local Types library also already has a `roomstruct` type applied to
  this global, UNVERIFIED against this build — likely a blind 2011
  import, given this project already independently proved this struct's
  own capacity constants (`MAX_INIT_SPR`, `MAX_HOTSPOTS`, `MAX_WALK_AREAS`)
  drift smaller here via `RoomStatus`/`RoomObject`/`GameState` work.
  Round 1, via `load_main_block` (already matched, AGS's own room-file
  loader): confirmed `width`/`height`/`resolution`/`numwalkareas`/
  `numhotspots` via exact literal-value inits, `objbaseline[10]` via the
  function's only `0xFF`-valued memset, and a genuine architectural-drift
  find — `hotspotnames[20][30]` is a FIXED INLINE array here, not 2011's
  `char*[]` pointer array with individually-`malloc`'d names.
  **Round 2 (immediate follow-up) closed every pad round 1 left open**:
  reading `load_main_block`'s real `fread()` sequence (not just its init
  preamble) confirmed `numobj`/`objyval[]`'s start, gave `numwalkareas`/
  `numhotspots`/`hotspotnames` second independent confirmations, and
  closed `hswalkto[20]` plus the decisive find — `hscond[20]`/
  `objcond[10]`/`misccond` are `roomstruct`'s own SOURCE copies of the
  same `EventBlock` command lists `RoomStatus` holds RUNTIME copies of
  (identified via three `fread`s sharing `EventBlock`'s own already-
  confirmed exact size, `0x94`/148 bytes). Genuinely interesting
  architectural point: 2011 keeps this subsystem only as a dead,
  commented-out declaration, but this 2002 build's room-loading code
  still actively reads and uses it on every room load — not vestigial
  save-compat cruft here, the live implementation.
  **Round 3 (same session) found the SAME "still live in 2002" pattern
  one step further back**: a version-gated branch (room-file versions
  7-8) still actively `fread`s 2011's own "obsolete v2.00 action editor"
  arrays (`whataction`/`val1`/`val2`/`otcond`/`points`), capacity 130
  confirmed via four consecutive zero-slack boundaries in a row. Past
  that, the version≥9 path (this build's real path) confirmed
  `left`/`right`/`top`/`bottom`/`numsprs`/`sprs[10]` via a double
  cross-check (read order vs. declared order agreeing on the same four
  addresses). A genuine surprise: the same path also re-reads INTO the
  already-confirmed `objbaseline` address using `numsprs` as the count —
  but `objbaseline` sits at `+0x3858`, far from `sprs[]` at `+0x936`,
  where 2011 declares them as neighbors. Not a capacity difference like
  everywhere else in this project — a genuine field-order reordering,
  with the entire hotspot/walkarea block interposed between two fields
  2011 keeps adjacent.
  **Round 4 (same session) closed the entire `+0x936..+0x1570` span**:
  `password[11]`/`options[10]` close cleanly (exact-size freads, zero
  gap). `message[]`/`msgi[]` turn out more interesting than a plain
  read — `message[]` is populated by a dedicated decrypt-and-`malloc`
  loop, genuinely a `char*[100]` pointer array matching 2011's declared
  TYPE exactly (unlike `hotspotnames`, this one did NOT drift to a fixed
  array); `msgi[]` closes right after with zero gap, its 2-byte stride
  matching `MessageInfo`'s own packed layout, `MAXMESS=100` confirmed
  twice over. `anims[10]`/`numanims`/`shadinginfo[16]` close via the
  same version-gated fread-or-memset pattern seen throughout this
  struct. Round 1's one remaining loose end — "two `0x20`-byte memsets,
  ambiguous which field" — resolves cleanly: `shadinginfo` lives
  elsewhere entirely, and the two memsets near `width`/`height` are
  `walk_area_zoom[16]`/`walk_area_light[16]`, individually confirmed via
  their own version-gated reads, sitting with zero gap between them
  (confirming 2011's intervening `walk_area_zoom2` is absent too).
  **MAJOR SELF-CAUGHT CORRECTION (round 5, same session)**: the struct's
  own leading four fields, round 1's supposedly cleanest zero-drift
  find, were never actually independently confirmed — `walls`/`object`/
  `lookat`/`regions`@`+0x00..+0x0C` rested on pre-existing IDA field
  names, but those names come from IDA's own `roomstruct <?>` type,
  which round 1's OWN caution note had already flagged as unverified —
  the disassembly's `rstruc.walls`-style display was that same unverified
  type rendering itself back as if it were independent evidence, exactly
  the trap the caution warned against. Genuine untyped-parameter offset
  arithmetic in `load_main_block`'s picture-loading calls shows all four
  fields sit 4 bytes later than recorded (`walls`@`+0x04` through
  `regions`@`+0x10`), with a fifth, previously-unknown field —
  `ebscene[0]` — occupying the real `+0x00` (confirmed via `load_lzw`'s
  distinct signature and the real AGS global `recalced`). Bonus: this
  also fully resolves the standing "`pal[256]` is 4 bytes short of
  `numobj`" puzzle — `pal[256]`@`+0x14` now lands exactly on `numobj`
  with zero slack. Corrected in place, old offsets struck through and
  explained rather than silently rewritten.
  **Round 6 (same session) landed a SECOND correction — this time on
  round 5's own work.** `load_room`'s pre-load cleanup confirmed
  `scripts`@`+0x39F4`/`compiled_script`@`+0x39F8` (freed via a plain
  `free()` vs. a specialized destructor respectively, matching their
  different 2011 types), then a cleanup loop destroying
  `[rstruc+c*4+0x3A0C]` proved `ebscene[]`'s REAL array base is
  `+0x3A0C` — directly overturning round 5's own claim that `+0x00` was
  `ebscene[0]`. Round 5's reasoning wasn't baseless (`+0x00` really does
  get read/written in a sequence matching source's `load_lzw`/`recalced`
  assignment), it just stopped one step early: that value gets copied
  into the real destination, `+0x3A0C`, immediately after — a transient
  holding spot, not `ebscene[0]`'s own home. `+0x00`'s true role is left
  honestly unconfirmed rather than re-guessed. Bonus: `num_bscenes`@
  `+0x3A00`/`bscene_anim_speed`@`+0x3A04` close via a reset-to-default
  matching 2011's own constructor literals (`1`/`5`) exactly. First time
  in this struct's work that a correction landed on this SESSION's own
  immediately-preceding round rather than an older mistake — same
  discipline both times: keep reading past a plausible-looking match and
  let contradicting evidence override it.
  **Round 7 closed `localvars`/`numLocalVars` as CONFIRMED ABSENT** — an
  exhaustive count of every `getw()` call across `load_main_block`'s
  entire body finds exactly two, both already identified, with no third
  call for the version≥19 gate 2011's own `localvars` read needs;
  independently reinforced by `unload_old_room`'s own already-exhausted
  body never containing the matching copy loop either. Also caught a
  small real mistake in passing: `anims[10]` (round 4) was captioned as
  drift without ever checking 2011's own `MAXANIMS` constant — it's
  actually `10` too, a zero-drift match, corrected in place. The same
  lookup gives `MAX_BSCENE=5`, replacing round 6's unverified guess of
  `10` for `ebscene[]`'s capacity (still not independently confirmed
  against the disassembly itself, just now grounded in the right
  reference point instead of a guess). Lesson stated plainly: "matches
  this project's common smaller-capacity pattern" is a plausibility
  check, not evidence — the actual 2011 constant still has to be looked
  up before citing drift.
  **Round 8 landed a capstone finding**: listing EVERY room-file-version
  gate check across `load_main_block`'s entire body shows a clean,
  gapless run — `3` through `14`, nothing higher anywhere. This build's
  COMPILED ENGINE never had code for room-format version 15+ at all, not
  just this game's own room files happening to be old-format. One fact,
  checked once, retroactively confirms absent everything 2011 gates
  version≥15: `walk_area_zoom2`/`walk_area_top`/`walk_area_bottom`
  (ruling out the leading candidates for the still-open 300-byte pad
  before `scripts`, though its real contents remain unknown),
  `numLocalVars`/`localvars` (a second independent confirmation of round
  7's finding), the entire `NewInteraction`-based interaction-
  deserialization block including `intrObject[]`/`objectScripts`
  (upgraded from "plausible by precedent" to decisively confirmed),
  `hotspotScriptNames`, `gameId`, and `hotspotScripts`/`objectScripts`/
  `regionScripts`/`roomScripts`. Bonus: resolves `sub_403024` (round 4)
  as necessarily `fgetstring_limit`, not `read_string_decrypt`. Also
  caught and fixed a stale round-1 comment that still listed
  `left`/`right`/`top`/`bottom`/`numsprs`/`sprs[]` as candidates for a
  span two rounds had already shown they don't occupy.
  **Round 9 discovered `load_room` doesn't read the file directly — it
  dispatches on a per-block TYPE byte to separate handlers**, of which
  `load_main_block` is only one. Two big payoffs: `BLOCKTYPE_
  OBJECTNAMES`(5)'s handler closes the 300-byte mystery flagged since
  round 6 — `objectnames[10][30]`@`+0x38C6`, `MAXOBJNAMELEN`/
  `MAX_INIT_SPR` matching exactly. `BLOCKTYPE_ANIMBKGRND`(6)'s handler
  confirms `bpalettes[]` is CONFIRMED ABSENT — this build passes the
  SAME shared `pal[256]` to every background-frame load instead of a
  per-frame address, matching 2011's own older, commented-out
  predecessor line still sitting next to the live code (the same
  "matches a historical artifact preserved in source comments" pattern
  as the almp3 `MP3CHUNKSIZE` find). A second, stronger-than-usual
  absence closes alongside it: the dispatch loop only handles block
  types 1–7 and an EOF sentinel — any other type, including
  `BLOCKTYPE_PROPERTIES`(8) and `BLOCKTYPE_OBJECTSCRIPTNAMES`(9), hits
  an explicit "unknown block type" `quit()` — this build's engine would
  actively crash on either, confirming `objectscriptnames[]`/
  `CustomProperties`-based fields absent by direct positive evidence.
  With this, every field either function's own read/write sequences
  reference has been mapped or ruled out.
  **Round 10 checked `load_new_room` (the caller of `load_room`) for
  anything past its own call into it** — found reinforcement rather than
  new territory: `resolution`/`ebscene[]` each pick up further
  independent confirmations, and `+0x00`'s role gets clarified (a
  working cache of the active background bitmap, refreshed after every
  room load/resize) without its "official" identity closing. A
  legitimate "kept looking, found nothing new" result, recorded
  honestly. Struct total size still not established; still open for a
  future round.
- **Fresh survey, resolved in one round: `AmbientSound`** (2011's
  `ambient[MAX_SOUND_CHANNELS+1]`, `Common/acruntim.h:25-33`) — CONFIRMED
  ABSENT as an array: `PlayAmbientSound` (already matched) hard-locks
  `channel` to `1` via an explicit check, unlike 2011's range check, and
  every field write that would be `ambient[channel].FIELD` targets a
  bare scalar global instead (`num`/`maxdist`/`x`/`y`/`vol`, plus the
  loaded `SOUNDCLIP*` itself) — this build only ever supports one
  ambient channel, so no indexable struct was needed. Closed the loop by
  naming `update_ambient_sound_vol` (`sub_4089CC`, previously an unnamed
  lead from a much earlier GameState round) — a complete, exact match to
  source's per-channel distance-based volume falloff, giving every
  scalar global a second independent confirmation. Bonus drift: tries
  MP3 then WAV, unlike 2011's later unified loader. A useful contrast
  with `RoomStruct`'s ten-round slog — sometimes the fastest path to a
  complete answer is proving there's nothing left to map.
  **Immediate follow-up correction**: the flagged "near-duplicate MP3
  loader" open lead resolved into a real mismatch. `sub_408811`
  (`PlayAmbientSound`'s own MP3 loader) is `my_load_static_mp3`,
  confirmed via a COMPLETE field-offset match (`vol`/`mp3buffer`/
  `repeat`/`tune` all landing on `MYSTATICMP3`'s exact declared member
  order) — a much stronger standard than the call-shape-only evidence
  `sub_4083FC` (called from `PlayMusic`) had been carrying under that
  same name. Re-checked against that standard, `sub_4083FC` doesn't
  meet it: it stores `almp3_create_mp3`'s raw result directly into
  `PlayMusic`'s own stream-handle global with no wrapper object and no
  `vol`/`mp3buffer`/`repeat` fields at all — a genuinely different
  function, not a smaller variant. `sub_4083FC`'s match retracted and
  left unnamed; `sub_408811` gets the name instead. This build keeps two
  separate, near-identical MP3-loading implementations rather than one
  shared function, the same "no unified loader yet" pattern already
  found repeatedly elsewhere in this project.
- **Fresh survey: `SOUNDCLIP`/`MYWAVE`/`MYMP3`/`MYSTATICMP3`** —
  consolidated scattered evidence from several earlier rounds into
  formal struct declarations, using `Common/acsound.h` (still present in
  this repo) as a direct reference. Headline finding: this build's
  `SOUNDCLIP` base class is drastically smaller than 2011's — just
  `{vtable; int done;}` (8 bytes) vs. 2011's ~0x40 bytes of volume-
  percentage/positional-audio state — a whole abstract base class
  collapsed to its minimum, not just a smaller array or an absent field.
  `MYWAVE` (16 bytes) closes with zero drift in its own two fields
  (`wave`/`voice` match 2011's declared order exactly — an earlier,
  hastier pass at this same comment had wrongly called it drifted, fixed
  by reading the actual struct declaration instead of guessing from
  constructor-assignment order); its confirmed total size proves 2011's
  `firstTime`/`repeat` are absent — this build plays samples eagerly at
  load time, not lazily. `MYMP3` (24 bytes) shows the familiar kind of
  drift layered on the unfamiliar one: `stream`/`in`/`chunksize` all
  zero-drift, but `buffer` sits with zero gap right after `in`,
  confirming 2011's intervening `filesize` is absent. Bonus: identifies
  `almp3_create_mp3stream` as a new function match. `MYSTATICMP3` (24
  bytes) shows the base-class shrinkage forcing real structural
  adaptation — `vol`/`repeat`, which 2011 inherits from the bulky
  `SOUNDCLIP` base, had to become this build's own LOCAL fields since
  its minimal base doesn't carry them, shifting `mp3buffer` 4 bytes
  later to make room.
- **MAJOR CORRECTION: `EventBlockCmd`/`GameAnimation` (an earlier
  session's structs, explicitly documented as having "no 2011 source at
  all to anchor to") turn out to be real, still-declared 2011 structs —
  `AnimationStruct`/`FullAnimation` (`Common/acroom.h:218-232`).
  Discovered incidentally while re-reading `acroom.h` for an unrelated
  `RoomStruct` thread. Confirmed three independent ways at once: (1)
  size arithmetic — `AnimationStruct` is 5 ints + 2 chars, naturally
  padded to 24 bytes, matching `EventBlockCmd`'s own disassembly-derived
  stride with zero slack, and `10×24+4=0xF4` lands exactly on
  `GameAnimation`'s own independently-confirmed 244-byte total; (2)
  field-by-field semantics — every one of `EventBlockCmd`'s 7 fields
  (independently named/described from pure disassembly evidence in an
  earlier round) matches `AnimationStruct`'s declared fields exactly:
  `data0`→`x`, `data1`→`y`, `data2`→`data`, `target`→`object`,
  `data3`→`speed`, `type`→`action`, `waitUntilDone`→`wait` — three of
  these (`data`/`object`/`speed`) were already described in this
  project's own prior prose using almost 2011's literal field names,
  with zero knowledge at the time that a match existed; (3)
  `RoomStruct.anims[10][0xF4]` (previously a raw byte blob, matched only
  by position/size to 2011's `FullAnimation anims[MAXANIMS]` field) is
  now retyped to `FullAnimation anims[10]` directly, and its
  immediately-following `numanims` field matches 2011's declared
  adjacency with zero drift — a third, fully independent confirmation.
  ARCHITECTURAL NUANCE: the one part of the earlier claim that was
  right — 2011's own room loader no longer reads the `anims[]` payload
  on load (`acroom.h:1897-1908` reads `numanims` then `fseek`s past the
  data, the real `fread` left commented out) — the FORMAT survives to
  2011, declared and dead, but this build's own specific USE of it as a
  standalone 10-slot global "Animations" resource table
  (`unk_52024C[10]`, triggered via `EventBlock.respond[i]==4`,
  processed by two still-unnamed functions with no 2011 counterpart at
  all) remains a genuine, still-undocumented-in-2011 discovery — matches
  the "dead-but-declared by 2011, still fully live here" pattern already
  seen with `RoomStatus`/`RoomStruct`'s `hscond`/`objcond`/`misccond`.
  Renamed throughout `apply_structs.py` (fields renamed to match 2011
  exactly, per aim #3), with the old placeholder names kept visible in
  both structs' own header comments and appended (not overwritten)
  correction notes on the three affected `matches.json` entries
  (`run_event_block`, and the two still-unnamed `sub_40C3E0`/
  `sub_40C75E`), per this project's "visible retraction" convention.
- **RoomStruct's last big gap closes, plus a whole constructor cluster
  found by accident.** The single largest remaining unexplored span
  (`0xE4C`/3660 bytes, between `numwalkareas` and `numhotspots`) turned
  out to be `PolyPoints wallpoints[15]` (`Common/acroom.h:840`,
  `PolyPoints` itself at `acroom.h:252-255`) — size arithmetic
  (15×244=0xE4C) and a direct `load_main_block` fread
  (`ElementSize=0xF4`, `Count=numwalkareas`) closed it immediately, with
  `MAX_WALK_AREAS=15` matching zero drift from 2011. Chasing the two
  remaining small pads (`cscriptsize`/`bytes_per_pixel`) led to a
  previously-uncharacterized function turning out to be
  `roomstruct::roomstruct()` — this build's `RoomStruct` DEFAULT
  CONSTRUCTOR — whose body is an almost line-for-line match to source's
  own constructor, closing `cscriptsize`, reconfirming
  `bytes_per_pixel`, and independently reconfirming nearly every other
  already-established field via literal constructor defaults (also
  pinning this build's own `ROOM_FILE_VERSION`-equivalent at exactly
  14). Its three per-element array-constructor callbacks turned out to
  be even more valuable: `sprstruc::sprstruc()` (a brand new struct
  formalized this round, `sprnum/x/y/room/on`, confirms `on`@+0x08
  directly), `FullAnimation::FullAnimation()` (reconfirms `stage[]`/
  `numstages` a third way), and — the headline result —
  `AnimationStruct::AnimationStruct()`, whose ENTIRE body
  (`action=0;object=0;wait=1;speed=5;`) matches 2011's declared
  constructor word for word and value for value, decisively closing any
  remaining doubt about the previous round's `EventBlockCmd`->
  `AnimationStruct` rename. A sibling callback,
  `PolyPoints::PolyPoints()`, directly confirms `PolyPoints.numpoints`
  @+0xF0 — the one field `wallpoints`'s fread-only evidence could never
  reach (a pure AGS-editor concern with zero `Engine/` usage) — letting
  `wallpoints` be retyped from a raw byte blob to a proper typed array.
  Also caught in passing: this build's own `MAX_OBJ`-equivalent default
  is 15, not 2011's declared 16 (a genuine one-off capacity reduction).
  Five new function matches, two new formalized structs, RoomStruct's
  last big gap and both remaining small pads closed, all from following
  one dangling function reference.
- **Two quick follow-ups closed out RoomStruct's remaining loose ends.**
  `roomstruct__roomstruct`'s body, already read in full, ends right
  after `bytes_per_pixel`'s init — 2011's own constructor continues past
  that point initializing `numLocalVars`/`localvars`/
  `lastLoadNumHotspots`/`lastLoadNumRegions`/`lastLoadNumObjects` (plus
  the already-confirmed-absent `walk_area_zoom2`/`top`/`bottom` trio).
  The constructor's own completeness makes its silence on these fields
  direct evidence they're CONFIRMED ABSENT too — a third confirmation
  route for `localvars`/`numLocalVars`, and a first one for the three
  `lastLoadNum*` fields. Separately, `ebscene[]`'s still-unconfirmed
  capacity was run to ground: every one of its 6 references anywhere in
  the disassembly uses the dynamic `num_bscenes` field as a loop bound,
  never a fixed literal — matching 2011's own idiom exactly
  (`SetBackgroundFrame`/etc. bounds-check against `num_bscenes`, not
  `MAX_BSCENE`) — so this project's usual bounds-check-literal technique
  has no site to find it at. Recorded as an exhausted lead, not an open
  one; capacity stays at 2011's own `MAX_BSCENE=5` as an unconfirmed
  working estimate.
- **Pivoted to `GUIMain`'s remaining fields; `clickEventHandler` confirmed
  unused, plus a new function match.** With `RoomStruct` essentially
  exhausted, re-read `process_interface_click` (already matched)
  specifically for `clickEventHandler` — 2011's version calls
  `run_text_script_2iparam(gameinst, guis[ifce].clickEventHandler, ...)`
  when `btn<0` ("clicked the GUI background, not a control"). This
  build's version has NO such branch at all: it unconditionally decodes
  the control-type dispatch as its first action, and its caller
  (`process_event`) pushes only 2 arguments for the call, not the 3
  2011's signature needs — confirmed by the matching stack-cleanup size
  immediately after. `clickEventHandler`'s own byte offset stays
  positional-only, but its associated read code is now confirmed
  entirely absent — a genuine 2-argument predecessor that predates the
  feature. Along the way, `process_interface_click`'s other branch
  turned up a clean new match: `sub_409F23` → `run_text_script_2iparam`
  (`Engine/AC.CPP:3381`), confirmed via `prepare_text_script`/
  `ccCallInstance` calls, a distinctive `"run_text_script2: error %d
  (%s)"` string, and an exact `strnicmp(tsname,"interface_click",15)`
  match — missing only 2011's later `"on_event"`/`run_claimable_event`
  special case. Bonus: identifies `dword_523134` as `gameinst`.
- **`GUIMain.fgcol` closes via a newly-matched `wtextcolor`.**
  `_display_main` (already matched) turns out to have an entire chunk of
  2011's `draw_text_window_and_bar` inlined directly into it — another
  "one big pre-refactor function" case. One inlined branch matches
  `adjust_y_for_guis`'s role but is a genuinely simpler predecessor,
  missing 2011's `bgcol`/`bgpic` transparency check and full-height-GUI
  exclusion entirely (so no new evidence for those two fields). The other
  inlined branch does `wtextcolor(guis[ifnum].fgcol)` for custom-speech-
  GUI text color — `ifnum` being the already-confirmed `GameState.
  speech_textwindow_gui` global picking up a new reader. `sub_401F62`
  needed independent confirmation first and got a decisive one via
  `GUILabel__Draw`'s own `wtextcolor(textcol)` call (matching
  `acgui.cpp:354` exactly) — now matched as `wtextcolor` (24 call sites
  total). This upgrades `GUIMain.fgcol`@+0x50 from MEDIUM to HIGH
  confidence, the first of `GUIMain`'s remaining fields to close this
  round.
- **Two more `GUIMain` negative results, plus a `guin`/`objn`
  architectural lead.** `focus` has zero usages anywhere in 2011's own
  source — genuinely vestigial there too, not worth chasing. `guiId`
  looked promising (2011 writes it in exactly two places, both already-
  matched functions), but reading both in full end to end found neither
  write present: `GUIMain__rebuild_array` never sets `guin`/`objn` on a
  resolved object and never calls `resort_zorder()` (independently
  reinforcing the already-known z-order absence from inside its own
  body), and `read_gui`'s post-load loop goes straight from the `hit<2`
  clamp to calling `rebuild_array`, skipping every one of 2011's
  version-gated `name`/`zorder` defaults and the unconditional
  `guiId=ee`. A genuine architectural lead fell out of this: this
  project's own independently-confirmed `GUIObject.x`@+0x08 only leaves
  room for ONE 4-byte field before `x`, not the three (`guin`, `objn`,
  `flags`) 2011 declares there — suggesting `guin`/`objn` may not exist
  in this build's `GUIObject` base at all, plausibly the same later
  addition as z-order and the dynamic-GUI script-object system. Also
  confirmed: the `guiScriptObjNames`/`scrGui` startup export loop that
  would give `GUIMain.name` a promising lead doesn't exist in this build
  either — `rebuild_array` has only one caller anywhere (`read_gui`), not
  the two 2011 has.
- **`GUIMain::init()` found, closing nearly every remaining field at
  once — with a genuinely new kind of caveat.** Chasing `bgcol` further
  (its 2011 reader sites all ruled out — `wbar`, needed by
  `draw_gui_for_dialog_options`'s bgcol path, has ZERO occurrences
  anywhere in this 917k-line binary) led to reading the code around
  `GUIMain__rebuild_array` more closely. Sitting between a neighboring
  function's `endp` and `rebuild_array`'s own `proc` is a block of loose
  instructions matching source's `GUIMain::init()` (`acgui.cpp:985-1000`)
  almost line for line: 11 of 12 field assignments match exactly
  (`focus=0`, `numobjs=0`, `mouseover=-1`, `mousewasx=-1`,
  `mousewasy=-1`, `mousedownon=-1`, `highlightobj=-1`, `on=1`, `fgcol=1`,
  `bgcol=8`, `flags=0`, plus a single-byte `vtext[0]=0`) — closing or
  reconfirming nearly every remaining `GUIMain` field at once, with
  `fgcol=1` landing exactly on this same round's separate `wtextcolor`-
  based confirmation. **Genuinely new wrinkle**: this code has NO formal
  IDA function boundary at all (no `proc`/`endp`, no name, no CODE XREF)
  — it's part of a static-initializer chain that runs automatically
  before `main()`, so it can't be given a normal `matches.json` function
  entry (`apply_matches.py` needs an existing IDA name to resolve). A
  human needs to manually define the function in IDA before it can be
  named. The one asymmetry worth flagging: `clickEventHandler[0]=0`
  (present in source right next to `vtext[0]=0`) is missing here — a
  second, independent hint (on top of last round's absent reader) that
  `clickEventHandler` may not exist as a distinct field in this build.
- **`GUIObject` base class: last unconfirmed pad closes as `zorder`.**
  With `GUIMain` essentially exhausted, pivoted to the shared `GUIObject`
  base (`GUIButton`/`GUISlider`/`GUILabel`/`GUITextBox`/`GUIListBox`/
  `GUIInv` all inherit it) — a target explicitly flagged as a candidate
  two rounds ago. All six structs carried an identical unconfirmed 4-byte
  pad at `+0x18`, between `hit` and `activated`. 2011's `GUIObject::
  WriteToFile`/`ReadFromFile` read/write the whole base class as ONE
  bulk block sized `BASEGOBJ_SIZE=7` ints starting at `flags` — this
  build's own confirmed `flags`@+0x04 through `activated`@+0x1C already
  spans exactly that same 7-int range, meaning the one remaining pad
  MUST be a real field for the bulk-block argument to hold. 2011's
  declared order leaves only one candidate for that position: `zorder`
  (this build's per-CONTROL z-order, distinct from — but plausibly
  sharing the inert fate of — `GUIMain`'s own already-shown-unused
  per-GUI z-order, since `resort_zorder()` is likewise never called
  here). Retyped across all six structs at once. This closes the last
  gap in the shared `GUIObject` base layout — a fitting capstone to this
  session's `GUIObject`/`GUIMain` field-recovery arc.
- **`RoomStruct.objyval[]` closed by connecting two already-known facts,
  a new technique for this project.** A sweep for `_pad_*` gaps whose
  missing piece might already be established elsewhere under a different
  investigation thread turned up `objyval[]`: its start/element type were
  confirmed many rounds ago via a dynamic-count `fread`, leaving its
  30-byte capacity unconfirmed for lack of a known `MAX_OBJ` value — but
  that value WAS independently found several rounds later
  (`roomstruct__roomstruct`'s constructor default, `numobj=0xF=15`),
  just never cross-referenced back. 15 shorts is exactly 30 bytes, zero
  remainder. Retyped `_pad_objyval_tail[0x1E]` to `short objyval[15]`
  accordingly — worth remembering as a technique going forward. Two other
  long-open leads were revisited but didn't move: `GameSetupStructBase.
  __old_spriteflags[2100]` has zero usages anywhere in 2011's own source,
  a genuine dead end; `GameState.play_invorder[100]`'s "real member or
  coincidentally-adjacent global?" question remains undecidable by this
  project's techniques, since standalone-global and struct-member array
  accesses compile to identical code with no exploitable distinction.
- **`curscript` identified**, a small loose end left over from
  `run_text_script_2iparam`'s own match two rounds ago: the single
  dereference it does before calling `ccCallInstance` matches source's
  `ccCallInstance(curscript->inst,...)` (`Engine/AC.CPP:3281`) exactly —
  `dword_52314C` is `curscript` (a global `ExecutingScript*`, set inside
  `prepare_text_script`), and the dereference itself gives
  `ExecutingScript.inst`@+0x00 a third independent confirmation route.
- **Fresh survey, resolved in one round: `MYMIDI`** (2011's `SOUNDCLIP`-
  derived MIDI wrapper, `Engine/acsound.cpp:916-1007`, the one sibling
  in the `MYWAVE`/`MYMP3`/`MYSTATICMP3`/`MYMIDI` family the earlier
  SOUNDCLIP round hadn't covered) — CONFIRMED ABSENT as a wrapper
  object, matching `AmbientSound`'s own "bare globals instead" pattern.
  `PlayMusic` (already matched, previously undocumented despite being
  correctly named — a retroactive-documentation case) calls Allegro's
  `load_midi`/`play_midi` directly, gluing results into three bare
  globals: `dword_5231B4` (the raw `MIDI*` handle, checked non-NULL by
  five already-matched functions as the "is MIDI active" gate),
  `dword_4BD8F8` (Allegro's own `midi_pos`), and `dword_5231BC` (a
  write-only "current MIDI music number?" hypothesis, not asserted).
  Five new Allegro function matches (`load_midi`/`play_midi`/
  `stop_midi`/`destroy_midi`, all via exact signature/call-shape checks
  against `allegro/midi.h`), plus field evidence newly added to four
  previously-bare-linker-matched functions (`scr_StopMusic`/
  `IsMusicPlaying`/`GetMIDIPosition`/`SeekMIDIPosition`).
- **Immediate follow-up: `MYMOD` closes the same way.** `MYMIDI`'s
  resolution made its JGMOD-based sibling an obvious next check.
  `load_mod`/`play_mod` were already matched from an earlier Task #10
  round; re-reading `PlayMusic`'s `.mod`/`.xm`/`.s3m` cascade and
  `scr_StopMusic`'s cleanup branch confirms explicitly what that
  evidence already implied: no `MYMOD` wrapper object exists here either
  — `dword_5231B8` is the bare `JGMOD*` handle, checked/cleared by the
  same five-function pattern already established for MIDI. Three new
  matches fall out (`is_mod_playing`, `stop_mod`, `destroy_mod`, all
  identified by call-shape/role since no JGMOD source tree exists in
  this repo to check names against). Two SOUNDCLIP-family siblings down;
  `MYOGG`/`MYSTATICOGG` remain as the last untouched pair, plausibly
  absent given OGG is likely a later addition than MP3.
- **`MYOGG`/`MYSTATICOGG` confirmed absent — the cleanest result of the
  four.** A search for "ogg" (any case) across the full extracted string
  dataset AND a direct pass over the entire 917k-line disassembly found
  ZERO occurrences either way — the same exhaustive-negative standard
  already used to rule out `apeg-1.2.1`/`dumb-0.9.2`. This build has no
  Ogg Vorbis support at any level: no wrapper objects, no loader
  functions, and unlike `MYMIDI`/`MYMOD` (absent as wrapper objects, but
  built on real, present, already-matched libraries), not even the
  underlying `vorbisfile`/`ogg.h` dependency appears to have existed yet
  at this build's 2002-07-21 link date — closing the loop on this
  project's much earlier finding that speech loading tries MP3 then WAV
  with no OGG attempt in between. All four `SOUNDCLIP`-family siblings
  surveyed this session are now accounted for.
- **`InventoryItemInfo.name[25]` closes via `GetInvName`.** A sweep for
  other structs' `_unconfirmed`/positional-only fields with an
  obvious-but-untraced reader turned up `name[25]` — its own comment
  already named `GetInvName` as the likely candidate, just never
  followed up. `GetInvName` (already correctly named, but only a bare
  mechanical linker match with zero field evidence) reads `invinfo[].name`
  starting at the array's own base address and passes it straight to
  `GetTranslation`, matching 2011's implementation almost verbatim —
  confirming `name[25]`@+0x00 directly and cross-confirming the array's
  base address a third independent way against `pic`@+0x1C. Only
  `cursorPic`@+0x20 remains open in this struct now.
- **`MoveList.xpermove`/`ypermove` close via `do_movelist_move`.** Same
  sweep, immediate next hit: these two fixed-point fields sat at MEDIUM
  confidence (boxed in with zero slack, no access site of their own)
  since `find_route`'s own round — `find_route` computes the route once,
  but the per-frame consumer is `do_movelist_move` (already matched),
  called every frame by `update_stuff`. Its opening block reads both
  fields via an identical `onstage`-indexed pattern at +0xA4/+0x144,
  matching source's single line `xpermove=cmls->xpermove[cmls->onstage],
  ypermove=cmls->ypermove[cmls->onstage];` exactly — and goes on to
  re-derive `pos[onstage+1]` the same way, a bonus reconfirmation from a
  different function than the one that first confirmed it. `MoveList` is
  now fully behaviorally confirmed field by field.
- **A self-caught correction: `sub_41D49B` is `run_dialog_script`, not
  `run_dialog_request`.** Chasing `DialogTopic.entrypoints[]`'s own
  predicted reader in `do_conversation` led to reading the FULL body of
  a function previously matched to `run_dialog_request` on a genuine but
  incomplete string match. It's actually a byte-code dispatch loop
  reading `dtpp->optionscripts + offse` one opcode at a time —
  `run_dialog_script` itself — with `run_dialog_request`'s entire body
  fused in as just one opcode case (a decisive line-for-line match,
  including its exact `DIALOG_NEWTOPIC`/`DIALOG_STOP` literal
  comparisons). `run_dialog_request` has no separate existence in this
  build at all. Both of `do_conversation`'s call sites into this
  interpreter confirm the correction and, as a bonus, both
  `DialogTopic.entrypoints[15]`/`.startupentrypoint` fields at once.
  Renamed with the old name kept visible per the usual retraction
  convention; the interpreter's full opcode set is only partially traced
  (0/6/7 so far), left open for a future round.
- **The full `DCMD_*` opcode table, plus a bonus `CharacterInfo.talkview`
  confirmation.** An immediate follow-up read `run_dialog_script` to
  completion: its opcode byte is 2011's own `DCMD_*` dialog-script
  byte-code set (`Common/acroom.h:2653-2669`) — still declared in the
  reference source even though 2011's engine no longer uses the format
  at all (it now compiles dialog topics into real script bytecode
  instead). 13 of 16 declared opcodes are confirmed handled here exactly
  matching their `DCMD_` role (`SAY`/`OPTOFF`/`OPTON`/`RETURN`/
  `STOPDIALOG`/`OPTOFFFOREVER`/`RUNTEXTSCRIPT`/`GOTODIALOG`/`PLAYSOUND`/
  `ADDINV`/`SETSPCHVIEW`/`NEWROOM`/`ENDSCRIPT`); any other opcode byte
  hits an explicit `quit()`, proving the switch exhaustive and
  confirming the 4 unhandled `DCMD_*` constants (`SETGLOBALINT`/
  `GIVESCORE`/`GOTOPREVIOUS`/`LOSEINV`) absent by direct positive
  evidence. Bonus: the `SETSPCHVIEW` handler decisively confirms
  `CharacterInfo.talkview`@+0x04, a TENTATIVE positional-only guess
  since a much earlier round — it was right all along.

**Detour: the `ags-archives/` resource, and pinning Rob Blanc 1 to AGS
2.4b (July 2002).** The user added official AGS release archives
(versions 2.00-3.30+, docs-only through ~2.7x). Cross-referencing this
project's own already-confirmed findings against `CHANGES.TXT`'s dated
version entries pinned the binary's actual engine version tightly (see
"Rob Blanc 1's AGS version" near the top of this file for the summary,
`reversing/notes/ags-archives-cross-reference.md` for the complete
writeup) and independently DATED roughly a dozen of this project's own
findings via version-numbered changelog entries: `IsMusicVoxAvailable`
(added 2.4b, the lower bound), `MYOGG`/`MYSTATICOGG` absence/
`MAXTOPICOPTIONS=15`/`DCMD_SETGLOBALINT` absence (all pre-2.5, the upper
bound), `GameSetupStructBase.spriteflags[6000]` ("Increase limit to 6000
sprite slots", 2.4), `GameState.globalscriptvars[300]` ("Upped
GlobalInts to 300", 2.22/Dec 2001), `DCMD_ADDINV`/`DCMD_SETSPCHVIEW`
(2.22/Dec 2001), `DCMD_NEWROOM` (2.3/Jan 2002), `GUIMain.zorder`/
`GUIObject.guin`/`.objn`/`SetGUIClickable` absence (all added 2.6/Dec
2003, over a year later), and `RoomStatus.hotspot_enabled[20]`'s
capacity (reconciles exactly with 2.3's "Upped limit to 19 hotspots" --
19 usable + hotspot 0 reserved = 20 slots). Separately, `ags240/docs/
TECHINFO.TXT`'s official `.CHA` CHARACTER FILE format (dated 26 December
2001, contemporary with Rob Blanc 1) independently confirms
`CharacterInfo`'s `defview`/`talkview`/`view`/`room`/`x`/`y`/
`animspeed`/`name`/`scrname` field layout byte-for-byte with zero
contradiction, and `ags261/docs/TECHINFO.TXT`'s `.DLG` DIALOG FILE
format confirms `DialogTopic`'s field order/types (a later-era
declaration, but same lineage). This resource is now a standing
reference for dating "was this feature present" questions going
forward -- check `CHANGES.TXT` before assuming a gap needs fresh
disassembly work.
- **`CharacterInfo.prevroom` closes**, a follow-up to the `ags-archives/`
  detour: `load_new_room` (already matched) does `offsetx=0; offsety=0;
  forchar->prevroom=forchar->room; forchar->room=newnum;` right near the
  top of the function, gated on `forchar!=NULL` -- matching source
  (`AC.CPP:4429-4432`) instruction for instruction.
- **`CharacterInfo.loop` closes too**, via `update_stuff`'s "turning
  around before walking" branch (a 2.3-era feature, per `ags-archives/`)
  -- a complete match to source's `AC.CPP:6526-6558`: reads `loop`@+0x38
  as the sole argument to a newly-matched `find_looporder_index`
  (`sub_40EB43`), validates the result against a newly-identified global
  `turnlooporder[8]={0,6,1,7,3,5,2,4}` table (`dword_4B42C8`) and the
  already-confirmed `flags`/`CHF_NODIAGONAL`, then writes the result
  back into `loop`@+0x38 as the sole target — while also reconfirming
  `walking`, `view`, `flags`, `wait`, and `animspeed` in the same pass.
  `CharacterInfo` is now fully confirmed field by field except `actx`/
  `acty` (already shelved as a likely later addition) — one of the most
  thoroughly-confirmed structs in the whole project.
- **`ViewStruct272.numloops` closes almost by accident**, connecting
  evidence already on record from `CharacterInfo.loop`'s own round:
  `update_stuff`'s "turning around" branch reads `views[view]+0` (no
  added offset) — that IS `numloops` itself. `MoveList.direct` got a
  real but incomplete answer: `move_object`'s own body (already fully
  read) ends immediately after `moving=mslot` with no further write,
  unlike 2011's `mls[mslot].direct=ignwal;` right after it — genuine
  negative evidence for this one call site, but 2011 has at least one
  other write site not yet checked, so not treated as fully confirmed
  absent. **`RoomStatus.obj[10]` got the biggest upgrade**: its own
  `load_new_room` initialization loop (already partially cited for a
  single `RoomObject.transparent` write) turns out to write NINE
  separate fields per iteration, matching 2011's own `croom->obj[cc]`
  init block field for field — no longer an arithmetic fit, direct
  behavioral proof. The same "evidence already on record, just never
  connected" pattern as `objyval[]`/`MAX_OBJ` a few rounds back.
- **`RoomStatus.flagstates[]` closes the same way, right next door.**
  Reading a little further past the `obj[]` initialization loop turned
  up the last open `RoomStatus` field for free: immediately after the
  object-init loop, `load_new_room` does `for(chaa=0;chaa<0xF(15);
  chaa++) [croom+chaa*2+0x148]=0` — a direct, literal loop matching
  2011's `for(cc=0;cc<MAX_FLAGS;cc++) croom->flagstates[cc]=0;`
  (`AC.CPP:4308`) exactly, confirming both position and capacity in one
  shot rather than an arithmetic remainder. The code right after that
  does three more `rep movsd` block copies reconfirming `misccond`/
  `hscond[20]`/`objcond[10]` from the exact same pass. `RoomStatus` now
  has no remaining MEDIUM-confidence fields at all.
- **`MoveList.direct`@+0x1FD closes as CONFIRMED ABSENT, and
  `RoomStruct.flagstates`'s standing hypothesis gets corrected.** Two
  loose ends closed in the same round. First, `RoomStruct.flagstates`'s
  comment had guessed it was "copied into the per-save-slot
  `RoomStatus.flagstates` on first visit," mirroring the established
  `hscond`/`objcond`/`misccond` source-copy pattern — but the round
  above already proved `RoomStatus.flagstates` is populated by an
  unconditional zero-reset, not a copy, so there's nothing to copy from.
  A check of 2011's own `Engine/` source reinforces this independently:
  `thisroom.flagstates` has ZERO usages anywhere in the reference build
  either — genuinely dead weight even in 2011, not just unfound here.
  Retracted in place, field stays MEDIUM (position/arithmetic still
  solid). Second, `MoveList.direct` — left at "one negative site found,
  two more to check" the previous round — closes completely: reading
  `MoveCharacterDirect` shows it's a thin wrapper calling
  `walk_character(...,ignwal=1,...)` (this build unifies "direct" and
  "avoid walls" through one shared function, same as `move_object`
  already showed for objects), and neither `walk_character`'s own body
  nor its multi-stage-route helper (`sub_40EB7B`, ~420 lines) ever
  writes the offset. `NewRoom`'s `inside_script` branch — 2011's third
  write site, a "nasty hack" — turns out to be a genuine simpler
  predecessor with no such hack at all here, just a plain store into the
  already-confirmed `ExecutingScript.newnum`. All three of 2011's known
  write sites checked, none present — confirmed absent by the same
  exhaustive-multi-site standard used for the `DCMD_*` opcode table.
- **`GUIButton`'s long-standing "minimum size" caveat closes.** Its
  field list (`vtbl` through `rclickdata`) had carried an explicit note
  since the original vtable-recovery round that it was a MINIMUM size
  only, since 2011 declares more trailing fields
  (`textAlignment`/`reserved1`/`eventHandlers[]`) read/written
  individually rather than via the bulk fread/fwrite calls the recovery
  was based on. Reading `GUIButton::ReadFromFile`/`WriteToFile`
  (already matched) in full end to end — not just their three known
  bulk calls — shows each is a tiny, fully linear function (one
  early-exit branch for a `textcol` default) that does exactly those
  three calls and then returns, with no fourth call and no offset past
  `+0x80` ever touched by either. `sizeof(GUIButton)==0x84` (132 bytes)
  is now positively confirmed rather than a lower bound, and 2011's
  trailing fields are CONFIRMED ABSENT — the usual "later AGS addition"
  pattern. A check of the other five `GUIObject`-derived structs found
  none carrying an equivalent open-tail caveat.
- **`InventoryItemInfo.cursorPic` confirmed absent, via a smoking-gun
  comment in the 2011 reference source itself.** Re-reading
  `SetInvItemPic` (already matched) end to end shows it does ONE
  unconditional write (`pic`@`+0x1C` only), no `pic==piccy` early-return
  check, no second-field sync branch. 2011's own `set_inv_item_pic`
  (`Engine/AC.CPP:5262-5278`) explains exactly why: `"if
  (game.invinfo[invi].pic == game.invinfo[invi].cursorPic) { //
  Backwards compatibility -- there didn't used to be a cursorPic, so if
  they're the same update both. set_inv_item_cursorpic(invi, piccy); }"`
  — 2011's own source documents `cursorPic` as a later addition, kept in
  sync with `pic` purely for save-compatibility with builds from exactly
  this era. Reinforced by an exhaustive search: no
  `set_inv_item_cursorpic`/`InventoryItem::SetCursorGraphic`-equivalent
  function or export string exists anywhere in the binary. `pic` alone
  serves both roles here — closing `InventoryItemInfo`'s last open field.
- **`RoomStruct`'s `+0x00` mystery gets a plausible unifying explanation,
  still not a confirmation.** A structural asymmetry not called out
  before: `load_room`'s `BLOCKTYPE_ANIMBKGRND` loop (scenes 1..
  num_bscenes-1, already matched) reads/writes `ebscene[c]` DIRECTLY at
  `+0x3A0C+c*4` for every scene, no staging through `+0x00` anywhere;
  only scene 0's own load (inside the separate `load_main_block`) routes
  through `+0x00` first and copies the result into `+0x3A0C` afterward.
  Combined with `load_new_room`'s already-known `+0x00`-refresh-from-
  `ebscene[0]` behavior, this fits a tidy story: `+0x00` is a fast-access
  cache of "the currently displayed background," and only scene 0 (drawn
  every frame by default) has any reason to keep it synchronized — the
  other, less-frequently-shown animated frames don't. Still short of a
  confirmation: no drawing function has been found reading from `+0x00`
  itself. Recorded as the most coherent explanation tying together three
  rounds of evidence, not a new confirmation of the field's identity.
- **Found the missing drawing-code reader — and a correction: it reads
  `ebscene[]` directly, not `+0x00`.** `RawSaveScreen`/`RawRestoreScreen`/
  `RawDrawImage` (previously mechanically linker-matched with zero field
  evidence) all fetch the active background via
  `dword_523094[dword_4EEB58*4]` — `dword_4EEB58` is the confirmed
  `GameState.bg_frame`, and `dword_523094` turns out, via a decisive
  zero-slack arithmetic chain (`dword_523088`/`52308C`/`523090`/`523094`
  sit at four consecutive +4-byte offsets, landing exactly on the
  already-confirmed `num_bscenes`@+0x3A00/`bscene_anim_speed`@+0x3A04/
  `bytes_per_pixel`@+0x3A08/`ebscene[0]`@+0x3A0C), to BE `ebscene[0]`
  itself — just accessed via IDA's own auto-generated standalone-global
  name, since `rstruc`'s applied IDB type doesn't extend far enough for
  IDA to resolve it as struct-relative. Matches 2011's own `RAW_START`
  macro (`"abuf=thisroom.ebscene[play.bg_frame]"`, `AC.CPP:14355`)
  exactly — a fifth independent confirmation of `ebscene[]`'s offset.
  The catch: this is exactly the drawing-code reader last round went
  looking for, and it reads `ebscene[bg_frame]` STRAIGHT from
  `+0x3A0C+bg_frame*4`, never touching `+0x00`. Correcting last round's
  theory in place: the "read by drawing code" support for `+0x00`'s
  cache theory doesn't hold up — the actual drawing code bypasses it
  entirely. `+0x00`'s identity is no worse off than before, but that
  specific supporting claim is retracted.
- **`ccFreeScript` found, closing a long-open "candidate for a future
  round."** `RoomStruct.compiled_script`'s cleanup helper (`sub_42A4DB`)
  had sat unmatched since the round that first found it. Reading it in
  full: an exact, line-for-line match to `ccFreeScript(ccScript*)`
  (`Common/cscommon.cpp:116`) — conditional frees of `globaldata`/`code`/
  `strings`/`fixuptypes`/`fixups`, a zero-out of all five, a loop freeing
  non-null `imports[]` entries, then a loop freeing EVERY `exports[]`
  entry unconditionally (source has no null check there either — an
  easy-to-miss detail that confirms genuine identity, not just a generic
  pattern), then zeroing `numimports`/`numexports` and returning. 2011's
  own version keeps going for another dozen lines: a `numSections` loop
  and explicit frees of the `imports`/`exports`/`export_addr` ARRAY
  POINTERS themselves. This build has neither — confirming `numSections`/
  `sectionNames`/`sectionOffsets` are absent from this build's `ccScript`
  entirely, and independently reinforcing (via the missing array-level
  frees) the already-suspected drift that `imports[600]`/`exports[600]`/
  `export_addr[600]` are fixed embedded arrays here, not 2011's
  separately-`malloc`'d dynamic ones. `compiled_script` is retyped from
  a placeholder `void*` to a proper `ccScript*` now that its destructor
  is known.
- **`main_loop_until` found, plus four new globals and a genuine
  architecture difference.** `GameState.disabled_user_interface`'s
  writer (`sub_40C395`) had been flagged as "plausible role match, not
  yet individually confirmed." Reading it in full: matches
  `main_loop_until(int,int,int)` (`AC.CPP:4664-4676`) almost line for
  line, including a neat confirmation of its unused third parameter —
  `do_main_cycle` (already matched) calls it with a literal `0` for
  `mousestuff`, matching source's own call exactly, even though this
  build's function body never reads that argument at all. Identifies
  four new globals via literal-constant cross-checks:
  `dword_4EDA7C`=`cur_mode`, `dword_523180`=`restrict_until`,
  `dword_523158`=`user_disabled_data`, `dword_523154`=`user_disabled_for`
  (the last via `do_main_cycle`'s own `=3` matching `FOR_EXITLOOP=3`) —
  with `restrict_until`/`user_disabled_data` independently reconfirmed a
  second way via `wait_loop_still_valid`'s own UNTIL_MOVEEND/
  UNTIL_CHARIS0 checks. Two drift points recorded: `do_main_cycle` here
  skips 2011's `EndSkippingUntilCharStops()` call and nested-context
  save/restore (a simpler, non-reentrant version), and — more
  interesting — this build fuses 2011's separate `main_game_loop()`/
  `wait_loop_still_valid()` into one: `wait_loop_still_valid`'s own body
  opens with a call to `mainloop()` before its condition checks, which
  is why `do_main_cycle` can poll it alone in a tight loop instead of
  2011's `while(main_game_loop()==0);`.
- **`wait_loop_still_valid`'s full body closes, plus a bonus
  `run_animation` rename.** Immediate follow-up on the previous round's
  own open lead: every remaining `UNTIL_*` branch (`NEGATIVE`/
  `NOOVERLAY`/`INTIS0`/`SHORTIS0`) matches source exactly, and
  `UNTIL_ANIMEND`(1) having no explicit case in EITHER build (both fall
  through to the same unknown-event quit) turns out to be a genuine
  match, not a gap. The "end restrict_until" cleanup resolves last
  round's open loop-polarity question — `do_main_cycle`'s
  `while(!wait_loop_still_valid());` exits exactly when `restrict_until`
  clears AND `user_disabled_for==FOR_EXITLOOP`(3, set by `do_main_cycle`
  itself), returning `-1`, fully consistent with the caller. The
  headline: the `FOR_ANIMATION`(1) branch calls
  `run_animation(user_disabled_data2,user_disabled_data3)` — matching
  2011's OWN commented-out dead code (`"/* if(user_disabled_for==
  FOR_ANIMATION) run_animation((FullAnimation*)user_disabled_data2,
  user_disabled_data3); */"`, `AC.CPP:25723-25725`), the only place that
  name and call shape survive anywhere in 2011's source. This is a
  second, independent caller of this project's own previously-
  deliberately-unnamed `AnimationStruct`/`FullAnimation` command-list
  iterator (whose only other known caller was `run_event_block`'s
  `respond[i]==4` dispatch) — clearing the bar for a real rename where
  there previously wasn't a 2011 name to use. Two more globals fall out:
  `user_disabled_data2`/`user_disabled_data3`. `FOR_SCRIPT`'s error
  string also turns out to differ from 2011's wording (`"err: user_dis:
  FOR_SCript"` vs. 2011's `"...obsolete (v2.1 and earlier only)"`) —
  this build's message doesn't call it obsolete, since here it isn't.
- **`run_graph_script` found: a whole ancient AGS subsystem 2011 has
  completely forgotten.** Chasing the previous round's dangling
  `sub_41CDC3` lead (the "other unmatched caller" of `run_animation`)
  led to `respond[i]==0Ah`(10) in `run_event_block`, which — cross-
  referenced against `whataction[]`'s own documentation comment already
  on file from an earlier `RoomStruct` round (`Common/acroom.h:89-104`)
  — is exactly `GRAPHSCRIPT`(10), "v1.00 SR-1: Run graphical script".
  This feature has NO trace anywhere else in 2011's source beyond that
  one comment line — no function, no struct, no string, nothing. This
  build still runs it in full. Named directly from its own four
  self-identifying error strings (`"run_graph_script: ..."`/`"Run_
  Graph_script: ..."`) since no 2011 body exists to compare against at
  all. Full behavior: builds a temp filename `"~acsc%d.tmp"`, validates
  a script/block version header, loads a 254-slot table of 254-byte
  command records (4-byte count + up to ten 25-byte commands), and runs
  slot 0 through a recursive 26-opcode interpreter (`sub_41CDC3`, left
  unnamed — nothing supplies IT a specific name). Five opcodes read so
  far: `NewRoom`, `GiveScore`, `StopMoving`, an unhandled slot, and —
  notably — the same `run_animation` iterator already shared by
  `run_event_block` and `wait_loop_still_valid`, now with a third
  independent caller.
- **An immediate follow-up closed all 26 graph-script opcodes, plus two
  long-open `GameState` fields.** The full table (see
  `reversing/notes/struct-layout-drift.md` for the complete listing)
  covers room changes, score, dialog, sound/FLIC playback, inventory,
  and a full set of conditional "if flag/random/timer/inventory-used,
  run a nested list" branches. Opcode 10 (RUN_SCRIPT) calls into AGS's
  classic fixed exported function `"gscript_request"`, confirmed via
  that literal string. Opcodes 11/12 (SET_FLAG/CLEAR_FLAG) validate
  their flag number is outside `[15,100)`, erroring with a second
  independent `"!graph_script: ..."`-prefixed string; tracing the
  shared getter/setter helpers shows flags 0-14 are the already-
  confirmed `RoomStatus.flagstates[15]`, while flags ≥100 hit a
  previously-unknown standalone global array with no other
  reader/writer anywhere. Opcodes 23/24 (SET_TIMER/IF_TIMER_EXPIRED)
  and 26 (IF_USED_INVENTORY_ITEM) supply the first individually-
  confirmed instructions for two long-"?"-flagged `GameState` fields
  from the very first `GameState` survey rounds — `gscript_timer`@+0x0C
  and `usedinv`@+0xE0 — both upgraded to HIGH confidence.
- **Two quick wins from a fresh self-identifying-error-string sweep.**
  `script_SetTimer`/`isTimerExpired` (the generic script Timer API, distinct
  from the graph-script-specific timer closed last round) were already
  correctly named in the IDB but had thin-to-nonexistent matches.json
  entries; both match `AC.CPP:21172-21187` exactly, giving
  `GameState.script_timers[21]`@+0x838 a second/third confirmation route
  and reconfirming `MAX_TIMERS=21`. Separately, `DisplayMessage`'s
  "global message" branch (`msnum>=500`) reads `dword_51CB50[msnum]` with
  no visible `-500` subtraction — resolved by `dword_51D320`
  (`messages[500]`'s own confirmed base) minus `dword_51CB50` landing
  exactly on `500*4`, zero slack: the compiler folded the `-500` offset
  into the base address at compile time, and IDA gave the folded address
  its own unrelated-looking label — the same pattern already seen with
  `ebscene[]`/`dword_523094`. A third independent confirmation for
  `messages[500]`.
- **`add_to_sprite_list`/`clear_sprite_list` found, plus a new
  `SpriteListEntry` predecessor struct.** The same error-string sweep
  turned up a genuine 2002-era typo doubling as naming evidence:
  `sub_4106EF`'s overflow-quit string reads `"ad_to_sprite_list: roo
  many sprite added"`. Matches `add_to_sprite_list`
  (`Engine/AC.CPP:7441-7470`), the per-frame list that orders objects
  and characters by baseline before drawing; both call sites sit inside
  `prepare_characters_for_drawing` (already matched), one passing the
  already-confirmed `RoomObject.transparent`@+0x08 straight through. A
  companion two-instruction function matches `clear_sprite_list()`
  verbatim and identifies `sprlistsize`. The function's own 5 field
  writes land on 5 separate-looking globals exactly 4 bytes apart (the
  same "IDA doesn't recognize the struct" pattern seen twice already
  this session) — formalized as `SpriteListEntry` (`bmp`/`baseline`/
  `x`/`y`/`transparent`, 20 bytes). DRIFT: 2011's `hasAlphaChannel`/
  `takesPriorityIfEqual` fields are confirmed absent, and this build's
  own overflow limit (39) is roughly half of 2011's
  `MAX_SPRITES_ON_SCREEN=76` — the usual capacity-increase pattern,
  here for the first time on a per-frame runtime list rather than a
  save-data array.
- **The flagged `add_to_sprite_list` offset asymmetry resolves as
  ordinary bookkeeping, not a puzzle.** Reading the surrounding loop
  body in full: `var_10`/`var_28` (its `x`/`y` arguments) are computed
  once, already `offsetx`/`offsety`-subtracted (room-space converted to
  screen-space up front). The sibling `sub_410631` call runs only on
  the walk-behind-aware sort path (`RoomObject.flags&
  OBJF_NOWALKBEHINDS` clear, already documented on `flags` itself), and
  adds the offsets back because IT needs original room-space
  coordinates for walk-behind occlusion, while `add_to_sprite_list`
  wants the already-converted screen-space values everything else uses.
  Corrected in place, no longer an open lead.
- **A genuine surprise: the live IDB's `rstruc.FIELD` symbolic display
  is shifted one position early.** Investigating `sub_410631` (the
  walk-behind-occlusion helper from last round) found it reading the
  field DISPLAYED as `rstruc.lookat` to look up `RoomStatus.
  walkbehind_base[]` — but a walk-behind helper reading the hotspot
  mask made no sense. A four-way cross-check resolved it: `sub_40AD11`
  (newly matched as `redo_walkable_areas`, confirmed via `GameState.
  walkable_areas_on[]`) can only touch the walkable-areas mask, yet
  displays as `rstruc.object`; `sub_410631` can only touch the walk-
  behind mask, yet displays as `rstruc.lookat`; `get_hotspot_at`
  (already matched) can only touch the hotspot mask, yet displays as
  `rstruc.regions`. All three land exactly one field position early
  relative to this project's own load-order-confirmed declaration —
  and the fourth slot resolves perfectly too: `rstruc.walls`
  (displayed) is exactly the already-documented `+0x00`
  `ebscene[0]`-cache line from several rounds ago. This project's own
  struct declarations needed no correction — the opposite, they're now
  confirmed via usage for the first time, not just load order. The live
  IDB's own `roomstruct` type was simply never re-applied after the
  earlier round-5/6 offset correction; it needs a fresh
  `apply_structs.py` run and re-export to catch up.
- **`animate_character` fully documented, retroactively confirming
  eight `CharacterInfo` fields.** Already correctly named in the IDB
  but with zero field evidence recorded. Matches `AC.CPP:14774-14805`
  almost line for line, touching `view`/`idleleft`/`idletime`/
  `walking`/`animating`/`loop`/`frame`/`wait` in one pass — none
  needed to change (all already HIGH confidence), but each gets a
  fresh confirmation route, and `animating` picks up its first
  confirmed bit value: `CHANIM_REPEAT=2`. A retroactive-documentation
  round, same spirit as earlier ones for `SpriteCache`/`GUIMain::init`.
- **A new (if inconclusive) argument for `play_invorder`'s `GameState`
  membership.** `add_inventory` (also retroactively documented this
  round) treats `play_invorder[]` and the already-confirmed
  `GameState.inv_numorder`@+0xEC as one atomic, always-synchronized
  pair — `play_invorder[inv_numorder]=inum; inv_numorder++;`. This
  doesn't resolve the standing "genuine member or coincidentally-
  adjacent global?" question (a struct member and a standalone global
  still compile identically — the underlying limitation is real), but
  it's a genuinely different kind of evidence than the positional-
  adjacency approach that closed earlier fields: behavioral coupling
  with a confirmed member, mirroring 2011's own successor field
  (`obsolete_inv_numorder`) staying synchronized with its own order
  array. Recorded as a real, if inconclusive, case FOR membership.
- **`SetObjectIgnoreWalkbehinds` matched, naming `is_valid_object` along
  the way.** Already correctly named but with no matches.json entry.
  Matches `AC.CPP:20911-20919` exactly (minus a trailing hardware-cache
  invalidation line this build predates), giving `RoomObject.flags` bit
  1 (`OBJF_NOWALKBEHINDS`) a clean third confirmation. Its object-number
  validation helper (`sub_4256E0`, previously left unnamed) turns out to
  be an exact match for 2011's `is_valid_object` — named accordingly,
  with a small self-correction: the earlier entry's paraphrase of its
  return polarity was backwards (it returns 1 for VALID, not invalid).
- **`GetTime` documented, confirming a later `TIME_YEAR` addition
  absent here.** Already correctly named, no matches.json entry. A
  simpler predecessor of 2011's `sc_GetTime` (`AC.CPP:15447-15462`) —
  calls `time()`/`localtime()` directly and reads `struct tm*` fields
  by raw offset instead of going through a `ScriptDateTime` wrapper,
  matching source's `1=hour/2=minute/3=second/4=day/5=month` dispatch
  exactly. 2011 additionally handles `whatti==6` (year); this build's
  dispatch only checks 1–5, confirming year support absent — the usual
  "later AGS addition" pattern, closing out this round's error-string
  sweep.
- **`CharacterInfo.actx`/`.acty` found after all — a self-caught
  correction, closing the struct completely.** A much older round had
  shelved these as "checked and not found," reasoning 2011's only usage
  site sits deep inside hardware-accelerated drawing code this build
  predates. That conflated two different things: the SURROUNDING code
  had drifted (true), but the field ASSIGNMENT itself hadn't been
  checked independently of it (turned out to still be there).
  `add_to_sprite_list`'s match a few rounds ago supplied the anchor:
  `prepare_characters_for_drawing` calls it, then immediately writes
  `[chin+0x10C]`/`[chin+0x10E]` via two 16-bit `mov`s — matching 2011's
  `chin->actx=atxp+offsetx; chin->acty=atyp+offsety;` (`AC.CPP:
  8523-8526`) exactly, call order included. The pointer is independently
  confirmed as `CharacterInfo*` via `baseline`/`y`/`flags` read from it
  moments earlier. Both fields upgrade to HIGH confidence — `CharacterInfo`,
  one of the most heavily-worked structs in the whole project, now has
  no remaining open fields at all.
- **`getpixel`/`putpixel` identified, closing out this session's own
  mask-reading helper trail with a Task #10-style boundary.** The small
  pixel-access helpers behind `redo_walkable_areas`/`sub_410631`/
  `get_hotspot_at` turned out to be genuine Allegro library code, not
  AGS-side functions. `sub_423F20`/`sub_423EC0` dispatch through the
  bitmap's own vtable at consecutive slots — Allegro's public
  `getpixel`/`putpixel`. `sub_425490`/`sub_425450` are a different,
  lower-level pair — Allegro's own 8-bit-specific fast path, used only
  by `redo_walkable_areas`, matching that function's own 2011 source
  comment verbatim ("since this is an 8-bit memory bitmap, we can just
  use direct memory access") — its choice of the faster path is
  deliberate and source-documented. Per this project's third-party
  scope rule, recorded at the boundary only — the fast path's own
  line-lock/unlock callees aren't chased further.
- **`run_on_event`'s full event-type table closes, correcting a stale
  "called only from new_room" claim.** An exhaustive grep finds EIGHT
  call sites, not one. Cross-referenced against 2011's `GE_*` constants:
  1=`GE_LEAVE_ROOM`(new_room), 2=`GE_ENTER_ROOM`(process_event),
  3=`GE_MAN_DIES`(run_event_block), 4=`GE_GOT_SCORE`(GiveScore),
  5/6=`GE_GUI_MOUSEDOWN`/`MOUSEUP`(check_controls, two call sites),
  7=`GE_ADD_INV`(add_inventory), 8=`GE_LOSE_INV`(LoseInventory). The
  ninth, `GE_RESTORE_GAME`, is CONFIRMED ABSENT — all eight sites are
  accounted for and `restore_game_data` never calls this function at
  all. `GiveScore` (newly given field evidence) confirms `GameState.
  score`@+0x00 a further way (the disassembly's own `play` global IS
  `score`, since it's the struct's first field). `LoseInventory`
  (previously undocumented entirely) turns out to be `add_inventory`'s
  mirror image for the `play_invorder` question: it shifts every later
  `play_invorder[]` entry down by one on removal — decisively confirmed
  as `play_invorder[i]=play_invorder[i+1]` via the two globals' own
  adjacent addresses in the disassembly — giving the "synchronized
  pair" argument a second, independent instance alongside
  `add_inventory`'s.
- **`process_event`'s full `EV_*` dispatch table closes with zero
  drift.** Already matched via two of its own self-identifying error
  strings, but thinly evidenced. Its switch on `EventHappened.type`
  matches all five of 2011's declared `EV_*` constants exactly, with no
  gaps and no additions — `1=EV_TEXTSCRIPT`, `2=EV_RUNEVBLOCK`,
  `3=EV_FADEIN`, `4=EV_IFACECLICK`, `5=EV_NEWROOM` — falling through to
  the function's own "unknown event to process" quit for anything else,
  proving it exhaustive. Pins down `EventHappened`'s leading fields:
  `type`@+0x00, `data1`@+0x04, `data2`@+0x08, and a fourth field@+0x0C
  whose exact role is left for a future round. A small correction along
  the way: last round's `GiveScore` note flagged `byte_513337` as
  unidentified — it was already confirmed as `GameSetupStructBase.
  options[1]` (`OPT_SCORESOUND`) several sessions ago, just not
  cross-referenced back into `GiveScore`'s own entry.
- **`EventHappened` formalized — the fourth field closes, a fifth turns
  up for free, zero drift throughout.** 2011's own `check_new_room()`
  hand-constructs one specific event right next to the comment "run
  Player Enters Screen and on_event(ENTER_ROOM)":
  `evh.data3=5; evh.player=game.playercharacter;` — matching
  `process_event`'s own `data1==EVB_ROOM && data3==5` check exactly,
  and handing over a bonus fifth field (`player`) along the way. Both
  decisively confirmed via `setevent` (already matched, zero field
  evidence until now): it writes its four arguments plus
  `game_playercharacter` into five separate-looking globals at a shared
  stride — formalized as `EventHappened` (`type`/`data1`/`data2`/
  `data3`/`player`, 20 bytes). Capacity closes with genuinely ZERO
  drift: `setevent`'s own overflow check matches 2011's `MAXEVENTS=15`
  exactly — the second struct this session with no capacity reduction
  at all.
- **`EventHappened.data3`'s full value space closes, plus a bonus
  `TS_KEYPRESS` event and a `RoomStruct` edge-boundary confirmation.**
  `check_controls`'s own room-edge-crossing detector computes a 0-3
  edge index from `playerchar->x`/`y` vs. four boundary globals, then
  calls `setevent(EV_RUNEVBLOCK,EVB_ROOM,0,edge)` — matching 2011's
  `edgesActivated[]` loop exactly. This closes `data3`'s full value
  space for `EVB_ROOM` events: 0-3=edge crossed, 5=player-enters-screen
  (already confirmed). The four boundary globals turned out to be
  `RoomStruct.left`/`right`/`top`/`bottom` accessed via absolute address
  — the same stale-label pattern already found on `RoomStruct`'s
  leading fields, now extending to these too, with zero drift. A bonus
  `TS_KEYPRESS`(2) text-script event surfaced in the same code region,
  alongside the already-confirmed `TS_REPEAT`(1).
- **A mislabeled function found and fixed, and `EventHappened.data3`'s
  value space closes exhaustively.** A function auto-named
  `_EVP_PBE_cleanup` — a real OpenSSL symbol, no plausible reason to
  exist in this codebase — turned out to be a FLIRT false positive: its
  three-instruction body matches 2011's `update_events()` verbatim.
  Renamed accordingly, with the mismatch documented in case the same
  signature misfires elsewhere. Its own helper is `processallevents`,
  an exact role match minus 2011's defensive `copyOfList`/`memcpy` step
  (a later safety addition this build predates). Separately,
  `check_controls` and `mainloop` supplied the last missing pieces of
  `EventHappened.data3`'s value space — `TS_MCLICK`(3, completing all
  three `TS_*` constants) and `EVB_ROOM` values `4`/`7` (pre/post-fadein
  room-entry triggers). Every `EVB_ROOM` `data3` value 2011 declares
  anywhere is now individually confirmed present with zero drift and
  zero gaps: `0-3` edges, `4` pre-fadein entry, `5` enters-screen, `6`
  repeatedly_execute, `7` post-fadein entry.
- **`fade_interpolate` found, and `process_event`'s own `EV_FADEIN`
  branch turns out to do real work.** `sub_40A21C` (the other unmatched
  caller of the MP3-crossfade check from last round) is this build's
  own AGS-side palette-fade-over-N-steps helper — no clean 2011
  counterpart exists since 2011's `my_fade_out`/`my_fade_in` delegate
  entirely to `gfxDriver->FadeOut()`. Its own inner call, `sub_43C8A0`,
  is genuine Allegro library code: a six-argument signature and a
  64-step weighted-palette-blend algorithm matching Allegro's public
  `fade_interpolate()` exactly — named accordingly and recorded at the
  library boundary. The bonus: `fade_interpolate` has a SECOND caller,
  `process_event`'s own `EV_FADEIN` branch, confirming that branch
  performs genuine per-frame palette interpolation for the room-entry
  fade-in rather than delegating anywhere — another small confirmation
  that this build's whole rendering pipeline predates `gfxDriver`.
- **The full `FadeOut` call chain closes, plus two more Allegro
  public-API matches.** `sub_40A358` (`FadeOut`'s own helper, called
  with the literal args `(speed,0,0xFF)` — the full palette range)
  captures the current screen palette then hands it to `sub_40A21C`
  (last round's fade-step helper) to interpolate down to `unk_553CC0` —
  a 1024-byte buffer never written anywhere in the binary, i.e. an
  implicitly BLACK palette purely from BSS zero-init, not an explicit
  constant. Two more Allegro functions identified along the way:
  `get_palette` (a thin wrapper calling `get_palette_range(p,0,255)`,
  matching Allegro's own header) and `get_palette_range` itself (checks
  a video-driver vsync callback, then copies from Allegro's own internal
  current-palette global) — the latter also independently confirmed via
  a second caller, `PlayFlic`, grabbing the screen palette before
  switching to the FLIC's own. `sub_40A358` gets no 2011-derived name,
  same reason as its own callee — `FadeOut` delegates to `gfxDriver`
  entirely in 2011, leaving nothing to compare this build's manual
  implementation against.
- **A self-correction: `dword_52321C` isn't a plugin hook, it's
  `speechmp3`, plus `PlaySound` closes its own single-channel
  predecessor.** Last round's `sub_40A21C` entry guessed `dword_52321C`
  was "a generic plugin-hook-style function pointer" from the shape of
  its check alone, without tracing where it's actually assigned.
  Reading its real setter (inside `play_speech`, already matched) shows
  it's the already-confirmed `speechmp3` handle — corrected in place,
  wrong guess kept visible. Lesson: a plausible access pattern isn't
  identity evidence; always trace the pointer to its real origin.
  Separately, `dword_523220` (the neighbor that prompted the check)
  turns out to be `PlaySound`'s own sound-effect channel. `PlaySound`
  itself was already correctly named but had no `matches.json` entry —
  it's a genuine standalone predecessor of 2011's `PlaySoundEx`, with
  matching validation logic but one piece of ancient AGS history baked
  in that 2011 has since removed: `"if(val>=1000) { PlayMusic(val-1000)
  ; return; }"` — sound numbers ≥1000 redirect to music, a historical
  quirk with zero trace in `PlaySoundEx`. Single-channel throughout,
  predating the later multi-channel design entirely.
- **The full per-frame audio-polling picture completes: three single-
  channel systems, one shared pattern.** A small helper, `sub_425230`
  (a one-line null-check `__thiscall` method), kept appearing in
  `mainloop`/`FadeOut`/`sub_40A21C` right alongside the `speechmp3`/
  sound-effect polling already identified. It turns out to be checking
  `dword_4EDA58` — this build's own single ambient-sound handle,
  already confirmed several sessions ago. This completes a satisfying
  picture: this build's per-frame audio polling is three entirely
  separate single-channel systems (speech, sound effects, ambient
  sound), each checked the exact same way (null-check, call vtable slot
  0 if set) from otherwise-unrelated call sites — the "later AGS
  versions generalized several independent 2002 globals into one
  array-based system" pattern, here observed across a whole subsystem
  rather than one struct. `sub_425230` stays unnamed — too generic and
  trivial to confidently pin to a specific 2011 identifier.
- **A GUI script-API sweep confirms the `GOBJ_*` enum with zero drift.**
  `SetSliderValue`/`GetSliderValue`/`SetTextBoxText`/`GetTextBoxText`/
  `SetLabelText` (all previously bare linker-symbol matches with no
  field evidence) share one validation skeleton — bounds-check
  `guin`/`objn`, call the already-matched `GUIMain::get_control_type`,
  compare its result against a literal constant — and each literal
  matches `Common/acgui.h:655-660`'s declared `GOBJ_*` values exactly:
  `SetSliderValue`/`GetSliderValue` check `==4`(`GOBJ_SLIDER`),
  `SetTextBoxText`/`GetTextBoxText` check `==5`(`GOBJ_TEXTBOX`),
  `SetLabelText` checks `==2`(`GOBJ_LABEL`) — 3 of 6 constants now
  confirmed via live script-API dispatch, all zero drift. Bonus finds:
  a previously-undocumented 190-character text-length validation
  constant shared by `SetTextBoxText`/`SetLabelText` (10 bytes under
  `GUITextBox`/`GUILabel`'s own confirmed 200-byte `text[]` capacity),
  and further reconfirmation routes for `GUISlider.min`/`.max`/`.value`
  and `GUITextBox`/`GUILabel.text`@+0x20 (all already HIGH confidence).
- **An immediate follow-up closes two more `GOBJ_*` constants.**
  `is_valid_listbox` (already matched, its enum angle never previously
  cited) checks `==6`, confirming `GOBJ_LISTBOX=6`. `SetButtonPic`
  (previously bare) checks `==1` (`GOBJ_BUTTON`), and its own 3-way
  `ptype` dispatch (NORMAL/OVER/PUSHED) exercises ALL SIX of
  `GUIButton`'s picture/state fields (`pic`/`overpic`/`pushedpic`/
  `usepic`/`ispushed`/`isover`) via genuine matching BEHAVIOR — each
  branch's conditional `usepic` refresh depends on `isover`/`ispushed`
  — the strongest confirmation route any of the six has had, previously
  each only individually confirmed via a single `MouseDown`/`MouseUp`
  site. Bonus: `check_controls`'s own internal mouse-click/scroll-wheel
  dispatch independently re-derives `==1`(`GOBJ_BUTTON`) and
  `==4`(`GOBJ_SLIDER`) a second way each, and `SetLabelFont` (also
  previously bare) checks `==2`, a second confirmation of `GOBJ_LABEL=2`
  and of `GUILabel.font`@+0xE8. `GOBJ_INVENTORY=3` remains the only
  constant with no individually-read confirmation site — left open
  rather than guessed. A case-insensitive search for
  `"inventory window"`/`"InvWindow"` (the kind of error-string wording a
  dedicated validator would use) finds zero matches anywhere in the
  disassembly — not conclusive, but consistent with `GUIInv`'s already-
  established minimal footprint here (no `charId`/`itemWidth`/
  `itemHeight`/`topIndex`, only the shared `GUIObject` base persisted).
  Also retroactively documented in full: `SetActiveInventory` (bare
  before, despite `CharacterInfo.activeinv`'s own comment already citing
  its `-1` branch) — the general-case path validates `iit` against
  `inv[iit]@+0x44>=1` (player must own the item) and writes
  `activeinv@+0x34=iit` directly, a third confirmation route.
- **`SetScreenTransition` closes `GameState.fade_effect`'s valid range,
  and `process_event`'s dissolve effect gets characterized.**
  `SetScreenTransition` (previously bare) validates its argument in
  `[0,2]`, not 2011's `[0,FADE_LAST=4]` (`Common/acroom.h:2753-2758`) —
  cross-checked against `process_event`'s own `EV_FADEIN` dispatch
  (already matched), which likewise has branches only for
  `fade_effect==1`(instant)/`==0`(normal)/`==2`(dissolve) with nothing
  for higher values, CONFIRMING `FADE_BOXOUT`(3)/`FADE_CROSSFADE`(4)
  absent both at the API boundary and the runtime dispatch — this
  build's fade-effect enum genuinely spans only 3 values, not 2011's 5.
  The `FADE_DISSOLVE` branch itself is a substantial manual
  implementation with no 2011 counterpart to diff against (2011
  delegates it entirely to `gfxDriver`): 16 passes over a 4-pixel grid,
  each revealing 1-of-16 sub-positions per 4x4 block via a fixed order
  table and `getpixel`/`putpixel` (two further call sites for those
  already-matched Allegro functions) — a classic pre-hardware-
  acceleration checkerboard dissolve, documented at the level of
  confirmed existence/rough operation rather than pixel-exact fidelity.
- **`SetTalkingColor` shows `CharacterInfo.talkcolor` isn't a standalone
  field — it's packed into `flags`' top byte.** Validates its color arg
  to `[0,0xFF]` then does `flags=(flags&0x00FFFFFF)|((ncol<<24)&
  0xFF000000)`, a decisive match to `Common/acroom.h:3013`'s own
  documented `OldCharacterInfo`→`CharacterInfo` upgrade code: `"ci->
  talkcolor=(oci->flags&OCHF_SPEECHCOL)>>OCHF_SPEECHCOLSHIFT"`
  (`OCHF_SPEECHCOL=0xff000000`, `acroom.h:2498-99`). 2011 itself
  documents this packing as the OLD pre-refactor layout — this build's
  `CharacterInfo` (already matching `OldCharacterInfo` with zero drift)
  still uses it live, not as a save-compat artifact. No 33rd field to
  find: the struct's already-known `0x140` total size was correct all
  along, `talkcolor` was hiding inside an already-mapped field's byte.
  Immediate follow-up: `SetCharacterIgnoreLight` (also previously bare)
  confirms `CHF_NOLIGHTING=0x20` on the same `flags` field, zero drift,
  with no collision against the `talkcolor` byte (`OCHF_SPEECHCOL`
  occupies bits 24-31, clear of every declared `CHF_*` bit).
- **A data-hygiene fix: found and removed three duplicate `matches.json`
  entries.** `RunCharacterInteraction`/`SetDialogOption`/`GetInvName`
  each had a stale bare `"kind":"function"` mechanical entry sitting
  alongside a fuller `"kind":"manual"` entry that had already superseded
  it — exactly the risk this file's own workflow section warned about
  (`build_matches.py` can re-add a bare entry without checking for an
  existing manual one first). All three removed with no information
  loss (`matches.json`: 620 → 617 entries). Also fully closed
  `SetDialogOption`'s remaining logic: a complete match to 2011's own
  body decisively confirms `DFLG_ON=1`/`DFLG_OFFPERM=2`
  (`Common/acroom.h:2648-2649`) with zero drift, and reconfirms
  `DialogTopic`'s `0x484`(1156)-byte size a further way.
- **Two new `Common/Wgt2allg.h` matches from a `CyclePalette`/
  `SetPalRGB` sweep, plus a genuine feature-absence finding.**
  `FollowCharacter`/`SetGUIPosition` (both bare) get retroactively
  documented (the latter giving `GUIMain.x`@+0x28/`y`@+0x2C a WRITE-side
  confirmation). `CyclePalette`/`SetPalRGB` yield two brand-new matches:
  `wsetrgb` and `wcolrotate` (AGS's own small platform-compat header,
  not third-party). `wcolrotate`'s match reveals this build's version
  implements ONLY 2011's `dir==0` ("rotate left") branch — the `dir`
  argument is pushed by every caller but never read in the function
  body, so the "rotate right" branch isn't dead code, it was never
  written. Tracing to `CyclePalette` (its only caller) confirms the
  whole chain: no forwards/backwards branch, no bounds-check `quit()`,
  and no hi-color `invalidate_screen()` call — three features confirmed
  absent in one small function. `SetPalRGB` shows the same missing
  hi-color check at its own call site.
- **`SetPlayerCharacter` shows a genuinely simpler predecessor of 2011's
  `Character_SetAsPlayer`-delegating version.** This build inlines just
  four steps (save old room, update `playercharacter`, swap `playerchar`,
  call `update_invorder`, conditionally `NewRoom`); 2011 wraps that same
  core in `Character_SetAsPlayer` (`Engine/acchars.cpp:1011-1043`) with
  a same-character no-op check, a `displayed_room<0`/game-start guard,
  a `GetRegionAt` same-room fallback, and `activeinv`/cursor revalidation
  — all CONFIRMED ABSENT here. Corrects an earlier struct-comment note
  that had overstated the match as "exactly" matching source.
- **`ShakeScreen` closes cleanly, plus a new `Common/Wgt2allg.h` match.**
  Its screen-capture call, made right after setting `abuf=screen`, is
  decisively `wnewblock` (new match) — the capture step matters, since
  `wnewblock` specifically blits FROM the current `abuf`. Its 40-pass,
  50ms-delay shake loop matches 2011 with zero drift, alternating
  `wputblock(0,0,Block,0)`/`wputblock(0,severe,Block,0)` — calling the
  already-matched `wputblock` DIRECTLY, with no `render_to_screen`
  wrapper or `gfxDriver` layer in between. A third independent instance
  this session (after `FadeOut`/`FADE_DISSOLVE`) of this build's entire
  rendering pipeline predating the later hardware-acceleration
  abstraction wholesale, not just in fade-related code.
- **`GameState.disabled_user_interface` upgraded to fully confirmed,
  plus a stale comment fixed.** Its comment cited "an as-yet-unmatched
  helper" as its only evidence, but that helper had been matched to
  `main_loop_until` (which increments this field) several rounds ago
  without the comment being updated. `DisableInterface`/
  `EnableInterface`/`IsInterfaceEnabled` (all previously bare) confirm
  it's a genuine NESTING COUNTER, not a boolean — increment/decrement-
  with-clamp/`==0`-read — backed by four independent call sites now
  instead of one stale reference.
- **Two quick wins from a fresh self-identifying-error-string sweep.**
  `SetCharacterProperty` (previously bare) is a generic caller-supplied-
  bitmask setter on `CharacterInfo.flags`; 2011's equivalent
  (`Character_SetOption`) redirects `CHF_MANUALSCALING` to a different
  function as an explicit "backwards compatibility fix" — CONFIRMED
  ABSENT here, this build applies the bitmask uniformly to every flag.
  `SetCharacterSpeed` (also bare) validates the already-confirmed
  `walking`@+0x3C and writes only `walkspeed`@+0x40 — its single write
  target confirms 2011's `walkspeed_y` (independent X/Y walk speed,
  `Character_SetSpeed`) is absent from this build's `CharacterInfo`
  entirely, and no `SetCharacterSpeedEx` entry point exists either.
- **`PauseGame`/`UnPauseGame`/`GetCursorMode` close trivially;
  `InterfaceOff` cross-confirms a vtable slot and finds a real bounds-
  safety gap.** The first three are one-or-two-instruction wrappers
  around already-established globals. `InterfaceOff`'s turn-off body
  calls `objs[mouseover]->vtbl[2]()` then `mouseover=-1` — vtable slot 2
  was already established as `MouseLeave` (the original `GUIButton`
  vtable round), so this cross-confirms both `GUIMain.mouseover`@+0x54
  and that vtable ordering from an independent site, and confirms
  `control_positions_changed()` absent (a later addition). Its tail
  confirms `POPUP_SCRIPT=2` (alongside the known `POPUP_MOUSEY=1`) —
  but INLINES `game_paused--` directly instead of calling the already-
  matched `UnPauseGame()`, skipping its `>0` guard: a real, narrow
  correctness gap where this path could drive `game_paused` negative.
- **`RoomObject.on` picks up two confirmed bit values, plus a new
  `MergeObject` architectural lead.** `RemoveWalkableArea`/
  `RestoreWalkableArea` close trivially (exact matches, further
  confirmation routes for `walkable_areas_on[]`/`redo_walkable_areas`).
  `ObjectOff`/`ObjectOn` are near-mirrors, but `ObjectOff` is missing
  2011's `StopObjectMoving(obn)` call entirely — confirmed absent, and
  no such function exists in this build at all. `MergeObject`
  decisively confirms `on`'s third value: it sets `on=2` unconditionally
  (no guard, unlike `ObjectOff`/`ObjectOn`'s own checks), matching
  2011's own inline comment (`"don't change it if on==2 (merged)"`)
  and its own final write. Bonus reconfirmations of `RoomObject.x`/`y`/
  `num` (the last cross-indexing into `spriteheight[]`) from a new
  site, plus two drift points: this build's merge-draw call takes 3
  args where 2011's `draw_sprite_support_alpha` takes 4 (omitting the
  num/alpha argument), and 2011's trailing `invalidate_screen()`/
  `mark_current_background_dirty()` calls are confirmed absent — another
  instance of this build's pipeline predating that later invalidation
  machinery. `array1` (plausibly this build's `actsps[]`) and two
  drawing helpers (`sub_410AFA`/`sub_410771`) are left unidentified by
  name, candidates for a future round.
- **`SetAreaLightLevel` closes `walk_area_light[]`'s valid range, and
  the room-tint-override absence gets a second confirmation.** Its
  write target, `word_522F2E[area]`, is decisively
  `RoomStruct.walk_area_light[]`@+0x38A6 (already confirmed via `fread`,
  now also confirmed by write) via `0x522F2E-0x38A6=0x51F688` landing
  exactly on `thisroom`'s own established base address. DRIFT: this
  build validates `area` in `[0,14]`, narrower than 2011's own
  `MAX_REGIONS=16` bound check (the array's own 16-slot capacity is
  unaffected, zero drift). More interesting: 2011's second write,
  clearing a region's `TINT_IS_ENABLED` bit, is CONFIRMED ABSENT here —
  a second, independent confirmation (after the `rtint_*` globals and
  `SetAmbientTint`'s own absence) that the room-tint-override subsystem
  doesn't exist in this build at all.
- **`TintScreen` closes cleanly, plus a new `MoveToWalkableArea` signature match.**
  `TintScreen`'s whole body (the disabled/all-zero branch and the
  `*25/10`-scale-then-pack branch) is an exact, instruction-for-
  instruction match to 2011 — minus a leading `invalidate_screen()`
  call, CONFIRMED ABSENT, joining several other functions this session
  showing the same predates-hardware-invalidation pattern.
  `MoveToWalkableArea` calls `sub_40AE7D(&x,&y)`, matching 2011's
  `find_nearest_walkable_area(int*,int*)` signature exactly — called
  directly, without 2011's intervening `Character_PlaceOnWalkableArea`
  wrapper. `sub_40AE7D`'s own ~90-line body (plausibly that function
  fused with its own `_within` helper) is left untraced, an honest open
  lead rather than a forced identification.
- **`GetSaveSlotDescription`/`RestoreGameSlot` correct `restore_game_data`'s
  own signature and reveal it's dual-mode, not a heavier predecessor.**
  This build's `restore_game_data` takes `(int slotNumber, char
  *descriptionOrNull)`, builds the save filename and opens it itself —
  fusing 2011's separate `load_game()` (open+header) and
  `restore_game_data(FILE*,...)` (deserialize) into one function.
  Better: it's genuinely dual-mode — a `Destination!=NULL` early-exit
  path reads only the description and returns, never touching the
  full-state deserialization; only `GetSaveSlotDescription` passes a
  real buffer, every other caller passes `NULL` for a full restore. Not
  a behavioral regression, just both 2011 roles fused behind one
  argument. `RestoreGameSlot` also gives `ExecutingScript.ooo`@+0x0C (the
  pending-restore-slot field, previously only confirmed from its
  consumer side) a WRITE-side confirmation: writes `slnum` there
  directly when called mid-script, deferring the restore — matching
  2011's role but predating its unified `postScriptActions[]` queue.
- **`RestartGame` found with no `matches.json` entry at all**, despite
  being correctly named in the live IDB and repeatedly cited by OTHER
  functions' own entries for several sessions. Closes cleanly against
  2011's `restart_game()` — confirms `RESTART_POINT_SAVE_GAME_NUMBER=999`
  with zero drift, and (like `RestoreGameSlot`) defers via its own
  dedicated `ExecutingScript`+0x64 field rather than 2011's unified
  queue. That field's write turns out to be a genuine INCREMENT, not a
  set-to-1 — a small correction to its earlier "unnamed bool"
  description. DRIFT: 2011's `can_run_delayed_command()` call is
  confirmed absent.
- **A small sweep turns up one real behavioral drift.** `IsGamePaused`/
  `GetGlobalInt` close as exact matches (the latter reconfirming
  `globalscriptvars[300]` from a new site). `UpdatePalette` is the
  interesting one: 2011 gates its palette-apply call behind BOTH a
  hi-color `invalidate_screen()` check AND `!play.fast_forward` — this
  build has NEITHER guard, calling `set_palette_range` unconditionally
  even during fast-forwarded sequences. Unlike most "confirmed absent"
  findings this session, this one is player-visible: this build keeps
  re-applying palette changes during fast-forward that 2011 explicitly
  skips — worth flagging for the eventual ScummVM port rather than
  "fixing" by default.
- **`RemoveOverlay` found with no `matches.json` entry despite being
  fully covered by an earlier correction.** Its 2-line body is a clean
  match to 2011 (`find_overlay_of_type` + `remove_screen_overlay`, both
  already matched). Worth noting: the currently-exported `.asm` still
  displays the second call as `stop_fast_forwarding` — the STALE, pre-
  correction IDA label from several rounds ago (that correction to
  `remove_screen_overlay` was discovered by reading this exact call
  site; the IDB rename just hasn't been re-exported since). Recorded
  explicitly so a future session doesn't re-trust the stale text.
  `CreateTextOverlay` also closes cleanly, with zero drift on every
  constant checked (`OVR_AUTOPLACE=30000`, default width/color/
  centering logic) and a new global identified: `dword_4B42E8`=
  `crovr_id`. One open detail: its `_display_main` call passes only 7
  arguments vs. the 10-parameter signature established elsewhere —
  plausibly compiled-in C++ default arguments, not confirmed this round.
- **`SetTextOverlay`/`MoveOverlay` close the overlay-script-API sweep
  with zero drift.** `SetTextOverlay` — remove-then-recreate-with-same-
  ID via the already-matched `RemoveOverlay`/`CreateTextOverlay`,
  `crovr_id` included — matches 2011 line for line. `MoveOverlay`
  writes newx/newy directly to `ScreenOverlay.x`@+0x08/`y`@+0x0C
  (already established via `add_screen_overlay`'s creation-time
  writes), a further WRITE-side confirmation of both fields from a
  genuinely different (runtime-mutation) call site. `RemoveOverlay`/
  `CreateTextOverlay`/`SetTextOverlay`/`MoveOverlay` are now all fully
  documented.
- **`SetVoiceMode`/`IsVoxAvailable`/`SetSpeechStyle` close cleanly;
  `SetSpeechVolume` identifies `SOUNDCLIP`'s first vtable slot; a new
  `GetTextDisplayTime` name closes `DisplaySpeechBackground`.**
  `SetVoiceMode`/`IsVoxAvailable` are exact matches confirming
  `GameState.want_speech`'s negative-means-unavailable convention.
  `SetSpeechStyle` confirms `options[OPT_SPEECHTYPE]` from a new write
  site. `SetSpeechVolume` calls the already-established `speechmp3`
  handle's vtable SLOT 2 with the new volume — matching 2011's
  `channels[SCHAN_SPEECH]->set_volume(newvol)` exactly and newly
  identifying that slot, the first `SOUNDCLIP` virtual method actually
  named in this project. `DisplaySpeechBackground` gives the packed
  `talkcolor` byte a THIRD independent confirmation, closes the
  previously-medium-confidence `sub_4136AF` lead as `GetTextDisplayTime`
  via a second call site, and confirms 2011's per-character background-
  speech cleanup logic absent — unsurprising since `ScreenOverlay.
  bgSpeechForChar` was already known structurally absent, but this is
  the first behavioral confirmation of that gap.
- **`SetMusicVolume`/`SetMusicMasterVolume`/`SetSoundVolume` close, with
  a real validation-range drift and a second `SOUNDCLIP` vtable
  confirmation.** `SetMusicVolume` writes to `RoomStruct.
  options[ST_VOLUME]` (confirmed via absolute-address arithmetic
  landing exactly on `thisroom`'s base) — but validates `[1,5]`, not
  2011's `[-3,5]`: the negative "quieter than room default" range is
  confirmed absent at this API boundary. `SetMusicMasterVolume` closes
  with zero drift, reconfirming `music_master_volume`'s `+60` formula
  from a new write site. `SetSoundVolume` writes `GameState.
  sound_volume`, then — if a sound effect is currently playing — calls
  its channel's vtable slot 2 directly, a SECOND independent
  confirmation that slot 2 is `set_volume(int)` (now confirmed across
  two different `SOUNDCLIP` instances, not just two call sites on one).
  DRIFT: 2011's two `Game_SetAudioTypeVolume` calls (a later audio-type
  volume system) are absent — this build just nudges the one already-
  playing channel directly.
- **A quick, clean sweep of four `RoomObject` getters.**
  `GetObjectX`/`GetObjectY`/`IsObjectAnimating`/`IsObjectMoving` (all
  previously bare) close as exact matches to 2011 with zero drift and
  no new fields — a documentation-only round, useful mainly for
  shrinking the remaining bare-match pool. Noted for future sessions:
  the exported `.asm`'s own comments at these `is_valid_object` call
  sites still show the stale pre-rename `sub_4256E0` text, the same
  re-export lag already flagged for `stop_fast_forwarding`/
  `remove_screen_overlay` — trust `matches.json` over the `.asm` text
  at these specific sites until the next re-export.
- **`SetObjectGraphic` shows a genuine three-field behavioral gap.**
  This build writes `RoomObject.num` and clears `cycling` only — 2011's
  version additionally resets `frame=0`/`loop=0`/`view=-1`, fully
  disconnecting the object from any view-based animation when its
  graphic changes directly by sprite number. All three resets are
  CONFIRMED ABSENT here. Flagged rather than "fixed": whether this is
  player-visible depends on how the drawing code treats a stale
  `frame`/`loop`/`view` once `num` is set directly, not traced this
  round. `IsGUIOn` closes cleanly alongside it, zero drift.
- **The old bounding-box collision system found**:
  `AreObjectsColliding`/`AreCharObjColliding`/`AreCharactersColliding`
  (all previously bare) have no useful 2011 counterpart to diff against
  — 2011's own versions are one-line delegations into a later, hardware-
  accelerated pixel-overlap system this build doesn't have. This build
  instead does a plain axis-aligned bounding-box test using each
  object/character's position and current sprite's width/height (via
  the already-established `spritewidth[]`/`spriteheight[]` globals),
  characterized at the algorithm-shape level since there's no 2011
  source to verify individual instructions against. Bonus: confirms
  `RoomObject.on==1` as a collision precondition, and surfaces an open
  lead — `AreCharObjColliding` short-circuits on a character's room
  against a bare global displayed as `newnum` (NOT the already-
  confirmed `ExecutingScript.newnum` field), plausibly `displayed_room`
  but not confirmed by name.
- **Immediate follow-up resolves the `newnum`/`displayed_room` lead.**
  Reading `load_new_room`'s own opening instructions shows the exact
  same 3-step sequence, in the exact same order, as 2011's own opening
  (`set_color_depth(8)` — a new Allegro match — then `displayed_room=
  newnum`, then the `"room%d.crm"` filename `sprintf`). The bare global
  `AreCharObjColliding` reads (previously just "displayed as `newnum`,
  plausibly `displayed_room`") is now decisively confirmed — a pure
  naming coincidence between the local parameter and the global it gets
  copied into, not evidence by itself, but the surrounding instruction
  order settles it.
- **`scrWait`/`WaitKey`/`WaitMouseKey` close together, sharing one real
  validation-absence drift.** All three write `GameState.wait_counter`/
  `key_skip_wait` then call the already-matched `do_main_cycle`. The
  three `key_skip_wait` write values (`0`/`1`/`3`) match 2011's own
  convention exactly, the first individual-assignment confirmation for
  that field (previously only confirmed via a `>1` comparison).
  Shared drift: 2011's leading `if(nloops<1) quit(...)` bounds check is
  CONFIRMED ABSENT from all three — calling any of them with `0` or a
  negative loop count has no guard against it in this build.
- **`RawDrawLine`/`RawDrawTriangle` close as near-exact Allegro-backed
  matches; `RawPrint` shows this build predates a targeted 2011
  bugfix.** The first two draw onto `ebscene[bg_frame]` with the
  already-confirmed `raw_color`, matching 2011 with zero drift except
  for the now-familiar absent `raw_modified[]`/`invalidate_screen()`
  calls. `RawPrint` calls `wtextcolor(raw_color)` directly — but 2011's
  own source deliberately avoids exactly that call, with a comment
  explaining it causes a 16→32 color conversion bug, plus a dedicated
  hi-color-on-8-bit warning branch. Neither the workaround nor the
  warning exists here — this build predates the bug's discovery, not
  just a later feature. Two bonus matches: `wtexttransparent` (exact
  `TEXTFG=0` argument match) and `wouttext_outline` (call-shape only,
  its own body not traced).
- **`FlipScreen` gives `screen_flipped` its first write confirmation;
  `IsSoundPlaying` is missing fast-forward awareness.** `StringToInt`/
  `StrGetCharAt` close trivially, zero drift. `FlipScreen` writes
  `GameState.screen_flipped` directly (previously confirmed only via
  its startup zero-init), exact match otherwise. `IsSoundPlaying`
  checks only the established single sound-effect channel (the usual
  single-channel-predecessor shape) — but 2011's leading
  `if(play.fast_forward) return 0;` guard is confirmed absent, so this
  build can report a sound playing during a skipped/fast-forwarded
  sequence where 2011 always says no — a second confirmed instance of
  fast-forward-awareness missing from a function 2011 later added it
  to (after `UpdatePalette`'s own earlier finding).
- **`DisplayAtY` identifies two new globals and a second
  `disabled_user_interface`-bracketing absence.** `Display`/`DisplayAt`
  close cleanly (the latter reinforcing the "compiled-in default
  arguments" theory for `_display_at`/`_display_main` from an earlier
  round). `DisplayAtY` confirms a new function, `GetMaxScreenHeight`,
  and a new global, `screen_is_dirty` — but the
  `disabled_user_interface++`/`--` bracketing 2011 wraps around its
  `mainloop()` call is confirmed absent here, the second such gap found
  this session (after `InterfaceOff`'s). Also decisively confirms
  `GameSetupStructBase.options[9]`=`OPT_ALWAYSSPCH`, distinct from the
  adjacent already-confirmed `options[10]`=`OPT_SPEECHTYPE`.
- **`DisplaySpeech`/`DisplaySpeechAt`: the richest architectural pair
  this session.** This build's `DisplaySpeech` still has genuine
  variadic printf-style formatting built in — 2011's own version is a
  trivial fixed-2-parameter wrapper with no `...` at all, and its
  `_displayspeech` callee has a commented-out translation line noting
  "the strings are pre-translated," consistent with 2011 having moved
  formatting elsewhere entirely. Bigger find: `DisplaySpeechAt` calls
  `_display_at` (the plain positioned-text-box function `DisplayAt`
  uses) instead of `_displayspeech` (the real portrait-based speech
  system `DisplaySpeech` itself uses) — passing the character's talk
  color as a text-color argument. A colored-text-box approximation of
  speech, not the real thing — and 2011's own source still carries a
  "doesn't use the right speech style" comment on this exact function,
  a suggestive (not confirmed) connection to this build's own simpler
  predecessor implementation.
- **`ProcessClick`'s walk-mode branch gives `hswalkto[]` a read-side
  confirmation, and is missing one real gate.** Only the opening
  `MODE_WALK` early-return branch was read at first (CORRECTION, see
  next bullet: the rest of the function turned out to be short, not
  the large continuation initially assumed). Confirms
  `options[12]`=`OPT_NOWALKMODE` and gives `RoomStruct.hswalkto[]` its
  first read-side confirmation. Gap: 2011 additionally gates the walk-
  to-point override behind `play.auto_use_walkto_points==0`; this
  build's branch applies a valid walk-to point unconditionally —
  confirmed absent from this call site only, not searched for
  elsewhere in `GameState` yet.
- **Immediate follow-up closes `ProcessClick` entirely, resolving the
  missing-`GetLocationType` question.** The rest of the function past
  the walk-mode branch is short: try `check_click_on_character` (new
  match), then `check_click_on_object` (new match), then fall back to
  a fresh `get_hotspot_at()` + `RunHotspotInteraction`. This build's
  `ProcessClick` never calls `GetLocationType`/caches a loctype at all
  — a genuinely different, more ad-hoc try-each-handler dispatch than
  2011's unified compute-once-then-switch design, not a smaller
  version of the same mechanism.
- **`new_room` closes cleanly — a suspected bug turns out to be a name
  collision, plus one real confirmed feature absence.** Its
  `run_on_event(1,newnum)` call looked at first like it passed the
  wrong room (the one being entered, not left) — resolved by noticing
  the function's OWN local parameter is also auto-named `newnum` and
  displays as bracketed `[ebp+newnum]` when actually accessed (as it
  correctly is, later, for the `load_new_room` call); the `run_on_event`
  call itself reads the BARE, unbracketed `newnum`, which is the
  already-established standalone global == `displayed_room` — so this
  build's call is `run_on_event(GE_LEAVE_ROOM,displayed_room)` after
  all, matching 2011 exactly, no bug. Real drift found instead: 2011's
  `new_room()` (`AC.CPP:4625-4644`) saves the target room into a global
  `in_leaves_screen` before firing leave-room events and re-reads it
  after, letting an OnRoomLeave-equivalent script redirect the actual
  destination room — CONFIRMED ABSENT here, no such write/re-read
  exists anywhere in this build's `new_room`.
- **`evblockbasename`/`evblocknum` globals renamed** — a third instance
  of the "identified in prose, never actually pushed to the IDB" gap
  this session's fresh IDB update already found and fixed twice
  (`run_dialog_request`→`run_dialog_script`,
  `stop_fast_forwarding`→`remove_screen_overlay`). `String1`@0x4EF37C
  and `iii`@0x51F684 renamed directly to match 2011's own globals of
  the same names. `evblocknum` picks up a second confirmation site:
  `check_controls` turns out to have 2011's entire
  `RunInventoryInteraction(iit,modd)` (`AC.CPP:5616-5633`) fused
  inline rather than called separately — `iii=iit` matches
  `evblocknum=iit;` (`AC.CPP:5620`) exactly.
- **`MergeObject`'s two open leads close: `construct_object_gfx`/
  `put_sprite_256`, plus `actsps`/`trans_mode`.** `sub_410AFA` is
  `construct_object_gfx` — exact caller match (both call sites match
  2011's own two exactly) plus full algorithm-shape confirmation,
  minus five subsystems (object scaling, tinting, mirroring, hardware
  accel, the `objcache`/`actsps` split) — resolving MergeObject's own
  "unconfirmed scaling" caution: object scaling is CONFIRMED ABSENT.
  `sub_410771` is `put_sprite_256` — an unusually thorough match to
  2011's `#ifdef USE_15BIT_FIX` color-depth-conversion path, picking
  up 3 new Allegro boundary functions (`bmp_bpp`/`set_trans_blender`/
  `draw_trans_sprite`) and a new global, `trans_mode`. `array1` renames
  to `actsps`, confirmed as a FIXED 60-slot array (`dd 3Ch dup(?)`)
  vs. 2011's dynamically `malloc`'d pointer — another fixed-vs-dynamic
  drift.
- **`MoveToWalkableArea`'s open lead closes: `find_nearest_walkable_
  area`, fused with its own `_within` helper.** `sub_40AE7D` matches
  2011's `find_nearest_walkable_area`+`find_nearest_walkable_area_
  within`'s whole-screen fallback pass (`range=-1,step=5`) near line
  for line, every literal constant (99999/14/5/90000) matching
  exactly. Two confirmed drifts: missing 2011's fast 20px-radius/2px-
  step first-pass optimization (always pays the expensive whole-room
  scan), and missing the "edge-widening" tweak for a character already
  past a room boundary. Bonus: second confirmation route for
  `RoomStruct.height`@+0x3882 (`word_522F0A`, zero slack).
- **`starting_room`/`done_es_error` close a small `load_new_room`
  caveat.** `mainloop` contains a near-literal match to 2011's `"if
  ((in_enters_screen!=0) & (displayed_room==starting_room)) quit(...);
  if ((in_enters_screen!=0) && (done_es_error==0)) {debug_log(...);
  done_es_error=1;}"` (`AC.CPP:25421-25424`) — the disassembly's
  `setnz`/`setz`+`and` sequence matches source's own unusual bitwise
  `&` literally. Identifies `starting_room` (new global) and confirms
  `done_es_error` via the `debug_log` call's matching warning string.
  2011's intervening `play.room_changes++;` still has no counterpart
  here — a small, still-open detail.
- **`GUIListBox::MouseDown` upgraded to high confidence.** A prior
  medium-confidence vtable-position guess turns out to be a complete,
  near-exact match to `MouseDown` fused with its own `IsInRightMargin`/
  `GetIndexFromCoordinates` helpers once actually read. Real drift:
  2011's `IsInRightMargin` additionally gates on `exflags &
  GLF_NOBORDER`/`GLF_NOARROWS` — neither check exists here, so this
  build always treats the right-margin zone as scroll-interactive.
- **`GOBJ_INVENTORY=3` confirmed, closing the full `GOBJ_*` enum.**
  The last open constant (flagged several rounds ago as having no
  individual confirmation site) turns out to already be sitting in
  already-transcribed `check_controls` disassembly from the
  `mouse_ifacebut_xoffs`/`yoffs` round: the branch computing those
  offsets is gated by `cmp control_type,3`, confirming `GOBJ_
  INVENTORY=3` with zero drift. All six `GOBJ_*` values are now
  individually confirmed.
- **`_display_main`'s assumed "10-parameter signature" was wrong —
  genuinely 7 parameters.** Checking `CreateTextOverlay`'s long-open
  "7 args vs. 10-param signature" detail for the first time (rather
  than continuing to treat it as settled) found the frame declaration
  itself has only 7 slots, matching both real callers exactly — no
  mismatch ever existed. 2011's trailing `isThought`/`allowShrink`/
  `overlayPositionFixed` are CONFIRMED ABSENT. Also fixed a real
  `extract_prototypes.py` bug found in the process: 9 library matches
  had a directory as `source_file` instead of `is_library:true`,
  crashing the extractor — fixed, and a new `KNOWN_SIGNATURE_OVERRIDES`
  mechanism added so this correction survives a future regeneration.
  `prototypes.json` was badly stale (pre-rename names, missing several
  hundred matches) and is now fully caught up.
- **`draw_screen_overlay`'s FPS-display tail resolved: `draw_fps`
  really is fused, but as a much simpler predecessor.** The function's
  own tail matches 2011's `if(display_fps) draw_fps();` gate exactly,
  confirming `display_fps` behaviorally for the first time and
  identifying `dword_523118` as `fps`. But this build draws the FPS
  text directly onto the live screen via one `wouttext_outline` call —
  no cached `fpsDisplay` bitmap, no `gfxDriver`/DDB machinery, no
  second "Loop %ld" line — all CONFIRMED ABSENT. Upgraded to high
  confidence.
- **`GUISlider::Draw` upgraded to high confidence, plus `wsetcolor`/
  `rectfill`/`currentcolor` identified.** A prior medium-confidence
  vtable match turns out to be byte-perfect through its floating-point
  handle-position formula (`fild`/`fidiv`/`fimul`/`fsub 2.0`/`ftol`
  matching source's cast expression exactly). Draws via `wsetcolor`+
  `rectfill`/`line()` (a 3D-bevel rectangle), newly identifying all
  three plus the `currentcolor` global. CONFIRMED ABSENT (upgrading a
  save-format-only inference to a full behavioral one): `handlepic`/
  `bgimage`/`handleoffset` custom-graphic support.
- **`GUILabel::Draw` upgraded to high confidence: word-wrap is fused
  directly into `Draw()`.** The full body matches source exactly
  (`check_font`/`GetTranslation`/`replace_macro_tokens`/`wgettextheight`
  /`wtextcolor`), but source's separate `break_up_text_into_lines()` +
  `lines[]` array has no counterpart here — this build does a
  single-pass byte-by-byte scan fused directly into `Draw()`, breaking
  on `[` markers or `wgettextwidth` exceeding `wid`, drawing each line
  via the newly-matched `GUILabel::printtext_align` as it's found. The
  same "later refactor extracted a reusable helper" pattern seen
  elsewhere, now confirmed for GUI label word-wrap too.
- **`GUIListBox::Draw` upgraded to high confidence: three clean,
  confirmed-absent simplifications.** Matches source closely
  throughout, including a second confirmed case of `ChangeFont`
  fused inline (via its own distinctive calibration string). Three
  behavioral confirmations of fields already suspected absent from
  save-format evidence: `exflags`' `GLF_NOBORDER`/`GLF_NOARROWS` bits
  are never checked (unconditional border/scrollbar draw — a third
  confirmation of this pattern), `selectedbgcol` is never read (the
  selection highlight always uses `textcol`/`backcol` instead), and
  `alignment` is never read (every list item is always left-aligned).
- **`GUIInv::Draw` upgraded to high confidence, closing this session's
  full sweep of all six `GUIObject`-derived `Draw()` methods.** Also
  corrects a wrong `source_file` (it's `Engine/AC.CPP:7194`, not
  `acgui.cpp`). Decisively confirms an earlier round's "no per-object
  grid fields found" as a full positive result: every layout
  computation routes through the already-established GLOBAL
  `play_inv_*`/`inv_item_*` fields instead of any per-`GUIInv`-object
  field. CONFIRMED ABSENT: the entire disabled/greyed-out darkening
  effect.
- **`find_next_enabled_cursor` named, a further confirmation that
  `numcursors` doesn't exist.** A complete, near line-for-line match
  once fully read. The bound check uses a hardcoded literal `10`
  instead of `game.numcursors` — a third independent site confirming
  that field doesn't exist here (the fixed `mcurs[10]` capacity IS the
  bound). One harmless instruction-order swap noted (`MCF_STANDARD`
  checked before `testing==MODE_USE`, source does the reverse) —
  functionally equivalent since the two are mutually exclusive.
- **`wouttext_outline` upgraded to high confidence.** Called from many
  already-matched functions but never independently traced. Matches
  `AC.CPP:12612-12646` completely except one confirmed absence: source's
  `FONT_OUTLINE_AUTO` 8-direction blur outline doesn't exist here — a
  single `>=0` check with no `else if` counterpart. Identifies
  `textcol` (the global text-color state) and `wouttextxy`.
- **`__my_setcolor` upgraded to high confidence.** Matches source
  except three clean, confirmed-absent later additions: the
  "already calculated" 32-bit-color caching check, the entire true-
  color (`depth>16`) `makeacol32` branch (colors ≥32 at those depths
  just pass through unconverted), and the trailing alpha-channel
  visibility fixup. Identifies `col_lookups[]` and Allegro's
  `makecol_depth`.
- **`cant_skip_speech` resolved: raw option value, not a converted
  bitmask.** Its write site is a plain copy of the raw `OPT_NOSKIPTEXT`
  byte, no conversion call. All four read sites test the raw 0-4 range
  directly, confirming `user_to_internal_skip_speech()` doesn't exist
  in this build — every use site tests the raw value instead of a
  converted `SKIP_*` bit. Renamed, upgraded to high confidence.
- **Fresh vein opened: 113 bare mechanical matches never read for
  field evidence.** `SetObjectBaseline` confirms no `objcache`
  cache-invalidation logic exists (unconditional write, reinforcing
  the already-established `objcache`/`actsps` split absence).
  `setup_for_dialog`/`restore_after_dialog` confirm `mouse_cursor_
  hidden` doesn't gate their `domouse()` calls, and resolve a
  long-standing placeholder note: their `sub_409756`/`sub_4096B5`/
  `sub_40976A` callees form a coherent virtual-screen-to-real-screen
  tint-compositing trio with no single clean 2011 name (predates
  `gfxDriver` entirely) — documented, deliberately left unnamed.
  `StopAmbientSound` confirms a hard `channel==1` check (not a range)
  and a third confirmation of the SOUNDCLIP "stop/destructor" vtable
  slot. `IsButtonDown` confirms no middle-mouse-button support at all
  (bounds check and error string both differ from source's LEFT/
  RIGHT/MIDDLE).
- **`FileOpen`/`FileClose`: a security-relevant absence, and a rare
  zero-drift capacity.** `FileOpen`'s own traversal-protection checks
  (rejecting `/`, `\`, `..`, `:`) match `validate_user_file_path`'s
  `currentDirOnly` block exactly, fused directly in rather than a
  separate function — but source's `$SAVEGAMEDIR$`/`$APPDATADIR$`
  special-path-prefix resolution is CONFIRMED ABSENT: filenames go
  straight to `fopen` with no prefix handling, worth flagging for the
  ScummVM port. Identifies `num_open_script_files`/`valid_handles[]`
  (10 slots, zero drift from `MAX_OPEN_SCRIPT_FILES=10`).
- **`SetViewport`/`ReleaseViewport`/`GetViewportX`/`GetViewportY`
  close, naming `check_viewport_coords`.** All four are complete,
  zero-drift matches. `SetViewport` identifies `check_viewport_coords`
  (itself a complete match) and gives `offsets_locked` a third
  confirmation; `ReleaseViewport` a fourth. Confirms `divide_down_
  coordinate` is just inlined division, matching the same pattern
  already established for `multiply_up_coordinate`.
- **`SetBackgroundFrame`/`GetBackgroundFrame` close, and `ListBox*`
  confirms a flattened abstraction layer.** `SetBackgroundFrame`
  matches closely but confirms absent the "already on this frame"
  early-out and the entire `on_background_frame_change()` cache/
  palette housekeeping call. The `ListBoxClear`/`Add`/`GetSelected`/
  `GetNumItems` cluster all confirm 2011's intermediate `ListBox_XXX`
  script-object wrapper layer doesn't exist — these call straight into
  `GUIListBox`'s own member functions or do inline field reads, one
  layer flatter throughout.
- **`MoveCharacterStraight` names `can_see_from`/`line_callback`.** A
  genuine self-contained implementation (2011's own `Character_
  WalkStraight` wrapper has no body in this repo to compare against),
  fully recovered: validate character, `StopMoving`, try a straight
  line via the newly-matched `can_see_from`, fall back to the last
  reachable point if blocked. `can_see_from`/`line_callback` are both
  complete, essentially instruction-for-instruction matches to
  `routefnd.cpp`. Renamed `line_failed`/`lastcx`/`lastcy`/`wallscreen`.
- **`MoveCharacterPath` names `calculate_move_stage`, confirming
  `MoveList.pos[]`'s packed-XY encoding.** Another genuine self-
  contained implementation. Appends a waypoint as a packed `(tox<<16)|
  toy` dword, then calls the newly-matched `calculate_move_stage`
  (two real callers, matching source's own two exactly) to compute
  the new segment's per-move deltas — its early-return branch and
  XY-unpacking both match verbatim, confirming the packed encoding
  from two independent functions in the same round.
- **Anti-tamper/copyright-check cluster closes.**
  `get_route_composition`/`route_script_link`/`print_welcome_text` are
  2011's own deliberately-misleadingly-named copyright-validation
  functions (nothing route-related at all) — all match verbatim
  including literal exit codes. Renamed `welcome_text_validated`→
  `routex1` and identified `walk_area_zone5`, both matching source's
  own equally obscure names. `init_pathfinder` closes with a rare
  zero-drift capacity match (`MAXPATHBACK=1000`, no reduction).
- **Three flag setters confirm new bit values; `SetCharacterView`/
  `ChangeCharacterView` reveal a real drift.** `SetCharacterClickable`/
  `SetCharacterIgnoreWalkbehinds` confirm `CHF_NOINTERACT=4`/
  `CHF_NOWALKBEHINDS=0x80` with zero drift; `SetObjectClickable`
  reconfirms `OBJF_NOINTERACT=1`. Both View functions are genuine
  self-contained implementations (no 2011 wrapper body to compare
  against) — `SetCharacterView` names `numviews` (a stale un-renamed
  global) and confirms a new bit, `CHF_FIXVIEW=2`; `ChangeCharacterView`
  confirms a real behavioral difference — it sets both `defview` and
  `view`, unlike `SetCharacterView`, so it can't preserve a distinct
  original view to revert to later.
- **Six more bare matches close, including a genuine 10x speed-cap
  drift.** `NewRoomEx`/`ResetRoom`/`SetRestartPoint`/`CallRoomScript`
  all close cleanly (the first three each give an already-established
  field/global a further confirmation). `SetGameSpeed` clamps its
  upper bound to `100`, not source's `1000` — a real 10x lower max
  game speed, not a mere capacity reduction — and both `SetGameSpeed`/
  `GetGameSpeed` confirm the `game_speed_modifier` adjustment is
  entirely absent.
- **`StopDialog` shows a hard-crash-vs-soft-warning drift.** Source's
  "not in a dialog" case is a soft warning that returns gracefully;
  this build's version calls `quit()` instead — a real behavioral
  regression, not just a missing feature. `GetHotspotAt`/`GetPlayer
  Character`/`RefreshMouse` all close cleanly; the last directly
  confirms `mgetgraphpos`'s own entry (previously only inferred) and
  identifies `scmouse_y`.
- **Two confirmed robustness gaps.** `MoveCharacterToObject` and
  `MoveCharacterBlocking` are both missing their leading validation
  guards entirely — source's own `is_valid_object`/`is_valid_
  character`/`on!=1` checks (the last specifically guarding against a
  game-hanging call when "Hide Player Character" is ticked) don't
  exist here at all. `MoveCharacterToHotspot`/`StopMoving` both close
  as clean, exact matches.
- **`FaceLocation`/`FaceCharacter` close cleanly.** `FaceLocation`
  names a genuine self-contained 8-direction facing implementation (no
  2011 wrapper body to compare against): `dx`/`dy` computation, a
  `CHF_NODIAGONAL`/`numloops<8` cardinal-only check, then the classic
  tan-approximation octant classifier writing the result into `loop`.
  `FaceCharacter` closes cleanly, fusing two 2011 script-object wrapper
  hops into one direct call to the already-matched `FaceLocation`.
- **`SetInvDimensions` confirms no multiple-inventory-window support.**
  Matches source's first three statements exactly, but confirms absent
  the entire trailing `numguiinv`/`guiinv[]` backwards-compatibility
  loop — inventory display dimensions are purely global here. The
  `SeekMODPattern`/`SeekMP3PosMillis`/`GetMP3PosMillis` cluster all
  gate on the already-established single music-format handles instead
  of 2011's later `channels[]`/`crossFading` multi-format dispatch,
  reinforcing the single-channel predecessor pattern already found
  across the music subsystem.
- **`IsTranslationAvailable`/`ParseText`/`Said` close, directly
  confirming `comparetonum`/`compareto`.** Both text-parser functions
  match source exactly, and directly confirm (from their own bodies,
  not just positional inference) that these two standalone globals
  replace `GameState.num_parsed_words`/`parsed_words` — closing the
  loop on that field's earlier confirmed-absent finding: the feature
  isn't missing, it's just not a `GameState` member here.
- **A mislabeled function found and fixed.** A function auto-named
  `VALIDATE_STRING` (a real 2011 identifier, but one that's actually a
  MACRO there, never a real function) turns out to be a decisive,
  complete match to `check_strlen` instead: matches
  source instruction for instruction, including its literal `200`/`30`
  MAXSTRLEN constants. Renamed, with the mismatch documented in case it
  recurs elsewhere. `_sc_strcpy` closes as a zero-drift match; `_sc_
  strcat` confirms absent the separate `VALIDATE_STRING(s2)` safety
  check; `_sc_strlower`/`_sc_strupper` do that same check inline
  instead of via a shared function. All four confirm `my_strncpy`
  doesn't exist as a separate function — each inlines its own
  null-termination step after a raw CRT `strncpy` call instead.
- **The `FileWrite`/`FileRead` script-API cluster closes, with two real
  crash regressions found.** `FileWrite`/`FileWriteRawLine`/
  `FileWriteInt` all close as complete, zero-drift matches. `FileRead`/
  `FileReadInt` are both missing source's leading `if(feof(haa))
  {...return early...;}` guard entirely — reading either at EOF hits
  their own bounds/tag-byte check instead and CRASHES via `quit()` with
  a misleading "file was not written by FileWrite"/"read back in wrong
  order" message, rather than gracefully returning an empty string/-1.
  `FileReadRawChar`/`FileReadRawInt` are missing the same guard too, but
  harmlessly — `fgetc()`/`getw()` already surface EOF as -1 through
  their own normal return path, so nothing is actually lost there.
- **`ListBoxGetItemText`/`ListBoxSetSelected`/`ListBoxDirList` close,
  finding three different kinds of drift.** `ListBoxGetItemText`
  matches 2011's bounds check exactly but copies via a raw, unbounded
  `strcpy` — CONFIRMED ABSENT is 2011's own later length-capped
  `strncpy`/null-terminate safety hardening, a real overrun risk.
  `ListBoxSetSelected` does one unconditional write with none of 2011's
  out-of-range clamping, unchanged-value skip, or `topItem` auto-scroll
  — a real validation gap against the read side's own bounds check.
  `ListBoxDirList` has no `validate_user_file_path` call at all (unlike
  `FileOpen`'s own confirmed inline protection) — a real path-traversal
  security gap — and uses raw CRT `_findfirst`/`_findnext`/`_findclose`
  instead of Allegro's `al_find*` wrappers.
- **`MoveObject`/`MoveObjectDirect` close as zero-drift matches;
  `MoveCharacter`/`MoveCharacterDirect` find an exhaustive
  `autoWalkAnims` absence.** Both `MoveCharacter`/`MoveCharacterDirect`
  match `walk_character`'s first four arguments exactly, but push a
  literal `0` for the fifth (`autoWalkAnims`) where source passes
  `true` — checked against all 9 `walk_character` call sites anywhere
  in the disassembly, none ever pass a nonzero value. That argument
  gates automatically clearing a character's custom animation when a
  new walk starts (`acchars.cpp:56-57`) — never happens in this build.
- **`QuitGame`/`SetSkipSpeech`/`quit` close, with an 8-function bonus
  cluster falling out of `QuitGame`'s own call chain.** `sub_426E8E` is
  a decisive, zero-drift match to `quitdialog()` (its literal message-ID
  constants match `MSG_QUITBUTTON`/`PLAYBUTTON`/`QUITDIALOG` exactly),
  which in turn names `get_global_message` (CONFIRMED ABSENT: the
  `get_translation()` call) and `myscimessagebox` — an exceptionally
  clean match where every `CNT_*`/`CNF_*`/`CM_COMMAND` constant lines
  up — plus 5 more CSCI-primitive functions by call-signature evidence.
  `quit()` itself is a drastically simpler predecessor of its already-
  elaborate 2011 counterpart (no editor-debugger protocol, no plugin
  hooks, no `gfxDriver` cleanup), with 4 further drifts: an
  unconditional `Debugger::CloseDebugger()` call, `set_gfx_mode`
  passing literal 80×25 text-mode dimensions, a third instance of the
  raw-CRT-vs-Allegro-wrapper drift, and — rare in this project — an
  ADDED cleanup step (`agssave.999` deletion) 2011 later removed.
- **`IsKeyPressed` closes: a genuinely simpler predecessor with several
  confirmed key-handling gaps.** Names a new Allegro match,
  `keyboard_needs_poll` (trivial global return). The ASCII-key path
  remaps `'A'-'Z'`/`'0'-'9'`/TAB/ENTER/SPACE/ESC to exact `KEY_*`
  constants, but ENTER has no numpad-Enter fallback, and any OTHER
  ASCII key in range (`-`/`+`/`/`/`=`) unconditionally reports "not
  pressed" — CONFIRMED ABSENT is source's own dual-check-then-numpad-
  fallback logic for all of those. The function-key path's numpad-
  equivalence checks (`LEFT`/`RIGHT`/`UP`/`DOWN`↔`4/6/8/2_PAD`) match
  source instruction for instruction via Allegro's own `key[]` array —
  but stop there; 3 more pairs 2011 checks (PGUP/PGDN/HOME/END/INSERT/
  DEL numpad equivalents) are confirmed absent.
- **`check_write_access` closes, finding a real minimum-free-space drop
  and a save-directory-location drift.** Computes free disk space
  directly (no platform indirection, as expected) but requires only
  ~195KB free where source requires 2MB — an order-of-magnitude-smaller
  threshold, not a rounding artifact. The write-test file is a bare
  `"~tmptest.tmp"` with no directory prefix, testing the current working
  directory rather than a dedicated save-game directory — consistent
  with `FileOpen`'s own already-established `$SAVEGAMEDIR$`-prefix
  absence.
- **`script_debug` closes, with a major bonus: the binary's own
  embedded version/build-date string.** Its `cmdd==1` "show engine
  info" debug command's literal format string reads, verbatim, `"...ACI
  version 2.40.325|Compiled on Jul 20 2002 at 18:56:30..."` — a direct,
  self-reported confirmation of this project's "2.4b, July 2002"
  version pinning, one day before the PE header's own link date. See
  the new section in `reversing/notes/ags-archives-cross-reference.md`.
  Also finds: a differently-worded VOX-enabled message, a walkable-
  areas debug view missing `prepare_walkable_areas()`, a room-teleport
  command missing the room-list-selector alternative, and an
  unconditional `display_fps` write missing source's locked-value guard.
- **`atexit_handler` closes as a zero-drift match** — its literal error
  text and `ACI_VERSION_TEXT` (`"2.40.325"`) provide a second,
  independent confirmation of the binary's self-reported version from
  a different code path than `script_debug`'s own finding.
- **A mislabeled function found and fixed: `dxmedia_abort_video` is
  really `dxmedia_play_video`.** `PlayVideo`'s own call target had been
  named `dxmedia_abort_video` based on a real but misleading string
  match — the full 241-line body is far too large for that ~15-line
  source function, and turns out to decisively be `dxmedia_play_video`
  instead, with `dxmedia_abort_video`'s own logic (including that same
  `"Played successfully."` string) fused into its own tail — this build
  has no standalone `dxmedia_abort_video` at all. Renamed, with the
  correction documented. Also names `ExitCode`/`InitRenderToSurface`/
  `RenderToSurface`, and confirms `PlayVideo` itself fuses two 2011
  layers (`AGSWin32::PlayVideo` + `dxmedia_play_video`) into one flat
  function with no `gfxDriver`/platform indirection and no file-
  existence pre-check.
- **`FadeIn` closes: a second confirmed fast_forward-gating gap, and a
  third confirmation of the missing skip-cutscene subsystem.** Reduces
  to a single direct call to its own fade-in helper, missing both of
  source's leading checks: no `EndSkippingUntilCharStops()` call (third
  confirmation the whole subsystem is absent) and no `fast_forward`
  guard (proceeds during fast-forward, joining `UpdatePalette`'s own
  already-established finding).
- **`domouse` closes, confirming which reference file applies and
  finding the alpha-blend-cursor feature absent.** Matches
  `Common/MOUSEW32.CPP` (not the older, same-named `mouse32.cpp`) via
  its specific `savebk!=0` null-check guard. For the cursor draw itself,
  calls `put_sprite_256` directly — matching `drawCursor()`'s own non-
  alpha-blend branch exactly, but with the entire `alpha_blend_cursor`
  check/branch confirmed absent: this build inlines that one branch
  since the feature it gates doesn't exist here yet.
- **`cd_player_init` closes as a near-exact match**, called directly
  from `main` with no `platform->InitializeCDPlayer()` indirection.
- **`enternumberwindow` closes, naming `enterstringwindow` and finding
  a real empty-input ambiguity.** Its call target decisively matches
  `enterstringwindow` via an exact `CSCIDrawWindow` literal-argument
  match plus a two-caller correspondence. `enternumberwindow` then
  calls `atoi()` directly with no empty-input sentinel check — a
  cancelled/empty dialog and the user entering `"0"` are
  indistinguishable here.
- **`malloc_fail_handler`/`winclosehook`/`minstalled` close** — the
  first is missing its error message's `our_eip` diagnostic suffix, the
  second is missing its third assignment (consistent with the
  dynamic-sprite-leak check being absent from `quit()`), and the third
  is a complete, zero-drift match.
- **`render_to_screen` has a genuinely expanded signature, not just a
  predates-`gfxDriver` gap** — takes 6 parameters, not 2011's 3, fusing
  a separate stretch/scale step directly into its own call shape.
- **The CLIB asset-library format supports only versions 6/10, and the
  live `mflib` global uses the OLD struct layout directly.** `csetlib`'s
  version check is `if(lib_version!=6 && lib_version!=10) return -3;`
  — only two valid values, where source accepts six (6/10/11/15/20/21).
  `clibfindindex`'s own 25-byte filename stride confirms the live
  `mflib` global is declared as the OLD, smaller `MultiFileLib` struct
  directly, not 2011's `MultiFileLibNew` — CONFIRMED ABSENT is the
  entire old-to-new-format conversion machinery 2011 carries for
  backward compatibility; this build has no newer format to bridge to.
  Also confirms `ci_fopen()`'s case-insensitive-path wrapper is absent.
- **`clibopenfile`/`clibfopen` close the CLIB cluster, finding a third
  confirmation of `ci_fopen`'s absence.** `clibopenfile`'s own
  `data_filenames[]` access confirms a second independent 20-byte
  stride matching the OLD `MultiFileLib` struct. Both functions use
  plain CRT `fopen()` everywhere instead of a case-insensitive wrapper.
- **`load_graphical_scripts` found: the missing loader half of the
  graphical-scripts subsystem.** No 2011 counterpart exists, but its
  own behavior connects directly to `run_graph_script`'s own already-
  matched reader: called from `load_main_block` during room loading, it
  extracts embedded graph-script blocks from the room file into
  numbered `"~acsc%d.tmp"` temp files — the exact same filename pattern
  `run_graph_script` reads back later at execution time. Loader and
  reader are now both identified and connected.
- **`set_game_speed`/`dj_timer_handler` close; `INIreaditem` turns out
  to be a genuinely different implementation behind the same identity.**
  `set_game_speed` is an exact match; `dj_timer_handler` is missing its
  leading `timerloop++`. `INIreaditem` is decisively the same function
  (confirmed via matching config section/key names at every call site)
  but with a completely different 4-parameter, binary-mode,
  character-by-character implementation, not source's clean 2-parameter
  fgets-loop rewrite. Chasing its `filetouse` global found a bonus:
  `main` fuses all of 2011's separate `read_config_file()` inline, and
  `filetouse`'s own default value is the literal string
  `"C:\TC\CHRISJ.INI"` — a harmless leftover from the original
  developer's own build environment, always overwritten before real use.
- **`getshort`/`get_col8_lookup`/`_sc_sprintf` close, a third
  confirmation of `my_sprintf`/`my_strncpy`'s absence.** The first two
  are complete, zero-drift matches. `_sc_sprintf` matches source's
  `VALIDATE_STRING`/`check_strlen`/`get_translation` sequence exactly,
  but confirms (a third time, after `_sc_strcat`/`_sc_strcpy`) that
  `my_sprintf`/`my_strncpy` don't exist as separate functions — this
  build calls `vsprintf` directly, then a raw `strncpy` plus manual
  null-termination.
- **A mislabeled function found and fixed: `fix_filename_case` is
  really `INIgetdirec`.** Unlike earlier `sub_*`-placeholder
  corrections, this address already carried a real name, matched via
  an exact linker-symbol hit — but 2011's actual `fix_filename_case` is
  a trivial one-argument Allegro function that doesn't match this
  address's real two-argument body at all. Decisively `INIgetdirec`
  instead, matching source's backward path-separator scan and its
  caller (`main`) exactly. CONFIRMED ABSENT: the `/`-as-separator
  alternative — this build's scan checks only `\`.
- **`quit()`'s last open lead closes: `gamescript`/`thisroom.
  compiled_script` confirmed, plus a stale-rename gap fixed.**
  `dword_523130` is `gamescript`, confirmed via `load_ac2game_dta`'s own
  `fread_script`→`ccCreateInstance` sequence producing `gameinst`.
  `dword_523080` is `thisroom.compiled_script`, confirmed via zero-slack
  address arithmetic against `thisroom`'s established base and
  behaviorally via `compile_room_script` producing `roominst`. Along
  the way, `compile_room_script` itself was still `sub_421957` in the
  live IDB despite an existing "confirmed match" comment — another
  "documented in prose, never pushed to the IDB" gap, now fixed.
- **`check_click_on_character`/`check_click_on_object` get their own
  `matches.json` entries** — both already correctly named and
  described in passing via their callees' own evidence text, but
  neither had a dedicated entry until now. Both close as complete,
  exact, zero-drift matches.

## Third-party library identification (Task #10)

Statically-linked third-party libraries (`Engine/libsrc/libcda-0.4`,
`Engine/libsrc/allegro-4.2.2-agspatch`, `Engine/libsrc/dumb-0.9.2`,
`aastr-0.1.1`, `almp3-2.0.5`, `hq2x`) don't move the "reconstruct Rob
Blanc 1" goal forward the way Engine/Common matches do. **Per the
"Third-party library scope" rule above, this task is now narrower than
its original "IDB completeness" framing**: identify a library's PUBLIC
API surface (what AGS/game code actually calls) and the AGS-side struct
fields/globals that hold its results, but do NOT chase a library's own
internal helper functions once the library itself is confirmed — a
ScummVM reimplementation replaces those wholesale, so investigating them
further has no payoff. Historical work done before this rule was
introduced (below) sometimes went deeper than this into library
internals (e.g. some JGMOD/ALMP3 internal helpers got individually
characterized) — that work isn't wrong, just no longer the model to
follow going forward. A productive session got ~40 new matches
(all of `libcda-0.4` bar one function, a good chunk of Allegro's
Windows driver/config code, `apeg` conclusively ruled out) — see
`reversing/notes/third-party-library-identification.md` for full
detail. A follow-up round resolved the previously-open "dumb-0.9.2 XM
loader" lead completely: it was never DUMB at all, it's **JGMOD** (a
tracker-music library with no source tree in this repo) — `load_mod`
and `play_mod` matched via caller pattern and distinctive JGMOD error
strings. Separately, **`dumb-0.9.2` itself is now conclusively ruled
out**, the same way `apeg` was: it was released April 2003, seven
months after this binary's 2002-07-21 link date — chronologically
impossible for it to be linked in. Do not search for `dumb-0.9.2`
matches; treat it like `apeg-1.2.1`. A THIRD round tackled the
remaining untouched group: **`aastr-0.1.1` and `hq2x` are genuine
string-matching dead ends** (zero quoted strings in either library's
source at all, ruling out this project's main technique categorically
— low priority, only a structural/callgraph approach could make
progress). **`almp3-2.0.5` yielded 5 solid matches** via caller-shape
analysis of `PlayMusic`'s already-known `"music%d.mp3"` attempt:
`my_load_static_mp3`, `almp3_create_mp3`, plus 3 supporting Allegro
`PACKFILE` functions (`pack_fopen`/`pack_fread`/`pack_fclose`, MEDIUM
confidence — the `PACKFILE.todo` field offset this build reads doesn't
match the 4.2.2 reference declaration, likely struct-layout drift, not
a function-identity doubt). One open lead remains there (`sub_47E7A0`,
called right after `almp3_create_mp3` with an argument shape that
doesn't cleanly match a single known ALMP3 API function). A later round
found a SECOND call site with the identical argument shape, inside a
small cluster of two AGS-side functions (`sub_4084E0`/`sub_408392`,
left unnamed) implementing this build's own much simpler, single-
stream MP3-crossfade check/cleanup — no clean 2011 counterpart exists
since 2011's crossfading is built around a `channels[]` array this
build predates entirely. Recorded at the ALMP3 boundary per the scope
rule above, not individually resolved. A follow-up
round characterized 6 of JGMOD's 8 cascade branches by their magic-
string checks (JGMOD-native, IT, S3M, MOD, and two XM-adjacent checks
sharing an unresolved 4-byte constant) — still unnamed, pending a
JGMOD source tree ever being added to the repo, but each now has a
documented format role instead of being an undifferentiated block.

**`Engine/libsrc/apeg-1.2.1` is NOT linked into Rob Blanc 1 at all —
conclusively ruled out, don't search for it.** Zero `"mpeg"`/`"apeg"`
strings exist anywhere in the extracted 2727-string dataset, and none of
apeg's distinctive error strings appear in the disassembly. Two leads
that superficially looked like `apeg-1.2.1/display.c` matches (based on
string-overlap alone) turned out, once the full Allegro tree existed to
disambiguate, to actually be Allegro's Windows DirectDraw driver
(`init_directx_ovl`/`init_directx_win`). Rob Blanc 1 plays video via
**DirectShow** instead (`Engine/acwavi.cpp`, already matched — see
`dxmedia_abort_video`/`RenderFileToMMStream`/`PlayVideo`) — `apeg` is an
AGS video-backend feature added sometime between 2002 and the 2011
reference build. Treat this the same as any other confirmed 2002-vs-2011
feature gap, just at library scope instead of a single function/struct.

Key addition: the full upstream Allegro 4.2.2 source tree was added at
`Engine/libsrc/allegro-4.2.2/` (separate from the pre-existing
`Engine/libsrc/allegro-4.2.2-agspatch/`, which only ever had the
Windows-specific `win/*.c` patch files, not the generic library core like
`sound.c`/`config.c`/`unicode.c`). **Caution**: the two trees have
overlapping filenames (`win/wwnd.c`, `win/wmouse.c`, etc.) with
*different* content (`agspatch` is AGS's patched fork, confirmed via
`diff`) — a string-matching lead pointing at one of those overlapping
filenames needs the disassembly body checked against both versions, not
assumed to be the vanilla one. `libcda-0.4` (fully done, minus one
function still needing an IDA function boundary — see the notes file) and
several Allegro Windows-driver/config-reading functions are matched so
far; both `apeg-1.2.1` and `dumb-0.9.2` are ruled out entirely (see
above) — the actual module-music library linked in is JGMOD instead.

Leftover low-value lead category, same caution as before:

- Occasional coincidental substring matches from unrelated subsystems (e.g.
  a lead pointing at `AC.CPP` that turns out, once the caller is checked, to
  actually belong to a third-party library because the string match was a
  false positive). Always sanity-check the disassembly caller before
  committing a rename, not just the string match.

The remaining ~2000 unmatched `sub_*` functions mostly have **no** string
evidence at all (no distinctive literal referenced in their body), so
`build_leads.py`'s technique is running out of runway. Next productive
avenues, roughly in order of effort:
1. Follow-the-callgraph from already-matched functions (as done repeatedly
   above: an unmatched function called from / calling an already-matched
   one is a strong lead even with zero string evidence of its own — check
   argument count/order and control-flow shape against the source
   candidate).
2. Match on distinctive numeric constants / struct field offsets rather
   than strings.
3. Structural/size fingerprinting against the reference build (function
   byte-length, local variable frame size, branch count) — not yet tooled.

## Conventions when annotating the IDB

- Function comments added by `apply_matches.py` are tagged with a
  `[reversing] confirmed match` marker line so re-runs can find and replace
  just that block without clobbering hand-written comments above it.
- Prefer recording *evidence*, not just conclusions, in `matches.json` —
  future sessions (and future you) need to know *why* a match was made to
  judge whether it still holds after the disassembly changes.
- When a struct/function's disassembly diverges from the 3.2.1.1115
  reference in a way worth remembering for the eventual C reconstruction
  (e.g. Rob Blanc 1 predates a feature, or has an extra field), write it up
  in `reversing/notes/` rather than losing it in a chat transcript.
