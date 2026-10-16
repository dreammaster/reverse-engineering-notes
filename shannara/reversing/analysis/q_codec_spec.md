# Q video codec (demo `q_open`/`q_load`/`q_expand`, block decoder `sub_1652E0`)

Retail tries `NAME.Q` before `NAME.FLC` (see retail_game_layer.md section 13), so this is the main cutscene format.
**The block decoder is verified**: `scripts/q_decode_ref.py` matches the real machine code (run under unicorn by
`scripts/q_emulate.py`) on thousands of random streams (`scripts/q_fuzz.py`: pixels and consumed bytes identical;
frame sizes 3x2 to 160x2, full frames, windows (types 3/4) and raw mode (type 7)). The container layer below is
from reading `q_open`/`q_load` only (not fuzzed; no .Q files on this machine).

## File and chunks (container, from code)
Header 0x1A bytes: `+0 u16 0x6839`, `+4 u16 blocksWide`, `+6 u16 blocksHigh`, `+8 u8 blockWidth`, `+9 u8 blockHeight`
(both 4 in practice: the decoder hard-codes 4x4 blocks and a 640-byte pitch), `+0Ah u16 frame count`, `+0Ch u32 loop
offset`, `+10h u8 frames/s (0 -> 5)`, `+16h u32 audio sample rate` (audio per frame = rate/fps bytes, 8-bit mono).
Then 6-byte prefixes `{u16 type; u32 size}` followed by `size` bytes; the next prefix is read together with the
payload. Types: 0 audio-only chunk; 1 palette (0x300); 2 full frame; 3 and 4 partial frame with an 8-byte trailer
`{minCol, minRow, maxCol, maxRow}` (u16s) after the payload; 5 pattern table (<= 0x100 bytes -> `qmap`);
6 snapshot current frame as the "composite" background; 7 switch to raw mode; 0xFFFF end (seek to the loop offset).
A frame chunk payload = `audioBytes` (rate/fps) then the block stream. Window semantics as implemented:
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

Open: palette use / audio sync follow `flix_play` (demo) and `video_play?` (retail); real .Q files would validate the container.
