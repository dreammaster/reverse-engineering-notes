# Ultima V (DOS 1.16) decompilation audit

Audit of the [u5d](../u5d-main) decompilation (copied to [`../src`](../src) as our working tree) against the original game
binaries in `C:\games\ultima5` (`ULTIMA.EXE` plus the 23 `*.OVL` overlays and `DATA.OVL`), done before starting the ScummVM
engine. Everything below is reproducible with `python audit/run_all.py` (needs IDA Pro 8.3 for the disassembly step).

## Summary

| Question | Answer |
|---|---|
| Is any original function missing from the C? | **No.** All 491 overlay functions and 198 of the 224 `ULTIMA.EXE` functions have a C counterpart. The other 26 are C runtime (19), the file/LZW/memory loader (5), an empty function and the entry stub — replaced by the host. |
| What does "95% matched" mean then? | Instruction-level equality with the Microsoft C 5.1 output, which we cannot reproduce (no compiler, no original linker). It says nothing about missing functions; the 95% is a *codegen* figure. |
| Do the C functions do what the binary does? | Mostly verified: 568 of 688 functions have identical callee sets and data references (607 once video-driver state and negative-offset constants are ignored); the rest were triaged and are explained (jump tables, string pools, stubs) except where listed under *Defects found*. |
| Is the data right? | Yes: 529 initialised variables, 47 pointer tables and 2,214 string literals equal the original `DATA.OVL`, apart from one reversed table (fixed). |
| What actually deviates from the original? | The platform layer, the graphics-driver code (not checked pixel for pixel), and 157 conditional blocks that the modern build adds. The 74 hand-written assembly functions were all compared with the disassembly. See *Gaps*. |
| Can we compile and test? | Yes. `mingw32-make` builds a native Win32 binary with no SDL; a headless key-script/screenshot harness and unit tests are included (see [README](../README.md)). |

## Binary layout (needed for any further analysis, and for the ScummVM port)

* `ULTIMA.EXE` is a Microsoft-C real-mode EXE (header 0x800 bytes; CS offset 0 = file offset 0x800). Code is `0000`–`72e0`,
  matching the `0000.c`–`7000.c` split of the sources (`ULTIMA_<offset>_<name>`); the u5d function names are code offsets.
* The 23 overlays are **raw code images with no header**, linked with offsets starting at 0 (`TOWN_0170` = `TOWN.OVL` offset
  0x170). They are loaded into one of four *load areas*; the overlay's CS is the EXE's CS plus a per-area shift, so an overlay
  `call` into the EXE has target `F - shift` (mod 64K):

  | area | shift | overlays |
  |---|---|---|
  | A | `81d0` | TOWN, MAINOUT, DUNGEON, INTRO, FLAMES |
  | B | `a290` | BLCKTHRN, COMBAT, DNGLOOK, ENDGAME, LOOKOBJ, NPC, OUTSUBS, SHOPPES |
  | C | `bf80` | CAST, CMDS, SJOG, TALK |
  | D | `e1e0` | CAST2, COMSUBS, FONT, SHOPPES2, SHOPPES3, ZSTATS |

  (Overlays in the same area are never resident together.) The shifts were recovered automatically
  (`audit/derive_overlay_bases.py`): for every overlay one shift sends 5–54 external call targets (about half to all of them)
  exactly onto EXE function starts, against at most 6 for the runner-up; the rest are calls into the stub table.
* Calls to an overlay function from the EXE or from another overlay go through **12-byte stubs** in the EXE at `7a16`–`81ba`:
  `9A ec 02 xx xx` (far call to the overlay loader) `NN 00` (overlay number, 1-based in the order of the name table at file
  offset 0x8113: TOWN, MAINOUT, DUNGEON, INTRO, FLAMES, NPC, COMBAT, BLCKTHRN, LOOKOBJ, DNGLOOK, OUTSUBS, SHOPPES, ENDGAME,
  SJOG, CMDS, CAST, TALK, CAST2, ZSTATS, COMSUBS, SHOPPES2, SHOPPES3, FONT, DATA) `EA lo hi 00 00` (`jmp far`, offset = overlay
  offset + area shift). 164 stubs = the overlay API; `audit/parse_stubs.py` decodes them.
* `DATA.OVL` is the DGROUP image with a 0x10-byte header (DS offset = file offset − 0x10). Overlay string literals live here
  too, which is why the sources write `_TEXT(0x4c114, "...")`.
* The graphics drivers (`EGA/CGA/HER/T1K.DRV`) are 38-entry `jmp` tables (3-byte entries at offsets 0x00–0x6f). The EXE calls
  them with `mov word [5350h], <slot>` / `call far [5350h]`; it uses 24 distinct slots, all of which are implemented by
  `grap_drv.c`/`grap_buf.c`. `MOSLO.COM` is Mo'Slo, a third-party CPU-slowdown utility shipped with the collection, not game code.
  `FLAMES.OVL` is a 32-byte busy-wait stub.

## Method

1. **Function inventory** — headless IDA (`ida_scripts/export_functions.py`) finds functions from `push bp; mov bp,sp` plus call
   targets plus every offset the C names, sweeps the bodies of functions for switch cases IDA left undefined, and exports callees,
   data addresses and immediates per function.
2. **Function coverage** (`audit_functions.py`) — maps each C definition `MODULE_<hex>` to a binary function.
3. **Semantic fingerprint** (`fingerprint.py`) — we cannot recompile with MSC 5.1, so instead compare what each function *does*:
   its callees (overlay calls resolved through the stub table and area shifts) and the data-segment addresses it touches against
   the C after preprocessing in the original-matching configuration (`TARGET_DOS16`).
4. **Constants** (`constants.py`) — integer immediates vs integer literals in the preprocessed C (strings keep their DS address).
5. **Data** (`data_audit.py`, `pointer_tables.py`) — every initialised global in `vars.c` and every string/pointer table against `DATA.OVL`.
6. **Manual review** of the assembly-derived functions, by disassembling and comparing line by line (below).
7. **Deviation catalogue** (`deviations.py`) — everything the modern build changes.

Generated detail: [function_audit.md](../audit/function_audit.md), [fingerprint.md](../audit/fingerprint.md),
[constants.md](../audit/constants.md), [data_audit.md](../audit/data_audit.md), [deviations.md](../audit/deviations.md).

## Results

### Coverage

* Overlays: 491 functions, 491 covered (BLCKTHRN 11, CAST 35, CAST2 20, CMDS 26, COMBAT 23, COMSUBS 22, DNGLOOK 17, DUNGEON 33,
  ENDGAME 9, FLAMES 2, FONT 10, INTRO 21, LOOKOBJ 26, MAINOUT 35, NPC 17, OUTSUBS 12, SHOPPES 25, SHOPPES2 14, SHOPPES3 8,
  SJOG 36, TALK 39, TOWN 32, ZSTATS 18).
* `ULTIMA.EXE`: 224 functions, 198 covered (including `main` = offset 0). Not covered:
  * `017e`–`0877` C runtime: 18 documented in `0000.c` (`_astart`, `exit`, long-math helpers, `_setargv`, ...) plus the
    undocumented `0350`; the host libc replaces them. The long-math helpers are why some C expressions have no call in the fingerprint.
  * `135a` LZW file reader, `15c6` allocator, `160e`/`1649` read file, `1654` file size — replaced by `common/lzw.c`, `common/file.c`.
  * `2316` is an empty function (frame setup/teardown only, never called); `81d0` is the entry stub.
* 13 EXE functions + `FONT_0e52/0e7b` are stubs in C: video-mode restore/init/detect/driver loading/page allocation (`0878`, `0892`,
  `0e94`, `0f2a`), timer calibration (`11b4`), default drive (`16a6`), BIOS key-buffer flush (`1b16`, `1b24`), the PC speaker
  (`22e2`, `230e`), the DOS read/write wrappers (`7234`, `7296` — implemented through stdio), `ULTIMA_1f4e` (never called) and
  the FLAMES busy-wait delays.

### Fingerprint and constants

| | functions | no differences |
|---|---|---|
| callees + data refs | 688 | 568 (607 if the video-driver state variables `52ba`/`5350`/`5354`/`535c`/`5394` are ignored) |
| constants | 688 | 555 |

Triage of the remainder (hand-checked samples from each category): jump-table dispatchers whose case labels or function-pointer
tables appear as constants (`CAST_0dba`, `TALK_0f32`, spell/command tables); base+offset constants folded by the compiler
(`&D_b21e[0x229]` → `b447`, `D_3810[x - 0x29]` → `37be`); overlay string-pool addresses; and the video-driver state in
`0000.c`/`1000.c`/`2000.c` that the host graphics layer replaces. No remaining difference changes the control flow of a
non-assembly function; the three defects below were found by the data audit and by reading the assembly-derived functions the
fingerprint singled out.

### Data

* 529 of 535 non-pointer initialised variables identical; 47 of 47 string/pointer tables identical; 2,214 of 2,214 `_TEXT`/`_STATIC_TEXT`
  literals identical to `DATA.OVL`.
* The six exceptions: `D_52ba_vdp`, `D_52f3`, `D_5304`, `D_5358` (initial *video driver* state — the host layer sets it),
  `D_6708_map` (206 stale bytes in the image, overwritten at run time) and `D_b21e` (runtime scratch buffer, beyond the image).
  `D_1a58` also differed — see below.

### Defects found (all fixed in `src/`)

1. **`D_1a58` reversed** (`vars.c`). The table of leading spaces used to centre the character name in the stats header
   (`ZSTATS_0082`) was `{0,1,1,1,1,1,2,2,1}`; `DATA.OVL` holds `{1,2,2,1,1,1,1,1,0}` (indexed by `"AMBFDTPRS"`).
2. **`ULTIMA_6f1e` took its behaviour from the FM Towns version** (`6000.c`). The IBM routine is a case-insensitive substring
   search — index of the first argument inside the second, or −1, ignoring bit 7 and folding with `& 0x5f`, and resuming
   `k + 1` characters on after a partial match of `k` (so `"AAB"` is *not* found in `"AAAB"`). The FMT port compared an uppercased
   prefix with `strncmp`. Callers: the shrine "Upon what virtue dost thou meditate?" / mantra prompts (`cast2.c`, `cmds.c`), the typed-word checks in
   `cmds.c`, and the "Thy wish?" vehicle easter egg (`lookobj.c`). Replaced by a byte-for-byte port; unit-tested.
3. **Non-DOS builds used the C library `rand()`** (`2000.c`) instead of the game's 16-bit generator
   (`seed = ror(seed + 0x9248, 3) ^ 0x9248, + 0x11; result = (seed & 0x7fff) % n + min`). The game re-seeds with game state in
   places (`town.c`: `srand(D_587e)`) and expects the same sequence back, so every non-DOS build diverged from the original
   there. Now all builds use the original generator and the original clock-derived seed (`GetTime`); `U5_SEED=n` fixes the seed.

### Assembly-derived functions (74)

These are the functions the u5d author could not match and re-wrote in C. All 74 were compared with the disassembly
(`audit/asm_review.py` prints each one next to its C port). Results:

* **Equivalent** (about 60): the text engine (`16ba`, `17f4`, `1b94`, `1bf2`, `1c22`/`1c5b`, `1c9e`, `1cca`, `1cee`, `1f12`, `1f26`, `1f77`,
  `1184`, `1226`), clip window and rectangle helpers (`08ca`, `08e6`, `0991`, `0a22`, `0c3c`, `0ccd`, `0d2b`), the driver wrappers that
  only marshal registers (`0a70`, `0aa6`, `0ace`, `0b86`, `0b10`, `0bfc`, `0c22`, `0c64`, `0c9c`, `0cf2`, `0d4c`, `0f46`, `0f6e`, `0f90`,
  `1044`, `1068`, `10e0`, `1112`, `1140`, `6f9e`, `6fbc`, `6fd6`, `INTRO_2090/20ae`), `1588` (compressed-file name test), `6f90`, `6ff0`
  (symmetric 6x6 table lookup), `NPC_12e0` (schedule slot, including its quirk of returning 1 instead of 3 for the fourth entry),
  and the audio entry points `22c0`/`2192`/`223c` (parameter order checked; the synthesis itself is in `audio/`).
* **Fixed** (earlier): `2056`/`2092`/`207e` (PRNG, clock seed), `6f1e` (substring search).
* **Fixed in this pass** (`0000.c`, `1000.c`):
  * `ULTIMA_0935_ClipLineCoord` tested the outcodes wrongly — it returned "nothing to clip" as soon as *either* endpoint was inside
    (the original: only when *both* are), rejected when the outcodes differed (the original: when they share a bit), and
    returned the endpoints swapped after clipping the first one. Rewritten from the register flow.
  * `ULTIMA_0b2d_GRAP_Line` chose between pixel / horizontal / vertical / general line using the *unclipped* arguments; the original
    uses the registers after clipping (matters for a line clipped down to a point or an axis-parallel run).
  * `ULTIMA_0ff4_LoadTileset` ignored a load failure; `1674` (`TestOpenFile`) and `1d02` (`LoadCharset`) did not report failures
    the way the original does (disk-swap message plus a zero return so the caller's retry loop runs; `LoadCharset` also rejects
    index >= 4 with -2). These only matter when a file is missing.
  * `ULTIMA_0aa6_GRAP_3f_FillRect` had debug assertions that fail for valid one-pixel-wide rectangles.
* **Differences kept** (platform): file/memory/drive helpers (`0bae`, `0be4`, `0fae`, `0fdc`, `125d`, `1eac`, `1ef7`), timer
  (`1158`, `11b4`, `20c8`, `20fa`), video detection (`0de0`) and `1d5e` (`PollKey`; the original also maps F1-F10 to `c9`-`d2`, which
  nothing in the game consumes, and flags keypad digits via the BIOS shift/NumLock state, which the host cannot reproduce).
  `20c8` indexes its shift table by byte offset rather than word index (harmless: both entries are 0).
  `ULTIMA_0ccd` swaps pointers instead of values, so the horizontal-line driver call sees (x1, x2) in caller order rather than
  (max, min); the driver treats both the same.

Not covered by the review: the *behaviour* of the graphics-driver functions themselves (`DRV_xx` in `grap_drv.c`, `graphics/`),
i.e. what the EGA/CGA/Hercules/Tandy driver code does with the pixels. That would need either an x86 emulator running the original
`EGA.DRV` or reference screenshots from the original game.

## Gaps and what to do about them

| gap | status | note |
|---|---|---|
| Missing game logic | none found | |
| Platform layer (video/timer/speaker/drive/keyboard/CRT) | replaced by the host | ScummVM provides its own; the list of replaced functions above is the complete API surface to re-create |
| Hand-written graphics driver code (EGA/CGA/Hercules/Tandy) | re-implemented in `graphics/grap_buf.c` | the C drivers were not compared pixel for pixel with the original; 24 driver entry points are used, all present |
| PC speaker / music | `audio/` synthesises sfx (SDL) and plays FM Towns-style BGM; the Win32 build is silent | original IBM sound is PC-speaker only |
| Save format | `CAST2_10fe` uses the FM Towns-style writer (`FILE_WriteSavegameFile`) | on-disk layout equals `SAVED.GAM`; the IBM sequence also rewrites `UNDER.OOL`/`BRIT.OOL` — compare before relying on it |
| 157 conditional blocks / 162 annotations | catalogued in [deviations.md](../audit/deviations.md) | each needs a keep/revert decision for ScummVM (`ENABLE_BUG_FIX`, `ENABLE_SPEED_FIX`, `GOG_BUILD`, ...) |
| Original bugs preserved in the C | see `// BUG` comments (e.g. `cmds.c:927` always-false range test, `cmds.c:1028` uninitialised `local_a`, `6000.c:620`) | keep for fidelity; ScummVM may want to fix |

## Next steps

1. Verify the graphics-driver routines (`DRV_xx`) against the original EGA driver: run `EGA.DRV` in an emulator, or capture
   reference screenshots from DOSBox, and compare with the goldens.
2. Decide the keep/revert policy for the deviation list; make `MATCHING_BUILD`-style behaviour selectable at run time.
3. Build a save-game fixture with a party (the pristine `SAVED.GAM` has none) so the headless harness can drive towns, combat and
   dungeons, and record golden screenshots from the original-matching configuration.
4. Document the data formats the ScummVM engine will read (maps, `*.TLK/*.NPC`, `*.CBT`, `*.DAT`, `*.16/*.4` image formats); the
   structures are already declared in `src/structs.h` and `src/macros.h`.
