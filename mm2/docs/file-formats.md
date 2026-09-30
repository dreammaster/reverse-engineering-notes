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
| `ITEMS.DAT` | 5120 | not compressed; 256 items × 20 bytes, loaded to `DGROUP:6960` (`g_items`). Item name is the first bytes (`print_item_list` prints `20*id + 6960h`); other fields not traced |
| `SPELLS.DAT` | 192 | not compressed; loaded to `DGROUP:7D60` (`g_spells`); layout not traced |
| `ATTRIB.DAT` | 3840 | LZW; not traced (used by character creation, 1MENU2) |
| `MONSTERS.DAT` | 6656 | LZW; 416 records × 16 bytes, see below |
| `STR.DAT` | 7707 | LZW; then every byte `+ 1Ch` (mod 256) gives ASCII; `1Dh` = newline. Tavern jokes/rumours |
| `MAP.DAT` | 18748 | 60 × `u16` file offsets (index 0 = 120), each a LZW chunk that decompresses to **512 bytes** = one 16×16 map, two layers (256 B walls + 256 B flags, see below) |
| `EVENTSI.DAT`, `EVENTSO.DAT` | 49609 / 25797 | 71 × `u32` chunk offsets (0 = none). `I` holds maps 0-4 and 17-… ("indoor"), `O` maps 5-16 (`g_outdoors` picks the file, `load_map_events` `16008`). Each chunk is a LZW chunk; see [events.md](events.md). Chunks 60-70 have another structure (not traced) |
| `ROSTER.DAT` | 8292 | first `1860h` bytes = 48 character records (24 characters + 24 hirelings, `82h` bytes each) loaded to `g_characters` (`load_roster` `1276C`); remaining 2052 bytes not traced (presumably game state) |
| `DEFAULT.DAT` | 780 | new-game default roster template (loaded by 1MENU2 `19660`); not traced |
| `*.DRV` | 3-5 KB | video driver code modules (`TCGA/EGA/TGA/HGA/MCGA.DRV`) and `TIMER.DRV`; loaded whole into memory and called through a jump table at offset `fn*3` (`driver_call` `11CDA`) |
| `*.16`, `*.4` | | 16-colour (EGA/VGA) and 4-colour (CGA) graphics for each map style / monsters / screens (names at `DGROUP:1DD…`); not traced yet |

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

### `MONSTERS.DAT` record (16 bytes) — `monster_decode_stats` (`13B80`)

| Off | Meaning |
|---|---|
| 0-13 | name, each byte `& 7Fh`, space padded |
| 14 | bits 0-5: dice count − 1; bits 6-7: index into word table `DGROUP:4DB8` (dice size) → HP |
| 15 | bits 0-4 + 1: multiplier; bits 5-6: index into `4DB8`; bit 7: ×1000 → experience reward |
| 16.. | further fields decoded into flags/values `DGROUP:7660..7695` (attack dice, speed, accuracy, resistances, special powers); layout partly traced — see the function |

## Character record (`82h` = 130 bytes, `g_characters + roster_id*82h`)

From the display code (`show_character_sheet` `12A6A`, `show_party_roster_screen` `144AE`,
`char_reset_current_stats` `13572`, `age_party_one_day` `15092`, …).  Offsets are hex.

| Off | Size | Field |
|---|---|---|
| 00 | ≤11 | name (NUL terminated) |
| 0C | 1 | sex (0 = male) |
| 0E | 1 | race (index into `DGROUP:0456` names: Human, Elf, Dwarf, Gnome, H-Orc) |
| 0F | 1 | class (index into `DGROUP:0446`: Knight, Paladin, Archer, Cleric, Sorcerer, Robber, Ninja, Barbarian) |
| 10-15 | 6 | base Might, Intellect, Personality, Speed, Accuracy, Luck |
| 1E | 1 | thievery |
| 20 | 1 | base level |
| 21 | 1 | age (years; capped at 200) |
| 22 | 1 | day counter (rolls at 181 → age+1) |
| 23 | 1 | base spell level |
| 24 | 1 | armour class |
| 25 | 1 | food |
| 26 | 1 | condition bits (`print_condition`: good, cursed, silenced, diseased, poisoned, asleep, paralyzed, unconscious, dead, stone, eradicated); ≥ 80h = out of action, `E0h` mask = unable to act |
| 27 | 1 | base endurance |
| 28-2D | 6 | item slots list A (`print_equipped`, left column) — item ids |
| 34-39 | 6 | list A: per-item charges/bonus (`& 3Fh`) |
| 3A-3F | 6 | item slots list B (`print_backpack`) |
| 46-4B | 6 | list B charges |
| 40-45 | 6 | list B extra byte (set by `evt_op25_give_item`; meaning not traced) |
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
