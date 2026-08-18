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
- `reversing/analysis/matches.json` has 555 entries (function + struct-field
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
  `screen_tint`@`+0x10C6C` genuinely is outside it, so that one call
  stands. `apply_structs.py`'s `GameState` struct is rebuilt as one
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
