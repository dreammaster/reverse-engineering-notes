# Q video codec (demo `q_open`/`q_load`/`q_expand`, block decoder `sub_1652E0`)

**Confirmed on real data (2026-10-16):** `scripts/q_play.py` decodes the demo's `Logosvga.q` (79 frames) and `Brona_2.q` (191 frames) completely,
every frame consuming exactly its chunk, and the frames are the correct pictures (Legend logo animation, the Brona/sword scene). Real files
use only chunk types 1, 7, 5, 2 (no audio in the .Q: `audioSampleRate` = 0; sound is a separate VOC), 640x480 (160x120 blocks), 10 fps.
Type 6/7 chunks carry a size field of 768 but **no payload**. Decoder tables are saved in `scripts/q_tables.json`.

Retail tries `NAME.Q` before `NAME.FLC` (see retail_game_layer.md section 13), so this is the main cutscene format.
**The block decoder is verified**: `scripts/q_decode_ref.py` matches the real machine code (run under unicorn by
`scripts/q_emulate.py`) on thousands of random streams (`scripts/q_fuzz.py`: pixels and consumed bytes identical;
frame sizes 3x2 to 160x2, full frames, windows (types 3/4) and raw mode (type 7)). The container layer is verified the same way: `scripts/q_container_test.py` builds synthetic .Q files (all chunk types, windows, composite
snapshots, raw mode), runs the real `q_open`/`q_load`/`q_expand` under unicorn with the file I/O stubbed, and compares every frame with a
Python player that follows this document (80 files, 0 mismatches). No real .Q files exist on this machine.

## File and chunks (container, from code)
Header 0x1A bytes: `+0 u16 0x6839`, `+4 u16 blocksWide`, `+6 u16 blocksHigh`, `+8 u8 blockWidth`, `+9 u8 blockHeight`
(both 4 in practice: the decoder hard-codes 4x4 blocks and a 640-byte pitch), `+0Ah u16 frame count`, `+0Ch u32 loop
offset`, `+10h u8 frames/s (0 -> 5)`, `+16h u32 audio sample rate` (audio per frame = rate/fps bytes, 8-bit mono).
Then 6-byte prefixes `{u16 type; u32 size}` followed by `size` bytes; the next prefix is read together with the
payload. Types: 0 audio-only chunk; 1 palette (0x300); 2 full frame; 3 and 4 partial frame with an 8-byte trailer
`{minCol, minRow, maxCol, maxRow}` (u16s) after the payload; 5 pattern table (<= 0x100 bytes -> `qmap`);
6 snapshot current frame as the "composite" background; 7 switch to raw mode; 0xFFFF end (seek to the loop offset).
**The size field of frame chunks (types 2/3/4) does NOT include the audio bytes**: the chunk occupies `size + audioBytes` bytes (`audioBytes` =
rate/fps), audio first, then `size` bytes of block stream, then (types 3/4) the 8-byte window trailer, then the next prefix. Type 0 and the
other types use `size` as is. (Found by the container test: the first draft assumed size covered the audio.) Quirk: `qloop1/qloop2` persist, so a type-2 frame after a windowed frame reuses the last window. Window semantics as implemented:
`maxCol+1` blocks per row starting at `minCol`, `maxRow` rows starting at `minRow` (full frame: maxCol=W-1, maxRow=H).

## Block stream (verified)
Frame = W x H blocks of 4x4 8-bpp pixels, raster order, rows of blocks 0xA00 bytes apart in a 640-pitch buffer.
A 2-colour pattern byte pair gives rows 0,1 (byte A) and rows 2,3 (byte B); in each byte the high nibble is the upper
row, bit 3 = leftmost pixel, bit set = colour 1 (tables `dword_18B6BC`/`18B2BC` expand a byte to two 4-pixel masks).

| First byte | Form | Bytes | State after |
|---|---|---|---|
| c0 < F8, next == c0 | solid block | `c c` | SOLID |
| c0 < F8, c1 != c0, 3rd < 80 | 2-colour, pattern = qmap[i]: rows01 = qmap[2i+1], rows23 = qmap[2i] | `c0 c1 i` | BLK3 |
| c0 < F8, c1 != c0, 3rd >= 80 | 2-colour, explicit: rows23 = b2, rows01 = b3 | `c0 c1 b2 b3` | BLK4 |
| F8 F8 | copy co-located composite block | 2 | COMP_COPY |
| F8 FB | 16 literal bytes, F8 = keep composite pixel | 18 | COMP_OVL |
| F8 c b2 b3 | composite where mask=1, colour c where mask=0 | 4 | COMP_OVL |
| F9 n | repeat previous op n times (state dependent, n >= 2 except SKIP/ABS) | 2 | back to start state |
| FA lo hi | copy block with absolute index | 3 | ABS |
| FB n ... | run of n with inline data (state dependent; n even in the paired states) | 2+ | start state |
| FC b | copy block at pixel offset `motion[b]` from the current position | 2 | MOTION |
| FD b | copy block idx+b+1 | 2 | FWD |
| FE b | copy block idx-b-1 | 2 | BACK |
| FF | skip one block (keep old pixels) | 1 | SKIP |

`motion[b]`: dx = [0,4,..28,-32,-4..-28][b & 15], dy (pixels) = [32,36,8,12,16,20,24,28,-32,-36,-8,-12,-16,-20,-24,-28][b >> 4],
offset = dy*640 + dx. Runs: F9 after BLK3/BLK4/SOLID/start = replicate block idx-1; after SKIP = skip n; after
BACK/FWD/MOTION = same copy continued over n consecutive blocks (source slides with the destination); after ABS =
same source block n times; after COMP_COPY = composite co-located n blocks; after COMP_OVL = quirk, 2 bytes, skips one block.
FB after SOLID = n colour bytes; BACK/FWD/MOTION = n offset bytes; ABS = n u16 indices; BLK4 = n x (b2 b3) with the
previous colours; BLK3 = n patterns, each qmap index (<80) or `b2 b3` (b2 >= 80), previous colours; at start/SKIP/composite
states or in raw mode = n raw 16-byte blocks. Exact code: `q_decode_ref.py`. Row ends reset the state.

## Raw runs contain richer blocks (found on real data)
In a raw run (`FB n` in the raw states, or any `FB` once a type-7 chunk was seen) each of the `n` blocks starts with a byte `b`:
`b < F8` = 16 raw pixel bytes; `F9`/`FA` = 2-bit palettised block `[b][u32 bits][b&7+2 colours]` (pixel i = colour[(bits >> 2i) & 3], row-major, 8 or 9 bytes);
`FB..FE` = 3-bit palettised block `[b][u32 at +1 (pixels 0-7)][u32 at +4 (pixels 8-15)][(b&7)+2 colours]` (pixel i = colour[3 bits of its dword], 7 + b&7 + 2 bytes);
`F8`/`FF` = terminator: 2 bytes, skip one block and leave the run (the engine counts n+1 blocks, so the encoder only uses it consistently).
Both the raw and palettised forms are fuzz-verified against the real decoder.

Open: palette use / audio sync follow `flix_play` (demo) and `video_play?` (retail); real .Q files would validate the container.
