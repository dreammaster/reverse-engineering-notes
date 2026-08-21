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
- [x] Identified **and renamed** the 5 unknown globals (`word_2A256`/
      `58`/`5A`/`5C`/`5E`) passed as `argv[4]`-`argv[8]` to
      `GATEMAIN.EXE` alongside the confirmed `xmouse`/`videoMode`/
      `soundMode`. Resolved from the `gatemain.idb` side while tracing
      that IDB's `word_C84D0` callback thread (they're
      `gatemain_start`'s `cmdline_param4`-`8`), then applied here:
      `word_2A25A` → `streamMode` (`argv[6]`, corroborated
      independently on *this* side — it's set to the literal value `4`
      in one code path, matching `gatemain.idb`'s `Stream_configure`
      mode==4 special case exactly), and `word_2A256`/`58`/`5C`/`5E` →
      `gatemainArg4`/`5`/`7`/`8` (position-only, neither side decoded
      what these four individually control). Applied via
      `apply_renames_gate.py`'s second batch. Full writeup in
      [overview.md](overview.md#word_2a256-58-5a-5c-5e-named-to-match-gatemainidbs-argv-parsing).
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
- [x] Named `sub_1535E` (92 callers) → `Score_add`, the scoring
      subsystem's accumulator/notification function, plus 5 underlying
      globals (`_score`, `_turnCount`, `_scoreNotifyEnabled`,
      `_scoreNotifyTipShown`, `_gameTicks` — previously `Persisted_val*`
      placeholder names left over from an earlier session's
      `SaveField`-table enumeration). Confirmed definitively by writing
      `ida_scripts/dump_gatestr_messages.py` — a standalone Python
      `huffman_decompress` reimplementation — and decoding the actual
      `GATESTR.DAT` message text those functions print, rather than
      guessing from message-id numbers alone. Kept the script in the
      repo as a reusable tool (same precedent as `ultima1`'s
      `dump_msg_strings.py`) for future message-id ambiguity. Full
      writeup in
      [overview.md](overview.md#the-scoring-subsystem-confirmed-by-actually-decoding-real-game-text).
- [x] Traced `sub_14A37` (80 callers) and its four then-unnamed
      callees — a generic, reusable **chunked file-streaming
      subsystem**, not filename-specific logic: reads a whole file into
      up to 8 RAM buffers (~64KB each, since one DOS allocation can't
      span an arbitrary file), abortable at any point via a keypress or
      config flags, dispatching each chunk through a **configurable
      callback function pointer** for actual processing. Named
      `Stream_loadFile`/`Stream_readChunks`/`Stream_processChunks`/
      `Stream_processChunk`/`Stream_freeChunks` (a new subsystem
      prefix) plus `TextWindow_flushPendingText` (a smaller finding
      picked up along the way). High confidence on the mechanism, lower
      confidence on the exact end-use since the callback target itself
      wasn't traced. Full writeup in
      [overview.md](overview.md#stream_-subsystem-named--a-generic-chunked-file-loader).
      **Tooling note**: hit and fixed a real fragility in
      `apply_renames_gatemain.py` — entries that key by a symbol's *old*
      name break once that rename is actually applied, since the name
      no longer exists to look up. Every entry now keys by hex address.
- [x] Pulled on the `word_C84D0` callback thread. Turned out to be
      several NEAR-call entry points into **one shared decoder
      continuation body** (not 4 independent functions — same "no clean
      chunk boundary" pattern as the RTLink thunks), selected by
      `_streamMode` (was `cmdline_param6`), traced through
      `Stream_selectHandler`/`Stream_configure` (were `sub_18042`/
      `sub_1DDC0`) all the way back to `gatemain_start`'s own `argv`
      parsing. **Major cross-IDB find**: this directly resolves
      `gate.idb`'s long-standing "5 unidentified globals passed to
      `GATEMAIN.EXE`" open item from its very first session —
      `cmdline_param3`/`4`/`5`/`6`/`7`/`8` are `argv[3]`-`[8]`, matching
      `gate.idb`'s `soundMode`/`word_2A256`/`58`/`5A`/`5C`/`5E` exactly
      (`word_2A25A` = `_streamMode`). Renamed `_soundMode`/`_streamMode`
      here; the other four `gate.idb` globals were renamed in a
      following session using this confirmed mapping. Full writeup in
      [overview.md](overview.md#word_c84d0-traced--a-shared-decoder-continuation-not-4-functions).
- [x] Kept pulling on the same thread and found the real gating
      criterion behind `Stream_selectHandler`'s mode branches: not
      resource type (that was only ever a hedge, never confirmed) but a
      **CPU speed rating**, measured fresh via a complete, correctly-
      bracketed calibration routine — `Cpu_beginSpeedTest` (reprograms
      the PIT, installs a temporary tick-counting ISR), `Cpu_measureSpeed`
      (a busy loop timed against it, producing `cpuSpeedRating`), and
      `Cpu_endSpeedTest` (restores the PIT, the ISR vectors, *and* DOS's
      date/time to correct for clock drift — were `sub_18182`/
      `sub_18148`/`sub_181D8`). This refines rather than replaces the
      earlier `Stream_*` naming. Full writeup in
      [overview.md](overview.md#cpu-speed-calibration-decoded--refines-the-stream_-mode-dispatch).
      **Still open**: what operation is actually being performance-tiered
      (needs `sub_18242`/`sub_18432`/`sub_18682`/`sub_186F0` traced),
      and `cmdline_param4`/`5`/`7`/`8`'s individual roles.
- [x] Resolved the above: it's a **digitized PC-speaker sound-effect
      playback engine** — a self-modifying `INT 8` timer ISR that
      bit-bangs the classic speaker-enable port sequence
      (`in al,61h`/`or al,3`/`out al,61h`), CPU-speed-tiered because
      single-bit PC-speaker audio is notoriously sensitive to the ISR's
      own execution time. Almost certainly the playback engine for the
      real install's `.RS` sound-effect files (`AIR.RS`, `ALARM.RS`,
      `BIRD.RS`, etc.) — confirmed a `"STEVE"`-magic header format
      against 5 real files, with an exact `dataLength == fileSize - 32`
      match in every case. New entry in
      [file-formats.md](file-formats.md#rs--digitized-pc-speaker-sound-effect-files);
      full writeup in
      [overview.md](overview.md#its-a-digitized-pc-speaker-sound-effect-engine--and-the-rs-files-are-its-samples).
      No further renames this pass — the remaining pieces are either
      unlabeled inline code within the shared continuation (no clean
      function to rename) or functions (`sub_18415`/`sub_18432`/
      `sub_1861F`/`sub_1863B`) whose exact role isn't confident enough
      yet.
- [x] Re-ran `rank_unnamed_functions.py` (1755 unnamed remaining) and
      named the top 2: `Queue_remove` (was `sub_12ED2`, the sibling to
      the already-named `Queue_add`) and `Logics_checkMoveRestriction`
      (was `sub_14B64`, confirmed by decoding its own printed messages —
      the collar-restraint and dismount-first checks). Full writeup in
      [overview.md](overview.md#queue_remove-and-logics_checkmoverestriction-named).
- [x] Named `sub_14ED6` (44 callers) → `Logics_tryMoveDirection` — the
      core room-exit resolution function: takes a parsed direction
      character, walks the current room's exit table (5 distinct
      exit-type shapes: direct link, `Logics_getBit`-gated door, fixed
      blocked-message, computed destination, and one more), calls
      `Logics_checkMoveRestriction` before committing, falls back to
      `"You can't go that way."` on no match. Full writeup in
      [overview.md](overview.md#logics_trymovedirection-named--the-room-exit-resolution-function).
- [x] Named `Logics_tryMoveDirection`'s three helpers:
      `Logics_getRoomMoveEnabled`/`Logics_getRoomExitCount` (were
      `sub_123F3`/`sub_12445`, matching `Room` struct field accessors)
      and `Logics_callSpecialExit` (was `sub_14742`, a 44-entry
      function-pointer dispatch table for the exit-type-4 "special
      exit" branch). Full writeup in
      [overview.md](overview.md#logics_getroommoveenabled-logics_getroomexitcount-logics_callspecialexit-named).
      The room-exit/movement cluster is now reasonably well documented
      end to end.
- [x] Re-ran `rank_unnamed_functions.py` (1749 unnamed) — top target
      `sub_312D1` (38 callers) turned out to be the game's actual
      win/lose ending, confirmed by decoding its own message text
      ("You have failed." / "You have won the game, scoring %d out of
      1500 points."). Named `Game_showEndingMessage` and its follow-on
      `Game_endGameMenu` (the classic Infocom-style restart/restore/
      undo/quit post-ending prompt). Full writeup in
      [overview.md](overview.md#game_showendingmessagegame_endgamemenu-named--the-actual-winlose-ending).
- [x] Named `sub_149D8` (28 callers) → `Logics_describeContents` — the
      classic "On/In the X you see: ..." container/surface description
      sentence, plus its two helpers `Logics_countVisibleContents`/
      `Logics_listContents` (were `sub_66DFD`/`sub_667D0`). Full writeup
      in [overview.md](overview.md#logics_describecontents-logics_countvisiblecontents-logics_listcontents-named).
      **Also fixed a real tooling bug** found while refreshing the two
      thunks these renames made stale: `apply_rtlink_thunks_gatemain.py`
      (and `find_rtlink_thunks.py`) only recognized `sub_*`-named
      functions as thunk candidates, so every one of the 712
      already-renamed `thunk_*` functions was silently invisible to any
      later re-run — the script's own "safe to re-run" maintenance note
      was wrong the whole time. Fixed and confirmed working (found and
      updated exactly the 2 stale thunks, left the other 710 alone).
- [ ] Continue working down the re-ranked list — the rest of the ~1750
      still-unnamed functions. Also open: `Room.field_16`/`field_18`'s
      deeper semantics (a future `apply_structs_gatemain.py` target),
      the 44 individual `Logics_callSpecialExit` handler routines, the
      two still-undistinguished exit-type branches in
      `Logics_tryMoveDirection`, `sub_C48E4` (`Game_endGameMenu`'s
      actual prompt/choice-reading function), and bit 8's exact meaning
      on contained items (used as a visibility/hidden flag throughout
      the `Logics_*Contents` cluster, not independently confirmed).
- [x] Named `sub_136AF` (27 callers) → `Game_updateStatusLine` — builds
      the status-line text combining the room name with an in-game
      date/time, confirmed by reading its two literal format strings
      directly (`seg067`-embedded, not `GATESTR.DAT`). Also named the
      underlying clock globals `_gameMinutes`/`_gameDayNumber`/
      `_statusTimeHidden` (were `Persisted_val4`/`5`/`7`) — a second,
      more granular clock distinct from the already-named `_gameTicks`.
      Full writeup in
      [overview.md](overview.md#game_updatestatusline-named--the-in-game-clockstatus-bar-builder).
- [x] Named `sub_1496B` (24 callers) → `TextWindow_showMorePrompt` — the
      classic "-- MORE --" screen-pagination prompt: flushes pending
      text, saves cursor, shows `"- MORE -"`, waits for a keypress,
      restores cursor, blanks the prompt, resets the page-line budget.
      Full writeup in
      [overview.md](overview.md#textwindow_showmoreprompt-named--the-classic---more----pagination-prompt).
- [x] Named `sub_153B6` (22 callers) → `Logics_takeObject` — the TAKE
      command mechanics, confirmed by a real call site printing the
      literal `"You take%s"` right before calling it. Also named its
      one-time pickup-score accessor pair `Logics_getTakeScore`/
      `Logics_setTakeScore` (were `sub_12109`/`sub_12179`). Full writeup
      in
      [overview.md](overview.md#logics_takeobject-named--the-take-command-mechanics).
- [x] Named `sub_24FFB` (16 callers) → `Mouse_pollPosition`, confirmed
      via its one real caller `get_mouse_input`. Sighted a real
      animated-picture-overlay engine (`Image_Init`/`Image_load`/
      `Image_draw` + three still-unnamed slot-table functions) while
      investigating the actual top target `sub_26F2A` (19 callers) —
      left that one unnamed, insufficient confidence in two of its
      arrays' downstream consumers. Full writeup in
      [overview.md](overview.md#mouse_pollposition-named--and-an-animated-picture-overlay-subsystem-sighted-but-left-unnamed).
- [x] Named `sub_148E8` (17 callers) → `Logics_printKeyedMessage` — a
      generic keyed-message-table lookup+print utility (with a
      forward-scanning fallback for empty-message matches), confirmed
      generic via two call sites passing unrelated key sources and
      tables. Full writeup in
      [overview.md](overview.md#logics_printkeyedmessage-named--a-generic-keyed-message-table-lookupprint-utility).
- [x] Named `sub_27134` (17 callers) → `AnimPics_freeAll`, plus
      `Image_freeFrames`/`AnimPics_resetForRoom`/`_animPicsSlotCount`/
      `animPicsHandles` — the animated-picture-overlay subsystem's
      teardown routine and supporting globals, correcting last pass's
      mischaracterization of `sub_27134` as a per-tick driver. Full
      writeup in
      [overview.md](overview.md#animpics_freeall-named--correcting-last-passs-guess-about-sub_27134).
- [x] Investigated `sub_15DB2` (15 callers) — a sound-track-selection
      function tangled up with the already-documented `Stream_*`
      subsystem. Confirmed and renamed `get_buff_size?` →
      `get_buffer_size`. Flagged the existing tentative name
      `startGame?` as almost certainly **mislabeled** (it reads as
      "stop the currently playing sound stream", not anything
      game-start-related) but left it and `sub_15DB2`/`sub_15F35`
      unrenamed pending a dedicated follow-up pass. Full writeup in
      [overview.md](overview.md#sound-track-selection-subsystem-sighted--get_buffer_size-confirmed-startgame-flagged-as-mislabeled).
- [ ] Follow up on the sound-track-selection subsystem: nail down
      `sub_15DB2`/`sub_15F35`/`startGame?`'s exact semantics (the
      latter needs correcting, not just naming) and their two
      resource-variant IDs' meaning.
- [ ] Sighted a suspected new RTLink-flattening-bug instance at
      `sub_4A65C`/`sub_4A663`/`sub_4A69F` (jumbled labels,
      `sp-analysis failed`, far calls to unresolved literal targets) —
      not confirmed further, not renamed. Full note in
      [overview.md](overview.md#a-suspected-new-rtlink-flattening-bug-instance-sighted-and-skipped).
- [x] Named `sub_1D896` (15 callers) → `Midi_sendByte` — the MPU-401
      MIDI byte-output primitive, finally tracing the `.MUS` engine's
      hardware-output side flagged as needed several passes ago. Also
      named `_midiDataPort`/`_midiStatusPort` (were `word_C83AA`/
      `word_C83AC`). Full writeup in
      [overview.md](overview.md#midi_sendbyte-named--the-mus-engines-hardware-output-side-finally-traced).
- [ ] Unify the `.MUS`/MIDI playback engine into one confirmed writeup:
      `sub_1D966` (MPU-401 IRQ-driven init), `sub_1EE70`/`sub_1ECB6`
      (tempo meta-event processing), and the already-flagged
      `sub_1FE5C` (periodic background-music-channel refresh) all need
      naming and unifying with `Midi_sendByte`.
- [x] Named `sub_288F4` (14 callers) → `Clock_delayTicks` — a generic
      clock-based busy-wait delay primitive, used by `Screen_fadeOut`
      and others. Full writeup in
      [overview.md](overview.md#clock_delayticks-named--a-simple-busy-wait-delay-primitive).
- [x] Named `sub_2899D` (14 callers) → `Speaker_playErrorBeep`, plus
      its underlying tone-generation primitive `Speaker_playTone` (was
      `sub_28920`) — a real PC-speaker square-wave beep, confirmed as
      the "invalid mouse-click selection" error sound via a real call
      site reached from `get_mouse_input`. Full writeup in
      [overview.md](overview.md#speaker_playerrorbeep-named--pc-speaker-tone-generation-confirmed).
- [x] Named `sub_1CAF6` (13 callers) → `Opl2_writeRegister`, plus
      `_opl2BasePort` (was `word_D3BD0`, confirmed `0x388`) — a whole
      new AdLib/OPL2 FM-synthesis subsystem, distinct from the
      PC-speaker and MPU-401/MIDI engines already documented. Also
      caught a stale/misleading IDA auto-comment on the same
      instruction. Full writeup in
      [overview.md](overview.md#opl2_writeregister-named--an-adlibopl2-fm-synthesis-subsystem-sighted).
- [x] Named `sub_143F3` (12 callers, called from `main()`) →
      `Logics_autoTakeObject` — the "Taking the key first" auto-take
      mechanic, confirmed by decoding real `GATESTR.DAT` text
      (`"[Taking%s first.]"`). Reuses `Logics_takeObject`'s
      take-mechanics tail behind several gating preconditions, not all
      of which were nailed down. Full writeup in
      [overview.md](overview.md#logics_autotakeobject-named--the-taking-the-key-first-mechanic).
- [x] Named `sub_26EDC` (12 callers) → `AnimPics_resyncSlots` — another
      `AnimPics_*`-cluster function, confirmed via `room_load` as the
      "resync all active animation timers to now" step run after a
      room transition. Full writeup in
      [overview.md](overview.md#animpics_resyncslots-named--another-piece-of-the-animated-picture-overlay-subsystem).
- [x] Named `sub_12F81` (11 callers) → `Queue_find`, a read-only
      companion to `Queue_remove` confirmed via direct structural
      match. Skipped `sub_4A616` (12 callers) — another corrupted-
      looking stub in the `sub_4A69F` neighborhood. Full writeup in
      [overview.md](overview.md#queue_find-named--a-companion-to-queue_remove).
- [x] Named `sub_265B0` (11 callers) → `load_and_scale_pic`, confirmed
      via its two helpers' own caller lists matching the already-named
      `scale_pic`/`Image_load`. Full writeup in
      [overview.md](overview.md#load_and_scale_pic-named).
- [x] Named `sub_26D7E` (10 callers) → `AnimPics_registerSlot`, plus
      `Image_clearFrames` (was `sub_26C88`) — closing out the
      `AnimPics_*` cluster (register/free-all/reset-for-room/resync).
      Only `sub_26F74` (per-slot timing/draw loop) and `sub_26F2A`
      remain open in this subsystem. Full writeup in
      [overview.md](overview.md#animpics_registerslot-named--the-last-piece-of-the-animpics-cluster).
- [x] Named `sub_28BB7` (10 callers) → `Window_destroy` — full window
      teardown (close + release regions + clear slot + recompute
      window count + clear active-window), a strict superset of the
      already-named `Window_close`. Full writeup in
      [overview.md](overview.md#window_destroy-named).
- [x] Named `sub_9E8DF` (9 callers) → `Game_restartAfterDeath`, plus
      `_deathCount` (was `word_CE8A8`) — the player-death handler,
      confirmed via two real call sites each printing a decoded
      death message (cliff-jump easter egg; killed by an axe) right
      before calling it. Resets a large swath of persisted state and
      per-object handlers to restart the game after death. Skipped
      `sub_474F8`/`sub_4A722`, more corrupted-looking code near the
      already-flagged `sub_4A69F` cluster. Full writeup in
      [overview.md](overview.md#game_restartafterdeath-named--the-player-death-handler).
- [x] Named `sub_1019C` (8 callers) → `invoke_callback` — a trivial
      far-function-pointer call trampoline. Full writeup in
      [overview.md](overview.md#invoke_callback-named).
- [x] Named `sub_1DD41` (8 callers) → `Midi_bufferByte`, plus
      `_midiBufferPos` (was `word_C8445`) — trivial in itself, but its
      caller is an un-named state machine branching on byte values
      matching Standard MIDI status bytes almost exactly, a strong new
      lead for unifying the `.MUS`/MIDI thread. Full writeup in
      [overview.md](overview.md#midi_bufferbyte-named--a-midi-status-byte-state-machine-sighted).
- [x] Named `sub_1ECDE` (8 callers) → `Midi_readVarLengthValue`, plus
      its helper `sub_1ECB6` → `Midi_peekTrackByte` — a confirmed
      Standard MIDI File variable-length-quantity decoder and its
      per-track byte-peek primitive, further cementing the `.MUS`/MIDI
      thread. Full writeup in
      [overview.md](overview.md#midi_readvarlengthvalue-named--a-midi-vlq-decoder-confirmed).
- [x] Named `sub_2A933` (8 callers) → `Surface_getPixelOffset` — the
      bounds-checked pixel/byte-address primitive underlying
      `Surface_draw`/`Surface_draw2`. Skipped `sub_2609A` (8 callers,
      a graphics-mode color/position setter, insufficient confirmation
      available). Full writeup in
      [overview.md](overview.md#surface_getpixeloffset-named).
- [x] Named `sub_1D84A`/`sub_1D808` → `Midi_sendCommand`/
      `Midi_sendCommand_raw`, plus `_midiCommandPort`/
      `_midiDataCallback` — the MPU-401 command/acknowledge protocol,
      another clean confirmed piece of the growing MIDI cluster
      (byte output, command handshake, VLQ decode, track-byte peek,
      output buffering). Full writeup in
      [overview.md](overview.md#midi_sendcommand-named--the-mpu-401-commandacknowledge-protocol).
- [ ] Unify the whole music/sound-hardware picture: `Speaker_playTone`/
      `Speaker_playErrorBeep` (PC-speaker tones), `Midi_sendByte` +
      `sub_1D966`/`sub_1EE70`/`sub_1ECB6` (MPU-401/MIDI), and
      `Opl2_writeRegister` + `sub_1CB32` (AdLib/OPL2) are three
      separate confirmed sound-hardware backends, likely selected via
      the sound-track-selection subsystem (`sub_15DB2` et al., still
      flagged above) based on detected hardware.
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
