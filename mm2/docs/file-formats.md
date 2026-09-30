# On-disk formats

Everything here was confirmed against the code (function/address noted) **and** by decoding the
GOG copy with `tools/mm2_data.py` / `tools/mm2_lzw.py`.  "Not traced" marks what is still open.

## Common: how files are loaded

The resident library (`load_file_alloc` `11E64`, `load_lzw_file` `121B6`, `load_plain_file` `124B0`)
decides per file *name*, using two zero-terminated pointer lists in DGROUP:

| List | Meaning | Contents in v1.01 |
|---|---|---|
| `DGROUP:0516` | files that are XOR-encrypted (key string at `DGROUP:0514`, `xor_decrypt` `12616`) | empty (feature disabled) |
| `DGROUP:0518` | files that are LZW compressed | `map.dat`, `attrib.dat`, `monsters.dat`, `xxx` |

Note that `str.dat` and the two `events*.dat` are **not** in that list: they are handled by their
own loaders, but use the same LZW chunk format described below.

### LZW chunk (`lzw_decompress`, `12242`)
`u32 decompressed_size`, then the code stream: variable-width codes 9..12 bits, **LSB first**,
`100h` = clear table (back to 9 bits, next code `102h`), `101h` = end.  Code width grows when the next
free code reaches `1 << width` (up to 12 bits).  Python: `tools/mm2_lzw.py`.

## Files

| File | Bytes | Format |
|---|---|---|
| `MM2.EXE`, `*.OVL` | | see [exe-layout.md](exe-layout.md) |
| `MM2.CH` | 1024 | 128 characters × 8 bytes: the 8×8 text font (loaded by `load_resource_cached`) |
| `ITEMS.DAT` | 5120 | not compressed; 256 items x 20 bytes at `DGROUP:6960` (`g_items`); see *Items* below |
| `SPELLS.DAT` | 192 | not compressed; loaded to `DGROUP:7D60` (`g_spells`); layout not traced |
| `ATTRIB.DAT` | 3840 | LZW; **60 x 64-byte map attribute blocks** (`map_attr_load` `16171` copies block *map id* to `DGROUP:5986`); see *Map attributes* below |
| `MONSTERS.DAT` | 6656 | LZW; 416 records × 16 bytes, see below |
| `STR.DAT` | 7707 | LZW; then every byte `+ 1Ch` (mod 256) gives ASCII; `1Dh` = newline. Tavern jokes/rumours |
| `MAP.DAT` | 18748 | 60 × `u16` file offsets (index 0 = 120), each a LZW chunk that decompresses to **512 bytes** = one 16×16 map, two layers (256 B walls + 256 B flags, see below) |
| `EVENTSI.DAT`, `EVENTSO.DAT` | 49609 / 25797 | 71 × `u32` chunk offsets (0 = none). `I` holds maps 0-4 and 17-… ("indoor"), `O` maps 5-16 (`g_outdoors` picks the file, `load_map_events` `16008`). Each chunk is a LZW chunk; see [events.md](events.md). Chunks 60-70 have another structure (not traced) |
| `ROSTER.DAT` | 8292 | first `1860h` bytes = 48 character records (24 characters + 24 hirelings, `82h` bytes each) loaded to `g_characters` (`load_roster` `1276C`); remaining 2052 bytes not traced (presumably game state) |
| `DEFAULT.DAT` | 780 | new-game default roster template (loaded by 1MENU2 `19660`); not traced |
| `*.DRV` | 3-5 KB | video driver code modules (`TCGA/EGA/TGA/HGA/MCGA.DRV`) and `TIMER.DRV`; loaded whole into memory and called through a jump table at offset `fn*3` (`driver_call` `11CDA`) |
| `*.16`, `*.4` | | 16-colour (EGA/VGA) and 4-colour (CGA) graphics; **image banks, see below** (`MONSTERS.16/.4` differ) |

### Map styles / graphics sets

`map_style_for_id` (2PLAY `1B410`) maps map numbers to a style:

| Map ids | style |
|---|---|
| 0-4 | 0 (towns: `town*.16`) |
| 5-16 | 3 (outdoor) |
| 17-32 | 1 |
| 33-40 | 6 |
| 41-44 | 4 |
| 45-54 | 5 |
| 55-59 | 2 |

(the graphics file name lists are at `DGROUP:04CE…`: `sky`, `nwcp`, `master`, `town{f,t,b,}`,
`cave{…}`, `castle{…}`, `outdoor1-3`, `outb`, `outf`, `desert`, `ocean`, `tundra`, `swamp`, `endgame`).

### Map cell layers (in memory: `DGROUP:59D6` walls, `DGROUP:5AD6` flags; index `y*16+x`)

* Walls byte: 2 bits per side, N/E/S/W (`view_prepare_visible_cells` `1B4E0`).
* Flags byte: bit 7 = an event trigger is armed for this cell (`evt_op20_clear_trigger` clears it).
* Outdoor maps wrap into neighbouring maps: the four neighbour ids are in `byte_231DB..231DE`
  (N, S, E, W order not yet verified) and their cells are staged at `5BD6/5CD6/5DD8/5ED8`.

### `MONSTERS.DAT` record (26 bytes x 256) — `monster_decode_stats` (`13B80`), loader `sub_167E8`

`record = base + id*26` (`monster_get_record` `167E8` copies 13 words).  Bytes:

| Off | Meaning (decoded into DGROUP globals `7650..7695`) |
|---|---|
| 0-13 | name, each byte `& 7Fh` (high bit set in the file), space padded |
| 0E | bits 0-5: dice count - 1; bits 6-7: die index into `DGROUP:4DB8` = {1, 10, 100, 1000}; hit points = count x die |
| 0F | bits 0-4: multiplier - 1; bits 5-6: index into `4DB8`; bit 7: x1000; experience reward = multiplier x that |
| 10 | flag bits: 7, 6, 5, 2 -> flags (`27670`, `2766F`, `2766E`, `27672`); bits 0-1 -> `27673`; bits 3-4 -> `27671` |
| 11 | bits 0-4: monster spell / breath id -> `27674` (see table below); bits 5-7: index into percent table `4DC0` = {0,10,20,35,50,75,90,100} -> `27675` (cast chance) |
| 12 | bits 7,6,5: flags `27683/27682/27678`; bits 0-4: touch effect id -> `27677` (table `106C`) |
| 13 | bit 7 flag `2768F`; bits 0-3 + 1 (x10 if bit 4): `27679` (attack damage dice count); bits 5-6: `2767A` (damage kind/attack verb) |
| 14 | low nibble + 1 -> `2767E` (number of attacks); high nibble + 1 -> `27676` |
| 15 | bits 0-6 -> `2767B` (speed?); bit 7 unused |
| 16 | bits 7,6 flags `27684/27685`; bits 0-4 + 1 (x10 if bit 5) -> `2767C` (damage dice size / resistance?) |
| 17 | bits 7,6 flags `27687/27686`; bits 0-4 + 1 (x10 if bit 5, capped 250) -> `2767D` |
| 18 | bits 7,6 flags `27689/27688`; bits 0-4 + 1 (x10 if bit 5, capped 250) -> `2767F` |
| 19 | bits 0,1,2 -> flags `2768B/2768A/2768C`; bits 3-5 -> `27680`; bits 5-7 -> percent table `4DC0` -> `27681` |

Monster spell / breath names (`DGROUP:10AA`, id 0-31): sprays poison, sprays acid, casts a curse,
breathes fire/lightning/cold/energy/gas/acid, explodes, gazes, drains magic, drains spell level,
vaporizes valuables, juggles party, energy blast, sleep, lightning bolts, fireballs, fingers of
death, disintegrate, super shock, dancing sword, incinerate, and invokes power, implosion, inferno,
pain, silence, frenzies, paralyze, swarms.  Touch effects (`DGROUP:106C`, id 0-31): adds friends,
lost gold/gems, poisoned, diseased, asleep, cursed, silenced, paralyzed, collapses, dies, turns to
stone, eradicated, lost item/backpack/food/all food/all gold/all gems/valuables, aged (x2), lost
statistics (x3), lost level (x2), lost experience, items scrambled, lost spell points, assassinated,
sprays poison.  (Names of individual record bits will be refined while reading combat.)

## Character record (`82h` = 130 bytes, `g_characters + roster_id*82h`)

From the display code (`show_character_sheet` `12A6A`, `show_party_roster_screen` `144AE`,
`char_reset_current_stats` `13572`, `age_party_one_day` `15092`, …).  Offsets are hex.

| Off | Size | Field |
|---|---|---|
| 00 | ≤11 | name (NUL terminated) |
| 0B | 1 | location: town number + 1 where the character is stored (inn) |
| 0C | 1 | sex (0 = male) |
| 0D | 1 | original alignment (restored by the temple; current alignment is `+6A`) |
| 0E | 1 | race (index into `DGROUP:0456` names: Human, Elf, Dwarf, Gnome, H-Orc) |
| 0F | 1 | class (index into `DGROUP:0446`: Knight, Paladin, Archer, Cleric, Sorcerer, Robber, Ninja, Barbarian) |
| 10-15 | 6 | base Might, Intellect, Personality, Speed, Accuracy, Luck |
| 16-1D | 8 | resistances (copied from per-race tables `DGROUP:06AE + 6*race`... at character creation) |
| 1E | 1 | thievery (from per-class table `DGROUP:06DE`) |
| 20 | 1 | base level |
| 21 | 1 | age (years; capped at 200) |
| 22 | 1 | day counter (rolls at 181 → age+1) |
| 23 | 1 | base spell level |
| 24 | 1 | armour class |
| 25 | 1 | food |
| 26 | 1 | condition bits (`print_condition`: good, cursed, silenced, diseased, poisoned, asleep, paralyzed, unconscious, dead, stone, eradicated); ≥ 80h = out of action, `E0h` mask = unable to act |
| 27 | 1 | base endurance |
| 28-2D | 6 | **equipped** item ids (`print_equipped`; `char_equip_item` `1CA0B` fills these) |
| 34-39 | 6 | equipped items: flag byte (low 6 bits = charges/bonus, top 2 bits = alignment restriction of the item instance, FFh = cursed) |
| 3A-3F | 6 | **backpack** item ids (`print_backpack`) |
| 46-4B | 6 | backpack flag bytes (same layout as 34-39) |
| 2E-33, 40-45 | 6+6 | extra byte per equipped / backpack item (moved together with the id; meaning not traced) |
| 50 | 1 | two 4-bit skills (low/high nibble → titles `DGROUP:046A`: Arms Master, Athlete, Cartographer, …) |
| 51-56 | 6 | 48-bit known-spell bitmap (`expand_spell_bitmap` `14D5C`) |
| 58 / 5A | 2 / 2 | spell points current / max |
| 5C | 2 | gems |
| 5E | 2 | hit points current |
| 62 | 4 | experience |
| 66 | 4 | gold |
| 6A | 1 | alignment (Good / Neutral / Evil) |
| 6B-70 | 6 | *current* Might, Intellect, Personality, Speed, Accuracy, Luck |
| 71 | 1 | current level |
| 72 | 1 | current spell level |
| 73 | 1 | current endurance |
| 74 | 2 | hit point maximum |
| 79 | 1 | ≥ 80h marks a special character (a `+` is shown after the name) |

Party: `g_party_ids` = 8 words (roster ids, FFFFh = empty); ids ≥ 18h are hirelings.

## Image banks (`*.16` = 4 bpp, `*.4` = 2 bpp)

Decoded from the EGA driver's draw routine (`EGA.DRV` fn 13h at `0BCA`, called from the resident
`gfx_draw_op13` `114FE`) and verified by rendering every `.16` file (`tools/mm2_gfx.py`):

```
u32   decompressed size
LZW   (see above)  ->  bank:
  u16 count
  count x { u16 image_offset, u16 mask_offset }        ; mask_offset 0 = no mask
image (at image_offset):  u16 width, u16 height, then height rows of
      ceil(width/2) bytes (16 colours; ceil(width/4) for 2 bpp), each row padded to a multiple of 4 bytes,
      left pixel in the high bits; palette = default IBM EGA colours
mask  (at mask_offset):   no header, 1 bit per pixel, ceil(width/8) bytes per row
```

Contents seen: `town/cave/castle.16` = 32 wall/door pieces of the first-person maze view (full
walls, side walls at 3 depths, doors), `*b/*t/*f.16` = 36/36/1 images (ceiling/floor/background/tops),
`outdoor1-3.16` = 8 terrain pieces, `desert/ocean/swamp/tundra.16` = 20, `sky.16` = 208x60 sky bands,
`master.16` = title screen (`Might and Magic Book Two`, 320x200) and UI pieces, `globe/endgame/
book/throw/xfer/disk/nwcp.16` = special screens.  Some `.4` banks do not decode with the 2 bpp
rule above yet (`TOWN.4`, `CASTLE.4`, `CAVE.4`, `GLOBE.4`, `DISK.4`, `XFER.4`): row padding differs.

### `MONSTERS.16` / `MONSTERS.4` — monster pictures (`tools/mm2_monsters.py`)

Derived from `EGA.DRV` fn 16h (`1233`) and its piece decoder (`1422`); verified by rendering all 60
banks.  `MONSTERS.16` = 75 x `u32` offsets (0 = unused picture id) to LZW banks (`u32` size + LZW);
the loader `sub_16818` skips forward over unused ids.

```
bank:  u16 count (12 in the shipped files), u16 piece_offset[count],
       animation table (bytes up to piece 0), pieces
frame: 96x96 pixels, 4 bpp (48 bytes/row).  frame 0 = piece 0 painted over the screen background,
       frame k = frame 0 with piece k painted over it.
piece: u8 x, u8 y, u8 width, u8 height, then runs (u8): high nibble = length-1, low nibble = colour code;
       drawn at (x+4, y+6); runs wrap at `width`.  code 5 = transparent (leave the pixel),
       otherwise EGA colour = {0,1,2,9,6,8,10,3,4,5,7,11,12,13,14,15}[code]
```

Animation table (copied to `DGROUP:9E48` by the driver, played by `monster_anim_*`): sequences of
`(frame, delay)` byte pairs ending in `FFh`, the table ends with a second `FFh`; sequence 0 is the
idle animation (bit 7 of an entry = random delay).  The CGA `.4` pictures use the CGA driver's own
piece decoder (not decoded).

## Items (`ITEMS.DAT`, 20-byte records, id 0 = "BLANK")

Derived from `char_equip_item` (`2CMDS:1CA0B`), `char_use_item`, `item_effect_dispatch` and the shops;
the numbers were checked against the data (256 records, names are plain ASCII).

| Off | Size | Meaning |
|---|---|---|
| 00 | 12 | name, space padded |
| 0C | 1 | always 0 |
| 0D | 1 | **class restriction mask**: the item cannot be used by class *c* if `bit (7-c)` is set (`DGROUP:3408` = 80h,40h,...,01h for Knight ... Barbarian); 0 = everybody |
| 0E | 1 | magic bonus: high nibble = attribute (0-5 = Might, Intellect, Personality, Speed, Accuracy, Luck; `F0h` exactly = **not equippable**, e.g. tickets/keys/BLANK), low nibble = bonus amount (`2CMDS:1CC54` adds it to the current stat while equipped) |
| 0F | 1 | effect id used by "Use" (0 = the item cannot be used, error 0Fh); dispatched by `item_effect_dispatch` through range tests (`1CFDE`, `1CFF6`, `1D00E`, `1D026`, `1D03E`, `1D056`) |
| 10 | 2 | main value: weapon damage / armour class bonus |
| 12 | 2 | price in gold (`Sun Crown` 10000) |

Equipping checks, in order: free equipped slot (error 2), class mask (error 4), alignment
(instance flag top 2 bits mapped through `DGROUP:3404` = {0,2,0,1} must equal the character's alignment,
error 5), `F0h` (error 0Eh); a flag byte of `FFh` marks a cursed item that sticks (`char_equip_item`
sets the character's condition bit 1 and reports error 3).

## Map attributes (`ATTRIB.DAT`, 64 bytes per map, copied to `DGROUP:5986`)

Fields identified from their users (offsets from the start of the block; linear address of the copy =
`231D6 + off`); values checked against the file:

| Off | Meaning |
|---|---|
| 00 | the map's own number |
| 04 | low nibble = graphics/terrain style used for the sky/background picture (`view_load_sky` `1B1D4`) |
| 05-08 | neighbouring map ids used when the party walks off the 16x16 grid (`map_edge_transition` `1B75E`): `05` for y = 16, `06` for x = 16, `07` for y = -1, `08` for x = -1 (towns/dungeons point to themselves) |
| 09 | random-encounter chance: a roll `rand(1, value)` equal to 1 starts a fight on each step (`game_main_loop`) |
| 0A | minimum monster count for generated encounters (`combat_encounter_ok`) |
| 0B, 0C | maximum / minimum monster tier for generated encounters (`combat_generate_encounter`) |
| 0D | percent chance that "Run" succeeds (`byte_231E3`) |
| 0E | safe cell (`y<<4|x`) the party is moved to when it flees or is teleported (`byte_231E4`) |
| 0F | era number of the map: event triggers only run when it equals the current era (`evt_run_script`) |
| 11 | read by `combat_encounter` (`byte_231E7`; stealth/hide difficulty?) |
| 12.. | not traced (contains cell coordinates and bit patterns) |
