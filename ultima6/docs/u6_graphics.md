# Ultima VI graphics and text data files

Formats of the pictures, tiles, fonts and descriptions, with the C++ readers in `src/files/` (tests in `tests/test_graphics.cpp`).
Every format was validated by decoding every item of the shipped files and, for the pictures, by rendering them and looking at the
result (portraits, the gypsy scene, all 2048 tiles). One caveat: the original draws all of this through the graphics drivers
(`U6MCGA.DRV`, `U6EGA.DRV`, `U6CGA.DRV`, `U6TANDY.DRV`, loaded as raw code blobs and entered through `video_driver_call`), which have not
been disassembled yet, so the tile/shape layouts come from Nuvie's decoders plus that visual and consistency checking rather than from
the original's own drawing code. The two things read off the original are the LZW decoder and the palette unpacking below.

| File | Format | C++ |
|---|---|---|
| `MAPTILES.VGA`, `MASKTYPE.VGA` | one LZW stream each (`lzw.h`) | `TileSet` |
| `OBJTILES.VGA` | raw; follows MAPTILES in the same address space | `TileSet` |
| `TILEINDX.VGA` | u16[2048] positions in 16-byte units | `TileSet` |
| `LOOK.LZD` | LZW; tile descriptions | `LookData` |
| `*.SHP` (title, intro, ending, gypsy, portraits montage, borders), `*.PTR` (cursors) | LZW stream holding a size-headed archive of shapes | `loadShapeFile`, `LibN`, `Shape` |
| `PORTRAIT.A/B/Z`, `CONVERSE.A/B` | offset-table archive (`LibN`, no size header) | `LibN` |
| `U6PAL`, `PALETTES.INT` | VGA palettes, 6-bit | `Palette`, `IntroPalettes` |
| `U6.CH` | 256 8x8 glyphs, one byte per row, MSB left | `Font` |

* **Tiles**: 2048 tiles of 16 x 16 pixels. `MASKTYPE`'s first 2048 bytes give the kind: 0 plain (439 tiles), 5 transparent, colour 255
  see-through (177), 0x0A pixel blocks (1432). A pixel-block tile is a block-count byte followed by runs `u16 offset, u8 length, pixels`
  (length 0 ends). The offsets are in 320-pixel-wide screen terms: advance `offset % 160`, plus 160 once the offset reaches 1760 (the
  second row of a 160-byte double line), from the end of the previous run. 33 of them are completely blank. The terrain tiles are the
  first 512; the rest are objects, creatures and people.
* **Shapes**: see `shape.h` for the run encoding; the hot spot makes the 56 x 64 portraits of `MONTAGE.SHP` (6 female, 6 male heads and
  three more) and the 320 x 200 pictures use the same code. Archives inside `.SHP` start with their own size, then u32 offsets whose
  count is the first offset / 4 - 1.
* **Library archives**: a u32 offset table (24-bit offsets, a flag byte that is always 0 here) up to the first non-zero offset; a zero
  offset marks an absent item. `PORTRAIT.A` has 98 items, `CONVERSE.A` 99 (item 0 absent) and 97 present.
* **Descriptions**: `LOOK.LZD` entries are `u16 tile, NUL-terminated text`; an entry covers the tiles after the previous entry's number up to
  and including its own (tiles 2-4 are all "swamp"). `\plural/singular` markers are left in the text. This is the data `get_object_name`
  (`GAME.EXE` `1D15C`) reads; entry 0 is "Nothing".
* **Intro palettes** (read off `U.EXE` `1BFE2`): `PALETTES.INT` is eight palettes of 576 bytes; every 3 bytes unpack to four 6-bit components
  (`b0 & 3F`, `(b1 << 2 & 3C) + (b0 >> 6)`, `(b2 << 4 & 30) + (b1 >> 4)`, `b2 >> 2`), 192 groups = 256 colours.
