# Martian Dreams and Savage Empire versus Ultima VI

What has been established so far about how the two spin-offs relate to Ultima VI (roadmap step 4). Tests: `tests/test_spinoffs.cpp`.

## Data formats shared with Ultima VI (checked with the Ultima VI C++ readers)

The following files of `MARTIAN\` and `SAVAGE\` load with the Ultima VI code unchanged and satisfy the same invariants:

| File | Result |
|---|---|
| `MAP` (32256), `CHUNKS` (65536) | same layout: 64 superchunks + 5 dungeon levels of packed 12-bit chunk numbers; every chunk number is valid |
| `ANIMDATA` (194), `TILEFLAG` (7168), `BASETILE` (2048) | same sizes and layouts |
| `SCHEDULE` | same 5-byte entries (192 in Martian Dreams, 354 in Savage Empire; 563 in Ultima VI) |
| `SAVEGAME\OBJBLK<col><row>` | same 8-byte records and naming; every map object lies in the superchunk its file name says (7620 objects in Martian Dreams, 4872 in Savage Empire) |

So the world model — map, chunks, object blocks, schedules, tile attributes — can be one implementation for all three games.

## Containers that differ

* The compressed data files (`LZMAP`, `LZOBJBLK`, `LZDNGBLK`, `*.LZC`) are archives with a size header (the same shape as the inside of Ultima VI's `.SHP`
  files, `LibN` with `hasSizeHeader`) whose per-item flag byte says how the item is stored: `0xE0` and `0x20` are used for plain or specially coded items,
  `1` and `2` for LZW-compressed ones (Savage Empire's `TALK.LZC` items are flag 1; Martian Dreams' are `0xE0` and read as plain scripts), `0xFF` marks
  items that share the previous flag. Ultima VI's `LZMAP`/`LZOBJBLK`/`LZDNGBLK` are bare LZW streams instead. The item flags are not decoded by the C++ yet.
* Music/sound: `TM.DRV`, `SUPERTM.DRV`, `STRAX.DRV`, `*.TIM`, `*.LZC` music archives instead of `*.M` files and the `U6*.DRV` family.
* Each game keeps its program in `GAME.EXE` plus a second executable (`MARTIAN.EXE`, `SAVAGE.EXE`) that is the menu/intro/character creation; neither is
  EXEPACKed, and Martian Dreams' `GAME.EXE` has 17 Borland overlays (see `overview.md`).

## At the binary level

Position-independent function signatures (`ida_scripts/export_signatures.py`, `tools/match_functions.py`): Martian Dreams `GAME.EXE` and Savage Empire `GAME.EXE` share 244
unique matches (of 954 and 654 functions), the two menu executables 203 (of 453 and 342). Ultima VI shares only 39 and 44 functions with them: the spin-offs were built
from a reworked engine, Martian Dreams and Savage Empire from the same revision of it.
