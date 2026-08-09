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

- 2582 functions total: 633 named, 1949 unnamed (`sub_*`) (started this
  project at 535 named; ~100 identified across string-matching,
  callgraph-following, and struct-recovery-driven passes)
- 2727 string literals extracted from `.data`/`.rdata`
- 944 of those strings matched verbatim into `Common/`/`Engine/` source
  (776 to a single source file); this pool is now largely exhausted for
  Engine/Common code — remaining single-file leads are mostly third-party
  libraries (libcda, Allegro, apeg, dumb), deliberately deprioritized
- `reversing/analysis/matches.json` has 453 entries (function + struct-field
  matches combined)
- 5 struct definitions built entirely from disassembly evidence (not
  borrowed from the 2011 source — see `reversing/notes/struct-layout-drift.md`):
  `GUIMain`, `CharacterInfo`, `ccInstance`, `ccScript`, `GUIButton`. Struct
  work has repeatedly found genuine 2002-vs-2011 divergence (smaller fixed-
  capacity arrays, missing later-added fields/methods, different field
  order) — never assume a 2011 layout applies without independent
  verification via a known IDB size or an allocation-size site in the
  disassembly.
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

## Leads exhausted for now — what's left

As of this writing, `reversing/analysis/leads.json`'s single-source-file
list has mostly bottomed out on two low-value categories, deliberately
deprioritized:

- **Statically-linked third-party libraries** (`Engine/libsrc/libcda-0.4`,
  `Engine/libsrc/allegro-4.2.2-agspatch`, `Engine/libsrc/apeg-1.2.1`,
  `Engine/libsrc/dumb-0.9.2`). These are known open-source libraries, not
  game-specific code — identifying them doesn't move the "reconstruct Rob
  Blanc 1" goal forward the way Engine/Common matches do. Worth doing
  eventually (for completeness / a clean IDB) but low priority.
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
