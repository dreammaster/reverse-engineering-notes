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

`OBJLIST` of both spin-offs (7539 bytes, like the shipped Ultima VI save) is Ultima VI's layout with one extra 256-byte array inserted at 0x14F1 (before the movement points): everything from there on is 0x100 further (game time at 0x1CF3, Nuvie's `OBJLIST_OFFSET_WOU_GAMETIME`). `World::load` removes the array (and `save` puts it back); the clock then reads correctly (Martian Dreams starts on day 6 of month 1, year 1895).

## Per-game tables

`tools/extract_creatures.py <exe> <u6|md|se>` reads the creature table of a `GAME.EXE`: the object types that have a movement class of their own (a zero-terminated word array), seven byte columns (strength, dexterity, intelligence, experience, damage, hit points, alignment in bits 5-6), then a word of class flags per class (the creature flags of `CreatureStats` are those words with their 15 bits reversed). Ultima VI's output equals the hand-made `combat_tables.inc` (checked by script); the spin-offs differ in the capacity of the columns (Martian Dreams 50 bytes apart with 49 types, Savage Empire 48). `src/martian_dreams/tables.inc` and `src/savage_empire/tables.inc` are the results, `GameTables` (`game_tables.h`) hands the right ones to `Combat::stats` and `MovementClasses`, `World::load` chooses them by `World::game`. Not yet read: the weapon damage, weapon range and armour tables (found as word lists in Martian Dreams' data, not yet tied to code), the usable-object list, spells and texts.

## Martian Dreams: the Use command (findings, not ported)

MD's command loop is `sub_23C1A` (jump table at 243B2: events 130 Talk, 131 Look `sub_14ED4`, 132 Get, 133 Drop, 134 Move, 135 **Use `sub_361AC`**). Use is not Ultima VI's table of 85 usable types but a
recipe system (`sub_35F0D`):

1. `sub_35C31(type, buf)`: table 1 in `seg059` (file 0x31480, 125 records of 12 bytes: a word object type and up to ten recipe numbers ended by 0xFF) lists the recipes an object takes part in
   (duplicates removed). None: "%s can't be used." (ds-relative seg057 string at file 0x2EF40 + 0x1C19).
2. A recipe (`seg060`, file 0x31A70, 75 records of 36 bytes) has a list of first-partner object types at +0, a list of second-partner types at +0x10 (word lists ended by 0xFFFF; 0xFFFE
   means anything, a list that is just 0xFFFF means no partner) and a far pointer to its action at +0x20.
3. If some recipe wants a first partner the game asks "On - " and the player picks an object (`sub_35B9F`/`sub_35972`); the recipes whose list does not take its type are dropped
   (`sub_35DCD`); none left: "%s on %s has no effect.\n" style messages. A second partner is asked for with "To -\n" likewise.
4. `sub_35E6D` takes the recipe with the most specific partner lists (0xFFFE scores 0, no partner 1, a type 2) and calls its action with (item, first, second).

The 75 actions are in the three overlay stub segments 0x2F8D, 0x2F98 and 0x2FA1 (Borland overlay thunks `CD 3F <offset> 00` after a 0x20-byte header); I have not yet resolved the thunks to the
IDA segments (the overlay headers give file offset, code size and entry count: 0x2F8D 0x1A87 bytes, 0x2FA1 0x0B3D bytes) so none of the actions is read. The engine keeps using Ultima VI's use table
for Martian Dreams and Savage Empire.

Resolved: the thunks. The stub segments 0x2F8D, 0x2F98 and 0x2FA1 (file offset = 0x3400 + segment * 16) are IDA's `jmp sub_5xxxx` stubs at linear address segment * 16 + offset + 0x10000, so the 75 actions are the
functions `sub_598F6`, `sub_5F166` ... `sub_5F25E` in `md/md_game.asm` (the list is in the git history of this file's author notes; recipe n -> `sub_` is read from the stub at seg060 + 36 * n + 0x20).
Recipe meaning by the objects that use it: 0 tent, 1 book/note/scroll, 2 pool cue/gloves/tongs, 3 gong hammer on the gong, 4-6 wrench on switch bar/drill/panel, 7 sledge hammer on track, 8 pliers on cable,
9-16 hoe/shovel/pick uses (dig, plant, fill barrow, furnace), 17-18 chisel/hammer/pry bar on crate/hatch, 19/28/29/63 fire and lamps, 20 knife on seed, 21-22 hammer on geode/chests, 23 barge controller,
24 lever/switch, 25 cannon, 26 berry, 30 lamp oil, 31 switch bar, 32 spyglass, 33 sextant, 34 containers, 35 pool table, 36 pipe organ, 37 potash, 38 lens, 40 ray guns/combine, 41 cannonballs, 42 bucket,
43 map, 44/56 doors, 45 panel, 46 belt, 47 radium chip, 48-49 sprayers, 50 oxium bin, 51 pigment, 52-54 dream machine and dreamstuff, 55 elixir, 57 instruments, 58-59 wormsbane, 60 potash/oil,
61 radium, 62 shovel on dirt and hole, 64 many, 65 heart stone, 66 spittoon, 67 ruby slippers, 68 manuscript, 69 mirror, 70 gate, 71 buckets, 72 valve, 73 chip on man, 74 air bottle.
