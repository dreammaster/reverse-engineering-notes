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

## Savage Empire: the Use command (`se_use.cpp`)

SE's command loop (jump table at 0x15564 in `se/se_game.asm`: events 129 `sub_302B8`, 130 `sub_2FBBA`, 131 `sub_2F530`, 132 **Use `sub_35E46`**, 133 a flag, 134 `sub_2EE8D`) is built like Ultima VI's, not like Martian Dreams'.
The pre-checks of `sub_35E46`: the target (an object chosen with the cursor, or a square), `sub_2E85C` (the 84 types at ds:11AA that may be tried at all: else "Not possible."), "Out of range!" beyond one square
unless the thing is carried, then a Borland switch: 81 words at cs:8967 (80 types and one stray) with the handler offsets 0xA2 bytes after them; **the offsets are relative to cs:0 = IDA linear 0x2DBD0**
(a handler's block starts at 0x2DBD0 + offset; the first dump of this table was two bytes off). Every handler ends in `loc_36508`: five moves (`sub_24EBC`), redraw.
`ida_scripts/dump_se_use.py` writes the table and the function (`se/se_use.txt`), `dump_se_use_subs.py` the routines the handlers call with their strings (`se/se_use_subs.txt`).

Data segment: ds = IDA linear 0x3FA60 (string offsets `push ds; mov ax, N` are ds:N; those after `mov ax, seg seg050` are in segment 0x3F650). The saved `OBJLIST` is read by `sub_1617A`/`sub_165B7`
(`ida_scripts/find_objlist_loader.py`): status 329C, positions 78E4 (3 bytes each), type+frame words 9CFA (so `[bx-6306h]` = the word of object bx), quantity/quality 54F0 (a word; `54F1` is the quality), actor status 6CF0
(`byte_4675B` = actor 0xB's status, bit 0x80 = in the party), strength 53E8, dexterity 4606, intelligence 7644, experience 51E4, hit points 6DF4, party names 5104, roster 4EDA, party size 9CF6, moves 44FA,
talk flags 6EFC (`byte_4695D` = actor 1's), and the **globals at ds:27E8, 0x82 bytes** = `ObjList::globals` (`byte_42252` = globals[0xA], the acting member; word_42254/56/58 the party's x, y, z from 0xC;
`byte_4226E` = [0x26], the liquor counter; `byte_422C0` = [0x70]; `byte_422C1` = [0x71], 0xFF in party mode, else the solo member).
Helpers: `sub_1BAD6` delete an object, `sub_1AD09`/`sub_1AD62` first/next object in a container, `sub_1ACDD` the actor that holds an object, `sub_2DBD3` that actor or the acting member, `sub_1C1C3` whether an
actor holds a thing, `sub_1C70E` the byte-quantity table of ds:806 (48 types), `sub_1B5FA(actor, quantity, flags, type)` add to a pack (weight limit; flags 0x10 carried, 0x18 readied), `sub_1B486(level, y, x, 0, 0, type)`
create on the map, `sub_1ABE3` whether an actor is within 5 x 4 squares of the leader, `sub_2E4FC` the acting actor (the leader when aboard), `sub_14746` = `advance_time`, `sub_17FC8` the cursor prompt,
`sub_1E046` the name of an object, `sub_18AB9` print.

Handlers (type -> routine -> what it does, by its strings); **ported** are marked:

| types | routine | what |
|---|---|---|
| 5F bottle of liquor | 31C2D | "You drink it." (**ported**) |
| 5D, 66-6F, 71, 72, 78, C5, D5, D6 food | 31DF5 | "You eat the food." (**ported**) |
| A1 door | 353D0 | opens when Yunapotli (actor 11) is in the party and in view: "... and they slide open!", else "You can't budge it!" (**ported**) |
| D0 torch | 30ECC | lights one in a free hand: lit torch D1 with 100 minutes; "Your hands are full." (**ported**) |
| BC crate | 361BA + 2EC8A | frame 1 opens it, else "Not possible."; then it is searched (**ported**) |
| 4F-52 bones, 3E bean stalk, 3D corn stalk | 33F4B, 33E52, 33EA5 | rib bones D3, beans 6D, corn 6C: "You are carrying too much already." (**ported**) |
| 0A magnesium ribbon | 34F6B | one used up, `sub_34DD4` at the member's square |
| 14 rope, 16 hole in the ceiling | 362F7, 36312 -> 19505 | not in solo mode; the descent into the underground city ("The mechanism supporting the trap door fails ...") |
| 19, 2C, 34 knives, scissors | 32488 | "On -": cut cloth, meat, skin a big cat, remove feathers (516 lines) |
| 1D, 77 hammers | 31436 | "On -": on a boulder (AC) a strength roll (`sub_30E72`: 1-30 below (45 - strength) / 2 fails: "Glanced off!") else "A piece of rock flakes off." (3A); on cold burnt logs (CF frame 0) 1 to strength charcoal (81); else "No effect." (**ported**) |
| 2A vine | 34FDE | "On -": "You can't use a rope here!" |
| 2B bell | 31293 | "On -": "You put the bell on the dinosaur's neck." (target type 118, frames 0D / 11; frame 11 names the head by its quality; the actor needs status bit 4 and no talk flags; actor B5 also sets bit 0x80 of actor 20's talk flags) (**ported**) |
| 2F turtle bait | 34783 | "You must be near water." / "Nothing takes the bait." |
| 31 notebook | 34D88 | Jimmys shorthand (**ported**) |
| 32 flintlock | 31E67 | loading with gunpowder |
| 33 camera | 34EDF | the film is ported, the flash (`sub_34DD4`: the picture, the ghost whispering "Closer...", actor 0x19) is not |
| 3B, 3C mortar, grinding stone | 310AD | grind corn / mix gunpowder |
| 3F jug | 354BC | "On -": pour (target type 1A) |
| 56-58 totems | 3646E | placing (`sub_1C1C3`, `sub_22EF7`, "`%s can't use that!") |
| 5C folded skin | 35C2E | unfolding it |
| 5E fire extinguisher | 343E9 | "On -": puts a fire out, hardens lava |
| 83 digging stick | 31F60 | "You get some clay." / "There is nothing to dig!" (**ported**) |
| 86 cloth strip | 33C06 | "On -": an actor gets 1 to 5 hit points back up to `sub_2872A` (2 x strength + 4 x level, 1 to 255); a body is "beyond help" (**ported**) |
| 88 metal bucket | 335A9 | frame 0 fills with tar or water, frame 2 pours water (`sub_33364`) (**ported**); frame 1 pours tar (`sub_32D11`: tars cloth strips, makes pteranodon nests and webs) is not |
| 89, F0 grenade, device | 31671 | "Device activated." |
| 8F machine | 35B44 | a gem in the machine |
| 94 jungle drum | 3542B | |
| A5 loom | 33F9E | weaving flax |
| AD, F2, FE, 101 oven, fires | 3215F | fire the pot, make charcoal, incinerate |
| B1 wire screen | 33B10 | sift sulphur (**ported**) |
| BA brazier, CF burnt logs | 36206, 3623D | frames, then 3215F |
| BE metal head | 34147 | replacing the statue's head |
| BF tarred cloth | 33777 | torch, grenade |
| C0 clay | 3204A | shaping a pot (**ported**) |
| CC gunpowder | 34C0A | loading the flintlock |
| D1 lit torch | 35686 | "On -": burn things (dinosaur egg ...) |
| D4 fishing pole | 31B48 | deep water (terrain 0x0C) on the diagonal, one catch in five (**ported**) |
| D8 crystal garden | 35D54 | potassium nitrate (Johann) |
| E5 tar pit | 33211 | cloth strips |
| E9, ED rafts | 31C98 | boarding |
| EA door | 35150 | "The door is blocked." |
| F7 headless statue | 3423A | |
| FC crystal | 35972 | "On -": the same with full hit points and the poison bit (status 0x08) cleared: "%s is healed." (**ported**) |
| 113 | 346A8 | frame changes of a rope-bridge-like object |
| 12E turtle, 133 | 34B00, 349A9 | riding the turtle, "Only next to land!" |

The Use on terrain (a tree, lava, a yucca plant, a turtle: `sub_33DFF`, `sub_3215F`, `sub_33EF8`, `sub_35E46` 35F72-35FE8) is not used by the engine (APPROX 79).
