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
- [x] Paul pointed at his installed copies of Gateway (`c:\games\gw`)
      and Gateway II (`c:\games\gw2`) for cross-checking real on-disk
      data. First use of this: traced `vocab_load` (once readable —
      needed the collapsed-function fix above) against the real
      `VOCAB.DAT` (22,081 bytes) and fully decoded its format — a small
      Huffman tree header, compressed text pool, word table, and
      synonym/link table. **First real entry in the new
      [file-formats.md](file-formats.md)**. Full writeup in
      [overview.md](overview.md#vocabdat-decoded--the-first-real-on-disk-format-and-its-huffman-compressed).
      The shared `huffman_decompress` primitive is also used by
      `get_message` (presumably `GATESTR.DAT`'s loader) — worth tracing
      that next given the primitive itself is already understood.
- [x] Traced `gatestr_load`/`get_message` and fully decoded
      `GATESTR.DAT`'s format — sections, per-string (not per-file)
      Huffman compression against one global tree, a common-word
      dictionary extension, and a 32-entry LRU decompressed-string
      cache. Confirmed against the real file (56 sections, header
      matches exactly). Full writeup in
      [overview.md](overview.md#gatestrdat-decoded--sectioned-per-string-compression-with-an-lru-cache)
      and [file-formats.md](file-formats.md#gatestrdat--compressed-messagestring-resource-file).
      `makeRoomInTextCache`'s eviction policy and the shared base
      128-symbol alphabet are the two loose ends left from this pass.
- [x] Traced the room/logic dispatch mechanism (`Logic_call`/
      `Logic_getMethodIndex`) expecting to find a `LOGIC.n`-style
      bytecode resource file — **found there isn't one**. `proc_table`
      (the `LogicIndexEntry` array) is static data linked directly into
      `gatemain.exe`/`gatemain.ovl`; every room/object/handler's actual
      logic is compiled 8086 machine code, not interpretable bytecode.
      Confirmed `Room` is literally variant `type == 1` of the same
      8-way tagged-union table the `LogicSection2`-`8` structs describe
      (each type places its `_methodIndex` field at a different offset,
      selected via a jump table). **Major scoping implication for the
      eventual ScummVM reimplementation** — see the writeup in
      [overview.md](overview.md#the-roomlogic-format--and-why-there-isnt-one-its-compiled-native-code-not-data)
      and the closing note in
      [file-formats.md](file-formats.md#roomlogic-format--there-isnt-one-its-compiled-code).
- [ ] Follow-up from that pass: connect `MethodSectionMap` (data
      immediately preceding `proc_table`, pairs like `<34, 2>`) to the
      `METHOD_SECTION_INFO`/`sub_11635`/`Logics_getPrehandlerMode`
      "prehandler chain" layer flagged two sessions ago — plausibly
      related, not confirmed. Also: what the 8 types actually represent
      semantically (only `Room` = type 1 is known), and the remaining
      unnamed shared fields (`_val1`-`_val4`, `_unkHandlerId`,
      `_prehandlerId`) across the 8 variant structs.
- [x] Traced `OBJECT.DAT` (`objects_load`/`Logics_getObjectString`) —
      the simplest of the three real external formats: a `u16`
      byte-length header + a raw blob of concatenated NUL-terminated
      ASCII strings (object/room/NPC names), confirmed against the real
      8,031-byte file. No index table in the file — each entity's name
      offset is a previously-unlabeled common field (offset 0) shared by
      all 8 `LogicIndexEntry` variant structs, itself part of
      `proc_table`'s static compiled data. Also found a 44-entry dynamic
      name-override table (`LOGIC_STRINGS`/`LogicStrings_call`,
      explaining the odd `LogicStringsNN` function names from the
      collapsed-function fix). Full writeup in
      [overview.md](overview.md#objectdat-decoded--plain-string-blob-offsets-baked-into-proc_table)
      and [file-formats.md](file-formats.md#objectdat--objectroomnpc-name-strings).
      Turned out **not** to be the `Thing` struct's on-disk form after
      all — `Thing` itself (`{id, str1, str2, str3}`) still isn't
      confirmed against any on-disk data; may just be an in-memory
      runtime structure built from these pieces rather than something
      read directly off disk.
- [x] Traced `GATE_XXX.RGN` (`load_regions`) and `GATE_XXX.PIC`
      (`load_picture`/`Image_load`). Regions: direct-seek `RegionIndex`
      → `RegionEntry` hit-rects, with a video-mode-dependent Y-coordinate
      rescale (96/224, 168/224, or unscaled depending on display height)
      confirming a fixed authoring resolution adapted per hardware mode
      at runtime. Pictures: derived the exact picture-numbering scheme
      from code (`bank = picNumber>>12` forced to 1 on non-default video
      hardware when 0; `fileNumber = bank*100 + ((picNumber>>8)&0xF)`)
      and it **exactly** reproduces the five real `GATE_0xx`-`4xx.PIC`
      file groupings — strong independent confirmation. Decoded the
      12-byte `PicIndexEntry` (direct-seek, no count prefix, same style
      as `.RGN`) and the multi-frame draw-position table following it.
      Full writeup in
      [overview.md](overview.md#gate_xxxrgngate_xxxpic-decoded--regions-and-multi-frame-pictures)
      and file-formats.md.
- [x] Traced `PictureDecoder_load2`/`_unpack`/`_fetch` — the actual
      pixel compression is a real **LZ77+Huffman hybrid** (canonical
      Huffman-coded literal/match tokens, 4096-byte sliding window, 8KB
      double-buffered output), not a simple RLE scheme. Confirmed the
      two video-mode-specific blit callbacks are exactly the two
      expected pixel strategies for this era — a linear byte copy for
      chunky/packed VGA modes, and classic 4-plane EGA/Tandy planar
      bit-unpacking (each decompressed byte is a 4-bit plane-membership
      mask, not raw pixel data) for EGA/Tandy modes — meaning the
      ScummVM renderer needs to reproduce EGA's plane-serial write
      behavior specifically, not just decompress to a generic pixel
      buffer. Full writeup in
      [overview.md](overview.md#picture-pixel-compression-decoded--a-real-lz77huffman-hybrid)
      and [file-formats.md](file-formats.md#picture-pixel-compression--an-lz77huffman-hybrid-plus-per-video-mode-blit).
- [ ] `PictureDecoder_getBlockOffset`'s match-distance bit-packing, and
      the precise per-table semantics of `_array1`-`_array13`, weren't
      fully traced — enough is understood to describe the compression
      architecture accurately, not to reimplement every table yet. Also
      still open: whether picture banks 1-4 correspond to the game's
      four story acts, `RegionIndex.field_2`'s meaning, and a couple of
      still-unnamed flag bits in the `.RGN`/`.PIC` formats.
- [x] Traced `GATE_XXX.FNT` (`Font_LoadFont`) — clean, complete: 10-byte
      header, optional 128-byte variable-width/spacing tables, packed
      1bpp glyph bitmap over the declared printable-char range. Same
      banking convention as `.PIC`. Full writeup in
      [overview.md](overview.md#gate_xxxfntgate_xxxmus-traced--one-clean-win-one-honestly-murky-one)
      and [file-formats.md](file-formats.md#gate_xxxfnt--bitmap-font-files).
- [~] Traced `GATE_XXX.MUS` (`sub_1FE5C`) partially — confirmed the
      file-numbering convention and a general lazy/memory-budget-gated
      full-track-loading architecture, but **not** the exact per-song
      12-byte directory record layout (no pre-existing struct for this
      format, unlike every other resource type). First format in the
      whole project where a pass didn't reach full confidence — flagged
      honestly rather than backfilled. Next attempt should start from
      the sound-hardware-output side, not the memory-management side.
      Same writeup links as `.FNT` above.
- [x] Re-ran `rank_unnamed_functions.py` (thunk noise filtered out,
      1769 unnamed remaining). Top target `sub_11635` (196 callers) was
      the same one flagged two sessions ago — now readable end-to-end
      since `Logics_getPrehandlerMode` is no longer hidden by the
      collapsed-function issue. Named it `Logics_prehandlerChainReaches`
      plus its exact-match sibling `sub_115CE` →
      `Logics_prehandlerHasMode`, fully explaining the "prehandler
      chain" mechanism (including confirming `LogicSection6`'s
      `_prehandlerId`/`_prehandlerId2` are one 2-element array, not two
      separate fields). Applied via the new
      `ida_scripts/apply_renames_gatemain.py` (gatemain.idb's first
      accumulating rename script). Full writeup in
      [overview.md](overview.md#prehandler-chain-primitives-named).
      Checked but didn't confidently name `sub_14A37` (80 callers) — its
      callees are all themselves unnamed.
- [ ] Continue working down the re-ranked list — `sub_1535E` (92
      callers), `sub_14A37` and its unnamed callees, and the rest of the
      1767 still-unnamed functions.
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
- [x] `docs/file-formats.md` created 2026-08-21 with the Huffman
      compression primitive and `VOCAB.DAT`'s full format — room, logic
      section, and picture formats still to come.
- [x] Confirmed what produced the `_decoded` executables — see the
      RTLink-flattening-tool section in overview.md (Paul's own custom
      tool, not IDA-native). Whether the original packed distribution
      files are worth having as a separate reference wasn't specifically
      settled, but Paul's installed game copies at
      `c:\games\gw`/`c:\games\gw2` now serve the same cross-referencing
      purpose for on-disk resource data.
- [ ] Eventually: start the actual C++/ScummVM reimplementation, and
      begin generalizing findings toward documenting the shared engine
      for Legend's other titles — both explicitly deferred until real
      progress exists on at least one Gateway executable.
