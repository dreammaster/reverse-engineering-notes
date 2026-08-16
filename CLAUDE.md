# AGS Archaeology: Rob Blanc 1 Reconstruction

## Goal

Reconstruct usable C source for **Rob Blanc 1**, an early-2000s commercial
Adventure Game Studio (AGS) title, starting from an IDA Pro disassembly and
using the earliest *publicly available* AGS engine source as a reference.

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
   function's signature/struct layout, not just its name.

## Repo layout for this project

```
rob_blanc_1.asm                  - the disassembly (huge, script-only access)
Common/, Engine/                 - reference source, AGS 3.2.1.1115
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
5. Open the IDB in IDA, run `apply_matches.py` to push renames/comments in.
6. Re-export `rob_blanc_1.asm` from IDA when it's useful to snapshot
   progress, and go back to step 1.

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
- `reversing/analysis/matches.json` has 545 entries (function + struct-field
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

## Third-party library identification (Task #10)

Statically-linked third-party libraries (`Engine/libsrc/libcda-0.4`,
`Engine/libsrc/allegro-4.2.2-agspatch`, `Engine/libsrc/dumb-0.9.2`,
`aastr-0.1.1`, `almp3-2.0.5`, `hq2x`) don't move the "reconstruct Rob
Blanc 1" goal forward the way Engine/Common matches do, but are worth
doing for IDB completeness. A productive session got ~40 new matches
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
doesn't cleanly match a single known ALMP3 API function). A follow-up
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
