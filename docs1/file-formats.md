# Chapter 1 data file formats

Living reference. Each section says how it was established. Record sizes come
from the file descriptors `SW.EXE` builds (`ida_scripts/dump_file_descriptors.py`,
descriptor field `+6`); graphics formats were checked by decoding and viewing
the images.

All integers are little-endian. Every file is opened and read by the shared
"file record API" (see [`program-graph.md`](program-graph.md)): a file is an
array of fixed-size records, addressed by record number.

## Tile graphics — `FILE00.16` … `FILE06.16`

**Verified** (decoded and rendered with the matching palette).

- Record size **512 bytes** = one **32x32 pixel, 16-colour tile**.
- Layout of a tile: **4 planes, plane-major**, each plane 32 rows x 4 bytes
  (128 bytes). Plane order in the file is **3, 2, 1, 0** (the intensity plane
  first, the same order the video driver's image routine consumes). Within a
  byte, bit 7 is the leftmost pixel. Pixel value = `p3<<3 | p2<<2 | p1<<1 | p0`.
  This is exactly the format `vidPutImage` draws (rows 32, width 4 bytes, mask
  `FFh`; see [`video-api.md`](video-api.md)), so the game blits tiles from the
  file with no conversion.
- A tile is identified in the game by (file number 0–6, tile index). The shared
  tile cache loads tiles on demand into a 200-slot cache.
- Contents (tile counts from file size):

| File | Size | Tiles | Contents |
|---|---:|---:|---|
| `FILE00.16` | 778,240 | 1,520 | intro/title graphics: the sword, the studio logo, large lettering ("PRESENTS"), sparkle/flare animation frames |
| `FILE01.16` | 737,280 | 1,440 | male character portraits (each a multi-tile image) and the matching walking/fighting sprites in several poses, tiles in groups of 4x4–8x? sheets |
| `FILE02.16` | 532,480 | 1,040 | overland terrain: grass, forest, mountains, water, sand, roads |
| `FILE03.16` | 491,520 | 960 | towns: brick and stone walls, roofs, floors, doors, fences, signs |
| `FILE04.16` | 614,400 | 1,200 | monsters and animals (horses, spiders, bats, ants, crocodile…), people, combat sprites |
| `FILE05.16` | 368,640 | 720 | dungeon/mine and water/cave tiles, furniture, shop interiors |
| `FILE06.16` | 737,280 | 1,440 | female character portraits and sprites (same structure as FILE01) |

## Palettes — `FILE00.PAL` … `FILE06.PAL`

**Verified shape; selection rule not yet traced.**

- Records of **17 bytes**: 16 EGA palette-register values (6-bit `rgbRGB`) and one
  spare byte (0). The valid record counts are **19, 18, 13, 12, 15, 9, 18** for
  files 0–6. The file size equals `count*17` for every file except `FILE03.PAL`,
  which is 425 bytes (25 records' worth) but holds only 12 real records: the rest
  is stale directory data from when the file was created (it contains file names
  such as an old `AUTOEXEC`/`BAT` listing). The loader takes the leading run of
  17-byte records whose 16 values are `<= 3Fh` and whose spare byte is 0.
- EGA colour from a register value `v`: red = `2*bit2 + bit5`, green =
  `2*bit1 + bit4`, blue = `2*bit0 + bit3`, each scaled by 85.
- The registers are written with `INT 10h` AX=1002h by the palette helper
  (`VIDP16.DRV`). Which record applies to which scene (time of day / area) is
  still to be traced in `PROG4`.
- `FILEV0.PAL` (768 bytes = 256 x RGB) is the VGA/SVGA palette; the SVGA drivers
  and that file are not part of this install.

## Records

| File | Record size | Records | Notes |
|---|---:|---:|---|
| `CURRGAME\PARTY.DAT` | 100 | 10 | party/character records; the file begins with a header (`0Ah` then the title text "Yendorian Tales - Book I") *(layout: Phase 5, `PROG5`)* |
| `CURRGAME\NPC.DAT` | 32 | 446 | NPC/monster placement records; first record looks like a header (`0152h`, `019Ah`, …), then records with `FFFFh`-filled unused fields *(layout: Phase 5)* |
| `CURRGAME\AREA.DAT` | 128 | 56 | per-area records (lists of `0001h`, `0002h`… IDs with `FFFFh` padding) *(layout: Phase 5)* |
| `CURRGAME\BATPIC.DAT` | 2 | – | combat picture selection; 3 bytes in the shipped copy |
| `SAVEGAME.<x>\{PARTY,NPC,AREA}.DAT` | same | same | a saved game is a copy of the three `CURRGAME` files; the shipped `SAVEGAME.A` is byte-identical to `CURRGAME` (a new game) |
| `WORLD.DAT`, `TOWN.DAT`, `MINE.DAT` | 1,024 | 256 each | the three maps (overland, towns, mines); exactly 262,144 bytes each *(cell layout: Phase 5, `PROG4`/`PROG10`)* |

A saved party is therefore three files: `PARTY.DAT`, `NPC.DAT`, `AREA.DAT`;
`PROG0` copies them between a save directory and `CURRGAME\`.
