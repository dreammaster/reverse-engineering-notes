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

* The "compressed" data files (`LZMAP`, `LZOBJBLK`, `LZDNGBLK`, `*.LZC`) are archives with a size header (the same shape as the inside of Ultima VI's
  `.SHP` files, `LibN` with `hasSizeHeader`) and a flag byte per item. Ultima VI's `LZMAP`/`LZOBJBLK`/`LZDNGBLK` are bare LZW streams instead. The flag
  does not simply mean "compressed": the single item of `LZMAP` is the plain `MAP` (byte for byte) with flag `0xE0` in Martian Dreams and `2` in Savage
  Empire; Martian Dreams' `TALK.LZC` items (flag `0xE0`) read as plain scripts, Savage Empire's (flag 1) are what Nuvie treats as LZW, and `0xFF` marks
  items that share the next item's flag. The flag meanings still have to be worked out from the spin-offs' executables.
* Music/sound: `TM.DRV`, `SUPERTM.DRV`, `STRAX.DRV`, `*.TIM`, `*.LZC` music archives instead of `*.M` files and the `U6*.DRV` family.
* Each game keeps its program in `GAME.EXE` plus a second executable (`MARTIAN.EXE`, `SAVAGE.EXE`) that is the menu/intro/character creation; neither is
  EXEPACKed, and Martian Dreams' `GAME.EXE` has 17 Borland overlays (see `overview.md`).

## At the binary level

Position-independent function signatures (`ida_scripts/export_signatures.py`, `tools/match_functions.py`): Martian Dreams `GAME.EXE` and Savage Empire `GAME.EXE` share 244
unique matches (of 954 and 654 functions), the two menu executables 203 (of 453 and 342). Ultima VI shares only 39 and 44 functions with them: the spin-offs were built
from a reworked engine, Martian Dreams and Savage Empire from the same revision of it.

## LZC containers decoded (`LibN::get`)

Every `*.LZC`, `FONTS`, `SAVAGE.FNT`, `MAPTILES.VGA`, `MASKTYPE.VGA` (but **not** `OBJTILES.VGA` or `TILEINDX.VGA`, which are plain as in Ultima VI) is an offset-table archive with
a 4-byte size header (the file's own size) and per-item flags (as Nuvie's `U6Lib_n`, checked here on every `.LZC` of Martian Dreams): **0x01 and 0x20** the item is an LZW stream
(u32 decompressed size first, the same codes as Ultima VI's `LZ*` files); **0xC1** the item is stored plain behind a u32 that holds the item's own size; **0, 2, 0xE0** plain;
**0xFF** (an absent item sharing the flag of the next present one, as in `TALK.LZC` and `MDFACES.LZC`). `MAPTILES.VGA` is flag 0xE0 (MD) / 2 (SE) and then has exactly the pixel format of Ultima VI's
decompressed `MAPTILES.VGA`: `TileSet::load` accepts the pieces of both games (tests/test_spinoffs.cpp). Fonts (`FONTS.LZC`, `SAVAGE.FNT`) are Worlds-of-Ultima fonts (byte 0 the height,
byte 2 the colour index of a set pixel, widths at +4, low and high offsets of each glyph's pixel bytes at +0x104 / +0x204; see Nuvie's `wou_font.cpp`). Palettes `MDPAL`, `SEPAL` are 1536
bytes: the first 768 are a VGA palette of 6-bit components (the first 16 colours are the EGA ones).

## First stand-alone run of Martian Dreams and Savage Empire

`build/u6.exe md/game/MARTIAN` and `se/game/SAVAGE` (the directories with `MARTIAN.EXE` / `SAVAGE.EXE`) load the shipped saved game through the same `World` / `Game` code as Ultima VI
(`World::game` selects the file names: `MDPAL` / `SEPAL`, one-item archives for `MAPTILES.VGA` and `MASKTYPE.VGA`, `LOOK.LZC`; the font stays blank, the frame is item 0 of `MDSCREEN.LZC` / `SCREEN.LZC` (a 320 x 200 shape behind 4 / 8 bytes of archive header), there are no
portraits or conversations) and draw the map around the party correctly (`--keys "" --shot x.png`). Missing: the frame and panel graphics (`MDSCREEN.LZC`, `SCREEN.LZC`/`BKGRND.LZC`), the
WOU fonts, the per-game tables (movement classes, creatures, usable objects, spells, text), `TALK.LZC` scripts, menus, intros and endings.

Text now draws with the games' own proportional fonts (`Font::loadWou`, `TextWindow::setFont`): the message window wraps by pixel width and keeps as many lines as the 8 x rows pixels hold. Still U6's: the party panel
(positions and colours), the left-hand button strip is only a picture, the colours of the party list, all prompts and rules.

Conversations: `TALK.LZC` items hold the same scripts as Ultima VI's `CONVERSE.*` (`ff`, actor number, name, `f1`, description, code); the item number is *not* the actor number in Savage Empire (and item 30 of Martian Dreams), the script's second byte is, so `World::script` indexes them by that byte. 98 (MD) and 68 (SE) actors have a script; Martian Dreams' first item is a batch file. Talking works (`Talk`/`Conversation` unchanged); portraits are not drawn yet.
