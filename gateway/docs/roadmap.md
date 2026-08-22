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
- [x] Named `sub_AB180` (7 callers) → `Logics_describeCorridorOnce`,
      confirmed by decoding its real GATESTR.DAT corridor-description
      message, shared as a one-time-description gate across several
      distinct maze corridor rooms. Full writeup in
      [overview.md](overview.md#logics_describecorridoronce-named).
- [x] Named `sub_1D732` (6 callers) → `Opl2_writeRhythmRegister`, plus
      `_opl2TremoloDepth`/`_opl2VibratoDepth`/`_opl2RhythmEnabled`/
      `_opl2RhythmInstruments` — an exact match for OPL2's real
      hardware register `0xBD`. Skipped `sub_14A5F` (unclear verb
      semantics) and `sub_1A0FC` (another corrupted-looking fragment).
      Full writeup in
      [overview.md](overview.md#opl2_writerhythmregister-named--the-opl2-subsystem-grows).
- [x] Named `sub_255A8` (7 callers) → `Vocab_matchesAbbreviation`, plus
      its helper `sub_1AECE` → `Char_toLower` — the classic parser
      abbreviation-matching check (e.g. `n` matching `north`). Full
      writeup in
      [overview.md](overview.md#vocab_matchesabbreviation-named--a-parser-abbreviation-matcher).
- [x] Named `sub_27C31` (7 callers) → `Icon_drawButton` — the mouse-
      driven icon-toolbar drawing function (icon + 3D bevel border),
      confirmed via the already-named `button_strings` global. Full
      writeup in
      [overview.md](overview.md#icon_drawbutton-named--the-mouse-driven-icon-toolbar).
- [x] Named `sub_30D4F`/`sub_3119B` → `thunk_sub_5D9F3`/
      `thunk_sub_5D9F3_2` — two genuine RTLink thunks missed by the
      earlier batch pass due to an IDA function-chunk-merging quirk.
      Full writeup in
      [overview.md](overview.md#thunk_sub_5d9f3thunk_sub_5d9f3_2-named--stragglers-from-the-rtlink-thunk-batch-pass-caught).
- [x] Named `sub_2A90E` (6 callers) → `Surface_advanceSegmentOnCarry`
      — a low-level far-pointer-overflow fixup (add `0x1000` to ES or
      DS based on the caller's carry flag), in the same graphics
      neighborhood as `Surface_getPixelOffset`. Full writeup in
      [overview.md](overview.md#surface_advancesegmentoncarry-named).
- [x] Named `sub_A8577` (6 callers) → `Game_handleWeaponDischarge` —
      the "consequences of firing a gun" handler (Gateway station
      bans weapons), confirmed by decoding all six of its real
      GATESTR.DAT messages. Also named the 32-bit player-money field
      `_playerCreditsLo`/`_playerCreditsHi` (were `Persisted_val213`/
      `word_CF34C`). Full writeup in
      [overview.md](overview.md#game_handleweapondischarge-named--the-consequences-of-firing-a-gun).
- [x] Named `sub_12FC3` (5 callers, called from `main()`) →
      `Queue_processTurn` — the per-turn scheduled-event-queue
      processing entry point, also reused as the WAIT command's inner
      loop (confirmed via `waitMsg`/`j_continue_waiting`). Its
      companion `sub_130D6` and the exact roles of `word_CB7F6`/
      `word_CB808` left for a future pass. Full writeup in
      [overview.md](overview.md#queue_processturn-named--the-turnwait-event-queue-loop-sighted).
- [x] Named `sub_130D6` → `Queue_tickCountdowns` — the countdown-queue
      tick shared by `Queue_processTurn` and the WAIT command loop,
      closing the loop flagged in that earlier pass. Full writeup in
      [overview.md](overview.md#queue_tickcountdowns-named--closing-the-loop-on-queue_processturn).
- [x] Named `sub_15470` (4 callers) → `Logics_lookAtCurrentRoom` — a
      thin wrapper invoking action 8 on the current room, called from
      `main()` and `show_startup`. Full writeup in
      [overview.md](overview.md#logics_lookatcurrentroom-named).
- [x] Named `sub_15AD8` (4 callers) → `Logics_saveOrRestoreHandler` —
      a keyed push/pop save-and-restore of one object's handler state;
      the exact significance of the hardcoded object/handler index
      wasn't determined (no message anchor). Full writeup in
      [overview.md](overview.md#logics_saveorrestorehandler-named).
- [x] Named `sub_15BDA` (4 callers) → `Sound_selectTrackForRoom` — the
      room-to-background-music mapping entry point, called from
      `main()` and `show_startup`. Full writeup in
      [overview.md](overview.md#sound_selecttrackforroom-named).
- [x] Named `sub_16978` (4 callers) → `Windows_setCurrentWindow` —
      switches the active window, returning the previous one for
      save/restore use. Full writeup in
      [overview.md](overview.md#windows_setcurrentwindow-named).
- [x] Named `sub_17A12`/`sub_17A19` → `Listbox_resetStateStack`/
      `Listbox_pushState` — a nested-listbox state stack used to open
      a menu on top of the current one. Full writeup in
      [overview.md](overview.md#listbox_resetstatestacklistbox_pushstate-named--a-nested-listbox-stack).
- [x] Named `sub_18842` (4 callers) → `Speaker_sampleIsr` — the actual
      digitized PC-speaker sample-playback ISR body, closing a small
      piece of an earlier "not traced further" gap. Full writeup in
      [overview.md](overview.md#speaker_sampleisr-named--the-digitized-sample-isr-body-traced).
- [x] Named `sub_1D05A`/`sub_1D1A4` → `Opl2_noteOn`/`Opl2_noteOff` —
      the OPL2 FM-synth's fundamental note-on/note-off primitives,
      confirming a MIDI-to-OPL2 translation layer. Full writeup in
      [overview.md](overview.md#opl2_noteonopl2_noteoff-named).
- [x] Named `sub_1D85B` (4 callers) → `Midi_resetDevice` — an MPU-401
      reset-and-flush helper called from `Midi_initDevice`. Full
      writeup in [overview.md](overview.md#midi_resetdevice-named).
- [x] Named `sub_1E148`/`sub_1E168` → `Midi_peekByte`/
      `Midi_readVarLengthValue2` — a second compiled copy of the
      already-named `Midi_peekTrackByte`/`Midi_readVarLengthValue`
      pair. Full writeup in
      [overview.md](overview.md#midi_peekbytemidi_readvarlengthvalue2-named).
- [x] Named `sub_1FE30` (4 callers) → `Sound_shutdown` — the sound
      subsystem's own full teardown, called from `finish`/`shutdown`.
      Full writeup in [overview.md](overview.md#sound_shutdown-named).
- [x] Named `sub_22954` (4 callers) → `Screen_waitForVerticalRetrace`
      — the classic EGA/VGA vertical-retrace sync primitive. Full
      writeup in
      [overview.md](overview.md#screen_waitforverticalretrace-named).
- [x] Named `sub_2384F` (4 callers) → `getUppercaseKeypress` — a
      single-key menu-choice reader. Full writeup in
      [overview.md](overview.md#getuppercasekeypress-named).
- [x] Named `sub_26228` (4 callers) → `Screen_fadeIn` — the fade-IN
      counterpart to the already-named `Screen_fadeOut`. Full writeup
      in [overview.md](overview.md#screen_fadein-named).
- [x] Named `sub_26F74` (4 callers) → `AnimPics_tick` — the per-slot
      animation-timing/draw loop, closing out the `AnimPics_*` cluster
      entirely. Full writeup in
      [overview.md](overview.md#animpics_tick-named--the-animpics-clusters-last-piece).
- [x] Named `sub_28231` (4 callers) → `Windows_setContentWindow` — a
      trivial two-global window-tracking setter. Full writeup in
      [overview.md](overview.md#windows_setcontentwindow-named).
- [x] Named `sub_2881D` (4 callers) → `LogFile_close` — the
      transcript/log-file close function, called from `finish`/
      `shutdown`. Full writeup in
      [overview.md](overview.md#logfile_close-named).
- [x] Named `sub_2A597` (4 callers) → `Video_getValidIndex` — a
      validated video-mode-index getter used by several drawing
      entry points. Full writeup in
      [overview.md](overview.md#video_getvalidindex-named).
- [x] Named `sub_1063F` (3 callers) → `format_long_decimal` — a
      generic signed 32-bit integer-to-decimal-string formatter. Full
      writeup in [overview.md](overview.md#format_long_decimal-named).
- [x] Named `sub_2A163` (4 callers) → `Dialog_showFormattedPrompt` —
      the core formatted-message auto-sized dialog implementation
      `Dialog_prompt` wraps, confirmed via a real `_vsprintf` call.
      Full writeup in
      [overview.md](overview.md#dialog_showformattedprompt-named).
- [x] Named `sub_2A41D` (4 callers) → `Dialog_restorePrevious` — the
      pop/restore half of a nested-dialog stack, mirroring
      `Listbox_pushState`'s pattern. Full writeup in
      [overview.md](overview.md#dialog_restoreprevious-named).
- [x] Named `sub_13CC7` (3 callers) → `Parser_askForClarification` —
      the ambiguous-preposition disambiguation-request handler,
      confirmed via already-recognized literal strings. Full writeup
      in [overview.md](overview.md#parser_askforclarification-named).
- [x] Named `sub_1452B` (3 callers) → `Logics_checkIsHolding` — an
      implicit "is subject holding object?" precondition check,
      confirmed via a real decoded GATESTR.DAT message. Full writeup
      in [overview.md](overview.md#logics_checkisholding-named).
- [x] Named `sub_15674` (3 callers) → `Game_showIllustration` — the
      full-screen illustration/cutscene display sequence (picture +
      fade-in + delay + caption text), the "major hub function"
      referenced in passing throughout several earlier passes. Full
      writeup in
      [overview.md](overview.md#game_showillustration-named--the-cutsceneillustration-display-sequence).
- [x] Named `sub_158C3` (3 callers) → `TextWindow_addMessageList` —
      `Game_showIllustration`'s text-only fallback for printing
      caption content. Full writeup in
      [overview.md](overview.md#textwindow_addmessagelist-named).
- [x] Named `sub_26F2A` (19 callers, the busiest unnamed function at
      the time) → `AnimPics_finishPlayback` — settles the currently-
      displayed AnimPics frames and swallows a pending skip keypress,
      called immediately before `AnimPics_freeAll` at 6+ sites. Full
      writeup in
      [overview.md](overview.md#animpics_finishplayback-named).
- [x] Named `sub_13629` (4 callers) → `GameDate_format` — formats an
      elapsed day-count into an in-universe "MM-DD-21YY" date string;
      confirms the game's day-counter epoch is May 17, 2102. Full
      writeup in [overview.md](overview.md#gamedate_format-named).
- [x] Named `sub_21B15` (4 callers) → `RawFile_write` — a raw DOS
      INT 21h/AH=40h handle-write wrapper, distinct from the C
      runtime's own `_write`, in the small custom file-I/O group with
      `fseek`/`fsetpos`/`set_filename_prefix`. Full writeup in
      [overview.md](overview.md#rawfile_write-named).
- [x] Named `sub_1B81C`/`sub_1B80C` → `_tzset`/`_tzsetOnce`, plus the
      MSC runtime timezone globals they maintain (`_timezoneLo`/
      `_timezoneHi`/`_daylight`/`_tzname`/`_tznameDst`) — confirmed via
      the literal `"TZ"` getenv string and default `"PST"`/`"PDT"`/
      28800-second data. Full writeup in
      [overview.md](overview.md#_tzset-and-the-timezone-globals-named).
- [x] Named `sub_1B8F0` (3 callers) → `_isindst` — implements the
      pre-2007 US DST rule (first Sunday of April to last Sunday of
      October), closing out the `_tzset` cluster. Full writeup in
      [overview.md](overview.md#_isindst-named).
- [x] Named `sub_18F54` (3 callers) → `Dos_setErrnoFromCode`, plus the
      `errno` global — the real worker behind the already-named
      `__maperror`, translating DOS extended-error codes via a lookup
      table. Full writeup in
      [overview.md](overview.md#dos_seterrnofromcode-and-errno-named).
- [x] Named `sub_25B90` (3 callers) → `Picture_checkFormatMatch` —
      checks a picture's format code against the current video mode,
      called from `Picture_Load`/`load_and_scale_pic`/`scale_pic`.
      Full writeup in
      [overview.md](overview.md#picture_checkformatmatch-named).
- [x] Named `sub_2BCA5` (3 callers) → `Image_allocateSurface` —
      computes buffer size, allocates a handle, and builds the image
      into it; the core of loading an image into a surface, called via
      `sub_24A42` from `Image_load`. Full writeup in
      [overview.md](overview.md#image_allocatesurface-named).
- [x] Named `sub_5CD81` (3 direct callers + thunked callers) →
      `InputWindow_setDisplayMode` — a shared display/input-mode
      switch called from `InputWindow_getLine`, `get_mouse_input`, and
      several room-logic overlays. Full writeup in
      [overview.md](overview.md#inputwindow_setdisplaymode-named).
- [x] Named the undo-snapshot cluster: `sub_62AB0`/`sub_62AE2` →
      `Undo_resetSnapshotBuffer`/`Undo_allocateSnapshotBuffer`, plus
      `_undoSnapshotSize`/`_undoSnapshotHandle`/
      `Parser_undoSnapshotValid`/`Parser_undoBufferAllocated` — the
      in-memory "quicksave" buffer backing `Parser_performUndo`,
      written via `save_game`'s mode-3 path. Full writeup in
      [overview.md](overview.md#the-undo-snapshot-cluster-named).
- [x] Named `sub_1D492` (3 callers) → `Opl2_setOperatorVolume` — the
      volume half of an OPL2 operator update, called from `Opl2_noteOn`
      once per operator. Full writeup in
      [overview.md](overview.md#opl2_setoperatorvolume-named).
- [x] Named `sub_1D570` (3 callers) → `Opl2_setNoteSelect` — writes
      the OPL2 chip-wide NTS/keyboard-split-mode bit (register 8) from
      a global flag set by its caller `sub_1CF90`. Full writeup in
      [overview.md](overview.md#opl2_setnoteselect-named).
- [x] Named `sub_20390` (3 callers) → `Sound_initPlaybackTiming` —
      initializes per-track playback-timing state for one sound
      backend, gated by a bit in the shared `word_C8582` state word
      whose exact backend wasn't pinned down. Full writeup in
      [overview.md](overview.md#sound_initplaybacktiming-named).
- [x] Named `sub_203D6` (3 callers) → `Sound_getElapsedPlaybackTime` —
      the elapsed-time query half of the same per-track timing
      mechanism, sharing both callers with `Sound_initPlaybackTiming`.
      Full writeup in
      [overview.md](overview.md#sound_getelapsedplaybacktime-named).
- [x] Named `sub_157A9` (2 callers) → `Game_showCaptionText` — the
      long-pending `Game_showIllustration` caption-text helper,
      drawing drop-shadowed caption pages over a picture or a black
      background. Full writeup in
      [overview.md](overview.md#game_showcaptiontext-named).
- [x] Named `sub_15F35` (2 callers) → `Sound_lookupTrackVariant` — the
      long-pending "sound resource-variant lookup," mapping a logical
      track ID to a MIDI-specific or default resource number. Full
      writeup in
      [overview.md](overview.md#sound_lookuptrackvariant-named).
- [x] Named `sub_1057E` (2 callers) → `Game_refuseRestart` — the
      handler for a declined "restart" request, confirmed via its
      decoded refusal message; distinct from
      `Game_restartAfterDeath`. Full writeup in
      [overview.md](overview.md#game_refuserestart-named).
- [x] **Correction**: `Speaker_sampleIsr`/`sub_18883`/`sub_18905` were
      mischaracterized several sessions ago as a "digitized PC-speaker"
      engine — renamed to `SoundBlaster_dmaIsr`/
      `SoundBlaster_startNextDmaBlock`/`SoundBlaster_uninstallDmaIsr`
      after tracing confirmed they program the ISA DMA controller and
      the Sound Blaster DSP directly. Full writeup in
      [overview.md](overview.md#correction-the-digitized-pc-speaker-isr-is-actually-sound-blaster-dma-playback).
- [x] Named `sub_18950` (2 callers) → `SoundBlaster_writeByteFromIsr`
      — a private, unbounded-poll duplicate of `Sb_writeByte` used
      from interrupt context, completing the Sound Blaster DMA
      cluster. Full writeup in
      [overview.md](overview.md#soundblaster_writebytefromisr-named).
- [x] Named `sub_1E974` (2 callers) → `Opl2_stopTrack` — the long-
      pending OPL2/AdLib backend's `Sound_stopTrack` handler, silencing
      all 11 OPL2 logical channels. Full writeup in
      [overview.md](overview.md#opl2_stoptrack-named).
- [x] Named `sub_13CB1` (2 callers) → `Parser_printBeMoreSpecific` —
      a simpler, generic sibling of `Parser_askForClarification`. Full
      writeup in
      [overview.md](overview.md#parser_printbemorespecific-named).
- [x] Named `sub_13F85` (2 callers) → `Parser_printTalkingIsStrange` —
      the parser's "talking to a non-conversational target" response,
      confirmed via a decoded GATESTR.DAT message. Full writeup in
      [overview.md](overview.md#parser_printtalkingisstrange-named).
- [x] Named `sub_147A6` (2 callers) → `Parser_callActionHandler` — the
      core action/verb-ID dispatch primitive called from
      `Parser_perform`. Full writeup in
      [overview.md](overview.md#parser_callactionhandler-named).
- [x] Named `sub_169A6` (2 callers) → `Windows_switchListboxWindow` —
      the "cycle focus to the next/previous listbox window"
      navigation primitive. Full writeup in
      [overview.md](overview.md#windows_switchlistboxwindow-named).
      (100th naming batch this project.)
- [x] Named `sub_16A89` (2 callers) → `Listbox_getSelectedIndexForWindow`
      — a getter for a window's currently-selected listbox item,
      called from `Listbox_mouseButtonDown`. Full writeup in
      [overview.md](overview.md#listbox_getselectedindexforwindow-named).
- [x] Named `sub_1CC15`/`sub_1CC34` → `Pit_setReloadCount`/
      `Sound_setTimerRate` — the low-level 8253/8254 PIT reprogramming
      step and its higher-level caller, the shared master timer-rate
      control for the sound engine's tick clock. Full writeup in
      [overview.md](overview.md#pit_setreloadcount-and-sound_settimerrate-named).
- [x] Named `sub_15932` (2 callers) → `Logics_collectPlayerItemLists`
      — snapshots the player's two contained-items linked lists into
      flat arrays. Full writeup in
      [overview.md](overview.md#logics_collectplayeritemlists-named).
- [x] Named `sub_16B53` (2 callers) → `Listbox_getSelectedItemText` —
      reads the highlighted listbox entry as a string, handling both
      raw-text and vocab-word-index storage formats. Full writeup in
      [overview.md](overview.md#listbox_getselecteditemtext-named).
- [x] Named `sub_1796D` (2 callers) → `Listbox_handleNavigationKey` —
      the listbox keyboard-navigation dispatcher (Home/End/PgUp/PgDn/
      arrows/type-ahead). Full writeup in
      [overview.md](overview.md#listbox_handlenavigationkey-named).
- [x] Named `sub_1D58C` (2 callers) → `Opl2_setChannelFeedback` — the
      OPL2 Feedback/Connection-Type register setter, companion to
      `Opl2_setOperatorVolume`. Full writeup in
      [overview.md](overview.md#opl2_setchannelfeedback-named).
- [x] Named `sub_1D5E8`/`sub_1D63E` → `Opl2_setOperatorAttackDecay`/
      `Opl2_setOperatorSustainRelease` — the remaining two per-operator
      OPL2 envelope registers, completing the cluster. Full writeup in
      [overview.md](overview.md#opl2_setoperatorattackdecay-and-opl2_setoperatorsustainrelease-named).
- [x] Named `sub_1D694` (2 callers) → `Opl2_setOperatorModulationFlags`
      — the AM/Vibrato/EG-Type/KSR/Multiple register, the 5th and
      final standard OPL2 register this cluster covers. Full writeup
      in [overview.md](overview.md#opl2_setoperatormodulationflags-named).
- [x] Named `sub_1D786`/`sub_1D3C4`/`sub_1D448` →
      `Opl2_setOperatorWaveform`/`Opl2_setOperatorProperty`/
      `Opl2_applyOperatorSettings` — the Waveform-Select register,
      the property-ID dispatcher, and the "apply all settings"
      entry point, closing out the entire OPL2 per-operator register
      cluster. Full writeup in
      [overview.md](overview.md#the-opl2-per-operator-register-cluster-closed-out).
- [x] Named `sub_1D2FC` (2 callers) → `Opl2_loadOperatorPatch` — the
      "load a MIDI-instrument-patch into an OPL2 operator" entry
      point, the natural top of the whole OPL2 cluster. Full writeup
      in [overview.md](overview.md#opl2_loadoperatorpatch-named).
- [x] Named `sub_1CEC0`/`word_D1C82` → `Opl2_setRhythmMode`/
      `_opl2ChannelCount` — the master OPL2 rhythm-mode toggle and its
      channel-count global. Full writeup in
      [overview.md](overview.md#opl2_setrhythmmode-and-_opl2channelcount-named).
- [x] Named `sub_1CF6E`/`word_D3BD2` → `Opl2_setMasterVolume`/
      `_opl2MasterVolume` — a clamped 1-12 master-volume value feeding
      the OPL2 per-channel instrument-setup cluster. Full writeup in
      [overview.md](overview.md#opl2_setmastervolume-and-_opl2mastervolume-named).
- [x] Named `sub_1861F` (3 callers) → `Opl2_writeDetectRegister` — a
      duplicate-compiled-copy of `Opl2_writeRegister` used by the
      AdLib/OPL2 hardware-presence detection sequence. Full writeup in
      [overview.md](overview.md#opl2_writedetectregister-named).
- [x] Named `sub_1DD8E` (2 callers) → `Sound_takeTrackFlag` — an
      atomic take-and-clear of a per-track ISR-shared flag in the
      sound-timing cluster. Full writeup in
      [overview.md](overview.md#sound_taketrackflag-named).
- [x] Named `sub_1D953` (2 callers) → `Midi_setDataCallback` — an
      atomic setter for the already-named `_midiDataCallback` global.
      Full writeup in
      [overview.md](overview.md#midi_setdatacallback-named).
- [x] Named `sub_1F910` (2 callers) → `Midi_stopTrack` — the long-
      pending MIDI/MPU-401 backend's `Sound_stopTrack` handler,
      paralleling `Opl2_stopTrack`. Full writeup in
      [overview.md](overview.md#midi_stoptrack-named).
- [x] Named `sub_1F93E` (2 callers) → `Midi_stopTrackStep` — the
      20-step MIDI shutdown state machine `Midi_stopTrack`'s busy-loop
      drives, fully confirmed via its MIDI Control Change messages.
      Full writeup in
      [overview.md](overview.md#midi_stoptrackstep-named).
- [x] **Correction**: named `sub_1FB56` → `Midi_sendDisplayText` — a
      Roland-style MIDI SysEx "Display Data" message sender, not a
      screen-clearing call as guessed in `Sound_shutdown`'s much
      earlier writeup. Full writeup in
      [overview.md](overview.md#midi_senddisplaytext-named-corrects-a-much-earlier-guess).
- [x] Named `sub_1FA8E`/`sub_1FAFE`/`sub_1FC4E` →
      `Midi_beginRolandSysEx`/`Midi_endSysEx`/`Midi_busyWaitDelay` —
      the Roland SysEx start/end framing and settle-delay, confirming
      the prediction from `Midi_sendDisplayText`'s writeup. Full
      writeup in
      [overview.md](overview.md#the-roland-sysex-framing-trio-named).
- [x] Named `sub_1FE5C` (868 bytes, 2 callers) →
      `Sound_loadAndStartTrack` — the shared `.MUS` track-loading and
      playback-kickoff worker behind both `Sound_selectTrack` and
      `Sound_selectTrackForRoom`. Full writeup in
      [overview.md](overview.md#sound_loadandstarttrack-named).
- [x] Named `sub_1F63A` (2 callers) → `Midi_prepareTrackData` — the
      MIDI backend's parse/validate step for freshly-loaded track
      data, called from `Sound_loadAndStartTrack`. Full writeup in
      [overview.md](overview.md#midi_preparetrackdata-named).
- [x] Named `sub_1F692` (2 callers) → `Midi_serviceTick` — the MIDI
      backend's regular per-tick service routine, delegating to
      `Midi_stopTrackStep` when stopping. Full writeup in
      [overview.md](overview.md#midi_servicetick-named).
- [x] Named `sub_201C0` (2 callers) → `Sound_serviceTick` — the
      top-level, re-entrancy-guarded sound-engine tick dispatcher,
      called from `room_load`. Full writeup in
      [overview.md](overview.md#sound_servicetick-named).
- [x] Named `sub_24A42` (2 callers) → `Mouse_initCursorSurfaces` —
      allocates the mouse cursor's image surfaces and centers its
      initial position, called from `Mouse_init`. Full writeup in
      [overview.md](overview.md#mouse_initcursorsurfaces-named).
- [x] Named `sub_24FAE` (2 callers) → `Mouse_pollDriverState` — the
      shared low-level DOS mouse-driver poll (`INT 33h AH=3`) behind
      `Mouse_pollPosition` and `get_mouse_buttons`. Full writeup in
      [overview.md](overview.md#mouse_polldriverstate-named).
- [x] Named `sub_27582` (2 callers) → `Region_setValueAndStyle` —
      sets a listbox item's region value/style and refills it, called
      from `Listbox_add`/`Listbox_reset`. Full writeup in
      [overview.md](overview.md#region_setvalueandstyle-named).
- [x] Named `sub_25C52` (2 callers) → `ScalePic_scaleCoordinate` — the
      3/4 or 4/3 coordinate-scaling primitive behind `scale_pic`'s
      EGA↔VGA conversion. Full writeup in
      [overview.md](overview.md#scalepic_scalecoordinate-named).
- [x] Named `sub_25BCE` (2 callers) → `ScalePic_selectPalette` — swaps
      in the palette/lookup table for the scale direction, called
      from `scale_pic`/`load_and_scale_pic`. Full writeup in
      [overview.md](overview.md#scalepic_selectpalette-named).
- [x] Named `sub_26892` (2 callers) → `Surface_beginOverlay` — a
      reusable temporary-drawing-surface-with-save/restore primitive,
      called from `Icon_drawButton`/`Dialog_showFormattedPrompt`. Full
      writeup in [overview.md](overview.md#surface_beginoverlay-named).
- [x] Named `sub_2BE7A` (2 callers) → `Screen_setDrawMode` — validates
      and sets the screen's fill-mode/color before pixel writes,
      called from `Font_writeChar`/`fillRect`. Full writeup in
      [overview.md](overview.md#screen_setdrawmode-named).
- [x] Named `sub_2C6D1` (2 callers) → `Screen_dispatchSpanFill` — the
      shared fast/generic pixel-span rasterizer dispatcher behind
      `Screen_drawLine`/`Screen_fillRect`. Full writeup in
      [overview.md](overview.md#screen_dispatchspanfill-named).
- [x] Named `sub_2137A` (2 callers) → `Memory_fillBytes` — a private
      near-call buffer-fill helper distinct from `_memset`. Full
      writeup in [overview.md](overview.md#memory_fillbytes-named).
- [x] Named `sub_632B9` (2 callers) → `String_copyPadded` — a fixed-
      width, space-padded string copy called from
      `prompt_for_filename`. Full writeup in
      [overview.md](overview.md#string_copypadded-named).
- [x] Named `sub_70F07` (2 callers) → `Font_getTabStopDistance` — the
      tab-expansion distance calculation for `Commset_printText`'s
      proportional-width text layout. Full writeup in
      [overview.md](overview.md#font_gettabstopdistance-named).
- [x] Named `sub_2983A` (2 callers) → `prompt_for_line` — the generic
      line-input prompt loop `prompt_for_filename` specializes for
      filenames. Full writeup in
      [overview.md](overview.md#prompt_for_line-named).
- [x] Named `sub_608AA` (2 callers) → `Parser_clearResultStruct` — a
      release-then-clear cleanup step for a parser-result struct with
      two ref-counted pointer fields, called from `Parser_proc4`. Full
      writeup in [overview.md](overview.md#parser_clearresultstruct-named).
- [ ] Follow up on the turn/WAIT event-queue loop: `word_CB7F6`/
      `word_CB808`'s exact roles still aren't nailed down, and whether
      `word_CB808` is the same countdown mechanism as the weapon-
      confiscation timer from `Game_handleWeaponDischarge`.
- [x] Named `Sb_detectDsp`/`Sb_resetDsp`/`Sb_readByte`/`Sb_writeByte`
      (were `sub_18682`/`sub_186B2`/`sub_186D4`/`sub_186F0`), plus
      `_sbBasePort` — a fourth sound-hardware backend, Sound Blaster
      DSP detection, an exact match for the documented hardware
      protocol. Full writeup in
      [overview.md](overview.md#sb_detectdsp-named--a-fourth-sound-backend-sound-blaster).
- [x] Named `Midi_shutdown`/`Midi_initDevice` (were `sub_1D8CB`/
      `sub_1D966`), and finally corrected the long-flagged mislabeled
      `startGame?` → `Sound_stopTrack`, plus `sub_15DB2` →
      `Sound_selectTrack`. Full writeup in
      [overview.md](overview.md#sound_stoptrack-named--the-startgame-mislabeling-finally-corrected).
- [x] Named `Sound_selectDevice` (was `sub_1FDB8`, called from
      `gatemain_start`) plus `Opl2_detectAndInit`/`Midi_detectDevice`/
      `_midiBasePortConfig`/`_midiIrqConfig` — the `soundMode`
      command-line argument's device-selection dispatcher, ties
      cleanly into the already-confirmed `Sound_stopTrack` flag bits.
      Full writeup in
      [overview.md](overview.md#sound_selectdevice-named--the-sound-mode-dispatcher-confirmed).
- [ ] Unify all four sound-hardware backends (PC-speaker, MPU-401/
      MIDI, OPL2/AdLib, Sound Blaster) with `Sound_selectDevice`/
      `Sound_selectTrack`/`Sound_stopTrack` into one confirmed picture
      of how the engine picks a backend at runtime. `sub_1E974`/
      `sub_1F910` (the other two backend-specific stop routines) and
      `sub_15F35` (the resource-variant lookup helper) still need
      naming.
- [x] Named `sub_5C91C` (5 callers) → `InputWindow_redrawPromptLine` —
      redraws the parser prompt + typed-so-far input line. Full
      writeup in
      [overview.md](overview.md#inputwindow_redrawpromptline-named).
- [x] Named `sub_204CE` (5 callers) → `String_matchesPrefixCI` — a
      separate compiled copy of `Vocab_matchesAbbreviation`'s exact
      algorithm, used in the sound-config-parsing startup area (likely
      `BLASTER`-style env/command-line parsing). Full writeup in
      [overview.md](overview.md#string_matchesprefixci-named).
- [x] Named `sub_249FF` (5 callers) → `Mouse_shutdown` — resets the
      mouse driver and frees resources, confirmed via IDA's own inline
      comment plus several already-named `Mouse_*` helpers. Full
      writeup in [overview.md](overview.md#mouse_shutdown-named).
- [x] Named `sub_2661C` (5 callers) → `load_and_draw_pic` — a one-shot
      load/draw/free picture utility, matching the established
      `load_and_scale_pic` convention. Full writeup in
      [overview.md](overview.md#load_and_draw_pic-named).
- [x] Named `sub_791E2` (5 callers) → `Logics_describePondView` — the
      pond room's detailed environmental description generator,
      confirmed via a real decoded GATESTR.DAT message plus
      already-recognized direction-name string constants. Full
      writeup in
      [overview.md](overview.md#logics_describepondview-named--a-single-rooms-environmental-description-generator).
- [x] Named `sub_80894` (5 callers) → `Logics_describeBeastApproach` —
      a crystal-shard beast-deterrence puzzle handler shared across 4
      rooms, confirmed via five real decoded GATESTR.DAT messages.
      Full writeup in
      [overview.md](overview.md#logics_describebeastapproach-named--a-crystal-shard-beast-deterrence-puzzle).
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
