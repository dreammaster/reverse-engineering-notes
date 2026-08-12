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
- `reversing/analysis/matches.json` has 521 entries (function + struct-field
  matches combined)
- 15 struct definitions built entirely from disassembly evidence (not
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
  formalized), and `GameSetupStructBase` (34 of 30+ originally-guessed
  fields recovered — every field `OriGameSetupStruct`/
  `OriGameSetupStruct2` declares is now accounted for: `gamename`/
  `options`/`paluses`/`defpal[256]`/`iface[10]`/`numiface`/`numviews`/
  `mcurs[10]`/`globalscript`/`numcharacters`/`chars`/`__charcond[50]`/
  `__invcond[100]`/`compiled_script`/`playercharacter`/`totalscore`/
  `numinvitems`/`numdialog`/`numdlgmessage`/`numfonts`/`color_depth`/
  `target_win`/`dialog_bullet`/`hotdot`/`hotdotouter`/`uniqueid`/
  `reserved[2]`/`numlang`/`langcodes[5][3]`/`messages[500]`/
  `fontflags[10]`/`fontoutline[10]`/`numgui`/`dict`, spanning
  `+0x00`..`+0xA7F4` with large
  unrecovered gaps in between (confirmed anchors, not contiguous
  content); by far the biggest struct in the project (known total 0xBF84
  = 49028 bytes). MAJOR FINDING: this global's true identity is
  `OriGameSetupStruct` (`Common/acroom.h:2769`) — AGS's own OLDEST
  ancestor struct in its save-compatibility evolution chain
  (`OriGameSetupStruct` → `OriGameSetupStruct2` → `OldGameSetupStruct` →
  ... → `GameSetupStructBase`), preserved read-only in the 2011 header
  via `ConvertOldGameStruct` (`acroom.h:3017`) purely for old-save
  upgrading — this retroactively explains nearly all of the "drastic
  drift" found in earlier rounds (byte-sized `options[20]`,
  `gamename[30]`, the field-order divergence) as one single fact rather
  than scattered coincidences. `globalscript`/`compiled_script` were
  found via `restore_game_data`'s "preserve compiled assets across a
  savegame restore" pattern (explicitly saved/restored around the bulk
  struct reload, alongside `chars`/`numcharacters`); `totalscore` via an
  exact zero-slack positional fit plus a direct hit in `main`'s inlined
  `play.totalscore = game.totalscore;` initializer; `numdlgmessage` via
  a "too many dialog lines" quit-check; `dict` via a `WordsDictionary`-
  shaped `num_words`/`word[]`/`wordnum[]` deserializer — every one of
  the five lands exactly on `OriGameSetupStruct`'s (or
  `OriGameSetupStruct2`'s) own declared adjacency, zero slack.
  Found with a new mechanical byte-offset-counting script
  (`reversing/scripts/count_data_offsets.py`) — which initially had a
  real alignment bug (rounded `align N` directives against the running
  RELATIVE offset instead of the TRUE ABSOLUTE address, silently
  drifting +8 bytes at the one `align 10h` in this struct, since
  `game_gamename`'s real base isn't itself 16-aligned), caught only by
  cross-checking against raw hex-address subtraction on IDA's own
  literally-address-named labels — 14 independent direct computations
  all disagreed with the buggy script by the same +8 and all agreed
  with each other. Fixed in both the script and every recorded offset
  before this ever reached the live IDB; the script now takes the true
  base address as a parameter. See struct-layout-drift.md for the full
  round-by-round writeup (including the correction's own dedicated
  section — read it before trusting any offset this script produces),
  including a retracted `defpal` field guess, two `game_`-prefixed false
  leads, a real field hiding behind a generic `ElementCount` label,
  `numcursors` turning out to be a genuine 2002-vs-2011 behavioral gap
  (every cursor check in this build is hardcoded to `10`, never a
  runtime field — likely nothing to find), and `invhotdotsprite`/
  `default_lipsync_frame` both turning out absent from this build
  entirely — the latter conclusively so, after directly chasing
  `GetLipSyncFrame` and finding zero matching candidates for either of
  its two most distinctive calls (`strnicmp`, `strchr('/')`) anywhere in
  the disassembly. `default_resolution` — the last genuinely open lead —
  is now ALSO confirmed absent, found via `ConvertOldGameStruct`-style
  reasoning (`Common/acroom.h:3017`): that function hardcodes a
  `numcursors=10` fallback for old games but sets NO fallback at all for
  `default_resolution`, `default_lipsync_frame`, or `invhotdotsprite` —
  and the disassembly's actual resolution-selection code in `main`
  branches on `usetup_screenres` (a player/config setting, matching
  2011's `usetup.` namespace) rather than any `game.*` field, confirming
  this build's resolution model predates the "game declares its native
  resolution" feature entirely. `GameSetupStructBase` now has NO open
  field-identity leads — every candidate field known from 2011's
  declaration is either confirmed present (26) or confirmed absent (4);
  remaining work is unrecovered gap CONTENT, not field identity.
  **`__charcond[50]`+`__invcond[100]` — the single largest remaining
  gap (22200 bytes) — went from arithmetic-fit hypothesis to FULLY
  confirmed `EventBlock[150]` this round**: `RunCharacterInteraction`
  (already matched) builds `unk_515958 + cc*0x94` and passes it to a
  newly-identified `run_event_block` (`sub_417088`) — address and
  stride both land exactly on the predicted position/size. That
  function's own body then independently confirms EVERY field of
  `EventBlock` (`Common/acroom.h:239-246`) at its exact 2011-declared
  offset with zero drift: `list[8]`@+0x00, `respond[8]`@+0x20,
  `respondval[8]`@+0x40, `data[8]`@+0x60, `numcmd`@+0x80, `score[8]`
  @+0x84 (a one-time `GiveScore` award, zeroed after use). A second
  already-matched function, `run_event_block_inv`, confirms
  `__invcond[100]` the same way, forwarding straight to
  `run_event_block` and landing exactly at `__charcond`'s computed
  end with zero gap.
  **`mcurs[10]` was hiding in plain sight**: `MouseCursor`'s own struct
  was already fully recovered in an earlier round, but nobody had
  traced its array base address (`dword_51585C`) back to a position
  inside `GameSetupStructBase` itself — it sits exactly where a
  240-byte generic-padding placeholder used to start, right after
  `numviews`, matching `OriGameSetupStruct`'s declared adjacency
  exactly. A stray `align 10h` in the `.asm` at this exact spot turned
  out to be IDA's own heuristic mislabeling of `MouseCursor.name[10]`
  (a real but never-referenced field), not a genuine compiler gap —
  doesn't affect the separately-verified OFFSET CORRECTION, just
  explains why IDA emitted it. Also caught 6 `matches.json` entries
  still citing PRE-correction offsets from before that round (the
  correction pass had fixed `apply_structs.py`/`struct-layout-drift.md`
  but never swept `matches.json` itself) — fixed all 6 in place.
  **`numiface` found by revisiting an old dead end with fresh eyes**:
  `dword_515854` was investigated once before (as a `numdialog`
  candidate) and correctly ruled out, then dropped entirely — before
  `OriGameSetupStruct`'s discovery, nobody had reason to ask what it
  might otherwise be. It sits with zero gap immediately before
  `numviews` (matching `OriGameSetupStruct`'s declared `int numiface;
  int numviews;` adjacency exactly) AND gates a loop over `0x334`-byte-
  stride data in `load_ac2game_dta` — `0x334` (820 bytes) being
  `InterfaceElement`'s independently-confirmed size from the
  `EventBlock` round.
  **`defpal[256]`+`iface[10]` — the LAST major gap — fully resolved
  next**: the earlier `defpal` retraction only ever checked IDA's
  declared label extent, never whether the CODE reads further — it
  does: `main` has its own copy of the `defpal`-copying loop, reading a
  full unconditional 1024 bytes (4-byte stride, 256 entries) starting
  right after `paluses`, matching 2011 with zero drift. That addressable
  range extends past where IDA's `g_interface` label begins — proving
  `g_interface` was never a separate global at all (its address falls
  inside `defpal`'s own already-allocated memory), just IDA's own
  mislabeled sub-range. With `defpal` properly sized, `iface[10]` (`+
  2` bytes of natural alignment padding after `defpal`) lands EXACTLY
  10 elements later on the already-confirmed `numiface` — the earlier
  "11 elements, not 10" discrepancy was an artifact of double-counting
  `g_interface` as separate space. Confirmed with real field evidence,
  not just arithmetic: `byte_513B7C`/`byte_513B7D`'s addresses sit at
  EXACTLY the byte offsets `InterfaceElement.popup`/`.on` predict.
  **The entire remaining `uniqueid`-to-`numgui` gap resolved in one
  pass immediately after**: `reserved[2]`+`numlang`+`langcodes[5][3]`+
  alignment+`messages[500]`+`fontflags[10]`+`fontoutline[10]` sums to
  EXACTLY 2048 bytes, zero slack — and `messages[500]`'s own predicted
  address turned out to be a real, already-labeled global
  (`dword_51D320`) inside `load_ac2game_dta`, doing a per-slot
  conditional message loader immediately followed by the
  already-confirmed `set_default_glmsg` chain — an over-determined fit
  (two confirmed endpoints AND a confirmed interior anchor all
  agreeing exactly), not just arithmetic alone. `GameSetupStructBase`
  now has every field `OriGameSetupStruct`/`OriGameSetupStruct2`
  declares accounted for (34 total); remaining work is confined to the
  trailing ~6KB gap, which has no `OriGameSetupStruct` declaration left
  to anchor against — a genuinely harder kind of lead than anything
  resolved so far)
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

## Third-party library identification (paused — Task #10)

Statically-linked third-party libraries (`Engine/libsrc/libcda-0.4`,
`Engine/libsrc/allegro-4.2.2-agspatch`, `Engine/libsrc/dumb-0.9.2`,
`aastr-0.1.1`, `almp3-2.0.5`, `hq2x`) don't move the "reconstruct Rob
Blanc 1" goal forward the way Engine/Common matches do, but are worth
doing for IDB completeness. A productive round happened this session
(~40 new matches: all of `libcda-0.4` bar one function, a good chunk of
Allegro's Windows driver/config code, `apeg` conclusively ruled out) —
see `reversing/notes/third-party-library-identification.md` for full
detail. **Paused by explicit user request, not exhausted** — resume by
picking up its "Next up" section: the `dumb-0.9.2` XM-loader
format-detection cascade (`sub_477320`/`sub_477CE0`, better-characterized
but not resolved — may be AGS's own Engine-side code, not library
internals), plus `aastr-0.1.1`/`almp3-2.0.5`/`hq2x` (not yet touched).

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
far; `dumb-0.9.2` is queued next (`apeg-1.2.1` ruled out entirely, see
above).

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
