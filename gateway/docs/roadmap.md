# Roadmap

Working list of next steps, roughly in priority order. Check items off /
add notes as they're resolved — this file is meant to stay current across
sessions, unlike a one-off todo list. See [overview.md](overview.md) for
the per-executable breakdown this is tracking against.

## Infra (done, 2026-08-21)

- [x] Reviewed the sibling `ultima1` project's headless IDA pipeline
      (`ida_scripts/run_ida_script.ps1` + `batch_run_and_export.py` +
      `identify.py` + `rank_unnamed_functions.py`) as a model.
- [x] Ported the pipeline to `gateway/ida_scripts/`, generalized for
      Gateway's two IDBs (`gate.idb`, `gatemain.idb`) the same way
      `ultima1`'s driver generalized for its five — deriving export
      paths from whichever `.idb` `idat.exe` was actually pointed at,
      rather than hardcoding a filename.
- [x] Ran `identify.py -NoExport` against both IDBs to catalog current
      state (root file, entry point, input MD5, segments, function/
      struct naming progress) — see the table in overview.md.
- [ ] Smoke-test a full export+save round-trip against one Gateway IDB
      (pick the smaller `gate.idb` first) before the first real rename
      pass — confirmed working for `ultima1_space.idb` when that
      pipeline was first built, not yet exercised here.
- [ ] Confirm `.gitignore` at the repo root (`c:\dev\legend\.gitignore`)
      correctly excludes IDA's transient per-database files
      (`*.idb`/`*.id0`/`*.id1`/`*.id2`/`*.nam`/`*.til`) for both `gate`
      and `gatemain` — already present and pattern-based (not filename-
      specific), so should just work, but not independently re-verified
      for this project.
- [x] Fixed a real blind spot: some functions in both IDBs were left
      GUI-"collapsed" (`FUNC_HIDDEN`) by earlier manual sessions, which
      made `gen_file(OFILE_ASM)` print a one-line placeholder instead of
      real instructions for them — invisible to every grep/Read of the
      `.asm`. Wrote `ida_scripts/unfold_functions.py` and ran it against
      both IDBs: 171 collapsed functions found in `gatemain.idb`
      (including `vocab_load`, `objects_load`,
      `Logics_getPrehandlerMode`), 108 in `gate.idb` (mostly CRT
      internals). Both re-exported. Full writeup in
      [overview.md](overview.md#fixed-a-pipeline-blind-spot-collapsed-functions-were-invisible-in-every-asm-export).
      Worth re-running this any time a function search comes back
      suspiciously empty.

## Executable order

Not yet decided with Paul. Per `overview.md`'s working hypothesis:
`gate.idb` (`gate_decoded.exe`, 177/502 named, 32 segments) is the
smaller title-screen/cutscene executable; `gatemain.idb`
(`gatemain_decoded.exe`, 807/3288 named, 308 segments) is the much larger
main game engine (rooms, logic sections, parser/vocab, pictures).

`ultima1`'s own executable-ordering precedent went two different ways
across its five executables: it started with the *largest, most-central*
one (`OUT.EXE`, the main game) since that was the highest single payoff,
but also found real value starting with the *smaller* title-screen
executable (`ULTIMA.EXE`) early since it clarified the chaining
architecture before diving into the big one. Worth Paul's call which
pattern fits better here — starting with `gate.idb` (smaller, might
clarify the gate↔gatemain relationship and any CRT/engine layer shared
with `gatemain.idb`, analogous to how `ULTIMA.EXE`'s CRT-transfer pass
gave a head start before `GEN.EXE`) vs. starting with `gatemain.idb`
directly (much bigger payoff per function found, but 6.5x the size and
no smaller executable's findings to transfer in first).

## `gate.idb` — next steps

Started 2026-08-21. 177/502 functions already named from earlier
tentative manual work (untrusted — re-verify, don't assume). 21 structs
already defined, likely including shared engine plumbing (`REGS`,
`VIDEO_MODE`, `FONT`, `SCREEN`, `PIC_HEADER`/`PIC_DATA2`/`PIC_DATA`,
`MESSAGE`, `VOCAB_FILE_REC`, `VOCAB_ENTRY`, `STR16`, `POINT`, plus
DOS/CRT-shaped ones like `RTLINK_SEG`, `HANDLE`, `timeb`,
`WORDREGS`/`BYTEREGS`).

- [x] Smoke-tested the full export+save round-trip against this IDB —
      `gate.asm`/`gate.idc` now exist and are committed.
- [x] Read `_main`'s top-level flow, confirming the title-screen/
      cutscene role hypothesis: `set_file_prefix("GATE")`, `show_intro`,
      a `current_section`-keyed dispatch, then handoff to `GATEMAIN.EXE`.
      Full writeup in
      [overview.md](overview.md#_main-confirms-the-titleintro-role-and-the-gategatemain-handoff-is-real-dos-exec).
- [x] Confirmed the gate→gatemain handoff mechanism: **real DOS `EXEC`**
      (`_execl`/`_execve`/`__doexec`, genuine MSC CRT), not a custom
      overlay loader like every `ultima1` executable used — a real
      architectural difference between the two projects, not just a
      naming-convention difference. Simplifies the ScummVM
      reimplementation's job (no in-engine "mode switch" needed for this
      handoff). Full writeup in overview.md.
- [ ] Identify the 5 unknown globals (`word_2A256`/`58`/`5A`/`5C`/`5E`)
      passed as `argv[4]`-`argv[8]` to `GATEMAIN.EXE` alongside the
      confirmed `xmouse`/`videoMode`/`soundMode`.
- [ ] Trace `current_section`'s value meanings (0/1/2/3 confirmed as
      real/load-bearing via ~30 xrefs, semantics not yet decoded) and the
      `show_intro` cluster's functions (`0x1F000`-`0x23000` range,
      dozens of functions, not traced function-by-function yet).
- [ ] Trace `sub_1BED2` (called from `_main`'s EXEC-failure path and from
      `Font_writeString`) — confirmed **not** a simple print wrapper (it
      resolves a message ID via `get_message` and reads BIOS cursor
      position via `_int86`/`INT 10h`), but its exact role wasn't fully
      pinned down this pass.
- [x] Ran `rank_unnamed_functions.py` for the first time against this IDB
      (325 unnamed). Used it to pick the first real rename targets.
- [x] First renaming pass: decoded a 5-function/2-global color cluster
      shared between `_main`'s startup screen-clear and
      `Font_writeChar`'s glyph rendering — `max_color_index`,
      `current_draw_color`, `Font_setColors`, `Font_setColorsClamped`,
      `setDrawColor`, applied via `ida_scripts/apply_renames_gate.py`.
      180/502 functions now named. Full writeup in
      [overview.md](overview.md#gateidb-color-cluster-decoded).
- [ ] Follow-ups surfaced by this pass: name `sub_1C0C4` (the actual
      box-fill/draw-rect primitive, shared with `Font_writeChar` via
      `sub_24E26`/`sub_25D7A` — traced enough to know the call shape,
      not enough to name confidently yet); decide what `word_2F0AE`/
      `word_2F0AC` (raw/clamped shadow copies of fg/bg color) are for;
      and consider an `apply_structs_gate.py` pass on the `SCREEN`
      struct now that 3 of its fields (`0xC`, `0x22`, `0x24`) have
      confirmed roles from this session (all 21 fields are still
      `field_0`..`field_2A` placeholders).
- [ ] Still need a CRT/engine-layer cross-check against `gatemain.idb`
      (same `_fopen`/`_nmalloc`/DOS-primitive cluster pattern `ultima1`
      found identical across all five of its executables) — not done
      yet; likely the fastest next win once `gatemain.idb` gets its own
      pass, transferring names in whichever direction is missing them.

## `gatemain.idb` — next steps

Started 2026-08-21. 1519/3288 functions now named (was 807 at session
start). 49 structs already defined, including what looks like a full
AGI/SCI-style adventure-engine resource layer: `Room`,
`LogicIndexEntry`/`LogicSectionEntry`/`LogicSection2` through
`LogicSection8`, `VocabFileRec`/`VocabEntry`/`VocabSet`/`StateVocab`,
`Parser_Data1`/`ParserHandlerEntry`/`ParserHandlerData`/
`ParserHandlerArrEntry1`/`ParserHandlerArrEntry2`, `Picture`/
`PictureDecoder`/`PicIndexEntry`/`Image`/`Surface`, `Thing`,
`FunctionEntry`, `SaveField`, `MethodSectionMap`, `RegionIndex`/
`RegionEntry`, `QueueEntry`, `TempSavedEntry`.

- [x] Smoke-tested/performed the full export+save round-trip for this
      IDB for the first time — `gatemain.asm`/`gatemain.idc` now exist
      and are committed (15MB/394k-line `.asm`).
- [x] Read `main`'s top-level flow, confirming the main-game-engine role
      hypothesis and the Early-engine text-parser architecture directly:
      `setjmp`/`longjmp`-based save/load/undo restart, `PARSER_OOPS`/
      `PARSER_UNDO`/`PARSER_AGAIN` meta-command handling, the classic
      "I beg your pardon?" did-not-understand response. Full writeup in
      [overview.md](overview.md#mains-top-level-flow-confirms-the-early-engine-text-parser).
- [x] Ran `rank_unnamed_functions.py` for the first time (2481 unnamed).
      Top target `sub_11635` (196 callers) traced but not named yet — a
      real `Logics_getPrehandlerMode`-driven interpreter internal,
      deferred rather than guessed.
- [x] **Major finding**: decoded gatemain's RTLink overlay-linker
      call-thunk mechanism and batch-renamed all 712 genuine
      cross-function thunks via the new
      `ida_scripts/apply_rtlink_thunks_gatemain.py` — moved this IDB
      from 807/3288 (25%) to **1519/3288 (46%)** functions named in one
      pass. Also hardened `rank_unnamed_functions.py` to auto-exclude
      this IDB's thunk boilerplate from future rankings. Full writeup in
      [overview.md](overview.md#rtlink-overlay-architecture-decoded--and-a-major-function-count-correction).
- [ ] Name the real interpreter internal `sub_11635` (196 callers) —
      traced mechanically (recursive prehandler-stage walk via
      `Logics_getPrehandlerMode`/`METHOD_SECTION_INFO`) but not
      confidently named yet.
- [ ] Given the struct list, prioritize understanding the **room/logic
      section format** next — likely the single highest-value target
      for both the ScummVM engine and documenting the shared engine for
      other Early-engine Legend titles (see the engine-lineage note in
      overview.md) — analogous to how `ultima1`'s savegame/map formats
      were flagged as the top `file-formats.md` candidates. Not started;
      `logic238`-style already-named functions seen throughout the
      codebase are a likely entry point (probably one compiled logic
      script per room/number, echoing AGI's `LOGIC.n` convention, not
      confirmed by reading one directly yet).
- [ ] Re-run `rank_unnamed_functions.py` now that the thunk noise is
      filtered out, to pick the next real targets from the 1769
      remaining unnamed functions.
- [ ] Cross-check the structs shared by name/concept with `gate.idb`
      (`VocabEntry`/`VOCAB_ENTRY`, `Str16`/`STR16`, `Point`/`POINT`,
      `Screen`/`SCREEN`, `Font`/`FONT`, `REGS`, `HandleEntry`/`HANDLE`)
      actually agree field-for-field — different case conventions
      between the two IDBs suggest they were defined independently in
      separate sessions, same open item `ultima1` has for its own
      cross-IDB structs.
- [ ] Periodically re-run `apply_rtlink_thunks_gatemain.py` (idempotent)
      as more thunk targets get real names, so `thunk_sub_XXXXX`-style
      names stay in sync rather than going stale.
- [x] Checked the 243 same-owning-function thunk-shaped cases against
      Paul's known rare bug in the RTLink-flattening tool (unpatched
      segment word on an intra-segment far call) using the new
      `ida_scripts/diagnose_thunk_chunks.py` — all 243 came back clean
      (plausible segment values, valid decoded targets), confirming
      they're genuine split multi-chunk functions, not flattening
      artifacts. Full writeup in
      [overview.md](overview.md#the-_decoded-executables-are-pauls-own-rtlink-flattening-tools-output-not-ida-native).
      Keep `diagnose_thunk_chunks.py` around — worth re-running this
      style of check any time a far call/jmp elsewhere resolves to
      something implausible, since the bug is confirmed real even
      though it didn't hit this particular case.

## Cross-IDB follow-ups (parking lot, revisit once both executables have real passes)

- [ ] Rename segments to a `CODE`/`DATA` convention once roles are
      confirmed (almost none are renamed yet in either IDB — bigger gap
      than any single `ultima1` executable had).
- [ ] Reconcile the two IDBs' differently-cased same-concept structs
      (see above).
- [ ] Create `docs/file-formats.md` once the first on-disk resource
      format (room, logic section, picture, or vocab file) is actually
      traced — don't stub it empty, same rule `ultima1` followed.
- [ ] Confirm what produced the `_decoded` executables and whether the
      original packed distribution `.exe`s are available/worth having
      as a reference (may carry overlay/segment metadata the decoded
      form doesn't).
- [ ] Eventually: start the actual C++/ScummVM reimplementation, and
      begin generalizing findings toward documenting the shared engine
      for Legend's other titles — both explicitly deferred until real
      progress exists on at least one Gateway executable.
