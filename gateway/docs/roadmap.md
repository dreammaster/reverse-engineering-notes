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

Not started. 177/502 functions already named from earlier tentative
manual work (untrusted — re-verify, don't assume). 21 structs already
defined, likely including shared engine plumbing (`REGS`, `VIDEO_MODE`,
`FONT`, `SCREEN`, `PIC_HEADER`/`PIC_DATA2`/`PIC_DATA`, `MESSAGE`,
`VOCAB_FILE_REC`, `VOCAB_ENTRY`, `STR16`, `POINT`, plus DOS/CRT-shaped
ones like `RTLINK_SEG`, `HANDLE`, `timeb`, `WORDREGS`/`BYTEREGS`).

- [ ] Read `_main`'s top-level flow to confirm the title-screen/cutscene
      role hypothesis (same first move as `ultima1`'s `ULTIMA.EXE` pass).
- [ ] Check for a CRT/engine layer shared with `gatemain.idb` (same
      `_fopen`/`_nmalloc`/DOS-primitive cluster pattern found identical
      across all five `ultima1` executables) — if present, transferring
      already-known names from whichever executable gets analyzed first
      is the fastest early win, same as every `ultima1` pass after the
      first one.
- [ ] Check whether `gate.idb` chains to `gatemain.idb` (or vice versa)
      via a custom overlay loader, matching `ultima1`'s
      `writeInUseAndExit`/`chainToExecutable`/`execProgram` mechanism —
      important for how the ScummVM engine module should model switching
      between the two, if it needs to at all (`ultima1`'s `.EXE`s were
      launched as an actual DOS command-tail chain; Gateway's `_decoded`
      executables may instead reflect a single combined runtime split
      only for IDA's convenience — not yet known).
- [ ] `rank_unnamed_functions.py` hasn't been run against this IDB yet —
      do that before picking targets, same approach as `ultima1`'s
      `OUT.EXE` first pass.

## `gatemain.idb` — next steps

Not started. 807/3288 functions already named from earlier tentative
manual work (untrusted — re-verify). 49 structs already defined,
including what looks like a full AGI/SCI-style adventure-engine resource
layer: `Room`, `LogicIndexEntry`/`LogicSectionEntry`/`LogicSection2`
through `LogicSection8`, `VocabFileRec`/`VocabEntry`/`VocabSet`/
`StateVocab`, `Parser_Data1`/`ParserHandlerEntry`/`ParserHandlerData`/
`ParserHandlerArrEntry1`/`ParserHandlerArrEntry2`, `Picture`/
`PictureDecoder`/`PicIndexEntry`/`Image`/`Surface`, `Thing`,
`FunctionEntry`, `SaveField`, `MethodSectionMap`, `RegionIndex`/
`RegionEntry`, `QueueEntry`, `TempSavedEntry`.

- [ ] Read `_main`'s top-level flow to confirm the main-game-engine role
      hypothesis.
- [ ] Given the struct list, prioritize understanding the **room/logic
      section format** first — likely the single highest-value target
      for both the ScummVM engine and documenting the shared engine for
      Legend's other titles, analogous to how `ultima1`'s savegame/map
      formats were flagged as the top `file-formats.md` candidates.
- [ ] `rank_unnamed_functions.py` hasn't been run against this IDB yet.
      2481 unnamed functions is a lot more than any single `ultima1`
      executable (max was 353) — worth deciding a sub-scoping strategy
      (e.g. by segment/module, or by which struct's accessors) rather
      than one flat ranked list, before actually working through it.
- [ ] Cross-check the structs shared by name/concept with `gate.idb`
      (`VocabEntry`/`VOCAB_ENTRY`, `Str16`/`STR16`, `Point`/`POINT`,
      `Screen`/`SCREEN`, `Font`/`FONT`, `REGS`, `HandleEntry`/`HANDLE`)
      actually agree field-for-field — different case conventions
      between the two IDBs suggest they were defined independently in
      separate sessions, same open item `ultima1` has for its own
      cross-IDB structs.

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
