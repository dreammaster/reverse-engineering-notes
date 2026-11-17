# Ultima V picture files: `*.16` (EGA) and `*.4` (CGA)

Decoder and viewer: `tools/u5lzw.py` (decompression), `tools/u5img.py` (tiles and pictures to PNG). All 52 picture files of the
game (26 `.4` and 26 `.16`) decompress and parse with the rules below, with no bytes left over.

## 1. Container

Every `*.4` / `*.16` file is LZW-compressed (same code as `src/common/lzw.c`): a `u32` little-endian uncompressed length, then an LSB-first
bit stream of codes that start at 9 bits and grow to 12; code 256 = clear (reset to 9 bits), 257 = end. The next free code starts at 258;
the code size grows when the next free code reaches `1 << size`.

Two kinds of file result:

| file | uncompressed | content |
|---|---|---|
| `TILES.16` | 65,536 bytes | 512 tiles x 128 bytes |
| `TILES.4`  | 32,768 bytes | 512 tiles x 64 bytes |
| every other `*.4` / `*.16` | varies | a picture file (section 3) |

## 2. Tiles (16 x 16 pixels, tile number = index in the file)

* **CGA (`TILES.4`)**: 64 bytes per tile, 16 rows of 4 bytes, 2 bits per pixel, 4 pixels per byte, **leftmost pixel in the top two bits**
  (byte `b`: pixels `b>>6, b>>4, b>>2, b` each `& 3`). No mask, no header. This is exactly the BIOS mode 4 pixel format.
* **EGA (`TILES.16`)**: 128 bytes per tile, 16 rows of 8 bytes, 4 bits per pixel, high nibble = left pixel (this is what `src/graphics/grap_buf.c` draws).

CGA.DRV's put-tile routine (driver entry `51`, code at `0d99`) is a plain copy: 16 rows x two `movsw` to the screen, 4 pixels per byte, x position
= pixel x / 4, tile 16 x 16 only. The tile number is multiplied by 64 (`shl si,6`).

**Verified against the real game:** in the DOSBox CGA capture `tests/reference/abbey_hall_cga.png`, 109 of the 121 16x16 blocks of the map view are
bit-identical to a tile of `TILES.4` decoded this way (the remaining blocks are party/NPC animation frames and the border), with palette below.

## 3. Picture files

```
u16 n                      number of pictures
u16 table[2*n]             for picture i: table[2i] = offset of its image, table[2i+1] = offset of its mask (0 = no mask)
...                        the pictures, back to back, in table order, ending exactly at the end of the file
```
First picture starts at `2 + 4n`. Offsets are from the start of the uncompressed data.

**Image block** (`table[2i]`): `u16 width`, `u16 height`, then `height` rows with no padding between rows:

| | row size | pixel layout |
|---|---|---|
| `.4` CGA | `(width + 3) / 4` bytes | 2 bits per pixel, leftmost pixel in the top bits; a partial last byte holds the leftmost pixels |
| `.16` EGA | `((width + 7) / 8) * 4` bytes | four bit planes per row, plane 0 first; each plane `(width+7)/8` bytes, leftmost pixel = most significant bit; pixel = plane0 | plane1<<1 | plane2<<2 | plane3<<3 |

**Mask block** (`table[2i+1]`, identical in `.4` and `.16`): `u16 width`, `u16 height`, then `height` rows of `(width + 7) / 8` bytes, 1 bit per
pixel, leftmost = most significant bit. A **1 bit keeps the background** (transparent); a 0 bit draws the image pixel. CGA.DRV's masked
blit (entry `4b`, code `0a26`) expands every 4 mask bits into one byte (table `00 03 0c 0f 30 33 3c 3f c0 c3 cc cf f0 f3 fc ff` at `0252`) and does
`screen = (screen & expanded) | (image & ~expanded)`. Files with masks: `ITEMS`, `MON0`-`MON7` (every picture has one); all others have none.

Pictures can be wider than the table suggests only by the rules above; there is no padding and no compression inside a picture. The picture
files are: `CREATE DNG1-3 END1 END2 ENDSC ITEMS MON0-7 STARTSC STORY1-6 TEXT ULTIMA` (each as `.4` and `.16`; same picture count and sizes, only the
pixel data differ: the CGA pictures are separate artwork, not conversions).

`*.BIT` files (`BRITISH`, `TITLE`, `WD`) are a different, simpler resource: 1 bit per pixel pictures without masks (`IMAGE_GetBitImageView`).

## 4. CGA screen and palette

* Mode: BIOS video mode 4 (320 x 200, 4 colours), CGA.DRV sets palette 1 with a black background: colour 0 black, **1 cyan, 2 magenta, 3 white**
  (DOSBox renders them (0,0,0), (0,170,170), (170,0,170), (170,170,170)).
* Memory layout is the standard interleaved one: row `y` at byte `(y >> 1) * 80 + (y & 1) * 0x2000`; CGA.DRV builds this row table at start-up
  (code at `05b6`). Page 1 is an off-screen buffer of the same layout (16,384 bytes), page 0 is video memory `b800:0000`.
* Pen/fill colour (driver entry `2d`): colour `& 3`, expanded to a byte pattern `00 55 aa ff` (the game itself also masks colours with 3 on CGA).
* Text: the `IBM.CH` / `RUNES.CH` 8x8 1-bit character sets are expanded to 2 bits with the same nibble table; Hercules uses `IBM.HCS` / `RUNES.HCS`.

## 5. Not established

* **Which file set a video mode loads.** CGA is certain (the capture matches `TILES.4`). The sources only carry the names `TILES.16` etc. in `DATA.OVL`
  (no `.4` string anywhere in the EXE, overlays or drivers), and I could not find the code that selects the other extension: the file loader
  (`ULTIMA.EXE` +125d, +1588) uses the name unchanged. Hercules (`HER.DRV`, 4-colour dither of the same data) most likely also uses `.4`, and Tandy
  (16 colours) `.16`, but that is an inference from the file sizes and driver code, not confirmed. A DOSBox run with file-access logging would settle it.
* The tile animation routines (water, flags, moongates, clock, reveal) exist for CGA in CGA.DRV (entries `60`, `66`, `69`, `6c`, `6f` at `144c`, `1929`,
  `1b4a`, `1d71`, `128c`) but only the EGA versions have been ported to `src/graphics/animate.c`; the CGA ones work on the 2-bit tiles above and
  have not been analysed.
