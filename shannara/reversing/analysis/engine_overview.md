> **Addendum (later pass):** see [`entry_and_init_flow.md`](entry_and_init_flow.md)
> for the top-level `main`/`setup_demo`/`init` control flow (not covered
> below), [`drawing_primitives.md`](drawing_primitives.md) for the resolved
> pixel/line "graphics pen" layer, and — important correction —
> [`compression_lzhuf.md`](compression_lzhuf.md): `Pic` decompression
> (§3 "Picture / image system" below, and `Pic.isCompressed` in §4) is
> **LZHUF (LZSS + adaptive Huffman)**, not RLE as stated below.

# Shannara Demo — Reverse Engineering Notes

Working notes from an IDA Pro-assisted analysis of `Shannara Demo.asm` (the disassembly export
of the DEMO.LE executable). Covers the executable itself, the third-party libraries it statically
links, and the game's own engine subsystems (structures + functions). This is a snapshot of
understanding as of this pass — see "Open questions / follow-ups" at the end for what's still
unresolved.

---

## 1. The executable

| | |
|---|---|
| Original path | `D:\sources\legend\shandemo\DEMO.LE` |
| Format | Linear Executable (LE) |
| Target | OS/2-style LE container, but OS type flagged `MS DOS`, application type `Console Executable Singlethreaded 32bit` |
| Entry point | `1:00007638h` |
| Program flags | `Application Compatible with PM` |
| Attributes | Readable, Executable, PreloadPages, Big |

This is the classic pattern for a **32-bit DOS-extended executable** compiled with **Watcom C/C++**
(explicitly confirmed by a compiler-inserted comment: `; Watcom v9-*1.5  32bit common runtime` on
`set_pic_comp`) and packaged with a DOS extender that uses the LE format (e.g. the DOS/4GW family
common to Watcom-built DOS games of this era, ~1995-96). "Shannara" was published by Legend
Entertainment; `shandemo` is this title's demo build.

The disassembly (with embedded/partially-recovered debug information) is large: **1,162 functions**
and **15 recovered structures**. A first export from IDA had ~1,073 of those functions collapsed
(folded) in the outline view, which meant their instruction bytes weren't present in the exported
text at all — the working copy used for this analysis is a **fully expanded** re-export
(87,497 lines, 0 collapsed-function markers).

---

## 2. Third-party libraries statically linked in

Roughly a third of the 1,162 functions are not Shannara's own code — they're statically-linked
library internals. Recognizing these matters because it explains *why* a chunk of the binary
looks unfamiliar, and because two of them (Miles/AIL and MS Graphics) have public, documented
APIs that cross-validate what the disassembly shows.

### 2a. Miles Sound System / AIL ("Audio Interface Library") — 256 functions
Prefixes `_AIL_*` (118), `_AIL_API_*` (109, the internal implementation layer behind the public
`_AIL_*` wrappers), `_SS_*` (8, the DIG/"Sound System" digital-audio driver construction path),
`_XMI_*` (21, the MDI/XMIDI music driver construction path). This is John Miles' Audio Interface
Library (later renamed Miles Sound System) — an extremely widely-used commercial audio middleware
for DOS/Windows games of the era. Function names match the well-documented public AIL 2.x API
almost exactly (`_AIL_allocate_sample_handle`, `_AIL_set_sample_volume`, `_AIL_start_sample`,
`_AIL_register_EOS_callback`, etc.). The **`AILPreferences`** struct (`_AIL_preference`) belongs
to this subsystem — see §4.

Confidence note: the *presence and behavior* of these functions is directly readable from the
disassembly (e.g. `_AIL_API_set_preference` provably treats `_AIL_preference` as a flat array —
confirmed, not guessed). Exact historical AIL SDK header contents (precise parameter names/types
for every one of the 256 functions) were **not** asserted from memory-recall — that would risk
mismatching this specific old DOS-era AIL version. Phase 2's automated signature pass treats these
the same as any other function (mechanical register/stack analysis only).

### 2b. Watcom C runtime — ~66+ functions
Double-underscore-prefixed internals (`__SetIOMode`, `__fill_buffer_`, `__filbuf_`,
`__InitFiles`, `__set_binary`, etc.) plus single-underscore ones like `sopen`, `printf`,
`memmove`. The **`FILE`** struct (`_ptr`, `_cnt`, `_base`, `_flag`, `_handle`, `_bufsize`,
`_ungotten`, `_tmpfchar`) is the standard Watcom/MS C runtime `FILE` layout and was already
fully and correctly named by the embedded debug info — no changes needed.

### 2c. Microsoft C 7.0 Graphics Library ("graph.h") — `_GetState`, `_GrInit`, `_InitState`,
`_CalcNumPages`, `_clearscreen`, `_L2clearscreen`, `_TxtClear`, and friends. The
**`ConfigBuffer`** struct (`__ConfigBuffer`) turned out to be an exact match (by size *and*
independently-verified field usage — BIOS data-area reads, `int 10h` video-mode queries,
`__AdapTab`/`__MonTab`/`__MemoryTab` lookups) for the public, documented Microsoft C
`struct videoconfig` (returned by `_getvideoconfig()`). This is the highest-confidence struct
identification in the whole pass — see §4.

### 2d. Shannara's own low-level runtime (not third-party, but infrastructure rather than
game logic): the `gx*`-prefixed graphics primitives (16 functions — `gxVideoAddr`,
`gxSetVirtualHeader`, `gxCreateVirtual`, `gxClearVirtual`, `gxVirtualDisplay`,
`gxVirtualDecompress`, `gxDestroyVirtual`, etc.) form a small custom "virtual buffer" blitting
layer sitting on top of raw video memory (see `Display` in §4). Everything else (~800 functions)
is genuine Shannara game/engine code — room logic, UI, picture/font/animation/palette
management, the in-house "Q" cutscene codec, etc.

---

## 3. Engine subsystems (what the structures + functions actually do)

### Picture / image system — `Pic`, `PicCache`
Every on-screen graphic ("Pic") is loaded from a game data file through a **3-entry LRU file
cache** (`PicCache`/`_pic_file_cache[3]`, keyed by file number, evicted by an age counter). A
`Pic` slot carries a magic-number tag (`0xA712`) marking it as a valid/initialized slot
(independent of whether it currently holds loaded image data — that's tracked by a separate
"active" byte), an optional animation frame count, and either an inline 17-byte "gx virtual
header" (static images) or a pointer to an array of such headers (one per frame, for animated
pics). Loading goes through `load_palette` → `read_pic_dir` (which also does the LRU
housekeeping) → `load_pic` (allocates the pixel buffer via `new_handle`, optionally
RLE-decompresses via `gxVirtualDecompress`). `display_pic`/`clear_pic`/`unload_pic` are the
show/free/reset trio; `clear_pic` reuses `unload_pic`'s exact reset pattern after tearing down
the gx virtual buffer(s).

### Low-level blitting — `Display`, `WindowBuffer`, the `gx*` functions
`Display` (`_display`, one global instance) is the physical screen descriptor: a base pointer, a
scanline stride, a pointer-to-the-actual-pixel-buffer-pointer (`pixelsPP`, one level of
indirection so the buffer can be reallocated/moved), and a total-byte-count. The `gx*` functions
are a small custom blitter API operating on that plus the "virtual buffer" headers embedded in
`Pic`/`FlixEntry`-adjacent data (create/destroy/clear/display/decompress a virtual off-screen
buffer, and address-calculate into the physical screen). `WindowBuffer` is unrelated — it's a
16-byte accumulated-text-line buffer per UI text window.

### UI — `Region`, windows, fonts
`Region` is a mouse-hotspot / clickable-widget record, one array per window
(`_rgn[win][idx]`, count in `_num_regions[win]`). It's fundamentally a rectangle (`x1,y1,x2,y2`)
plus a type byte that drives a 7-way dispatch in `make_region` (plain clickable region, up/down
arrow buttons, or a "list elevator" — a scrollbar-style widget whose thumb is a set of
sub-regions that all translate together, tracked via a count + a pointer to an array of `{x,y}`
point pairs). `move_region`/`resize_region` manipulate the rectangle; `get_region`/`in_region`
do hit-testing (respecting a `flags` bit that marks a region hidden/disabled).

`Font` is a 2-slot LRU cache (same age-counter pattern as `PicCache`) of loaded bitmap font
resources: a first/last character range, base char width + inter-character spacing (each with a
sign-bit convention that switches to a per-character override table when negative — a classic
"fixed-pitch unless the flag says proportional" bitmap-font design), a packed-nibble kerning
table for an even more detailed proportional mode, and the actual 1bpp glyph bitmap data sized
`charCount × bytesPerRow × charHeight`.

### Memory management — `MemoryBlock`
A classic handle-based ("relocatable block") memory manager, `_master_table[]`, modeled closely
on the Mac OS/Windows 3.x "handle" idiom: each entry tracks a handle key, a *second* level of
indirection (`handleIndex` into `_handle_table[]`, which holds the actual current pointer — so
`compact_memory` can move blocks around without invalidating outstanding handles), a byte size,
and flags/type used to identify which purge callback (`_purge_vector_tbl[]`) can reclaim the
block under memory pressure.

### Cutscenes / video — `FlixEntry`, and the in-house "Q" codec (`QHeader`, `QInfo`, `QMap`)
`flix_play` interprets an array of `FlixEntry` "script" records — a tiny bytecode for cutscene
sequencing: each entry has a trigger frame/tick, an opcode (13 cases: play a DIB/PCX/GIF still,
open/close a Q video stream, call an arbitrary function pointer, etc.) and one or two generic
opcode-dependent parameter slots. The Q video format itself (`QHeader`/`_qheader`, one per open
stream) is a proprietary **block-based video codec**: a signature-checked header describing a
grid of fixed-size pixel blocks (`blocksWide × blocksHigh`, each `blockWidth × blockHeight`
pixels) plus interleaved audio (sample rate + a frame-rate divisor used to size each frame's
audio chunk, fed straight into AIL's `_AIL_set_sample_playback_rate`/sample-buffer API).
`QInfo`/`_qinfo` tracks the current decode window as a block-coordinate rectangle. `QMap` is a
low-level bit-expansion lookup table used by the block decompressor (`sub_1652E0`, invoked from
`q_expand`) — genuinely deep codec internals that weren't worth reverse-engineering bit-by-bit
for this pass.

### Palette / color cycling — `ColorCylce` (sic — that's the actual spelling in the recovered
struct name)
`_p_cycle[4]` holds up to 4 simultaneous color-cycle ranges. Each slot has a "pending" half (set
by `set_color_cycle(index, start, end, rate)`, called by game/room code) and an "active" half
that `display_palette` swaps the pending values into once per call (clearing pending back to
zero and flipping a global `_cycling_on` flag if any slot is live). Classic double-buffered
palette-animation design.

---

## 4. Structure reference (all 15)

All field renames below (and the reasoning behind them) were applied via an IDAPython script
already run against the IDB — this table is the human-readable summary. Confidence is noted
per field; anything left as `field_N` had insufficient evidence and was deliberately not guessed.

### `Pic` (0x1B / 27 bytes) — one loaded picture/animation slot
| Offset | Field | Notes |
|---|---|---|
| 0x0 | `signature` | magic `0xA712`, marks a valid `Pic` slot (not an "active" flag) |
| 0x2 | `picId` | picture index within its source file |
| 0x4 | `frameCount` | 0 = static image, >0 = animated |
| 0x5 | `_active` | real "currently loaded" boolean (already named) |
| 0x6 | `colorDepth` | medium confidence |
| 0x7 | *(unnamed)* | set once on load, never cleared — low confidence guess: "everLoaded" |
| 0x8, 0x9 | *(unnamed)* | sourced from two on-disk header bytes, purpose unclear |
| 0xA | `_gxVirtual` | buffer handle; high byte doubles as current-frame index when animated |
| 0xC | `nextFrameTime` | **resized dw→dd**; was mis-split across old `_nextTime`+`field_E`+`field_F` |
| 0x10 | `frameHeaders` | pointer to per-frame gx-header array (17-byte stride) when animated |
| 0x14 | `_vector` | set via `set_pic_vector`; likely an animation-complete trigger id |
| 0x18 | *(unnamed)* | inconclusive evidence |
| 0x1A | `isCompressed` | gates RLE decompression |

### `PicCache` (0x1408) — `_pic_file_cache[3]`, LRU-cached open picture files
`lastUsed` (age counter), `fileNum` (0xFFFF = empty), `_fp` (file handle, already named),
`_buffer` (5120-byte raw block, already named).

### `Region` (0x20) — `_rgn[win][idx]`, UI hotspot/widget
`regionType`, `flags` (bit 0x80 = hidden), `x1`/`y1`/`x2`/`y2` (rectangle), `elevatorItemHeight`,
one unnamed field, `elevatorCount`, `elevatorPoints` (list-elevator sub-widget, types 3/6 only).

### `Font` (0x1B) — `_fonts[2]`, LRU-cached bitmap fonts
`lastUsed`, one unnamed byte, `_grafx_mode` (already named), `_fontNum` (low byte only — see
caveat above), two unnamed on-disk header bytes, `firstChar`, `lastChar`, `lineSpacing`,
`charHeight`, `charWidth`, `charSpacing`, one unnamed byte, `charWidths` (table ptr),
`charKerningTable` (table ptr), `glyphBitmap` (raw bitmap data ptr).

### `Display` (0x10) — single global `_display`
`_ptr` (already named), `stride`, `pixelsPP` (already named — pointer-to-pointer to the pixel
buffer), `_count` (already named — total buffer byte size).

### `WindowBuffer` (0x10) — `_w_buffer[__num_windows]`
Single `buffer` field (already named), unsubdivided text-line accumulator.

### `MemoryBlock` (0xC) — `_master_table[]`, handle-based memory manager
`_ptr` (already named, the handle key), `handleIndex` (index into `_handle_table[]`), `size`
(already named), `_flags` (already named, bit 0x80 = valid entry), `_type` (already named,
purge-callback selector).

### `FlixEntry` (0x10) — `flix_play`'s cutscene-script command records
`frame` (trigger tick), `arg1`, `param` (**merged dw+dw → one dd**; function-pointer-or-resource
union slot), `arg2`, two unnamed fields.

### `QHeader` (0x1A) — `_qheader`, Q-codec stream header
`signature` (magic `0x6839`), one unnamed word, `blocksWide`, `blocksHigh`, `blockWidth`,
`blockHeight`, one unnamed word, one unnamed dword, `frameRate` (only byte 0 of a 6-byte array
is used), `audioSampleRate`.

### `QInfo` (0x8) — `_qinfo`, current decode window in block coords
`minCol`, `minRow`, `maxCol`, `maxRow`.

### `QMap` (0x2) — `_q_qmap_table[]`, codec bit-expansion entries
Both bytes left unnamed — genuine deep codec internals.

### `ConfigBuffer` (0x16) — `__ConfigBuffer` = MS C `struct videoconfig` (very high confidence,
public documented API, exact size + field-usage match)
`numXPixels`, `numYPixels`, `numTextCols`, `numTextRows`, `numColors`, `bitsPerPixel`,
`numVideoPages`, `mode`, `adapter`, `monitor`, `memory`.

### `ColorCylce` (0x10) — `_p_cycle[4]`, palette color-cycle slots
`counter`, `activeStart`, `activeEnd`, `activeRate`, `pendingStart`, `pendingEnd`, `pendingRate`.

### `AILPreferences` (0x48) — `_AIL_preference`
Not a real struct — confirmed to be one flat `preference[18]` S32 array (Miles/AIL SDK design).
Reshaped accordingly; per-index names deliberately not guessed (would need a period-correct AIL
SDK header to do safely).

### `FILE` (0x1A) — standard Watcom/MS C runtime `FILE`. Already fully and correctly named.

---

## 5. Function signatures (phase 2)

Of the 1,162 functions:
- **42** already carried a real applied prototype from the embedded debug info (trusted as-is).
- **731** got an automatically-derived best-effort signature applied (`idc.SetType`), based purely
  on mechanical analysis of the actual disassembly:
  - **Stack parameters** — read directly from each function's own IDA-computed stack frame (any
    declared local at a *positive* offset is, by x86 calling-convention construction, a real
    incoming parameter). Where a real name already existed it was kept; otherwise it stayed a
    generic `arg_N`.
  - **Register parameters** — a linear scan of each function's entry code (after skipping the
    callee-saved-register push prologue) tracks which of `eax`/`edx`/`ebx`/`ecx` are *read before
    written*, matching this codebase's consistently-observed calling convention (registers filled
    in that order, then overflow to the stack).
  - **Names** — under 2% of functions in this file carry any inline parameter-naming comment, so
    the large majority of generated signatures use generic `a1`/`a2`/… names. Every one of those
    is tagged with a function comment flagging it as low-confidence.
- **19** single-instruction thunks and **370** functions with no detected parameters were left
  untouched rather than assert something unverified.

**Known limitation:** the register scan stops at a function's first branch, so a real parameter
only consumed after an early guard-clause branch can be undercounted (verified example:
`set_color_cycle` genuinely takes 4 parameters — `ecx,al,dl,ebx` — but only `ecx` was detected,
because the other three are read after the function's first two branches). This was a deliberate
tradeoff against the opposite failure (misreporting a clobbered scratch register as a phantom
parameter), which was a bigger problem before it was fixed.

**Syntax note:** every `__usercall` declaration explicitly names its return register
(`int __usercall foo<eax>(...)`) — `idc.SetType` rejects `__usercall` with a non-void return type
if this is omitted, which was found and fixed after the first apply attempt.

---

## 6. Open questions / follow-ups

- `AILPreferences`' 18 individual preference indices are not named — would need a period-correct
  AIL/Miles SDK header (2.x-era) to do safely.
- Several `Pic`, `FlixEntry`, and `QHeader` fields have no confirmed purpose (`Pic+0x7/0x8/0x9/0x18`,
  `FlixEntry+0xC/0xE/0xF`, `QHeader+0x2/0xA/0xC`) — would need call-site-by-call-site tracing of
  every room/script that touches them.
- `QMap`'s two bytes are genuine block-decompressor internals (`sub_1652E0`) — untangling exactly
  what `dword_18B2BC[]`/`dword_18B6BC[]` represent would need real codec-level RE, not just
  disassembly skimming.
- Phase 2's automated signatures should be treated as a *starting point*, not ground truth,
  especially the ~700 LOW-confidence ones — real parameter names/types still need to come from
  actually reading each function (as was done by hand for the ~40-60 picture/region/font/memory
  functions referenced throughout §3, which are correspondingly higher-confidence).
- The 21 unnamed `sub_XXXXXX` functions and the ~800 named-but-unexamined game-specific functions
  (room logic, UI dispatch, etc.) haven't had their *bodies* read at all — phase 2 only touched
  their entry code for signature inference.
