# Cadaver — file formats

Status legend: **verified** = decoded by tooling in this repo and cross-checked
(against DOSBox screenshots, against the other game's copy of the same file, or
against the game's own routine run in the emulator harness); **partial** =
structure understood, not complete; **unknown**.

All multi-byte values are little-endian unless stated (the data files were
authored on 68000 machines and several formats keep big-endian words).

## 1. The `mjm` container — *verified*

Used by every file under `DATA/` and `TATA/` (`*.LNG`, `*.MP1/2/3`,
`ROUTINE*.TAD/PAK`, `ANDATA.DAT`, `PCMAN.DAT`). Four-byte header:

| Bytes | Meaning |
|---|---|
| `6D 6A 6D` | `"mjm"` magic |
| `30`/`31`/`32` | method `'0'` stored, `'1'` PKWARE DCL, `'2'` RNC1 |

* **Method `'1'`** — a stock **PKWARE Data Compression Library "implode"**
  stream (same format as zlib's `contrib/blast`). Stream header is the usual two
  bytes: `00` = binary literals, then dictionary bits `06` (4 KiB window).
  The game's own explode routine is in `CAD.EXE` seg001 (entry `sub_1ECD3`
  / worker `sub_1EDCD`, which returns `0x306` = end-of-stream; it validates the
  two header bytes exactly as above: type 0/1, dictionary bits 4..6). The
  error text `"DAC 1 HEAP TO BIG BY nnnn BYTES"` at `1F17:F892` is probably
  its. In the emulator the file is read in 2 KiB chunks by `sub_1C7D1`
  (`1000:C94E`), a coroutine that swaps stacks between the DOS reader and the
  decompressor, and the output is copied to its destination by `rep movsb`
  at `1000:C899/C8D5`.
* **Method `'2'`** — Rob Northen "Pro-Pack" `RNC\x01` stream (big-endian header:
  unpacked length, packed length, CRC16 of each). Cadaver 2 re-packed
  `ANDATA.DAT`, `PCMAN.DAT`, `ROUTINE*.PAK` this way (they decompress to the
  *same* bytes as Cadaver 1's `mjm0`/`mjm1` versions; verified by size and
  `cmp` for ANDATA/PCMAN; ROUTINE sizes match `*.TAD` minus 4).
* **Method `'0'`** — raw payload after the 4-byte header (`ROUTINE*.TAD`).

Tooling: `tools/mjm.py`, `src/engine/mjm.c` (identical output for all 101
decodable files; the other file, `cadaver2/DATA/3/FRE3.LNG`, has eight `0xFF`
bytes at offset 357 in the retail dump — corrupted copy; Cadaver 1's copy is
intact and the Cadaver 2 `DATA/` tree is otherwise byte-identical to it).

## 2. Directory layout

Both games read `data\<disk>\lev<n>.mp1|2|3`, `data\<disk>\routine<n>.tad`,
`data\<disk>\eng|fre|ger<n>.lng`, `data\andata.dat`, `data\pcman.dat`
(file-name strings in the initialised stack segment, `6C08:4C32…`; see `renames/cadaver1/cad.tsv`, `path_*`).
`?` in these names is replaced by the current disk/level number at run time.

| | Cadaver (1990) | Cadaver: The Payoff (1991) |
|---|---|---|
| Executables | `CADAVER.COM` → `INTRO.EXE`, `CAD.EXE` | `CADAVER.EXE` → `INTRO.EXE`, `GAME.EXE` |
| Levels | `DATA/1..5` | `DATA/1..5` (the original five levels again) **plus** `TATA/1..4` (the new Payoff levels) |
| Routines | `ROUTINE<n>.TAD` (`mjm0`) | `ROUTINE<n>.PAK` (`mjm2`) |
| Sound | `ADLIB/BEEPER/SBLASTER.BIN` | + `ROLAND.BIN` |

The drivers are loaded as raw code blobs (CS:0 and CS:4 entry stubs).
`CADAVER.INI` is a text file: sound driver, video, input, … (see the `SETUP`
programs).

## 3. `.PC1` pictures — *verified*

`LOGO.PC1` (title screen shown by INTRO), `LOADING2.PC1` (the scroll with
candles used as backdrop for the intro text and the loading messages).
Decoded by `INTRO.EXE seg000:0B09` (found by tracing reads of the file buffer in
the emulator harness).

File = blocks: `u16 length` (0 terminates), then an LZ stream.

```
c = next byte
c <  0x80 : copy c literal bytes
c >= 0x80 : n = c & 0x7F (n = u16 if c == 0xFF);  off = u16
            copy n bytes, byte by byte, from out[block_start + off]
```

Decompressed picture (`tools/pc1.py`):

| Offset | Size | |
|---|---|---|
| 0 | 2 | unused |
| 2 | 32 | 16 Atari-ST palette words, big-endian `0x0RGB`, 3 bits/channel |
| 34 | 32000 | 4000 groups of 16 pixels; each group = 4 big-endian words, plane 0..3 (ST "interleaved" layout) |

INTRO copies the palette to its palette table and writes the four plane words of
each group to VGA planes 0..3 with the map-mask register (mode 0Dh, 320×200×16).

## 4. `FONT2.SPR`, `FLAME.DAT` and the title scroll — *verified*

Used by `INTRO.EXE`; the files are identical in both games, as are `LOADING2.PC1` and `LOGO.PC1`.
Everything here was checked pixel for pixel against the running intro (0 differing pixels in a
full 320×200 frame, `tools/intro.py`, `src/engine/intro.c`, unit-tested for both games).

**`FONT2.SPR`**: 31 glyphs × 32 bytes, each 8 rows × 4 plane bytes (bit 7 = leftmost; colour =
bits of the four plane bytes). Only the top-left **6×6** pixels are used, drawn **opaque**
(the scroll colour is baked into the glyph; colour 0 only occurs outside the box). Order:
`A`–`Z`, `.`, `,`, `?`, `-`, `'`. Text is upper-cased, advance 6 px, lines on a 6-px grid,
space draws nothing.

**`FLAME.DAT`**: 6 frames × 24 rows × 8 bytes (16 pixels per row as four big-endian plane
words, ST interleaved), drawn as opaque 16×24 blocks (colour 0 = black — this wipes the static
flame in the picture). Left flame at (14, 20), right flame at (296, 20). Animation (driven from
the INT 8 handler at ~50 Hz; the original redraws as fast as it can): the left flame's frame
advances by 2 or 3 (mod 6) per tick (about 2.28), the right flame shows frame `(left + 2) mod 6`
(frames 2 and 4 are pixel-identical).

**Title page** over `LOADING2.PC1`, text starting at x = 68, lines at y = 76, 82, 100, 106, 118, 130:
`          cadaver` / `          -------` / `copyright nineteen ninety-one` /
`    the bitmap brothers` / `          -------` / `  published by imageworks`
(Cadaver 1) or `   published by renagade` (The Payoff). The story ("chapter thirteen — the story so
far …") and the credits are further plain-text strings in `INTRO.EXE` (padded with spaces for
centring); how those pages are sequenced is not yet analysed, and their text is not copied into
this repository.

## 5. Language files `*.LNG` — *verified*

`ENGn.LNG` / `FREn.LNG` / `GERn.LNG` (per level; `TATA/` has its own for the
Payoff levels). After the container is removed:

| Offset | Size | |
|---|---|---|
| 0 | 2 | `u16be count` (387 in `ENG1`) |
| 2 | 2×count | `u16be` string offsets, relative to the string area |
| 2+2×count | 2 | two bytes that belong to no string (purpose unknown) |
| 4+2×count | … | string data; **string area = this address**: string *i* starts at `area + offset[i]` |

Strings are runs of **6-bit codes, four per three bytes, MSB first**
(`c0 = b0>>2`, `c1 = (b0&3)<<4 | b1>>4`, `c2 = (b1&15)<<2 | b2>>6`, `c3 = b2&63`);
code 0 ends the string, codes 48–63 produce nothing. Code → character:

```
 1..26  A-Z      27 ' '    28 '.'    29 ','    30..39 '0'-'9'
 40,41  CR (line break)    42 '-'    43 '?'    44 '"'    45 '<'    46 '='    47 '>'
```

`<`, `=`, `>` are the German Ö, Ä, Ü glyphs in the Payoff (`TATA/`) texts
(`TR=GT`, `F>R`, `G<TTER`); Cadaver 1's German files use `<` for *every* umlaut
(`T<R`, `G<TTER`). French has no accents. Everything is upper case.

Decoder in the game: `text_get_string` (`1000:75AF`, index in DX) → `text6_decode`
(`1000:75CA`), which writes to `es:0A62` (`es` = `text_buffer_segment`). The game
copies the file into its stack segment as `ss:9738` (offset table, converted to
little endian) and `ss:9B20` (string area + 2). Verified in the emulator
harness: in-game it produced `CAVERN`, `DAY`, `SILVER COIN`, `STONE`, `BOAT`,
`DIARY`, and the same strings are in the decoded `ENGn.LNG`. Level 1 `ENG1.LNG`
(387 strings): `0` = `"1.0"` (a version string), `1` = `"YOU UNLOCK THE DOOR"`,
`2` = `"THAT DOOR IS BLOCKED"`, `3` = `"YOU FIND NOTHING"`, …, `28` = `"A DAY
PASSES ..."`, `29` = `"DAY"`. (Strings 16–25 are sentence fragments that the
game concatenates with numbers: `"THERE ARE " n " SHOTS LEFT EACH WITH A POWER OF "`.)

Tooling: `tools/lng.py`, `src/engine/lng.[ch]` (identical output for all 41
decodable language files; unit-tested).

The fixed system messages ("PLEASE INSERT DISK 2 …", save/restore prompts) are
*not* in the `.LNG` files: they are plain ASCII inside the EXE in English, French
and German (`6C08:4170…`, `6C08:4883…`, `6C08:4A23…`).

## 6. Level, animation and sprite data — *partly decoded*

Files: `LEVn.MP1` (~105–120 KB unpacked), `MP2` (24–37 KB), `MP3` (55–63 KB),
`ROUTINEn.TAD/PAK`, `ANDATA.DAT` (20 454 bytes), `PCMAN.DAT` (41 936 bytes).
Load order in the running game (traced): `ANDATA`, `PCMAN` at start-up, then per
level `ROUTINEn`, `LEVn.MP1`, `MP2`, `MP3`, `ENGn.LNG`.

### 6.1 Graphics storage — *verified*

The game keeps bitmaps in **EGA/VGA video RAM** (mode 0Dh, 320×200×16, planar),
with several screen-sized pages (the room is drawn into a larger scrolled buffer)
and sprite sheets in the top ~25 KB of each plane, and blits with the VGA latches
(write mode 1) and AND/OR masks. Consequences: on-disk bitmaps are plane-separate
(or ST-interleaved for `.PC1`), not chunky.

**Sprite drawing** (`sprite_blit`, `CAD.EXE 1000:8E26`): x pixel in AX, y in DX (the
scanline offset comes from the 200-word table `vram_row_offsets` at `ss:0000`:
0, 40, 80, … 7960), the blit variant in CL, sprite data in RAM at `es:si`.
The variants (`1000:8E81`, `9080`, `92FF`, `9602`; `sprite_blit_mode_table`
at `1000:8E5D`) shift the data right by `x & 7` and draw it plane by plane: first
AND the screen with the *inverted* mask plane (punching the silhouette out), then OR in
colour planes 0–3 (`1000:9198…9273`, per-plane stride `ss:3B16`) — exactly the
record layout of §6.2 (mask plane first, then four colour planes, each stored
as consecutive rows).

### 6.2 `PCMAN.DAT` (the player's sprites) — *verified*

48 frames of the player character (walking in all directions, sword swings, …),
identical in Cadaver 1 and 2. The unpacked file is two **raw VRAM plane dumps**,
loaded unchanged into VRAM (found by matching emulator VRAM against the file):

| File bytes | Destination | Sprites |
|---|---|---|
| `0 … 24575` (24 576) | plane 2, VRAM offsets `A000h … FFFFh` | 0–27 |
| `24576 … 41935` (17 360) | plane 3, VRAM offsets `A000h … E44Fh` | 28–47 |

Each area starts with the **directory**: 48 × `u16le`, a paragraph offset per sprite
(the directory itself fills the first 6 paragraphs; an `FFFF` follows the last entry).
A directory word whose high byte is ≥ 6 refers to the plane 3 area (subtract `0600h`).
Sprite data for entry `o` starts at byte `(o + 6) * 16` of its area:

```
i16le hot_x, i16le hot_y      small signed offsets (-6..+4); copied into the object by the loader
u16le w                       width in 16-pixel words (always 2 -> 32 pixels)
u16le h                       height in rows (38..45)
5 bitplanes, one after the other, each h rows of 2*w bytes (MSB = leftmost pixel):
    plane 0      transparency mask (1 = opaque; exactly the OR of the colour planes)
    planes 1..4  colour bits 0..3
(padded to a multiple of 16 bytes)
```

Colours index the game's 16-entry palette (§6.9). Verified by the game's own loader
`sprite_load_from_vram` (`CAD.EXE 1000:8D86`; traced reading VRAM `A000h+(dir+6)*16`),
by rendering all 48 frames (`tools/pcman.py`, `src/engine/sprites.c`; identical
output, unit-tested for both games) and by comparing with the player in the
running game (grey armour, brown boots). The loader copies `10·w·h + 4` words per
sprite into RAM, i.e. more than the 5-plane record — it over-reads into the next
record, presumably because the RAM sprite buffer is a fixed size; not investigated.

### 6.3 `ANDATA.DAT` — *block 1 verified (the screen frame)*

`u16be count` (6), `count × u16be` block sizes (1904, 15840, 576, 920, 1104, 96),
then the blocks back to back (the sizes tile the file exactly; `tools/mpsections.py`).
Identical in Cadaver 1 and 2.

* **Block 1 (15 840 bytes) = the frame around the isometric room view** (verified, `tools/panel.py`, `src/engine/panel.c`).
  The room is drawn into the whole 320×200 screen and then two wedge-shaped *wings* are laid over rows 115–199 (the diamond-shaped view is
  cut off by diagonal edges; everything outside — the bone/skull ornament, the six inventory slots, the status area — is wing). `CAD.EXE
  1000:D523` (called once, tick 751 of the trace) draws it from block 1 (ST-interleaved 16-pixel groups, 4 big-endian plane words each).
  For row `r = 115 … 199` with `cl = -1` for the first 8 rows, `0` for the next 8, … (one more group every 8 rows) the block holds, in this
  order: `cl+1` left-wing groups (columns 0…cl), the left edge group (column `cl+1`), the right edge group (column `18-cl`) and `cl+1`
  right-wing groups (columns `19-cl…19`). The two edge groups are ORed over the room pixels that a stair-step mask lets through
  (`bx = 0xFFFF >> 2·(row mod 8)` for the left edge, `bp = 0xFFFF << 2·(row mod 8)` for the right one: pixel = (old & mask) | wing; wings overwrite).
  The first half of the block (7 920 bytes) is exactly this; **the second half (7 920 bytes) is a second set in the same layout** (selected by the
  routine's `[401A]/[401C]` offsets — probably the alternate/dimmed or mask variant; not used on the first screen).
  Composite of floor + walls + objects + wings equals **98.2% of the original's full 320×200 first frame** (level 1 room 0; the remainder is the
  player, one animated object and the status text), unit-tested C vs Python on all rooms.
* Blocks 3 and 4 (920 and 1104 bytes) look like 1-bit-per-pixel fonts (the status text "DAY 1", room name); block 2 (576 bytes) and 5 (96 bytes)
  small sprites; block 0 (1904 bytes) a table of 8-byte entries (e.g. `80EE 0040 7F30 80FE`, `FFFE 0000 0000 FFFE` = empty). Not decoded.

### 6.4 `LEVn.MP2` — *structure verified*

Four **sections** back to back, which tile the file exactly in all 5 + 4 levels
(one Payoff file, `TATA/2/LEV2.MP2`, is 74 bytes short in the retail dump):

```
u32be data_len      bytes of record data in this section
u32be alloc_len     size of the table once expanded in memory = count * stride
u16be count
count x { u16be size, u16be offset }    directory, offsets contiguous from 0
data_len bytes      variable-length records (big-endian words)
```

| Section | count × stride | what it holds |
|---|---|---|
| 1 | 100 × 52 | script/event records (messages, rank titles such as PEASANT, ROGUE, SCOUT, INITIATE, PATHFINDER, RANGER, TRACKER; a word at `+0x0A` is a language string id, `FFFF` = none). Probably the ACL scripts |
| 2 | 400 × 8 | small 8-byte records (e.g. `36 3F 00 00 02 01 00 00`) — unknown |
| 3 | 100 × 80 | **room lists**: each record is a list of big-endian **thing ids** (see below). Record 0 holds global things (version string, "CANNOT", "SEARCH", the save-game prompts …) |
| 4 | 1000 × 26 | **things**: record *i* is thing *i*; the big-endian word at `+4` equals *i* in every non-empty record of every level (509 things in level 1, 815 in level 2 …) |

**Thing ids are language string ids.** A thing's name is `ENGn.LNG` string *i*
(`CAVERN` = 133, `BOAT` = 187, `STONE` = 201, `SILVER COIN` = 215, `DIARY` = 242 in
level 1), and its descriptive messages are other things (strings) in the same
list ("THE LID IS TIGHTLY CLOSED", "THE KEY IS COVERED IN SPIDERS WEBS"). In the
running game the name routine (`1000:22C4…2314`, called with the string id in DX)
follows an object → type record pointer and reads the name id from a word at
type-record offset `+0A`; `FFFF` (negative) means "unnamed".

**Room lists** (section 3): the words are thing ids, every thing appears in exactly
one list (no duplicates in any of the nine levels), e.g. level 1 list 14 =
`WELL, STONE BAG, STONE BLOCK, BREASTPLATE, "THE FLASK HOLDS A BRIGHT LIQUID", …`,
list 12 = the prison (`LESSER, COMMON, WARRIORS, GLUTTONS, TELEPORT, CELL 1, CELL 2,
CELL 3`). List *n* is room *n* (verified, §6.12). `tools/things.py` dumps
them with names.

**Thing record layout** (compact form, big-endian, 16–132 bytes, mostly 16–44):

| Offset | |
|---|---|
| `+0`, `+1` | `x1`, `y1`: the object's far corner in the room, in 1/8 cell units (verified: the game's object entries hold exactly these, §6.12) |
| `+2` | `z0`: height of the object's base, same units (0 for things on the floor; 0x30 for the flame bowl on the wall, 2 for the pentagram) |
| `+3` | flags: `0x80` = not in the room (inside a container / not placed yet — the game does not build an object entry for it); `0x20` = present in many bigger objects (undecoded) |
| `+4` | `u16` **thing id** (= the record index = the name string id) — all levels |
| `+6` | `u16` **index of the sprite block** (`LEVn.MP1` section 2) that draws the thing — verified for every placed object of level 1 room 0 (§6.12) |
| `+8` | `u16` **slot offset = 70 × (position of the thing in its room list)** (verified for every thing of 8 of 9 levels; level 1 list 39 restarts at 0 for its 4th entry). So the loader gives each room an array of 70-byte object slots |
| `+10` | byte **room list number** the thing belongs to — verified for every thing in all nine levels (BOAT → 29, SILVER COIN → 27, DIARY → 68). Room list *n* is room *n* (level 1 room 0 reproduced from list 0) |
| rest | variable tail (e.g. `16 16 16 00 06 90 48 …` on BOAT): per-thing state/behaviour data, undecoded |

(Earlier guesses — "class", then `+6/+8/+10` as x/y/z — were wrong; see above.)

The loader ("EXPANDING DATA") byte-swaps the 16-bit fields in place and stores the
records back to back (not at a fixed 52-byte stride): level-1 script record 0
`54 0C 0F 12 0A 0A 00 32 00 3B …` appears as `54 0C 0F 12 0A 0A 32 00 3B 00 …` at
`ds:3DDA`, and the next compact record follows directly after its 104 bytes.
Preceding it at `ds:3DC0` is a table of string ids `0085 0086 0087 …`
(**room names are strings 133 + n**).

### 6.5 `LEVn.MP1` — *sections 0 and palette verified*

```
72-byte preface                     (first word 0x0019, a few small constants)
3 sections, each starting on an EVEN offset (a pad byte follows odd-length data):
    u32be data_len, u32be alloc_len, u16be count, count x (u16be size, u16be offset), data
palette: 16 big-endian ST words      (right after the third section; §6.9)
1790-byte trailer
```

| Section | count | alloc | contents |
|---|---|---|---|
| 0 | 100 | 10 000 (100 × 100) | **floor maps**: record *i* = `w*h` tile-id bytes for room *i* (row major, 0 = no tile), where `w`, `h` are bytes `+4`, `+5` of `LEVn.MP2` section-0 record *i*. The match `size == w*h` holds for ~98% of rooms in all nine levels; the others are 1-byte placeholders (27 of 100 records are empty in level 1). The loader inserts one empty column and row (11×11 for a 10×10 room) |
| 1 | 100 | 32 008 | **wall records**, one per room (verified): a 2-byte header (byte 0 = `slots`, the number of tile slots per column — 6 in level 1; byte 1 = 0) followed by `w + h` columns of `slots` bytes. Columns `0 … w-1` are the right-hand wall starting at the corner, columns `w … w+h-1` the left-hand wall going outward (so level 1 room 0, 10×10, is 2 + 20×6 = 122 bytes). Each column is a stack of wall-tile ids (`MP3` block 2), **bottom to top**; `0` and ids ≥ `0x80` (e.g. `86`, `C6`) occupy a slot but draw nothing. Placement, with `(gx, gy)` the floor origin of §6.5: right wall column `i` at x = `gx + 16·i`, left wall column `j` (1-based) at x = `gx − 16·j`; slot `k` of a column has its top at y = `gy − 17 + 8·i − 16·k` (right) or `gy − 17 + 8·j − 16·k` (left); tiles are clipped to the 320×136 viewport (a tile cut off at the top is drawn from the corresponding source row, 4 bytes per row per plane). Columns are drawn in record order, each bottom to top, after the floor. `thing_expand_grid` / `room_expand_wall_columns` (`1000:39F9`) expands this record into a buffer |
| 2 | 255 | 255 000 (255 × 1000) | 255 blocks. The directory `size` is the block's size in RAM; **each block is stored in `floor(size/2)` bytes** (consecutive, in order; the sum over all 255 entries equals the section's `data_len` exactly — rounding odd sizes *up* shifts every later block by a byte and made ~40% of the blocks look unparsable; `offset` is a `u16` that wraps). Most of the blocks are **object sprite blocks** (verified): a 32-byte header (box size etc., §6.12), optionally an animation table (§6.12), and one or more frames `u16be w` (16-pixel groups, 1–8), `u16be h` (rows), `h·w·8` bytes of **Atari-ST interleaved pixels** (per row, per 16-pixel group, four big-endian plane words, plane 0 = colour bit 0; colour 0 = transparent — the game derives the mask as the OR of the planes). The frames consume the block exactly (114 of the 124 non-empty blocks in level 1 incl. 12 animated ones; 123/148/165/123 in levels 2–5; found by matching the RAM sprites that the game blits against the file). Level 1 contains the boat, barrels, jars, rocks, rats, bones, helmets, shields, the pentagram …; the Payoff levels have doors, crates, keys, potions. The other blocks (e.g. block 8, 29 424 bytes, starting `00 00 00 00 00 01 00 01 00 01 FF FF …`) are tables/definitions, not decoded |
Directory entries are `(size, offset)` with offsets contiguous in sections 1 and 2 of MP2; in
MP1 section 0 of level 1 the last entry is `(320, 3002)`, past `data_len` (so use `data_len`
for the section length). The trailer is a table of 8-byte records (`u16be id` + six
spaces, then zeros) followed by `u16be id, u32be value` pairs ending with
`0x4F 40000`, `0x50 45000`, `0x51 50000`, `0x52 60000` — probably per-object attributes
(prices/scores?) or a script-compiler symbol table; not decoded.

**Object sprites** (`tools/objsprites.py`, `src/engine/objsprites.c`, identical output for all 14 level
directories, unit-tested): the sprite sheet of any level is shown by `--scene objects`. A thing's sprite block is
its record's `+6` (§6.12) and its position in the room view follows from `+0/+1/+2` and the block header
(`tools/objplace.py`, `src/engine/roomobjs.c`, verified pixel-for-pixel against the running game).

**Room shell** (floor + walls, verified): `tools/roomview.py render_shell` and `src/engine/rooms.c
room_render_shell` reproduce the room exactly as the game draws it into its 320×136 viewport. Compared with the
running game's first frame (level 1 room 0) **89.8% of the 36 944 pixels they cover are identical, and every
mismatch is an object or the player** (boat, barrel, mushrooms, pickaxe, coin, character): the whole shell is right.
The C and Python renderers produce identical output for every room (sizes 3–10) of all 14 level directories
(unit-tested).

**Floor drawing** (verified; `room_draw_floor`, `CAD.EXE 1000:D1FA`): for each cell
`(col, row)` of the room, tile id `t = map[row*w + col]`; if non-zero the 32×18
tile `t` is drawn at screen position `(x0 + 16*(col-row), y0 + 8*(col+row))`
(video offset `col*0x142 + row*0x13E`), rows top to bottom, OR-blitted with its
mask. **Placement on screen** (`room_setup_floor_origin`, `1000:D2C5`): the video offset of tile
(0,0) comes from an 8×8 table of words at `ss:4D30` (`floor_origin_table`, identical in
`GAME.EXE`), indexed `[(h-3)*8 + (w-3)]`; offset = `y*40 + x/8`. A 10×10 room starts at
pixel (144, 29) so its floor spans exactly the 320-pixel width; 3×3 at (144, 80).
Checked by comparing the rendered floor with the running game's first frame: best
alignment is exactly (144, 29) with 61% of floor pixels equal (the rest is covered by
walls, objects and the player).
Rendering rooms this way reproduces the running game's floor
(`tools/roomview.py`, `src/engine/rooms.c`; identical output for every room of all 14
level directories, unit-tested).

### 6.6 `LEVn.MP3` — *floor tiles, verified*

```
u32be tile_bytes (17 280 in level 1 = 60 tiles)
tile_bytes / 288 tiles of 288 bytes; tile 0 is empty
u32be length (40960 in the levels checked = 80 tiles)
80 wall tiles of 512 bytes
```

A tile is 32×18 pixels: 18 rows of 16 bytes; each row = two 16-pixel groups of four
big-endian plane words (Atari-ST interleaved, plane 0 = colour bit 0 — like `.PC1`).
The level loader (`CAD.EXE 1000:B7B5…B86D`, reading 288 bytes per tile with `mjm_read`)
derives the mask as the OR of the four plane words and stores `mask + 4 planes`
(5 × 72 = 360 bytes per tile, the same layout as the player sprites) at
`36EF:000A + 360 * id`. Verified by finding the file bytes of tiles 1–3 in the RAM tiles
and by rendering all 60 tiles (isometric diamonds with an edge: water, stepping
stones, cave rock; in the Payoff paving and so on).

**Second block: wall tiles.** The same loader function (`CAD.EXE 1000:B87C…B94A`, after the
floor tiles) reads a second `u32be` length and then 512-byte chunks (32 rows of 16 bytes,
same ST-interleaved layout), converting each into `mask + 4 planes` (5 × 128 bytes) for
a 32×32 pixel tile. They are the isometric wall blocks the room walls are built from
(cave-rock cubes in level 1, gold/Egyptian blocks in Payoff level 3); 80 per level, the
ids used by the wall layouts in `MP1` section 1 (§6.5) go up to `0x4F`. After this the
function opens the language file (`ENGn.LNG` through the file table at `ss:4CD3`) and
loads its offset table to `ss:[60]` byte-swapped to little endian (§5). The wall *layout*
(MP1 section 1: per-room records that start `06 00 37 38 39 …` / `05 00 12 13 30 30 30 C2
82 82 …`, where bytes with the high bit set look like run lengths/flags) is not decoded.
`tools/roomview.py` (`load_wall_tiles`), `src/engine/rooms.c` (`walltiles_load`),
and the `--scene tiles` viewer show them.

### 6.7 `ROUTINEn.TAD` and the script language

`ROUTINEn.TAD` (after the `mjm0` header; 16 zero bytes, then a small x86 stub
at offset `0x10`, then data): the stub is
`nop nop; push ds; mov ax,cs; mov ds,ax; mov si,0E5Dh; mov cx,10h; rep movsw;
pop ds; mov bx,27h; mov ax,2Ch; retf` followed at `+0x27` by
`call word [078Ch]; retf`. i.e. the blob is loaded as a code segment, the game
far-calls offset `0x10` to initialise it (it copies a 16-word table from the
end of the blob and hands back BX=entry thunk, AX=data-table offset) and later
calls the thunk, which dispatches through a pointer inside the blob.

**The rest of the blob is x86 code and data** (3.5–4.6 KB per level; `ndisasm -b 16` decodes it from
`+0x3A0`, e.g. a routine at `+0x81` tests `[bp+0Fh]`, `[bp+0Ah]`, `[bp+0Eh]` of an object's position record, fetches script
bytes with `cs lodsb` — `0x80`/`0x81` terminate/continue a script — and calls engine services through a far pointer at
`ds:078E` with a service number in AX (`mov ax,40h … call far [078Eh]`). So per-level behaviour (creature AI, object animation
commands, …) is **native code in the level's TAD segment**, not a bytecode: a faithful port needs those routines either
translated by hand (they are small) or run by a minimal 16-bit interpreter.

(The animation script commands — §6.12 — are *not* TAD calls: they are dispatched through a table inside CAD.EXE itself,
`1000:3C77`. An earlier note here said otherwise; `3C77h` is a code offset, not the TAD segment.)

The game also contains a bytecode interpreter (the in-house "ACL" language;
INTRO's credits: *"cadaver uses an internal language … 'acl'. acl is copyright
the bitmap brothers"*). Likely opcode handlers (small routines that fetch
operands with `lodsb`/`lodsw` from SI and poke object fields via `DI`) sit in
`CAD.EXE seg000` around `1000:7AE0–7C50`; **the link between the `.TAD` data
and the interpreter is not yet confirmed.**

### 6.8 Input

The game installs its own INT 9 handler (`keyboard_isr`, `1000:11E9`) and keeps
a 256-word table `key_bit_table` (`1000:128E`) filled from `CADAVER.INI`:
make code → bits to set, break code → mask of bits to clear. Default bits:
Up `01`, Down `02`, Left `04`, Right `08`, PgUp `09` (up+right), PgDn `0A`,
Ins `05`, Del `06`, End `80`. Keys not in the table are reported through
`last_scancode`. Ctrl+C / Ctrl+Q are recorded in `abort_scancode`. The harness
can inject scancodes (`Emu.scan_queue`), which is how the object names above
were provoked by walking the emulated player past objects.

### 6.9 Palette — *verified*

The game uses one 16-colour palette (EGA mode 0Dh, DAC entries 0–15) stored as 16
Atari-ST words (`0x0RGB`, 3 bits per channel, scaled `v*9` to the 6-bit DAC). The
base palette (Cadaver levels 1–5, Payoff levels 2 and 4):

```
0000 0243 0400 0032 0310 0420 0111 0775 0555 0200 0752 0221 0333 0444 0531 0642
```

It is in the executables as little-endian words (`CAD.EXE 1F17:F8C2`, `game_palette_st`)
and **in every level's `MP1`, big endian, right after the three sections** (§6.5; found
in all nine levels). Payoff `TATA/1` and `TATA/3` differ slightly (entry 1: `0245`;
`TATA/3` also `0610` for entry 2 and `0765` for entry 7). Per-level palette loading:
`palette_set_from_st` (`1000:D4C6`).

### 6.10 Room drawing, other layers — *observations only*

Traced in the harness while level 1's first room is built (ticks 737–760):

* The room is composed from **tiles that live in VRAM** (`A000:xxxx`, offsets
  seen: `05AA 087A 1252 13BA 1522 17F2 2BA2 40BA 492A` …, i.e. in the first pages)
  with a **mask in RAM** at `36EF:xxxx` using the *same offset*. Each tile blit is a
  pair of calls from `1000:D299`: the AND-with-inverted-mask variant (`1000:920E`,
  ~180 times) and the OR variant `sub_19080` (~230 times, `cx = 1200h` = 18 rows × 256?),
  with destinations in the room page (`di` = `049A`, `05DC`, `071E`, `0860`, …,
  advancing by `0142h` per tile = one row of 8 pixels + 2 bytes).
* Whole pages are then moved with `vram_copy_page` (`1000:D7E2`) from the room buffer
  (`A600`) to the display pages, and `vram_clear_page` clears pages first.
* How the tile atlas gets into VRAM (it is not `PCMAN.DAT`, which fills `A000h–FFFFh`
  of planes 2/3 only) and what `MP1`'s big third section (255 records) holds is still
  unknown. The loader copies a sprite column-wise into page 0 (`1000:8D55–8D70`,
  2392 bytes per plane at tick 158); not yet understood.

### 6.11 Player movement and animation — *observed in the running game*

Measured in the emulator harness on level 1 room 0 by logging the sprite number passed
to `sprite_load_from_vram` and by template-matching the player's sprite in the frames
(exact pixel matches, score 1.0), holding each key for 25 and 50 timer ticks (one tick
= one INT 8, ~50 Hz):

| Key (bit) | Walk cycle (PCMAN sprite numbers) | Idle frame after stopping | Screen motion per 25 ticks |
|---|---|---|---|
| Right (08) | 0, 1, 2, 3 | 16 | (+24, +12) — along +column (lower right) |
| Left (04) | 7, 6, 5, 4 | 21 | up-left (blocked after a few steps in the test) |
| Down (02) | 11, 10, 9, 8 | 26 | (−24, +12) — along +row (lower left) |
| Up (01) | 12, 13, 14, 15 | 31 | (+24, −13) — upper right |
| Up+Right, Down+Right (PgUp / PgDn) | same frames as Right / Down | 16 / 26 | — |

So the four keys move along the four isometric grid axes (not along screen axes): about
1 pixel horizontally and ½ pixel vertically per tick. The walk cycle advances every
~6 ticks. The idle frames form 5-frame sets (16–20 right, 21–25 left, 26–30 down,
31–35 up); frames 36–47 are other actions (not yet identified, probably sword swings /
taking damage). In the first frame of level 1 the standing player (sprite 16) is at
screen (148, 32), i.e. on floor tile (0,0) of the 10×10 room 0, whose top-left is at (144, 29).

### 6.12 Objects in the running room — *observed, partly decoded*

How the first frame of level 1 (room 0) gets its 19 objects, from the harness (post-load memory dump + blit and
write tracing):

* The current room's objects are an array of **50-byte entries** at `ds:[002A]` (1E40h; count in `ds:[0224]`).
  A blit log (`sub_19080`, which takes the source segment from `ds:[075A]`, the destination segment from `ds:[0002]`,
  rows in `ch`, shift in `cl & 7`, `+8` pixels if `cl & 8`) shows that every floor tile (source segment `36EF`), wall tile
  (`3C35`) and object sprite (own segment per sprite) goes through the same blitter.
* An entry holds (offsets in bytes): `+0 x1`, `+1 y1`, `+2 x0`, `+3 y0`, `+4 z_top` (world coordinates in 1/8 cell
  units; the object occupies the box `[x0,x1] × [y0,y1]`, the sprite block header gives its size as `extent + 1`),
  `+6` sprite segment, `+0A` pointer to its 16-byte position record, `+0C` video offset of the sprite's top-left,
  `+10h` screen y, `+11h` pixel shift, `+2A` sprite segment, `+2C` offset of the sprite data (what the blitter uses).
* Measured over the 19 objects: **screen x = 158 + 2·(x0 − y1) exactly** (0 residual); the sprite's bottom edge (screen y + rows) ≈ 30 + ½·(x0 + x1 + y0 + y1) − z_base,
  i.e. the projection of the footprint centre, within ±2 px over these 19 objects (too few samples to pin down the
  exact rounding; the exact computation is `object_project` below).
* The projection routine (`object_project`, `CAD.EXE 1000:5A61`): the position record gives `x, y, z` bytes; `x>>3, y>>3`
  select the cell, `x&7, y&7` the sub-cell; the cell's video offset comes from the table at `ds:08DC` (built by
  `room_setup_floor_origin` from the room's floor origin), the sub-cell's offset and pixel shift from a 64-entry table at
  `ds:F8CE`; then `(z − hdr[+2] + hdr[+10h] + hdr[+12h] + hdr[+14h])` rows are subtracted. `hdr` is the sprite block's
  32-byte header as words: `[1]`, `[2]` = 3, 1 (small constants), `[8]`, `[9]`, `[10]` = x-, y-, z-size of the object's
  box (e.g. 6, 5, 0x16 for the boat-sized block 14), `[12]` = `FFFF` …
* The position records live in the things table area (`ds:65E4 …`, 16 bytes each, stride 16): their first bytes are the
  object's `x1, y1` (the same `+0`, `+1` of the thing record in §6.4, which I had called a "class" — they are positions).
* **Thing → sprite → screen (verified).** For a thing in the room list (MP2 section 2, big-endian ids), with the compact
  record of §6.4 and `h[i]` = the big-endian words of the 32-byte header of sprite block `thing[+6]`:
  `x1 = rec[0]`, `y1 = rec[1]`, `z0 = rec[2]`, `xs,ys,zs = h[8],h[9],h[10]`; the box is
  `x0 = x1-xs+1`, `y0 = y1-ys+1`, `z_top = z0+zs-1` (the 50-byte entry's `+0..+4` hold exactly these, 19 of 19 in level 1 room 0).
  Screen position of the sprite's top-left pixel, as a pixel index `P = y*320 + x`:
  `P = 320*gy + gx + 2576*(x1>>3) + 2544*(y1>>3) + SUB[(y1&7)*8 + (x1&7)] - 320*N - 16*(h[2]-1) + (xs&7 ? 2*(8-(xs&7)) : 0)`
  with `N = z0 - h[1] + xs + ys + zs` and `(gx,gy)` the room's floor origin (`floor_origin`). `SUB` is the irregular 64-entry
  table at `ds:F8CE` (initialised data in the EXE, a table not a formula; `tools/objplace.py`). This reproduces all
  17 sprites of level 1 room 0 exactly (x and y) and renders every barrel, boat, jar, rock, bone and shield of that room
  at the right place (`tools/room_objects.py`, `--scene room` in the C program; 97.7% of the room pixels equal the game's
  frame — the rest is the player, one animated object and a few draw-order details).
* A thing with bit `0x80` in byte `+3` is not placed in the room (it is inside a container / not there yet): the room list of
  level 1 room 0 has 21 things, the game builds 19 entries, and the two missing ones (510, 488) have `rec[3] = 0x80`.
  Thing 0 (the player, block 8 — a 29 424-byte block that is not a plain sprite) comes first in the list.
* Header words: `h[1]` (1–4) and `h[2]` (1–4) shift the sprite by rows / 16-pixel words (see the formula above),
  `h[8..10]` = box size, `h[11..15]` = `?, FFFF, 0, n, FF00` (not decoded; `h[11]` differs per object).
* **Animated blocks** (`MP1` section 2; 12–39 per level, 356 over the 14 level directories) have, between the 32-byte header
  and the frames, an *extra* area (verified on all of them; the plain/animated test is `blk[32]` = frame count,
  `blk[33] = 0`, and the table-length word equal to `8·nframes`):

  ```
  +0  u8   nframes      +1  u8 0      +2  u16be script_bytes      +4  u16 0      +6  u16be h0 (height of frame 0)
  +8  script            script_bytes bytes of u16be words
  +8+script_bytes  table: u16be table_bytes (= 8*nframes) = entry 0's first word, then per frame k an 8-byte entry
                        u16be ram_offset, s16be dx, s16be dy, u16be 0           (entry k at +8+script_bytes+8k)
  then the nframes frames (u16be w, u16be h, ST planes), exactly as in plain blocks
  ```

  `dx, dy` are the frame's pixel offset from the projected sprite position (e.g. the flame bowl of level 1 room 0, block 86:
  frame 0 at `(0,5)`, frames 1–3 at `(0,4)`; frame 3 drawn by the game at tick 900 matches 100%). `ram_offset` is the position of the
  frame inside the game's expanded in-RAM copy (not needed here). The script words are frame numbers (`0..nframes-1`)
  interleaved with command words. **Animation runtime (verified against a trace of the running game and against the code,
  `tools/animation.py`, `src/engine/anim.c`, `CAD.EXE sub_13B35/sub_13BC5`):** each animated object has a 10-byte state
  (`delay, frame, dir, pos, dx(w), dy(w), cnt, cad`; initial values = the thing record's tail from `rec[+0Eh]`, e.g. `… 01 01` =
  `cnt = cad = 1`). The game loop runs one animation step every **2 timer ticks** (25 Hz). A script word is a byte pair
  `(command, argument)`: command 0 = plain frame `argument`; otherwise the handler `table[0xFF − command]` of the table at
  `1000:3C77` runs: `FF` freeze, `FE n` wait n steps, `FD` jump to the start, `FC n` jump to n, `FB` run backwards, `FA` turn
  round, `F9` freeze (delay `FE`) and fire a trigger, `F8/F7` queue an engine event, `F6/F5` decrement/increment a counter in the
  object record, `F4/F3` set a flag. A step sets `frame := script[pos].argument` and advances `pos` only every `cad + 1` steps
  (`cnt` counts down); `dx, dy` come from the block's frame table. Example (flame bowl, block 86): script frames
  `0 1 2 3 2 0 3 1 2 3 2 3 1 2 3 2 FD00`, one position per 2 steps = 4 ticks.
* **Object entry construction and collision (verified, ported):** the room loader (`CAD.EXE 1000:4A00…4C50`,
  inside `sub_14A00`) fills the 50-byte entries from the room list: `+0/+1/+5` ← thing `x1,y1,z0`; `+2/+3/+4` ← `x0 = x1-xs+1`,
  `y0 = y1-ys+1`, `z_top = z0+zs-1` (sizes from the block header); `+6/+8` ← sprite segment / header offset; `+0A` ← the 16-byte position
  record; `+26h` ← thing byte `+3`; `+27h` ← **sprite header byte 12** (bit 0 = flat/non-solid, `04` = animated, `20` = special frame
  table); `+20h` starts at 4. Entry `+14h` is the "solid" marker: `FFh` unless header byte 12 bit 0 is set (then the entry number)
  — checked on all 72 rooms of level 1 (0 violations). `sub_116E0` (`collision_test`) takes a candidate `(x1, y1, z)` plus the
  mover's box sizes and reports every entry whose x, y and z intervals overlap (all bounds inclusive, signed bytes). **It is
  reproduced by `tools/collision.py` and `src/engine/collision.c` and was compared with the original routine called inside the
  emulator on 3000 random positions with no mismatch** (`tools/verify_collision.py`; the C port against the Python reference on
  random worlds in the unit test). Blocked (`count = FFh`): `z < 0`, `z+h >= F0h`, `x1 >= 8w`, `y1 >= 8h`, the wall corner
  `x0 <= ds:5DF and y0 <= ds:5E0` (always 2 and 10 in level 1), `x0 < rec[21]`, `y0 < rec[20]`, or a door rectangle (`ds:54`, unused in
  level 1); results `flag = FDh` mark a solid overlap. If the mover leaves the room the routine looks for an **exit entry**
  (list at `ss:[3A]`, 50-byte entries in the *stack* segment) it touches and returns `count = FEh` with that exit's id.
* **Collision box trimming (`sub_14CB9`, verified):** the entry's box is the sprite box (sizes = header words 8..10) *trimmed* by
  three header bytes: byte `0Eh` (z: top −= low nibble, base += high nibble), byte `1Ah` (x: `x1` −= low, `x0` += high) and
  byte `1Bh` (y: `y1` −= low, `y0` += high). `tools/objplace.py trimmed_box`, `object_trimmed_box` in `src/engine/roomobjs.c`.
* **Cross-check of thing → entry → screen position on levels 1–5 (`tools/verify_objects.py`, ground truth from `tools/oracle_rooms.py`,
  which calls the original's `room_enter` and `object_project` for every room):** of 3153 placed things, ~2480 plain sprites have
  an entry with exactly the predicted trimmed box and exactly the predicted screen position. The rest are animated blocks (the game adds the
  current frame's offset: compared separately), compound blocks (rope segments: header flag `04` without a frame script), objects clipped at the top
  of the view (the game clamps y to 0), and the special rooms 69/78/84 whose lists contain things that do not appear as entries.
* **Exits (verified against the original for every room of levels 1–5 — 363 rooms, one room with an odd record excepted —, `tools/roomexits.py`, `src/engine/roomexits.c`):** the room record lists up to 7
  *door ids* (u16be at `+6`, `FFFF` unused). A door id indexes MP2 section 1 (8-byte records `x y ?? ?? len ori ?? ??`: `x, y` = global
  map cell, `len` = 1 or 2 cells, `ori` = 0 / 1 = door along y / along x); the room record's `+1`, `+3` are the room's own map origin.
  Local cell `(dl, dh)` = door − origin; when the result is negative the door is on the near side (`ch` 0 / 1), else the far side
  (`ch` 2 / 3). The exit's box comes from sprite block `(len-1)*4 + ch` of MP1 section 2 (blocks 0–7 are 8×16×48 / 16×8×48
  door-opening boxes): `x0 = 8·dl`, `y0 = 8·dh` (8 = the wall thickness `rec[21]` / `rec[20]` replaces a block size of 8, and the box is shifted
  by 4 on the other axis), `x1 = x0 + xs - 1`, `y1 = y0 + ys - 1`, `z = 16·(door level − rec[0x1E]) … +47`. Both rooms joined by a door list the same door id, so
  the target room of an exit is the *other* room that lists it. (Ground truth: the room-entry routine `sub_1DA60` can be called for any room in
  the emulator, `tools/emu_call.py`, `tools/oracle_rooms.py N` — the exits and the bounds of every room were read back that way; the level
  number is forced by writing `ds:0708 = level-1` before the files are opened.)
* **Landing after a door (verified, `tools/landing.py`, `src/engine/roomexits.c room_landing`):** the player appears at the same door *in the
  new room*, just inside it: north door (box `y0 == 0`) `x1 = box.x1 − 7, y1 = rec[20] + 7`; south door (`y0 ≥ 8h`) `x1 = box.x1 − 7, y1 = 8h − 1`;
  west door (`x0 == 0`) `x1 = rec[21] + 7, y1 = box.y1 − 7`; east door (`x0 ≥ 8w`) `x1 = 8w − 1, y1 = box.y1 − 7` (7 = the player's box size, z = 0).
  101 door crossings in level 1 (the player placed next to each exit in the emulator and walked through) gave exactly these positions
  (`tools/oracle_landing.py`, `tools/verify_landing.py`); the position along the wall does *not* depend on where the player left.
* **The player's move and pushing (verified, ported — `tools/push.py`, `src/engine/push.c`).** The player moves one unit per game step
  (every 2 ticks) along the pressed axis. The candidate position is tested with `collision_test`; *every overlapped object that is not
  solid is pushed* by the same `(dx, dy)`: each pushed object costs its sprite header byte 13 (its weight; 1 for a small flat thing,
  10 for the stone sign, 30 for the big sign…) against the player's strength `ds:59C` (= 50 in level 1), the pushed object is
  tested the same way (using its position record `x1, y1, z0` and header sizes, itself skipped) and its overlaps are pushed in turn
  (depth < 32); if anything on the way is blocked (room edge, wall corner, exit, solid object) or the budget runs out the whole move is
  cancelled. The position record and the entry box move together (`sub_167DF`). Replaying 8 random key sequences (hundreds of
  steps, pushing chains of two objects against the room wall) in the original and in the model gave **identical positions for every
  object after every step** (`tools/verify_push.py`); one unexplained event (a pushed sign vanished: probably pushed onto a hole/water tile) and the
  halving retry / climbing onto low solid objects are not modelled.
* Not plain sprites although things refer to them: the player (thing 0 → block 8, 29 424 bytes) and compound objects such as
  the rope (block 112–119 in level 1: a table of segment records) — undecoded.

## 7. Text strings recovered from the executables

INTRO.EXE contains the plain-text story ("chapter thirteen — the story so far
…") and the credits as lower-case strings starting at file offset `0x1278` of the
unpacked `INTRO.EXE`. The credits list, in the order stored (the string layout does not make
the heading→name pairing unambiguous): levels design; architectural graphics;
creature graphics; object graphics; title screen graphics; musician; sound
effects; coding; additional coding; production and concept; game co-ordinator;
manual and introduction; additional thanks — names: Phil Wilcock, Dan Malone,
Robin Chapman, Richard Joseph, Tony Williams, Mike and Steve, Sean Griffiths,
Robert Trevellyan, Mike/Steve/Eric, Graeme Boxall, The Word Factory, Mark Lane.
