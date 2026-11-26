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
* **Block 4 (1104 bytes) = the status-text font** (verified, §6.3a): 46 glyphs of 6 rows × 4 plane bytes (8 pixels wide). Block 3 (920 bytes) is
  another font (not used by the status line); block 2 (576 bytes) and 5 (96 bytes) small sprites; block 0 (1904 bytes) a table of 8-byte entries (e.g. `80EE 0040 7F30 80FE`, `FFFE 0000 0000 FFFE` = empty). Not decoded.

### 6.3a Status text ("DAY n", room name) — *verified* (`tools/status.py`, `src/engine/panel.c`)

Drawn by `room_draw_name` (`1000:DD40`) and `status_draw_day` (`1000:DDB5`) with the text routine `1000:8776`:

* Every character advances **7 pixels** (a space is only an advance). A glyph is 6 rows × 4 plane bytes (24 bytes, ANDATA block 4, 46 glyphs) and
  acts like a transparent sprite: pixels where any plane bit is set replace the screen, colours come straight from the planes. `A`–`Z` = glyph
  `c − 'A'`; characters below `A` go through the 32-byte table at `ds:FA40` (index `c − 0x21`):
  `"`→42 `%`→39 `,`→37 `-`→40 `.`→36 `/`→38 `0`–`9`→26–35 `<=>`→43 44 45 `?`→41, everything else 0.
* "DAY n" (n = `ds:5B2 + 1`): the 60×7 box at (252,157) is cleared to colour 0 and the text is centred in it (x = 252 + (60 − 7·len)/2).
* The room name is centred in the 108-pixel box at x = 202, y = 186 (x = 202 + (108 − 7·len)/2), after being built as
  **prefix + ' ' + base** (the order flips when `ds:70B == 1`, i.e. another language) and cut to **15 characters**:
  *prefix* = string id in the big-endian word at room record `+0x18` (`MP2` section 0; `0xFFFF` = none, the space is still added, so
  level 1's first room reads `" CAVERN"`); *base* = string id `table[rec[+0x1A]]` (byte ≥ 0x80 = none), where `table` is **20 big-endian words
  at offset 1630 of the 1790-byte `LEVn.MP1` trailer** (`126 127 128 …` in levels 1–3, `128…` in level 4, `144…` in level 5, `135…` in the Payoff's
  `TATA` levels). The strings are the level's `ENGn.LNG`.
* Verified: the first screen of levels 1–5 has all 198/146/296/354/280 text pixels equal to the emulator's frame; vectors (every room of all 13 level
  sets) test the C code against the Python reference.

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

<details><summary>Paragraphs of this section (69)</summary>

- The rest of the blob is x86 code and data
- The ACL interpreter (found, structure verified by reading the code; opcodes not decoded yet)
- event queue
- ACL virtual machine, first working subset (`tools/acl_vm.py`, verified)
- Script format (parsed and checked on 2453 records of all levels, `tools/acl.py`)
- Event sources and condition tests (read from the code and the emulator)
- 14
- Touching objects (`1000:22C4`, verified by hooks in the emulator)
- Message scrolls (op `1Ch` = say; `tools/message.py`, `src/engine/message.c`, verified against the emulator's screen)
- Verb icons (`tools/verbs.py`, `src/engine/panel.c`, pixel-exact against the emulator)
- the verb menu
- Energy (`ds:23A`, maximum `ds:700`)
- `ROUTINEn.TAD` decoded enough to run it (`tools/x86.py`, `tools/verify_x86.py`, `src/engine/x86.c`, `src/engine/tad.c`)
- Behaviour routines — how the engine reaches them (read from `1000:5F4C…601E` and the emulator; ported in `creatures_step`)
- More ops
- Choosing verb 2 in the original (emulator trace, level 1 silver coin)
- Verb 14 (`0Eh`, shown for sprites with header byte `0x1D == 0x12`: leg of meat, loaf, chicken)
- Ops 41 and 85 (verified against the original, 60 random cases)
- Op 25h (37, change room), decoded and verified
- Death
- The character status dialog (`1000:2FD4`, verb 8 on class-7 objects; `tools/statuspanel.py`, `engine/message.c`; pixel-exact against the emulator except the random edge tiles)
- Level progression (op `33h` = 51)
- Taking things (emulator, `1000:2EA9` called on the level-1 sword, helmet and breastplate things)
- `rec[10]` to `0xFF` ("carried")
- Chest tails (verb 3, `1000:31A3`; sprite header `0x16 == 8`: CHEST, STRONG BOX, CASKET; `0x16 == 8` jars use another path)
- Where the level-1 lake creature comes from
- Op 36 (spawn) as seen in the emulator, and the chase behaviour
- Creature sheets
- Active creatures (read from `1000:6041`; movement approximated in `src/main.c` `creatures_step`)
- The engine's movement routine `1000:6F62` (read from the code; creature step in `src/main.c`)
- Scripted creature movement (mode 0, `1000:6F62` `loc_17149` …; ported in `creatures_step`)
- Creature hits (`1000:6F62` → `116E0` → `172EC`)
- Ops `3Eh/3Fh/40h` (62/63/64) are not raw memory writes (corrects the earlier note)
- The game clock and the day counter (timer interrupt `1000:14F0…1549`)
- Sound: the PC-speaker driver — *verified* (`tools/beeper.py`, `tools/verify_beeper.py`, `src/engine/beeper.c`)
- The rucksack (observed in the emulator; ported)
- Look / describe (op 48h = 72)
- The map (F1, `1000:369F`; drawing `3806`, `38C3`, `389F`; emulator screen compared)
- Diagonals
- Jumping (emulator trace, level 1 room 0)
- The lake creature (level 1 thing 467) — why it surfaces and sinks
- Ops 08h and 1Bh (read from the code)
- More ops read from the code (all in the port; ops 22h, 49h mirrored in `tools/acl_vm.py`)
- Animation command F7 n = run the thing's TAD action routine n (verified on the lake creature)
- Services used by the TAD routines (from the service table `ss:[ds:B6]`, 16-bit words in order: `410E 7E1C 407F 81C8 8706 D8BC 890A D912 7F3C 7DE1 D900 369F 8275 40A9 31A3 78DE 8840 6A44 164E 3DB8 799F 7903 5F18 9D8E 1043 D4C6 2FE3 D8BB 5DAA 1187`; the index in the hook is twice the position)
- Throwing (read from the code; ported)
- Held items that act: spells and weapons (read from the code; ported, see the paragraphs below)
- Spell and potion name tables, and how `84D9` composes a description (verified against the original routine; ported)
- More spell routines (disassembled from `ROUTINE1.TAD`, level 1 table `081E`)
- Drinking (verb 9, `1000:32B5`; potions are the carried things whose header byte 17h is 2)
- The effect timers (`1000:11EB5`, `8275`; ported as `acl_set_timer` / `tad_timer_unit`)
- Shield potions (TAD `0765 / 0774 / 077A / 077F` = potions 6 SHOT, 8 FIRE, 11 ICE, 12 MAGIC SHIELD; 14 TOTAL SHIELD `07AF` sets `ds:6AB = 7Fh`)
- Player shots (`1000:69CF` weapons, `16896` mode 0 spells, `16A37` launch, `6F62` hits; ported as `fire_weapon`, `launch_shot`, `shot_hits`)
- Spells cast on an object (A[7] = 2, `1000:169AB`; TAD entries of the spell table `081E`, level 1)
- More potions and the timer routines of level 1's table (`0C3F`)
- The spell / potion name tables are in `LEVn.MP1` (both games), and the Payoff renumbered its spells
- Payoff routines for its new spells and potions (`ROUTINEn.PAK`, level 1 table `06B5` / `085D`)
- CONFUSION and TURN MONSTER (effect timers 25h / 26h) act inside the creature behaviour routines
- Saving a position (`1000:BF92`; the keys are F5 / F6, the manual's S / L are not in the code)
- Placing a dropped or thrown thing, all eight facings (`1000:6C72`, table `cs:6C54`; verified by calling the original with a fake header of box 3 × 2 and the player record `x1, y1, x0, y0 = 24, 23, 18, 17`)
- The whole-rucksack grid navigation (`1000:125CF`, transcribed; ported as `grid_begin` / `grid_nav` / `grid_draw`)
- The Roland driver `ROLAND.BIN` (Cadaver 2, RNC-packed on disk, 3258 bytes unpacked; `tools/roland_midi.py`)
- Sound Blaster digital samples (`SBLASTER.BIN`, 11136 bytes; `beeper.c`, `cadaver_sound sblaster digital:N`)
- Event 25 (19h) of a cast spell item
- Three more ACL ops (read from `CAD.EXE`)
- Room zones and event 10 (`1000:11EB5` → `2022 … 2108`; ported as `roomview_zone_events`)
- The carried-thing verb list and the verbs 12, 15, 16 (`1000:12C72`, `3381`, `344D`, `2E40`; corrects the earlier reading of verb 15)
- Event 4: a thing comes to rest on another (`1000:16B02`, read from the code; ported in `fly_step`, not verified in play)
- The rucksack strip and the look descriptions (emulator with two carried things, `1000:125CF`, `84D9`, `30FE`)

</details>


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

**The ACL interpreter (found, structure verified by reading the code; opcodes not decoded yet).** The game's logic is data-driven by small
scripts stored in the *tail of the thing records* (and of the room records). `CAD.EXE sub_10247` calls `1000:7634` once per game step; it drains the
**event queue** (`ds:5A` = queue pointer, `ds:226` = count, entries of 3 words: `event type` (bit 15 of the high byte = extra argument word follows),
`object` = the position record that the event concerns, `argument`). For an event of type `cl` the object's **handler list** is searched: at
`[object+10h]` (or `[object+20h]` when the event word has bit 14 set) a list of `[object+0Bh]` (`+1Fh`) entries; each entry starts with a
condition byte (low 7 bits = event type to match, bit 7 = "keep the handler armed"). When the type matches, the *condition test* `table[0x76FC + 2·type]`
(29 event types: use, look, take, push/contact `9`, step-on, timer …) must pass; then the **op stream** is executed: `lodsb` opcode, `call [cs:0x7819 + 2·op]`, until opcode `0x17`
(end). Operands are read from the same stream (`lodsb`/`lodsw`, big-endian words). The handlers (`1000:78D5 … 826D`, ≥ 50 of them) do things like *set / clear thing flag bits*
(`and [obj+3], 7Fh` = "place the object", `or [obj+0Fh], 4`), print messages (strings = LNG ids), move objects, start sounds, change level (`mov [0708h], n` — the level number)
and queue further events. Things 704–709 of level 4 (the carpets over the pit) are made visible by such a script at start-up. A faithful game needs this VM (plus the
per-level `ROUTINEn.TAD` native code, which handles creatures and animation hooks).
**ACL virtual machine, first working subset (`tools/acl_vm.py`, verified).** The condition mechanism is a counter `ds:605`: every *test* opcode
increments it on success and zeroes it on failure; `IF` (`0Eh`) runs its block when it is non-zero (and clears it), `30h` is the negated IF, `3Ah/3Bh` compare it with a constant (so
a list of tests followed by `IF = k` means "all k tests passed"). Verified opcode effects (fuzzed against the original handlers, `tools/verify_acl_ops.py`, 24 ops × many random operands/pre-states, no mismatch):
`01` place (clear `rec[3]` bit 7; if the thing belongs to another room only set bit 3), `1A` hide, `11h/18h/12h` set / toggle / clear `rec[3]` bit 0, `14h/19h/15h` bit 1, `0Fh/10h/13h/1Eh` test bit 0/bit 1 (also of the current object), `1Fh/20h/21h` clear/set/toggle bit 0 of the current object, `36h/37h` set/clear flags2 (`rec[15]`) bit 2, `44h/43h` set/clear bit 6, `05` score, `26h/27h/28h` set / add / compare script variable (`ds:611+n`; modes `>`, `<`, `==`, `!=`), `2Bh` test the thing's room (`FEh` = current), `2Fh` test that the thing exists,
`4Fh/50h/51h` the register `ds:704` (random(a,b) / test / copy from variable), `1Dh` disarm the running handler. **Replaying the room-start event (type 14) of every room that has one in the original and in the VM gives identical thing flags and
variables in 65 of 66 rooms of levels 1–5** (`tools/verify_acl_rooms.py`; the original's random draws are fed to the VM, the remaining room uses animation opcodes 3/74 which are not modelled). Level 4 room 0's start script
(`01 02C0 … 01 02C5`) makes the six carpets over the pit visible, as in the original's first screen. Unmodelled opcodes (animation 3/4/74/75, spawn `24h`, moves, sounds, level change, energy …) are skipped and listed in `VM.unsupported`.
**Script format (parsed and checked on 2453 records of all levels, `tools/acl.py`).** A handler list entry is
`[len][type | 80h keep-armed][condition operands][ops …][17h][pad]` (`len` counts itself, is always even: one padding byte when the entry would be odd).
Things: list at record offset 16, count = byte 11 (this byte of the 16-byte position record is the handler count; byte 10 the room); rooms: list at `+0x20`, count at `+0x1F`
(room events carry bit 14 in the event word). Operand lengths of all 94 opcodes were measured by running every handler of the table `CS:7819` in the emulator
(`tools/acl.py OPLEN`; a few depend on the data: `3E/3F/40` = *set variable* with a 16-bit address whose bit 15 selects a byte value, the IF opcodes `0E, 30h, 3Ah, 3Bh` nest blocks
ended by `16h`, optional ELSE `0Fh len …`). Event condition operand lengths (29 types): 1→1, 4→2, 9→2, 10→2, 12→1, 13→1, 15→1, 17→1, 18→2, 19→1, 20→1, 24→1, 26→2, others 0.
With these rules 2391 of 2453 handler-bearing records parse to exactly their length (the rest are mostly the truncated Payoff TATA/2 dump).
Semantics read so far: `01` place a thing (clears its hidden bit 0x80 — this is how the level-4 carpets appear), `1A` hide it, `05` add to the score, `1C` print a message, `25h` change room
(room, landing offsets, z → `room_enter`), `27h` add to a script variable (`ds:611+n`), `28h` compare a variable → condition flag `ds:605`, `0Eh` IF on that flag, `1Eh/20h` test/set bit 0 of the
current object's flags; the others (`48h` look/describe, `2Dh` change energy, …) are listed in `tools/acl.py`. Event queue entries use the position record of the object; the level-start event (type 14 on the room record) runs the
room's start-up ops.
**Event sources and condition tests (read from the code and the emulator).** The dispatcher `1000:7634` takes `(type, object, argument)` triples; bit 14 of the type word = the room record's
list, bit 15 = an extra argument word follows. The condition table `CS:76FC` (operands are read from the entry after the type byte; a handler runs only when the test returns 0): types
0 3 5 6 7 11 14 21 22 25 27 28 always match; 2 never; 4 operand word == argument; 9 18 26 operand word == argument or negative (wildcard); 10 operand byte == current room and next byte == argument;
12 13 19 20 24 operand byte == argument's low byte; 15 17 same, and on a match the current object `ds:9C` becomes the extra argument; 1 compares a per-thing attribute byte;
16 clears flag `0x20` of the object; 23 adds the thing's value to the score (`ds:24C`). Enqueue sites (inline `inc ds:226`): 9 = contact (collision test `116E0`, `165D2`, `16B02`, `122C4`, …),
7/8 = step events, 10/16/18/… = thing events, and for rooms (bit 14): **28** (`0x401C`) the first time a room is entered (flag `0x40` of the room record byte `+0x17`), **6** (`0x4006`) on every entry (`room_enter`, `1000:DA60`),
**14** (`0x400E`) the room **timer**: `room_enter` loads `ds:6C1/6C2` from room record byte `+2`; `1000:11EB5` (once per game step) counts `ds:6C3` down from 17, and every 17 steps decrements `6C2`, at 0 it fires event 14 and reloads
(level 1 room 0: byte = 15 → every 510 ticks; level 4 room 0: 1 → every 34 ticks), **20** (`0x4014`) the one-shot timer set by op `2Ah` (arg, delay in the same units; `ds:6C5/6C4`), **19** (`0x4013`) queued by op `09` with a byte
argument, 15/17 (`0xC00F`, `0xC011`) from `12108`, 24 (`0x4018`). So "level start" scripts are the *first timer event* (14) or the entry events 28/6 — the earlier assumption that 14 is a one-time start event was only half right
(level 4 room 0's carpets appear when its timer first fires, 34 ticks after entering). Replaying the sequence 28, 6, 14, 14 through the original dispatcher and through `tools/acl_vm.py` (`fire_event`, with condition matching and the queue)
gives identical thing flags and variables in every room that has such handlers except two (one uses the unmodelled animation ops 3/74, one draws a different random number in the second timer firing, which the harness cannot reproduce).
**Touching objects (`1000:22C4`, verified by hooks in the emulator).** After the player's move test (`collision_test`) hits objects, the game calls `1000:22C4` with the list of hit objects: the first object whose
sprite-block header byte 12 does not have bit `0x40` ("ignore") is the *touched* object; its **name** — the string whose id is the **big-endian word at header bytes 10–11 of its sprite block** (header word 5, e.g. block 0x36 → 215 SILVER COIN),
cut to 12 characters — is drawn centred in the box x=229, y=172, width 84 (the line above the room name; it stays until another object replaces it), a verb list is built for the six panel icons (look "?", take …, driven by the definition bytes `+0x0C/+0x16/+0x17`
and the table at `ds:FA36`; the menu itself is `1000:2E2D`, handler table `1000:2E0B`, 17 entries), and unless the action key (End) is held, events **9** (argument 0) and **7** are queued for the touched thing
(`1000:6441` lets each ordered pair through once per step). Thing events are matched with the same condition table as room events (`tools/acl_vm.py` `cond_match`, `src/engine/acl.c`); replaying 7 and 9 on every thing that has such handlers gives identical
flags/variables in the original and the VM in all five levels (`tools/verify_acl_things.py`; one level-5 thing differs because ops 62–64 write to raw RAM addresses). Ops `31h` (49) and `5Ah` (90) start **native-routine countdown timers**
(`ds:62B+i` = delay in units of 17 steps, count `ds:62A`, reload copy at `+0x34`); when one expires `1000:11EB5` far-calls the level's `ROUTINEn.TAD` code (via `ds:788`) with a routine id — creature behaviour lives there.
**Message scrolls (op `1Ch` = say; `tools/message.py`, `src/engine/message.c`, verified against the emulator's screen).** The text (a string id, big-endian word operand) is copied into a buffer **with a trailing space**
(it counts for width and centring), wrapped to `w` columns (`1000:5DCB`: `w` starts at 12 and grows while the text is longer than `4w + (w+1) + (w+2) + …`; a word is added to the line while the line stays ≤ `w`; a longer word is cut at `w`)
and shown in one of four slots (`1000:3555`; slot *s* at x = 32 + 16 s, y = 20 + 16 s; the same string already on screen only restarts its timer; the timer is 75 game steps = 150 ticks, measured). The box (`1000:8A87`) is built from opaque 8×8 tiles
of **ANDATA block 2** (18 tiles × 32 bytes, 4 plane bytes per row): top rows `0 1 2` / `5 E C`, `fill` = ⌊6w/8⌋ columns of tile `D`, middle rows (⌊6·lines/8⌋ of them) with a random left tile from `6 7 7 8 8 8 8 8` and right tile
from `B A A 9 9 9 9 9`, bottom rows `10 F 11` / `3 1 4`; the lines follow in the **block 3 font** (5 rows × 4 plane bytes = 20 bytes per glyph, same glyph numbers as the status font, advance 6, line pitch 6, first line at y + 13,
each line centred in the 8·(fill+2) pixel width). Everything matches the emulator's pixels except the random edge tiles. The text appears over ~4 ticks (typewriter), not modelled.
**Verb icons (`tools/verbs.py`, `src/engine/panel.c`, pixel-exact against the emulator).** While an object is touched the six inventory slots of the panel show its *verbs*: **ANDATA block 0** holds 17 icons of 16 × 14 pixels (8 bytes per row =
4 big-endian plane words, colour 0 transparent; icon 0 = the empty slot). `1000:22C4` builds the list (at most six, always ending with verb 6) from the sprite-block header (file order: byte 12 flags, `0x16`, `0x17` class, `0x1D`) and the thing's `rec[15]`:
verb 2 if `h[12] & 2` and `!(rec15 & 4)`; the class verb `ds:FA36[h[0x17]]` (`04 09 FF 07 03 08 FF FF FF` for classes 1…9, negative = none) if `h[0x17] != 0` and `!(rec15 & 0x40)`; verb 14 (`0Eh`) if `h[0x1D] == 0x12`;
verb 10 (`0Ah`, "?") if `h[12] & 1`, `!(rec15 & 4)` and `!(ds:703 & 0x20)`; verb 11 (`0Bh`) unless `h[0x17] == 6` or `h[0x16] & 0x80`; then verb 6. Slot positions (`ds:FD8A`): (9,145) (36,152) (9,169) (36,177) (63,175) (90,177).
The icons stay until the next touch. The verb handlers (`1000:2E0B`, 17 entries) turn a chosen verb into events — 11: event 16 (look/script) when `rec[3] & 0x20`, 4 and 7: event 5, 2: event 0, 1: event 21 + pick-up into the inventory, 16: event 1 — and
**the verb menu** (`1000:2A33`, read from the code; highlight frame pixel-exact): End pressed while touching an object (`1000:244C`) waits for the keys to be released, then Left/Right move a highlight over the verbs (wrapping; one move per 13 steps while held;
Up/Down switch between two pages of six), End chooses (Space = verb 6), and the chosen verb is dispatched through the table at `1000:2E0B`. The highlight is a 18 × 16 frame around the slot (outer ring colour `A`, inner ring `E`, icon untouched). `src/main.c` implements the menu with
the verbs 11, 4, 7, 2 (events 16/5/0); the rest (take, drop, hold, use, drink, put into) came later — see the rucksack paragraphs.
**Energy (`ds:23A`, maximum `ds:700`).** The maximum — and the starting value — is the big-endian word at **offset 60 of the unpacked `LEVn.MP1`** (Cadaver levels 1–5: 100, 200, 350, 500, 1000; read by the loader `1000:B568` together with five more
words, one of them the random seed `ds:242`). The panel bar (`1000:D8C3`/`D8DE`): the 22×6 box at (277,143) is cleared to colour 0, then 22·energy/max pixels are filled with colour 4. Op `2Dh` (45) adds a signed word to the energy
(`1000:81C8`: clamp to the maximum, ≤ 0 = death, energy 0; verified against the original for 60 random cases).
**`ROUTINEn.TAD` decoded enough to run it (`tools/x86.py`, `tools/verify_x86.py`, `src/engine/x86.c`, `src/engine/tad.c`).** The unpacked blob is a code segment (`ds:788` = far pointer to its thunk at `+0x27`, `ds:71A/71C` = the data table at `+0x2C` returned by the init stub).
The table holds five near pointers (level 1: `7C 6DC 81E C3F D6D`): `+6` (`C3F`) is the **timer routine table** (words, 9 entries in level 1: `CA7 CBD CE3 …`, zero-terminated), indexed by the countdown number of ops `31h`/`5Ah` — `1000:11EF8` calls the routine
with `ds:78C` = its address; `+2`/`+4` are jump tables of the level's script helpers; and from word index 18 on the table holds 4-byte entries `(pointer, 0)` whose **pointer + 2** is the entry of a *behaviour* routine (level 1: `81 15A 194 1A1`, called through
`1000:601E` with an index in AX, and from `1000:5FBF`/`332F`/`698C` … by other event paths). The routines are plain 8086 code (≈ 60 opcodes: mov/add/sub/cmp/and/or/xor/test/inc/dec/neg/shifts/jcc/call/ret/push/pop/pushf/popf/lods…; `ss:` is used for the
service table) that work on the game's data segment directly: script globals (`ds:6A9` = operand of op `5Ah`, `ds:6AB` flag bits, `ds:59A` = message number, `ds:622` …), object position records through `bp`/`si`/`di` (`[bp+0Fh]`, `[bp+0Ah]`, `[bp+0Eh]`), and the event queue
(`ds:84` = write pointer, 6-byte entries `type, object/argument, argument`; `06BC` in the blob enqueues **event 8** = *show string id `arg` as a message* — `1000:7807` → `13543` → the scroll). **Engine services** are called as
`mov [792h], <ss:[ds:B6 + 2k]> ; call far [78Eh]` (`1000:6019` = `call word [792]`): the table at `ss:[ds:B6]` holds near addresses of CAD.EXE routines — k=0 `410E` (record by type/id), 2 `7E1C`, 4 `407F` (thing record), 6 `81C8` (change energy), 8 `8706`, 0Ah `D8BC`,
0Ch `890A`, 0Eh `D912`, 10h `7F3C` (change room), 12h `7DE1`, 14h `D900`, 16h `369F`, 18h `8275` (start countdown, = op 31h), 1Ah `40A9`, 1Ch `31A3` (verb 3), 1Eh `78DE`, 20h `8840` (random(ax..dx)), 22h `6A44`, 24h `164E` (sound), 26h `3DB8`,
28h `799F`, 2Ah `7903` (place, = op 01), 2Ch `5F18`, 2Eh `9D8E` (wait for key release), 30h `1043`, 32h `D4C6` (palette), 34h `2FE3`, 36h `D8BB`, 38h `5DAA`, 3Ah `1187`. Level 1 uses only random and sound; levels 2–4 add the record lookups, energy, `890A/D912/D900/6A44/31A3`; level 5 uses all of them.
`tools/x86.py` (Python) and `src/engine/x86.c` (C) interpret the code; **both agree with Unicorn running the original bytes** on every timer and behaviour routine of the five levels with random start states (identical data-segment bytes, registers and service-call traces; `tools/verify_x86.py`), and the C port equals the
Python one on the same scenarios (test vectors for all 5 TADs). `src/engine/tad.c` runs the timer routines in the game: every 17 steps `1000:11EB5` counts the active countdowns (`vars[0x19]` = number, `vars[0x1A+i]` = remaining units, reload copy at `+0x34`), a routine that reaches 0 is run with
`ax` = its index; implemented services: energy, random, sound; events queued by the routine are applied (event 8 → message). Cadaver 2 ships the same code for its levels 1–5 as `ROUTINEn.PAK` (`mjm2`-packed; loaded the same way); the Payoff's new `TATA` levels use another init stub (thunk `+0x2E`, data table `+0x80`, and the first call patches a byte of the engine) but the same table layout; the loader reads the table offset from the stub (`mov ax,imm` at `+0x23`) and their routines run on the interpreter too (vectors included). Not done yet: the behaviour routines (need object records and the movement services) and the other services.
**Behaviour routines — how the engine reaches them (read from `1000:5F4C…601E` and the emulator; ported in `creatures_step`).** `sub_15F4C` (once per game step) has two halves. (1) A queue at `ds:27E` (count `ds:27C`) of 4-byte entries `(type, index, word)` — filled by ops — calls
the TAD routine `word at [table+2+4·(type+9)] + index·2` with `bp` = the thing's tail. (2) The list at `ds:B8` (count + 6 thing ids; added by op `07` through `1000:5F18`, removed by ops `08`/`4Bh` through `1000:5F2F`) holds the things that are *active*: for each, `di` = its position record
(`1000:410E`), `bx = di + rec[12]` = the **tail** of the thing record (file offset `rec[12]`; e.g. thing 60, the player, has tail `07 01 1E 00`), and tail byte 1 = the **behaviour number** *b*; routine *b* + 9 of the TAD table (entry = word at `0x2C + 4·(b+9)` + 2) is called with `bp` = tail,
`di` = record, `si`/`es` = the engine's per-object state. In level 1 room 0 the list holds one thing, 900 (runtime-created, no file record), with behaviour 4 (`TAD:01A1`, a bare `ret`). The routines read/write the tail (`[bp+8]` countdown, `[bp+9]` reload, `[bp+0Ah]`
pointer into script bytes in the blob — `0x80` end, `0x81` restart — `[bp+0Eh]`, `[bp+0Fh]` flags) and the object entries (`[si]`, `[si+0Ch]`, `[si+0Dh]`, `[di+3]`, `[di+0Fh]`), and call services (random `20h`, sound `24h`, record lookups `00`/`04`, move helpers). Running them
needs the engine's RAM object entries (50-byte entries at `ds:2A`, expanded thing records) mirrored in the interpreter's data image — not done. Op `33h` (51) changes the level — see "Level progression" below.
**More ops.** `06` (6) adds its byte to the money counter `ds:248` (32 bit) and `byte >> 2` to the score `ds:24C` (verified against the original); `05` adds to the score; `02` (2) removes the current object (`1000:78DE`; modelled as flag `0x80`). Verb 2 raises event 0, whose handlers
are mostly `06 n; 02`: **verb 2 collects valuables** — verified in the emulator: choosing it on the level 1 silver coin raised money `ds:248` 0 → 7 and the score 0 → 1 and removed the coin; the C walk scene reproduces this (money 7, coin removed).
**Choosing verb 2 in the original (emulator trace, level 1 silver coin).** Open the menu (End tap while touching), End again: `1000:2BCB` → `2E2D` → `2EA9` run; the handler compares the carry capacity `ds:6AD` with the carried count `ds:5A0`
(equal → message `0x35`), otherwise calls `3E6D` (detach the thing from its container/room: `rec[10]` `0xFF`/`0xFE` are the special owners "carried"/"nowhere", container counters `[di+1D]`, `[di+1B]`, `[di+1C]` are decremented) and `3F35` (add to the carried list), then queues **event 0** for the thing. The
label and the verb icons are cleared; the thing's position-record slot is reused by the next thing (the record at `ds:8C42` held thing 412 before and thing 413 after), so the engine keeps its thing records in a packed array that shifts when objects are taken or created. How carried
items are drawn in the six slots (and what `ds:5A0`/`6AD` hold at start: `0020`/`00`) is not decoded.
**Verb 14 (`0Eh`, shown for sprites with header byte `0x1D == 0x12`: leg of meat, loaf, chicken).** `1000:3483`: class `0x17 == 7` → event 5; otherwise the thing is simply queued for deletion (`1000:62A9` appends its id to the list at `ds:2F6`) and sound `0Ah` plays. Calling it on the three
food things in the emulator changed no tracked state (energy stays), so eating gives no energy through this verb. `src/main.c` removes the thing.
**Ops 41 and 85 (verified against the original, 60 random cases).** `29h` (41) moves a thing: `[id16] room x y z` (room `FEh` = the current one): the position bytes `rec[0..2]` are set, `rec[10]` = the room, and when the room differs from the current one `rec[3] |= 08h`
(the thing then leaves/enters the room on the next visit; for the current room the original runs a collision-aware placement `1000:1CC1` that can adjust the position — not modelled). `55h` (85) adds a signed word to the score (negative: subtracts the raw word, clamped at 0).
In the C walk scene things whose `rec[10]` differs from the room list's room are skipped and things moved into the room are appended (`roomobjs_build_ex`).
**Op 25h (37, change room), decoded and verified.** `[room dx dy z]` (`1000:7F3C`): the player record is zeroed, `ds:232` = room, the landing is **(dx + room_rec[1], dy + room_rec[3], z)** (`ds:592/594/596`; checked on 25 random rooms against the original), then `room_enter` runs and the player is placed at the landing with `z`
increased until the spot is free (limit `2Ch`). Used by type-5 handlers of ladders/trapdoors (level 1: things 2, 56, 86, 129, 162). The walk scene applies it (`walker_teleport`).
**Death.** When `1000:81C8` brings the energy to 0 it plays sound `1Ch`, empties the bar, waits for a key (≤ 0x400 polls), copies the player statistics (`ds:248/24A/24C/24E/23A`, the day counter `ds:5B2…5B8`) into a record at segment `0400h` (this is the "THE GODS DEMAND A PRICE TO SAVE YOUR POSITION OF …" text, string 60) and jumps back to the game start `1000:0071`.
The walk scene shows the message (string 60) for 150 steps and restarts the level; the saved-record mechanism is not modelled. `CAD_ENERGY=n` starts a level with that energy (debugging).
**The character status dialog (`1000:2FD4`, verb 8 on class-7 objects; `tools/statuspanel.py`, `engine/message.c`; pixel-exact against the emulator except the random edge tiles).** A message-scroll box at (64,10) with 16 fill columns and 4 middle rows (x = `0x44` rounded down to a tile column),
text in the small block-3 font (the routine is entered at `1000:8776` without resetting the font chosen by the previous `8714` call: 5 rows, advance 6, line pitch 6). Labels: LNG string 13 (`HEALTH CR GOLD CR XP CR LEVEL CR COMPLETE CR LEVEL`) at (78,23); values at x = 134:
`energy/max`, **GOLD = money `ds:248`, XP = score `ds:24C`**, level number (`ds:708 + 1`), **COMPLETE** = `((0x6400 / (ds:6FA − TAD word at +0x36)) × visited_rooms(ds:582)) >> 8` percent (`ds:6FA` = third word at `LEVn.MP1` offset 60, 72 in level 1), and the **rank title**: the 20-entry table at
the end of `LEVn.MP1` (120 bytes: big-endian string id, score high word, score low word; `PEASANT 1000, NOVICE 2000, INITIATE 3000, ROGUE 4000, TRACKER 5000, … GOD 60000`) — the first entry whose threshold is ≥ the score. In the SDL walk scene `I` toggles it (`--sim info:1`).
**Level progression (op `33h` = 51).** Its operand is **one byte** (the earlier operand length 0 was wrong: the handler never returned in the length probe) — the current level index, `ds:708 = operand + 1` (0-based index of the next level, so the next level number = operand + 2), then the loader restarts (`1000:0097`).
It occurs exactly once per level: level 1 thing 84 (event 5, operand 0), level 2 room 88 (event 15, 1), level 3 room 80 (event 15, 2), level 4 room 35 (event 15, 3); level 5 has none (the game ends). All 2453 handler records still parse. The walk scene loads the next level and carries money, score and energy over.
**Taking things (emulator, `1000:2EA9` called on the level-1 sword, helmet and breastplate things).** Verb 2 is offered for 35 of the level-1 sprite blocks (swords, armour, helms, bags …; coins are the same verb with an event-0 handler that adds money and deletes the thing). The handler sets the thing's
**`rec[10]` to `0xFF` ("carried")** and queues event 0 (`rec[3]`, position and the other bytes stay); the carried things are exactly those with `rec[10] == 0xFF` (`0xFE` = "nowhere"). The capacity test compares `ds:6AD` (carried count, 0 at start) with the word at `ds:5A0` (`0x0020` at start).
A scripted run through the real game (hold Right to the coin, take it, walk on, take the **pickaxe** at (73,23) of level 1 room 0 with End, End) gave `ds:6AD = 1` — so the pickaxe is carried — but the six panel slots stay **empty**, the touched-object label disappears, and
`ds:27A` (the selected item) stays 0; Insert/Delete (`0x52/0x53`, input bits 05/06) are *diagonal moves*, not inventory keys. So carried things are not shown as icons in the slots during normal play; the inventory is presumably reached through the containers' verb 3 (`1000:31A3`, `THERE IS NO ROOM TO PUT THIS OBJECT IN`, string 27) —
not decoded — **verb 3 turns out to be "open/close a chest"** (`1000:31A3`, sprite class `0x17 == 5`/header `0x16 == 8`: key check through `86D1`, trap damage from `[bx+5]`, and the chest's content thing is *placed into the room* next to it with the op-41 routine `1000:8024`), not an inventory view. Holding End alone with a carried pickaxe
also leaves the six slots empty. A water creature (an orange head that surfaces and sinks in the level-1 lake) shows up in the same frames — creature animation driven by the TAD behaviour routines. (Calling the handler by hand corrupts the emulated game: invalid read at `1000:AB5B`.) Not ported.
**Chest tails (verb 3, `1000:31A3`; sprite header `0x16 == 8`: CHEST, STRONG BOX, CASKET; `0x16 == 8` jars use another path).** The tail of the thing record (file offset `rec[12]`) is: `[0..1]` big-endian id of the **content** thing (0 = empty; e.g. level 1 thing 83 holds thing 145, thing 224 holds 143), `[2..3]` big-endian id of the **key** thing
that must be carried (0 = unlocked; `FFFF` = never), `[4]` flags (bit 0 open, 1 = the key is consumed, 2 = the chest is removed when opened, 3 = animation step `1000:79C2` after toggling; typical `28h`, `2Ah`, `38h`), `[5]` trap damage. Opening: damage > 0 → energy −= damage and message `32h`; a key id → look through the carried
things (`1000:86D1`; missing → no opening), then the key id is cleared (and the key deleted when flag 1 is set); the content thing is **placed into the room at the chest's position** with the op-41 routine and the content id is cleared; the open bit toggles, sound `38h`, event 5 is queued. Ported in `src/main.c` (`acl_chest_toggle`, tested on level 1 chests 83 and 224); the collision-aware placement `1000:1CC1` is approximated by raising the content's z from the chest's until its box is free.
**Where the level-1 lake creature comes from.** Room 0 has a keep-armed timer handler (event 14, every 510 ticks) whose only op is **`24h` (36, spawn)** `01D3 44 42 02 FE` — thing **467** (a multi-frame sprite sheet, block 116, 4328 bytes; behaviour number 4) at x = 0x44, y = 0x42, z = 2 (room `FE` = the current one). Thing 467's own handlers: event 9 with argument 0000 (contact with the player, thing 0)
→ op `2Dh` energy −2 and op `32h` (50, queues event 23); event 23 → ops 8 (deactivate its behaviour) and 2 (remove it); event 13 → op 50. The room also has event-15 handlers (arguments 1, 2, 3) doing energy −1. The **player is thing 0** (sprite block 8, 14 712 bytes) and the head sprites 7 and 8 are plain static single-frame sprites (block 14, flag `0x40`).
So creatures = spawned things (op 36, 153 uses) + a TAD behaviour routine + contact events; the engine-side spawn (record allocation, active list `ds:B8`, entry creation) is the part that would have to be mirrored.
**Op 36 (spawn) as seen in the emulator, and the chase behaviour.** Running `24h` with `01D3 44 42 02 FE` allocates a **copy of the thing record** at the free pointer (`ds:24` / `ds:75A`, record bytes copied with the words swapped, size 0x4E written to `ds:76C` and a table of size words at `ds:F472…`) and appends a **spawn request** at `ds:346` (`count`,
`size 4Eh`, record pointer, `x y z room`, `0Fh`); the engine turns it into an object entry on a later step. Several spawns of one thing id give several instances. **Behaviour routine numbering (corrected):** behaviour *b* uses table index *b* + 9, i.e. the word at `table + 4·(b+9)` + 2 — level 1: *b* = 0 none (`FFFF`), 1 `0081`, 2 `015A`, 3 `0194`, 4 `01A1` (a bare `ret`), 5 **`0278`**, 6 `0217`, …; so the lake creature's behaviour 4 does nothing by itself and the earlier remark that thing 900 runs `0278` was wrong (`ax` = 13 = `01A1`). Behaviour 5 (`TAD:0278` = `call 03A0; call 0407`) is a **chaser**: `03A0` sets `si = di + [di+0Dh]` (movement state) and `bx = di + [di+0Eh]` (tail); `0407` reads the player's record
(`ds:5C`: x, y) and the creature's (`di`: x, y) and calls `055E`, which computes an **octant** 0…7 (a dead zone of ±2 cells gives 0 = stand still; with `ds:650`/`651`/`6D7` set the direction is randomised or mirrored through the tables `E11…`/`E15…`), maps it through the word table `DD2` (pairs `(vx, vy)` of `00`, `7F`, `81` = 0, +127, −127; `DE8` for the other variant) and stores the result in `[si+3]`, `[si+4]` —
the engine's per-step movement of an object entry then applies these velocities. So the lake creature simply swims towards the player and bites on contact (event 9). `src/engine/tad.c` `tad_run_behaviour` runs the real routine on the interpreter with a synthetic player/creature record (test: velocities have opposite signs for opposite directions and are 0 inside the dead zone). Porting needs: the spawn list, the velocity application, the block-116 frame selection and the contact events.
**Creature sheets.** The 9–11 animated blocks per level that did not parse (level 1: 95 and 112–119; level 2: 82, 96, 99, 132–139; …) are **animated blocks with 4 extra bytes after the last frame** (block 116: 10 frames of 32 × 24/32/10/14 pixels, script of 34 words, 8-byte table entries `(ram offset, dx, dy, 0)`); `tools/objsprites.py` and `src/engine/objsprites.c` accept the 4-byte trailer now
(the trailer's meaning — speed / hit points? — is not decoded). The animation script only moves the *drawing offsets* (`dy` = −5 … 0 per frame), so the lake creature (thing 467) bobs in place; spawn (op 36 `[id16] x y z room`) is modelled here as placing the thing at that position in the room — in the walk scene the head surfaces in the lake after the first room-timer period (700 ticks), where the original shows it.
**Active creatures (read from `1000:6041`; movement approximated in `src/main.c` `creatures_step`).** At room entry the engine builds the active list (`ds:B8`, ≤ 6): every object entry whose thing is not hidden (`rec[3] & 0x80` clear), whose tail byte 5 has bit 7 clear and whose sprite header byte `0x16` is **3** (creature class) — no script op is involved (ops 8/75 only remove things again; event 23 handlers of creatures do `08 FFFF; 02`).
Each step the TAD routine of the thing's behaviour number (`tail[1]`, table index + 9) runs; the C walk scene runs the real routine (`tad_run_behaviour`), takes the sign of the velocity it computes and applies it as the engine does (below), and overlap with the player raises event 9 (argument 0) on the creature. Op `32h` (50) queues event 23 for a thing (verified with the oracle); queued thing events run after the current handlers.
Level 1 room 69 is a creature room (things 127, 443–449, 465–467, 485 with behaviours 1–9): in the walk scene several of them walk to the player and the energy bar drops on contact.
**The engine's movement routine `1000:6F62` (read from the code; creature step in `src/main.c`).** A thing record has three sub-structures, at `rec[12]` (A: `[0]` hit-points-like value, `[1]` behaviour number, `[5]` bit 7 = inactive), `rec[13]` (**B, the movement state**) and `rec[14]` (C, TAD scratch). B: `[0]` mode (**3 = velocity driven**, 0 = scripted table
movement through `[+1]` stepping 4-byte entries `(dx,dy,dz …)` — not decoded, such creatures stand still here), `[3..5]` signed step counters per axis (the behaviour routine writes ±127), `[6..8]` step size per axis (1 for all level-1 creatures), `[9]` **speed gate**: low nibble counts down each game step, when it is 0 it reloads
`B[9] |= B[9] >> 4` and the creature moves (`0x11` = every 2nd step, `0x22` = every 3rd, `0` = every step); a moving step adds ± the step size to the position (`[di+0Ah..0Ch]`) and decrements the counter towards 0; the new position then goes through the collision test. Level-1 creatures: 443/446/467/485 `03 …01 01 01 11`, 445 `…22` (slower), 127 `…00` (fast).
**Scripted creature movement (mode 0, `1000:6F62` `loc_17149` …; ported in `creatures_step`).** After the 14 bytes of B comes a table of entries read at `B + 14 + B[1]`: **`00 cx cy cz`** — load the three step counters (signed; the creature then walks that many steps per axis at the gated speed, `B[6..8]` units per step) and advance 4 bytes; **`01 d`** — jump by the signed byte `d` (relative to the entry; loops); **`02 n`** — wait `n` game steps (mode 2, back to mode 0 afterwards); `03` stop (mode 4); `04 n` queue event 12 with argument n for the creature
(the dispatch table at `cs:6F58` has exactly these four entries `718B`, `7196`, `71A8`, `71AE`; the words after it are not code). Level 1 room 69's things 444, 447–449 and 485 walk squares/zig-zags this way (e.g. thing 444: `00 46 00 00`, `00 00 00 32`, `00 ce 00 00`, `00 00 ba 00`, … `01 e0`). In the walk scene they now patrol, and with the player standing still several creatures reach him and the energy runs out within a few seconds (contact damage is applied every step; the original's creature/player bounce is not modelled).
**Creature hits (`1000:6F62` → `116E0` → `172EC`).** When a creature's move attempt collides with the player (thing 0) it stays where it is and the player's energy drops by the creature's **tail byte 1** (`A[1]`: the same number that selects its behaviour routine — 1…9 in level 1) through `81C8` (unless the armour flags `ds:6AB`/`6AC` absorb it); event 9 (argument 0) is raised for the creature as well. A creature that is
blocked by the room's walls or by objects simply does not move that step. `src/main.c` `creature_move` does this (the armour absorption is not modelled).
**Ops `3Eh/3Fh/40h` (62/63/64) are not raw memory writes (corrects the earlier note).** Their first word is a *pseudo address* (bit 15 = byte-sized value): `1000:80CC` maps `048E` → the current room `ds:232`, `0864` → `ds:5A0` (the carry capacity) and **anything else → the energy `ds:23A`**. `3E` sets, `3F` adds, `40` compares (value, then a mode byte `0 >`, `1 <`, `2 ==`, `3 !=` → condition counter).
Level 4 uses `3E 0864 0028` (capacity 40) and `40 0496 003C 01` (energy < 60), level 5 `40 0496 000A 00` (energy > 10) and `40 048E 001B 02` (room == 27). Implemented in `tools/acl_vm.py` and `src/engine/acl.c`.
**The game clock and the day counter (timer interrupt `1000:14F0…1549`).** The INT 8 handler counts down `ds:5B7` from `ds:5B8` (= `0x32` = 50 ticks, set at start `1000:0F17`) → one second (`ds:5B5` seconds, `0x3C` → `ds:5B4` minutes, `0x1E` = **30** → `ds:5B6 = 1` (the "day passed" flag), **`ds:5B2` (the day, shown as "DAY n+1") += 1**, `ds:5B3` += 1, minutes reset); `1000:11EBE` then queues message 28 (**"A DAY PASSES …"**) and clears the flag.
So a game day is **30 minutes of real time** (90 000 ticks). Nothing else reads the day counter. The walk scene counts the same ticks, updates "DAY n" and shows message 28. (`1000:0EF4…` also initialises `ds:5A0 = 0x20`, `ds:59C = 0x32`.)
**Sound: the PC-speaker driver — *verified* (`tools/beeper.py`, `tools/verify_beeper.py`, `src/engine/beeper.c`).** `BEEPER.BIN` (5468 bytes, "beeper music by Sound Images (c) 1991"; **the same file in both games**, Cadaver 2's copy is RNC-packed) is loaded by `1000:B3CE` at offset 0 of a segment and called through far calls: offset 0 = **tick** (called by the game's INT 8 handler `1000:14E5` before it does anything else, 50.16 Hz: the PIT is reprogrammed to `5CECh` and the BIOS tick is chained every 3.1 ticks), offset 4 = init (returns AL=1). The command area is at offsets 8..0Dh: `[8]` music request, `[9]` music playing, `[0Ah]` **effect request** (effect numbers 1..45h; a number above 45h switches the voices off), `[0Bh]` effect playing, `[0Ch]`/`[0Dh]` stop requests (honoured when equal to `[0Bh]`). The effect data (two voices, note table at `0831h`, effect table `0DCCh`) are interpreted by the tick, which alternates the two voices on successive ticks and programs the speaker each time: `out 61h` (clear bits 0–1), `out 43h,B6h`, `out 42h` (divisor low, high & 1Fh), `out 61h` (set bits 0–1). The sound that is on after a tick is therefore *one* square wave of 1193182 / divisor Hz held for 20 ms.
The game side: `1000:156C` (play, AL = sound id) translates the id through the table at `ds:4064` (Cadaver 2: `ds:406E`; 70 bytes, identical in both games: the effect number is mostly id + 1, entries with bit 7 are **looping** effects — ids 1, 11, 12, 15, 16, 29, 34, 41, 44, 51, 52, 54, 57, 62–64, 66, 67) and writes it to `[0Ah]`; a normal effect is dropped while `[0Ah]` is non-zero, a looping one always starts and is remembered in the flag table `ds:40F0` (`1000:1613` stop clears the flag of effect id+1, writes it to `[0Ch]` and `1000:1685` re-issues every other flagged sound; `1000:15EB` silences everything at a level change). `1000:164E` is the same with all registers saved (the form the TAD services and the animation scripts use). Observed sound ids: the walking animation script (`FC 00` inside the streams at `ds:FA60…`) = **0** every 6 steps; pushing an object = a looping sound by the object's sprite header byte `1Dh` (`ds:FED9`: 39h, 10h, 39h, 33h, 34h, …) started when the push begins and stopped when it ends or changes class; an active object (sprite header byte `16h` = 3) that is put in the room's slot table (`1000:51E2`, 50-byte slots at `ds:2A`) plays the sound in its header byte `1Eh` (FFh and 3Bh = none; level 1: 11, 29, 54, 59, 65–68) and stores it in the slot (`[si+22h]`), `1000:5114` stops it when the slot is freed; death 1Ch; chest 38h; eating 0Ah; TAD service `24h` = any id the routines choose. The intro tune is started through `[8]` (`INTRO.EXE` `10F78`: write 1 unless `[9] == 1`, i.e. it restarts when it ends; `10F93` writes FFh to stop). Traced in the emulator (sound hooks on `156C/164E/1613` while taking things, using the rucksack and jumping): the verb menu / rucksack plays **02h** for every highlight move (`2B60`) and **37h** for every chosen verb (`2BD3`); op 06 (money) plays **03h**, op 05 (score) **1Ah**; a jump plays **0Eh** at its start (`2DF1`) and **18h** when it ends (`0548`); all ported. The energy routine `1000:81C8` plays **1Bh** for damage (negative change), **19h** for healing and **1Ch** when the energy reaches 0 (read from the code; the port's `acl_energy_change` is used by ops 2Dh, chest traps, the TAD service and creature hits). Other call sites (an object hitting another one: `FEC6[class]` = 09h/0Dh/13h, the 32-step ambient 3Ch …) are not ported. `tools/verify_beeper.py` runs every effect 1..45h on both `tools/x86.py` and Unicorn and compares the divisor after each tick (identical); the C port is checked against 73 generated vectors (`tools/gen_beeper_vectors.py`) for both games' BIN files. **ADLIB.BIN** (5312 bytes, same entry layout: tick at 0, init at 4, command area at 8..0Ch; its init detects the OPL by writing register 4 = 60h / 80h, reading the status, writing 4 = 21h and expecting `status & E0h = C0h`) runs on the same interpreter: its port writes (388h index, 389h data) are identical on `tools/x86.py` and Unicorn for the first 60 ticks of the intro music. The port feeds them to a small OPL2 synthesiser (`engine/opl.c`: 9 two-operator voices, 4 waveforms, ADSR from the rate numbers, total level, multiples, feedback, FM / additive, tremolo / vibrato LFOs; approximate, not sample exact; no rhythm section) — the driver plays pitch mostly through block 0 with the operator multiples (fundamentals around 100 Hz for the intro tune). `--sound adlib|beeper` selects it, otherwise the first letter of `CADAVER.INI` (A = AdLib, anything else = Beeper); the play / stop protocol for the AdLib path writes the effect byte without the busy check and without re-issuing looping sounds (`1156C` branch for cards other than B / T). `cadaver_sound GAMEDIR beeper|adlib ID|music out.wav [seconds]` renders a sound to a WAV. **SBLASTER.BIN** (11 136 / 11 053 bytes) drives the Sound Blaster's FM chip at 228h / 229h (it probes 228h, then 388h) with the same OPL register protocol and additionally resets the DSP (226h → 22Ah = AAh) and calls `INT 21h` (get / set vector for its IRQ handler); with those answered (`x86.c` now has `INT` through a callback and RCL / RCR) its music goes through the same synthesiser (`--sound sblaster` or INI letter S; digital sound effects via DMA are not synthesised). `ROLAND.BIN` (MIDI, port 331h) is not used by the port.
`CAD.EXE` `1000:7AC6…` contains examples of the small handlers (clear/set bits of a record, `7B0F`: *change level n* …).

**The rucksack (observed in the emulator; ported).** The game step (`1000:0247`, key in `ds:716`) reacts to Space (39h) and Return (1Ch) when the carried count `ds:6AD` is non-zero: `1000:24C9` opens the rucksack view (`1415C`, `2C67`, `2A33` are its loop). Scripted run (`scan_queue` make/break codes: hold Right 100 ticks, tap End twice = take the coin, Right 60 ticks, tap End twice = take the pickaxe, `ds:6AD` = 1, tap Space):
the six verb slots of the left panel show the *carried item's* icons (4 for the pickaxe, the first highlighted), the right panel shows a **brown square window with a grey frame (≈ 40 × 34 px at x 265, y 142) holding the item's picture** and the item name below it (no day / room name); Return then performs the highlighted verb — for the pickaxe a message scroll appears with **the item's picture in a green-framed brown square at the top left** and the text *"THE PICKAXE SHOWS SIGNS OF RECENT USE OBVIOUSLY BY HACKING AT A WALL."*; Return again dismisses it and leaves the panel with empty slots and an empty picture window, Space returns to the normal view (day/room status).
The manual (`PAYOFF.DOC`) describes the whole interface: Space = small rucksack (last handled item; moving the highlight off the ends cycles through the items), Return = the whole rucksack (up to 32 items, 16 shown at once), `H` toggles holding an item, F1 = map (`1000:369F`), F2/F3 icon behaviour, F4 4/8-direction movement; the icons are search, take, drop, drink, read, hold, cast, switch, press, open, insert (item from the rucksack into the touched object), pull, eat and give. **Verified details.** The picture is **the thing's sprite block, frame 0, drawn at native size** (pixel-exact for the pickaxe: block 20h, 32 × 11 pixels) into a 32 × 32 brown interior at (267, 143) with a black outline (x 266/299, y 142/175); the frame's origin was at (+5, +10) inside it (vertical = centred; horizontal offset 5 for a 32-pixel-wide sprite, other widths unverified). The item name replaces the room name line (centred in x 202..310, y 186; no "DAY n"). The carried verb slots of the pickaxe are icons **11 (look), 13, 1 (drop), 6 (joystick)** against 2, 10, 11, 6 when it is touched on the floor — so take (2) becomes drop (1) and the 0Ah icon becomes 13; the position of the class verb is unknown.
`src/main.c` (`ruck_*`): carried things are those whose room byte is `FFh`; verb 2 (take) sets it (after event 0, whose handlers delete coins), Space/Return open the view at the last handled thing, Left/Right move the highlight and cycle through the items past the ends, End/Return runs the verb (11 = look, 1 = drop: the thing goes back into the room at the player's position, 6 = leave), Space leaves. Verb handlers read from the code (table `1000:2E0B`): 1 drop `2F0E` queues **event 21 (15h)** for the thing and places it with `6C72` (the spot in front of the player by facing direction — right: `(x1 + 2 + xs, y1)`, left: `(x0 − 3, y1)`, down: `(x1, y1 + 2 + ys)`, up: `(x1, y0 − 3)`, with the player's z and the item's own box size; a blocked spot refuses the drop — message 27 "THERE IS NO ROOM TO PUT THIS OBJECT IN" in the port; read from the code, ported); 5 `2EF2` plays sound 0Eh and does the same with another flag (put into something); 4 `3159` / 7 `3163` play sound 30h / 3Dh, call `79C2` when header byte 0Ch has bit 2 and queue event 5; 8 `2FD4` (class 7: read — draws a text box); 9 `32B5` queues event 5 and for class 2 plays sound 0Ah and counts down hit points `[bx+2]`; 10 `336A` stores the object's looping sound `FED9[class 1Dh]` in `ds:6E2`; 13 `340D` = **hold / unhold** (`ds:27A` = held thing; holding queues **event 27 (1Bh)**); 15 `344D` queues event 26 (1Ah) with the touched thing (insert / give); 16 `2E40` queues event 1. The port runs 1, 4, 7, 8 (on header byte 16h = 7, the diary: the status dialog, closed by End/Space/Return), 13, 15 and 16; verb 15 (insert) is offered in the rucksack when the player touched something just before opening it, and raises event 26 on the touched thing with the carried thing's id as argument (the condition type 26 compares it with the handler's word: keys, items given to characters) — not tested against the original.
**Look / describe (op 48h = 72).** The pickaxe's event-16 handler is `48 01 49 17`: op 48h with the word `0149` (LNG string 329, "THE PICKAXE SHOWS SIGNS OF RECENT USE OBVIOUSLY BY HACKING AT A WALL") and the terminator 17h. The op shows the string **with a full stop appended** as a message, moved to x = 208 (`y = 20 + 16·slot`), and shows the thing's picture in a second, text-less scroll at (16, 10) (box of 5 fill columns, 4 rows) holding a double green frame (rings of colour 1 and 3 around (23,25)–(57,59), brown interior 14) with the sprite at window (24, 26) + (5, 10) — compared with the emulator's screen. The port does this for every describe op.
Not done: use of held things (the held-item window), the read, switch and press verbs, the whole-rucksack grid (Return twice), carrying items to the next level. The picture drawing, the original's verb order and the 16-slot grid are the open parts.

**The map (F1, `1000:369F`; drawing `3806`, `38C3`, `389F`; emulator screen compared).** A scroll at (96, 30) (14 fill columns, 9 rows, i.e. `18A87` with ax = 60h, dl = 1Eh, cx = 0Eh, dh = 9) with the text "ROOMS ENTERED n" (LNG string 42 + the visited count `ds:582`) at (6Bh, 74h) in the small font. Every room entered (room record byte `17h` bit 6, `ds:6C9` = 0) is an **outline** of `(rec[4] · zoom + 1) × (rec[5] · zoom + 1)` pixels at `((rec[1] − ox) · zoom + 6Bh, (rec[3] − oy) · zoom + 29h)`, colour 4 (the current room colour 10 = 0Ah), clipped to the window x 6Bh..D5h, y 29h..73h; opening centres the view on the current room (`ox = rec[1] − 35h/zoom + rec[4]/2`, `oy = rec[3] − 25h/zoom + rec[5]/2`), initial zoom 2 (`ds:210`). Doors (section 4 records with flag bit 3 of byte 7, colour 7) are drawn by `389F` as short lines; the arrow keys scroll (`ds:715` input bits), Return (0Dh) zooms in (up to 9, shifting the origin by 1Ah/zoom and 12h/zoom), 0Ch zooms out, F2 (3Ch) re-centres, End leaves. The port (`map_*` in `src/main.c`, F1 / `map:1` in `--sim`) draws the room outlines, scrolling and zooming; the doors are not drawn.

**Diagonals.** Both axis bits may be set (`ds:773`: Insert 05, Delete 06, PgUp 09, PgDn 0A in the default INI, or two arrow keys): the player moves one unit along both axes per step with one combined collision test. In the emulator a diagonal run into an obstacle stops both coordinates at once (no sliding). The port does the same (`walker_tick`; note PgUp/PgDn also change the room in the demo scenes `room`/`walk`, they are read as movement keys only through the keyboard state).

**Jumping (emulator trace, level 1 room 0).** Holding End (fire) with nothing touched makes the player jump: after 3 steps of holding, the player's z (`[ds:5C]+2`) follows the per-step deltas **+5 5 4 4 4 4 3 3 2 1, −1 −2 −3 −3 −4 −4 −4 −4 −5 −5** (peak 35, back on the ground after 20 steps), stays 2 more steps on the ground and jumps again while End is held; a held direction keeps moving one unit per step in the air. The port (`walker_tick`) reproduces the z sequence exactly for a flat room; landing on objects / hitting ceilings use the collision test at the new z, and walking off an object makes the player fall by 1, 2, … 5 per step (assumed). Not decoded: the jump frames of the sprite.

**The lake creature (level 1 thing 467) — why it surfaces and sinks.** Room 0's timer handler (type 14, kept armed) runs op 36 *spawn* `01D3 44 42 02 FE` every 510 ticks (15 timer units): the thing is copied from its record (flags `50`, so no longer hidden, handlers armed again) to (68, 66, 2); its sprite has sound id 67 in header byte `1Eh` (starts when it is put in the room's slot table, `1000:51E2`). Its **animation script** ends with command **F8 n**, which queues **event 13 with argument n** for the thing (`1000:3CE7`; F7 `3D17` queues into the list at `ds:86`); the thing's event-13 handler `01 32 FF FF 17` (argument 1) queues event 23 and the event-23 handler removes it (op 2, flag 80h) — the sound stops (`1000:5114`). In the emulator the sound ids 67 play at ticks 363, 873, 1383 … and stop 80 ticks later. The port now (a) delivers F8 events (`AnimState.ev`, `roomview_tick`), (b) lets op 36 restore the thing's record (`reset_thing`, mirrored in `tools/acl_vm.py`, vectors regenerated) and (c) follows the visibility for the sound: cycle 510 ticks, visible 68 ticks (80 in the original; the phase of the first spawn differs too).

**Ops 08h and 1Bh (read from the code).** `08 id16` (`1000:7A82`): unless the thing's A[5] bit 7 is set, remove it from the list of active (moving) things at `ds:B8` (`5F2F`) — the port sets that bit. `1B id8 word` (`7BC6`, op 27) stores a word at `[rec + 2]` of door record `id` (section 1 in the port's numbering, 8-byte records), `0A id8` (`7B8B`, op 10) clears it, and **op 3Dh (61) `82FF` tests it** (non-zero = locked → the condition counter): the word is **script-visible lock state only** (the loader `4D00` does not look at it; most doors carry `FFFF`, a few a key/lock number such as `009B`); when a set/clear changes it the engine plays sound 12h if the door is in the current room's door list (room record +6…) else 2Ah. Op 39h (57, `82F4`) tests whether the held thing (`ds:27A`) equals its word. Ported (`AclVM.doorw`, `held`; mirrored in `tools/acl_vm.py`, vectors regenerated) — the physical blocking of a locked door is done by other things (the door's wall sprite / scripts), not by this word. Skipped ops seen in a run over levels 1–5: only 08h (16×), 00h and 1Bh.

**More ops read from the code (all in the port; ops 22h, 49h mirrored in `tools/acl_vm.py`).** `03 id16` (`79BB`): if the thing's animation record C starts with `FE` (frozen by F9) set it to 0 and advance the position — **resume the animation** (460 uses, doors, levers, traps); `0B id16` (`7BE3`): movement state B[0] := 0, `B[1]++` if it was 4; `22h` (34, `7D88`): condition "the player carries thing id" (`86D1` walks the carried list, section 8); `46h` (70, `83BA`) plays sound `id8`, `57h` (87, `83C2`) stops it; `49h` (73, `8003`, two ids A, B): **move thing B to where thing A is** (same routine as op 41 `8024`; a carried A means the player's place in the current room). `41h` (65, `832F`): thing's A[1] += signed byte (clamped 0..F5h); `45h` (69, `839D`): movement state B[3..5] := the three bytes (ported). Still not ported (counts in levels 1–5 of both games): (spawn: ops `24h` (36, `7E99`: id, x y z, room), `2Ch` (44, `7E1C`: A, B, direction index, room — e.g. level 1 things 193 and 198 on look/event 11: `FFFF 01C0 06 FE` spawns thing 448 at the place of the current thing, FB46[6] = 12h = direction *down*) and `54h` (84, `7E7F`: A, B, dx dy z room) all copy the record of thing B into a new slot (`7F17`: `rep movsb`) and append a request to the list at `ds:346` (record, x y z, room, direction code from the table `ds:FB46` = 08 04 02 01 18 14 12 11 28 24 22 21 …: low nibble 8 right / 4 left / 2 down / 1 up); a spawned object with header byte 16h bit 7 is placed with the probe `6C72` in front of A instead; the port places a fresh copy at A's place (ops 44, 84; op 36 at the coordinates) and ignores the direction code and the probe — the original allocates a new thing id (900, 901, …) for every spawn; the port does the same when the template is already in the destination room and visible: `spawn_id` takes a free id in 900..999 whose Mp2 record slot shares the template's record (`AclVM.clone_src`, restored after a save is loaded), otherwise the template itself is reused); `38h` (56, `8294`) is ported: condition "the thing is in room a[2] and its box overlaps the region x a[3]..a[4], y a[5]..a[6], z a[7]..a[8]" (signed bytes; the box is `(x1 − xs, y1 − ys)` to `z0 + zs` with the sprite header sizes the front end stores in `AclVM.size`), `23h` (35, `7DB6`, 9), `38h` (56, `8294`, 43), `42h`/`41h`/`3Ch`/`5Ch`/`5Bh`/`4Ch`/`59h`/`45h`/`4Dh`/`54h`/`52h`/`4Bh`.

`56h` (86, `7A56`): money += word with sound 03h (op 06 with a word); `59h` (89, `7AC7`): A[6] bit 0 cleared (not modelled); `5Bh` (91, `8448`): condition byte 6 of the current thing's record == operand; `5Ch` (92, `845A`): the chest content word A[0..1] := 0. Not ported: `4Ch` (76, `825A`: tests a bit of `ds:6AB`), `3Ch` (60, `8312`: `5CDC` condition), `23h` (35, `7DB6`: give), `07h`/`59h` activation helpers (`5F18`).

`tools/verify_acl_ops.py` now also fuzzes op 5Bh (91) against the original: no mismatches on levels 1 and 4. Ops 00h/73 cannot be checked that way: op 00 (like 02) deletes the thing from the engine's lists rather than changing `rec[3]` (the port's flag 80h is a model), and op 73 moves records around (`3F25`), which shifts the emulator's packed record array.

**Animation command F7 n = run the thing's TAD action routine n (verified on the lake creature).** `1000:3D17` queues `(A[1], n, thing)` into the list at `ds:27E`; `1000:5F6E` takes each entry, looks up the **second word of the 4-byte entry of behaviour `A[1]`** (table entry `base + 4·(b+9) + 2`, e.g. `01A2` for behaviour 4 of level 1) — a table of routine offsets indexed by n (level 1: `01A6 01E0 05BE …`) — and far-calls the routine with `di` = the thing's record, `bp` = its A struct, `dx` = the thing id. Routine 0 of behaviour 4 (`01A6`) reads A[8], picks a word from the table at `0305` (A[8] negative: a random index through the service `02`) and stores its low byte into **C[3] = the animation position** and its high byte into A[9] — i.e. it **randomly chooses which of the three branches of the creature's animation script** it plays (positions 0, 13/14, 22), which is why the original's visible time differs from a straight run of the script. The port (`tad_run_action`, F7 in `anim_step`, `roomview_tick`) builds the record in the data image from the VM (position, flags, A struct now 16 bytes, B, C = delay, frame, dir, pos, cnt, cad), runs the routine on the interpreter and copies the result back. With it the lake creature is up for **exactly 80 ticks** (the original: 80) every 510 ticks.

Animation commands **F6 / F5 n** (`3D61` / `3D4B`) decrement / increment the byte `A[8 + n]` of the thing (the counters the action routines read, e.g. `[bp+8]` of the lake creature's routine 0); the port applies them to the VM's A struct. F4 / F3 (set flags) and F9 (freeze + mark finished) are still only counted.

**Services used by the TAD routines (from the service table `ss:[ds:B6]`, 16-bit words in order: `410E 7E1C 407F 81C8 8706 D8BC 890A D912 7F3C 7DE1 D900 369F 8275 40A9 31A3 78DE 8840 6A44 164E 3DB8 799F 7903 5F18 9D8E 1043 D4C6 2FE3 D8BB 5DAA 1187`; the index in the hook is twice the position).** `04` = `407F`: `dx` = thing id → `di` = its record (FFFF: the current thing), ZF set if none; `22h` = `6A44` "**fire a shot in direction `cl`**": sets the projectile's movement velocity `B[3], B[4]` from the table at `ds:FE1F` (pairs of ±127 / 0 for the direction bits 1 up, 2 down, 4 left, 8 right) when `[bx+4] = 0`, then (`6A9B`) builds a spawn request for the thing in `ds:57E` (the id the routine read from the shooter's A struct, big-endian at A[2..3]) at the shooter via `7E1C`. Level 3's Royal Armoury turrets (things 762, 808: action routines `0150`/`015A` → `016C`) fire thing 776 to the right this way. The port (`TadRt.cur_thing/proj_thing`, `build_rec/commit_rec`, `AclVM.spawn`, `acl_spawn_shot`) does it for action routines: one live copy of the shot thing; the shot (sprite header byte 16h has bit 7, movement mode 3 and no behaviour routine for its A[1] = 19h) flies with the velocity the shooter wrote into B[3], B[4] at B[6..7] units per step (3 here, so about 6 units per 4 ticks across the Armoury) and is removed when it is blocked or hits the player (event 9 / damage = its A[1] as for creatures; the original's exact removal is not checked).

Service `00` = `410E` (`ax` = table, `dx` = id → `di` = record): the Payoff's routines (`TATA/1`, `TATA/2`: `0488`, `044A`, `050C` …) use **type 5 = the thing list of the current room** — `050C` reads the room record `ds:5E` (byte `1Dh` = the number of things) and asks for table 5, then walks the list of thing ids with service `04` to find a thing by sprite block (`cmp cx,[di+6]`) or id, skipping hidden ones (`[di+3] & 80h`). The port prepares the list and the record before each timer / action run (`prep_room`) and fills the sprite block bytes of the records it builds (RAM words are little endian: the file's big-endian words are swapped). No unsupported service is left in the runs over all levels.

**Throwing (read from the code; ported).** The fire key (`ds:5F9` bit 7) with a held thing (`ds:27A`, its class in `ds:6C6`) does not jump: class 0 goes to `sub_12DE2` = sound 0Eh + the drop routine `2F1C` with `cl = E2h`, other classes (1, 4) to `10701` / `106F5`. The drop code places the thing with `6C72` (in front of the player), queues event 21 and then — for a throw (`cl ≠ 0`) — sets the player action `ds:601 = 4` and gives the thing's engine entry two **path pointers**: `[entry+18h]` = the vertical path (`FBA5` for things that fly, `FBA3` = a bare `00 50` for the others; falling things use `FB8E`, `FB9A`) and `[entry+1Ah]` = the horizontal path chosen by the facing direction from the word table `ds:FBDE` (`FC00, FC04, FC2C, FC00, FC54, FCA4, FCF4, FC00, FC7C, FCCC, FD1C …`). Paths are byte strings of per-step deltas ended by `50h` — the falling path `FB8E` is `FF FF FE FE FD FD FD FC FC FB FB FB FA FA FA F9 F9 F9 F9 F8 50` (accelerating fall, 20 steps), the throw's rise and fall `FBA5` is `06 06 06 04 04 02 01 00 00 FF FE FC FC FA FA FA F9 50`. The per-step consumer is in the object movement code (`[si+18h]` is used around `1000:5F…` and `1000:6A…`); landing and collisions during the flight were not read. The horizontal paths (table at `FBDE`, direction bits 1 up / 2 down / 4 left / 8 right, diagonals use the same magnitude on both axes) are, one entry per game step, `4 4 3 3 3 2 2 2 2 2 1 1 1 1 1 1 1 1` (35 units in 18 steps) ended by `00 00`. Ported for class-0 things (`throw_held`, `fly_step` in `src/main.c`): fire with a held thing places it in front of the player, plays sound 0Eh, queues event 21 and lets it fly along these paths with the collision test at every step (a blocked horizontal step ends the horizontal path, a blocked fall lands it); a pickaxe thrown to the left at x = 56 flew to x = 23 and landed after 16 steps. The landing rules of the original (`16E5D…16F53`) are not ported; gravity (`1572B` → `6BFC`) is ported (`gravity_scan`): the engine entry's flag byte `+27h` is the sprite header byte `0Ch` (copied by `1000:52A8`), so **only things whose header byte 0Ch has bit 0 set (the non-solid items) fall** — a hanging lamp (thing 366 of level 1 room 5, z = 64, byte 0Ch = E4h) or the statues 201/202 of level 2 (F8h) never do — provided the weight byte 0Dh is non-zero, record flags2 bits 4 and 8 are clear and the thing stands above the floor with nothing under it; it then gets the fall path `FB8E`. No thing falls at the start of any room of levels 1–5 of either game (a run over 40 rooms each); classes 1 and 4 (weapons, spells) are not either.

**Held items that act: spells and weapons (read from the code; ported, see the paragraphs below).** The held thing's class (`ds:6C6` = sprite header byte 16h) decides what fire does: 0 = throw (ported), **1 = a spell item** (`6896`), **4 = a throwing weapon** (`69CF`). In the data class 1 are the SCROLLs, RUNIC STONEs, ORBs and WANDs (A[0] = the spell number, A[1] = its power, **A[2] = the charges** (FFh = unlimited), A[3] bit 7 = not yet allowed, A[4..5] = the id of the projectile thing of spell 0 / 8 items, **A[6] = the cooldown** loaded into `ds:629`, **A[7] = the mode** (0 projectile, 1 TAD routine, 2 targeted): e.g. level 1 scroll 27 `00 0F 14 00 01 DC 08 00`, level 3 wand 343 `00 05 14 00 00 10 …`; the A struct runs from `rec[12]` to the B struct at `rec[13]`, so records with a 10-byte A carry bytes 6..9 that the port used to drop) and class 4 the STONE BAGs, SACKs, SHIRIKANs / SHURIKANs, DAGGERs and BAGs OF ROCKS (A[0] 00–02, A[1] = the target-side field, A[2..3] = a thing id). `6896` refuses (message 25h) when `A[3] & 80h`; with A[7] = 0 it plays sound 11h and **launches the projectile thing A[4..5]** (its A[0] := spell | 80h, A[1] := power, A[5] := A[8], placed in front of the player with the velocity of the facing by `16A37`); otherwise (A[7] = 1) it plays sound 06h, queues events 4018h and 19h and **far-calls the TAD routine `table2[A[0]]`** (A[7] = 2: the targeted variant, message 41); afterwards (`16AC9`) the charges A[2] drop by one (not for FFh) and at 0 the item is destroyed (message 15 `THAT WAS YOUR LAST SHOT`, un-held) — the table is the third word of the TAD data table (`base + 4`, level 1: `081E` → `085C 08C5 093E 095E 0974 097A 09F4 09F5 …`, TATA/1: `085D` → `08A5 091E 099E …`). Routine 0 (`085C`) is the **weapon strike**: it takes the target entry from `ds:5C2`, and when the target's sprite class is 3 (a creature) subtracts the weapon's A[1] from the creature's A[0] (its hit points); when that underflows it queues event 17h (23: the creature dies, its handler removes it). So creatures have hit points in A[0] and fighting is done by native routines. The routines read the target / weapon through `ds:5BA…5C4`, which the caller fills from the engine entries; the port has no such entries; it implements the strike of spell 0 (`cast_strike`: the first active creature in the facing line within the width of the player loses A[1] hit points; charges A[7] decrease; message 25h / sound 11h / sound 06h as above), throws class 4 as a plain thrown thing and ignores the other spells except spell 1 (`08C5`, `cast_smite`: up to A[1] active creatures with A[6] bit 0 clear die). Spell 4 (`0974`, e.g. the SCROLL 259 of level 1) sets `ds:6C9 = 81h`, which makes the map (`3806`) draw the rooms whose record byte 17h has bit 7 **clear** instead of the visited ones: the **magic map** (the port opens the map that way, `map_open_magic`; `--sim magicmap:1`). Spell 7 (`09F5`, the WANDs `07 …` of Cadaver 2) is **open a chest**: the target `ds:5C0` must be a sprite class 8 thing whose A[4] has bits 0 (open) and 6 clear, else message 2Fh; it clears the lock word A[2..3] and A[4] bit 1 and runs verb 3 (`31A3`) on it; spell 8 (`0A48`) refuses things with A[7] bit 5 (message 2Fh), clears A[2..3] and A[7] bit 6 and shows message 1. The port takes the nearest thing in the facing line within 24 units as the target (`line_target`, `cast_unlock`); the original's own target selection (`ds:5C0…5C4`) is not decoded. Which creatures a spell hits and how the original finds the target is not decoded.

**Spell and potion name tables, and how `84D9` composes a description (verified against the original routine; ported).** `ds:44` points to a table of 100 eight-byte entries (read from the save-game layout at `1B568`; in a fresh level at `ds:3772`), `ds:46` to the same for potions (`ds:3A92`); entry *n* starts with the big-endian **LNG string id of the name**: spells 0..9 = strings 84..93 (MAGIC MISSILE, MASSACRE, FREEZE, READ LANGUAGE, MAP, MIND BLAST, TRANSPORTER, UNLOCK CHEST, UNLOCK DOOR, DESTROY), 10 = 32 (BLESS WEAPON), 11..26 = 94..109 (SLOW CREATURE, DUPLICATE, BLESS MAGIC, BLESS POTION, DISPEL TRAP, LOCK DOOR, TALK WITH DEAD, CONFUSION, PURIFY POTION, READ MAGIC, LEARN POTION, TURN MONSTER, SLEEP, DISPEL MAGIC, ENCHANT LIQUID, POLTERGEIST); potions 0 = 31 (STAMINA), 1..3 = 110..112 (SUPER FAST, GIANT JUMP, ALCOHOL), 4 = 30 (POISON), 5..17 = 113..125 (blank, SHOT SHIELD, WATER, FIRE SHIELD, CURE, SLOW, ICE SHIELD, MAGIC SHIELD, CURE POISON, TOTAL SHIELD, IMMORTAL, STRENGTH, SUICIDE). `ds:70B` is the **language** chosen at start (0 English, 1 French, 2 German). `84D9(bp = header, di = A struct)` by the class (header byte 16h): **1** `THIS IS A ` (5) + name (string 39 `UNKNOWN` when `[bx+3] & 1`) + ` SPELL. IT HAS ` (9) + number `[bx+2]` + ` CHARGES REMAINING AND A POWER OF ` (10) + number `[bx+1]`; **2** the same with strings 11 / 12 and the potion table; for both, `[bx+3] & 4` appends two blanks and string 34 `THIS POTION APPEARS TO BE BAD`; **4** weapons: with `[bx+2] ≠ 0` string 19h (`THAT WAS...` area: the shots text) else `YOU NEED A <name of thing [bx..bx+1]> IN YOUR RUCKSACK TO USE THIS WEAPON.`; **0x80 bit** `THERE ARE ` (16) + number or `UNLIMITED` (18 when `[bx+2] = FFh`) + ` SHOTS LEFT EACH WITH A POWER OF ` (17) + number; **8** chests as above. French (`ds:70B = 1`) puts the name first (`5F8 = 9`: string 9, name, two blanks and a period, then the numbers). Verified by calling the real `84D9` in the emulator with level 1 records: the numbers come from the **item's own A struct** (A[0] spell / potion number, A[1] power, A[2] charges / doses; level 1 thing 27 gives `THIS IS A MAGIC MISSILE SPELL. IT HAS 20 CHARGES REMAINING AND A POWER OF 15`, no final period), class 4 with A[2] = 0 gives `THIS WEAPON IS LOADED AND READY TO USE.` followed by the shots text of the **ammunition thing** named by A[0..1] (its A[2] shots, FFh = UNLIMITED, its A[1] power), class 4 with A[2] ≠ 0 `YOU NEED A <ammunition name> IN YOUR RUCKSACK TO USE THIS WEAPON.`, and any class with bit 7 (e.g. 82h) its own shots and power. `A[7]` is therefore not the charge counter shown to the player (it is the state `6896` tests: 0 arms the spell target thing named by A[4..5] with `spell | 80h`, power A[1] and A[8]); the port's `cast_spell_item` now follows this (charges A[2], mode A[7]). The port (`thing_description`, debug aid `CAD_LOOKTEXT=<thing>`) shows these texts for floor things and carried things.

**More spell routines (disassembled from `ROUTINE1.TAD`, level 1 table `081E`).** Spell 5 (mind blast, `097A`) walks the creature list at `ds:B8` and subtracts the item's power A[1] from the hit points A[0] of every creature whose A[6] bit 0 is clear, queueing event 17h at underflow (ported as `cast_blast`, read from the code only). Spell 2 (freeze, `093E`) calls the helper `07FE` → service 0Ch (`8275`), which stores the power in the **effect timer table** `ds:62B + effect` (and a copy at `+34h`; `ds:62A` counts the active effects, the list of active ones sits at `ds:692`) with effect number 22h; the potions' timed effects (string 26 `THE EFFECT WEARS OFF`) use the same table. Entries `095E` / `0973` of the table are plain `RET`s: the spells 3, 17, 23, 24, 27 and 29 (and 25, 26) do nothing in level 1's table. The port has no effect timers yet.

**Drinking (verb 9, `1000:32B5`; potions are the carried things whose header byte 17h is 2).** Queues event 5 for the potion, plays sound 0Ah, clears the identified flag A[3] bit 0 (unless bit 1 is set), decrements the doses A[2] (signed: at 0 or below the potion is destroyed and leaves the hand), then a *bad* potion (A[3] bit 2) shows string 34 and calls `181C8` with `ax` = the thing's record byte 0Ch (the A-struct offset left in `ax` by the dispatcher; `neg dx` is ignored): **it heals by that amount (emulator: energy 100 → 122 with offset 16h)**, an original bug, otherwise it calls the TAD potion routine `table[A[0]]` (second word of the TAD header, level 1 `06DC` → `0710 072C 073C 074D 0753 0764 0765 076A 0774 0700 0779 077A 077F 07A2 07AF 07B4 07B9 07C4`): 0 STAMINA adds A[1] energy (service `81C8`); 1 SUPER FAST sets `ds:60E = ds:60F = 2` and effect timer 23h, 2 GIANT JUMP sets `ds:8A` and timer 21h, 3 ALCOHOL timer 20h, 4 POISON stores A[1] in `ds:6A9` with message 30, 6 / 8 / 10 / 11 the shot / fire / ice / magic shields set a bit of `ds:6AB` and the power in `ds:6AC` (message 44). The port (`drink_potion`) does stamina and bad potions; the timed effects need the effect timer table (not ported). Debug aids: `CAD_CARRY=<thing>` carries a thing from the start, `CAD_SPELLS` lists the spell / potion / weapon things of the level.

**The effect timers (`1000:11EB5`, `8275`; ported as `acl_set_timer` / `tad_timer_unit`).** `ds:62B + e` is a countdown per effect number *e* (`ds:62A` counts the active ones, `ds:692` lists them, `+34h` keeps a copy for re-arming); every 17 game steps each active counter drops by one and at 0 the routine *e* of the **fourth word of the TAD header** (level 1 `0C3F`: one word per effect, 0 = none, the entries 20h–28h follow a gap of zeros; a routine that leaves the counter at 0 ends the effect, one that sets it again re-arms it) runs. Effects 20h–24h are the potions': **20h alcohol** (`ds:64B` ≠ 0 negates the movement delta in `10AF6` / `10D4F`), **21h giant jump** (the expiry routine restores `ds:8A = ds:D8`; while active the jump profile is the table at `ds:D8 + 1Ch`: `+8 8 6 6 6 6 4 4 2 1 −1 −2 −4 −4 −6 −6 −6 −6 −8 −8`, peak 50), **22h freeze** (`ds:64D` ≠ 0: creature things (class 3) do not move: `13B87`, `15F4C`, `171DD`), **23h super fast** (`ds:60E = ds:60F = 2`, the walking step sizes; expiry sets them back to 1), 24h clears `ds:6CA`. The port runs the potions 1–3 and the freeze spell natively through these timers (the walk step doubles, reverses or the creatures stand still; the giant jump reaches z = 50 in the headless run).

**Shield potions (TAD `0765 / 0774 / 077A / 077F` = potions 6 SHOT, 8 FIRE, 11 ICE, 12 MAGIC SHIELD; 14 TOTAL SHIELD `07AF` sets `ds:6AB = 7Fh`).** They set bit 0 / 1 / 2 / 3 of `ds:6AB` (= `g[0x11B]`), store the potion's power A[1] in `ds:6AC` and show message 44; there is no timer. A creature hit on the player (`1000:172EC`) reads the creature's damage A[1] and its damage type A[5] (bit 7 set = unshieldable): when `ds:6AB` has the bit of that type, only the **low byte** of `−A[1]` is reduced by `ds:6AC` (`sub al, ds:6AC`): a negative byte (A[1] + power ≤ 128) swallows the hit, otherwise the hit costs `256 − byte` energy — **damage plus the shield power** (verified with 600 random cases run through `172EC`); without a shield bit it costs A[1] (`creature_move` ports this). Other potion routines seen: 4 POISON stores A[1] in `ds:6A9` (message 30, helper `06BC`), 7 WATER message 43, 13 CURE POISON `07A2` (needs `ds:6A9 ≠ 0`), 15 IMMORTAL timer 27h, 16 STRENGTH `ds:59C = 0C7h` and timer 28h, 17 SUICIDE energy −0x8300 (not ported).

**Player shots (`1000:69CF` weapons, `16896` mode 0 spells, `16A37` launch, `6F62` hits; ported as `fire_weapon`, `launch_shot`, `shot_hits`).** The ammunition / projectile things are ordinary things parked in a storage room (level 1: 121, 463, 469 of class 81h in room 69; the spell projectile 476 is class 82h, B = `03 … 03 03 01 00`, creature shots like 776 are class 80h). A class 4 weapon (A[0..1] = the thing flown, A[2] = 0 when no ammunition has to be carried, A[3] = cooldown `ds:629`, A[4] bit 0 = used up with the last shot) plays sound 14h and **moves that thing in front of the player** with the velocity B[3], B[4] = ±127 along the facing; the thing's own A[2] counts the shots left (FFh unlimited; at 0 message 15 and the weapon goes if A[4] bit 0). A spell item with A[7] = 0 arms the thing A[4..5] (A[0] := spell | 80h, A[1] := power, A[5] := A[8]) and launches it the same way. The movement routine then lets the shot fly one step per game step (B[6], B[7] = 1 for the shots of level 1, 3 for 476) and on a collision with a class-3 creature: **weapon shots (81h / 84h)** spare it when its A[5] has the bit of the shot's type A[5] or A[6] bit 0 is set, else (`17367`, run through the original with 7 cases) `sub dh, al; jg` on the **signed bytes**: when A[0] > A[1] the hit points drop by the power (a power above 127 wraps: 10 − 200 leaves 66), otherwise they **stay** and event 17h is queued (`6F62` loc_17327); the spell strike `085C` subtracts unsigned and leaves the wrapped byte when it queues the event; **spell projectiles (82h)** run the TAD routine `table[A[0] & 7Fh]` with the creature as target (`17250`); class 80h shots hit the player instead (`172D4`). Verified in the port: a shot of thing 121 takes 3 from creature 443's 10 hit points.

**Spells cast on an object (A[7] = 2, `1000:169AB`; TAD entries of the spell table `081E`, level 1).** The spell is started by the rucksack verb 16 (`1000:2E40`: it saves the touched thing's record / header / A in `ds:0DA..0E0`, queues event 1 for it, sets `ds:6A8 = 1` and calls the spell routine `1689C` for the carried item; a spell fired with the fire key has `ds:6A8 = 0`). Sound 06h, then the target is that touched thing (`ds:6A8` ≠ 0; message 41 `YOU MUST CAST THIS SPELL ON A SPECIFIC OBJECT` when there is none), `ds:5C2/5C4/5C0` hold the target's header / record / A and the routine `table[A[0]]` runs; **a spell that does not fit the target's class ends at `0A3D` = message 47 `YOU WASTED THAT SPELL`**. Entries: 10 BLESS WEAPON `0A74` (class 4: A[1] += power on the ammunition thing named by A[0..1]; class 80h: on itself; message 32), 11 SLOW CREATURE `0AE9` (`ds:6CA = 1`, timer 24h; the creature AI `15F4C` then skips every second step), 13 / 14 BLESS MAGIC / POTION `0AF5 / 0B09` (A[1] += power on a class 1 / 2 target, capped at 255), 15 DISPEL TRAP `0B40` (class 8, A[5] := 0, message 52), 18 CONFUSION `0B6E` (timer 25h), 19 PURIFY POTION `0B74` (class 2, clears A[3] bit 2, message 36), 20 READ MAGIC `0BA3` and 21 LEARN POTION `0BDF` (class 1 / 2 with A[3] bit 0 set: clears bits 0, 1 and 7 = the name becomes known), 22 TURN MONSTER `0BF7` (timer 26h), `0BFD` a class 3 target loses A[1] hit points. Ported: 10, 11, 13–15, 19–21 (`cast_on_target`; verb 16 of the carried spell with a touched thing starts it: `cast_target`); not ported: 18, 22 (timers whose effect is not modelled), 12 / 16 / 17 (empty in level 1's table).

**More potions and the timer routines of level 1's table (`0C3F`).** Timer 0 `0CA7` ends a poison (`ds:6A9 := 0`, message 45 `THE GODS CURE YOUR POISON`), timer 1 `0CBD` is its tick (energy −= `ds:6A9`), timers 2–5 `0CE3 … 0D0D` clear shield bit 0–3 of `ds:6AB`, timer 6 `0D1B` clears all of it, 7 `0D22` increments `ds:622`; op 5Ah arms timers 0 and 1 (duration, period) and sets `ds:6A9`. The potion routines (same table as above): 4 POISON only stores `ds:6A9` and shows message 30, 5 and 10 are empty, 7 WATER message 43, 9 CURE raises the energy to half of `ds:700` when it is below that, 13 CURE POISON clears `ds:6A9` when set (message 45), 17 SUICIDE removes 0x8300 energy (death). 14 TOTAL SHIELD `07AF` sets every shield bit (`ds:6AB = 7Fh`) and falls into 15 IMMORTAL `07B4` (effect timer **27h**: while `ds:652 ≠ 0` `1000:181C8` ignores every energy change), 16 STRENGTH `07B9` (`ds:59C := 0C7h`, timer 28h; `ds:59C` is the push strength that `push_move` gets, 50 normally; the expiry routine `0D58` puts `32h` back). The port (`drink_potion`) does all of them; the effect timer array now covers the numbers up to 33h (`g[0x81 + 0x1A + effect]`), and the unit loop runs the expiry routines of 0 … 29h.

**The spell / potion name tables are in `LEVn.MP1` (both games), and the Payoff renumbered its spells.** The 100 + 100 entries of 8 bytes (name string id big-endian first) sit in MP1 right where the table starts `00 54 20 20 20 20 20 20` (string 84 = MAGIC MISSILE; Cadaver 1 level 1 at unpacked offset `1C1D4`, Payoff TATA 1 at `1A436`), the potion table `0x320` bytes later; the port reads them (`spell_str` / `potion_str`) for the description texts. **Payoff (TATA) spell numbers** (by name, confirmed by matching the routine bodies of `ROUTINEn.PAK` against Cadaver 1's): 0 MAGIC MISSILE, 1 MASSACRE, 2 FREEZE, 3 TRANSLATE, 4 MIND BLAST, 5 TELEPORT, 6 UNLOCK CHEST, 7 UNLOCK DOOR, 8 REVEAL, 9 BLESS WEAPON, 10 SLOW CREATURE, 11 DUPLICATE, 12 BLESS MAGIC, 13 BLESS POTION, 14 DISPEL TRAP, 15 LOCK DOOR, 16 TALK WITH DEAD, 17 CONFUSION, 18 PURIFY POTION, 19 LEARN SPELL, 20 LEARN POTION, 21 TURN MONSTER, 22 SLEEP (FIREBALL in level 3), 23 DISPEL MAGIC, 24 ENCHANT LIQUID, 25 ACTIVATE, 26 DISPEL EVIL, 27 BANISH, 28 PETRIFY, 29 DISPEL CURSE, 30 MAP (level 3 adds 31 SUICIDE); the potions keep Cadaver 1's numbers 0–17 and add 18 OIL, 19 WISDOM, 20 STUPIDITY, 21 LEAPFROG, 22 WEAKEN. The Payoff routines for 0, 1, 2, 4 (= Cadaver 1's 5), 6, 7, 9, 10, 12–14, 18–20 are the Cadaver 1 ones (`spell_sem` maps them); 22 FIREBALL (level 3), 19 LEARN SPELL (a longer routine than READ MAGIC), 25 ACTIVATE and the new potions are not ported. Levels 1–5 of Cadaver 1 have identical spell and potion routines.

**Payoff routines for its new spells and potions (`ROUTINEn.PAK`, level 1 table `06B5` / `085D`).** Potions: 18 OIL is an empty routine (scripts test it), 19 WISDOM adds 10 × power to the experience `ds:24C/24E` (`07C3`), 20 STUPIDITY subtracts it (not below 0), 21 LEAPFROG (potion table `06B5`, entry `0807`: `cs:0E7D := 1`, then the generic `081D` = effect timer 7 := A[1]; the table at `085D` is the *spell* table) — the timer-7 routine `0DB6` runs when the counter expires (`11EFA` calls `table[effect]`, then reloads from `ds:62B + 34h + effect` if the counter is non-zero) and, while the flag is set, copies `ds:6A7 | 80h` (the key byte with the fire / jump bit) into `ds:6DC` and writes `FFh` to `ds:62C`; entry 8 (`0DCF`) clears the flag; the use of `ds:6DC` was not followed, so the port does not offer the potion (it is not consumed), 22 WEAKEN sets the push strength `ds:59C := 0Ah` and timer 29h (expiry `0E6A` restores 32h). The timer numbers 20h–28h are the same as in Cadaver 1. Spells (index → routine): 10 SLOW CREATURE `0B72` (`ds:6CA := 1`, timer 24h), 17 CONFUSION timer 25h, 21 TURN MONSTER timer 26h, **22 SLEEP / FIREBALL `08A5` = the magic missile strike**, 25 ACTIVATE / 26 / 28 / 29 empty, **27 BANISH `0C86`** = Cadaver 1's `0BFD` (a class 3 target loses A[1] hit points), 30 MAP `0CC8` (`ds:6C9 := 81h`). The port covers WISDOM, STUPIDITY, WEAKEN, OIL, SLEEP / FIREBALL (as spell 0) and BANISH (`spell_sem` 28).

**CONFUSION and TURN MONSTER (effect timers 25h / 26h) act inside the creature behaviour routines.** The shared direction helper of `ROUTINEn.TAD` (level 1 `055E`…, Payoff `0535`…) tests `ds:650` (effect 25h: the direction is a random number 0–7 from service 20h through a table at `cs:0E11`) and `ds:651` (effect 26h: the chase direction + 4, i.e. the creature runs away) before it computes the step towards the player. Because the port copies the script globals (which hold the timer array) into the routines' data segment, setting the timers is all that is needed (`cast_spell_item`, spells 18 / 22, Payoff 17 / 21): in level 1 room 69 creature 446 walks towards the player normally and away from him under TURN MONSTER (headless runs). The idle ammunition things parked in that room (469, 476: class 81h / 82h, mode 3) are stepped like shots with velocity 0, which does nothing. Debug aid: `CAD_CREAT=<thing|-1>` prints the creature step state.

**Saving a position (`1000:BF92`; the keys are F5 / F6, the manual's S / L are not in the code).** The game step reads the key in `ds:716` (a scancode from INT 16h): **`3Fh` (F5) sets `ds:703` bit 3, `40h` (F6) bit 4, `10F68` then calls the save `1BF92` / load `1BFE9`**; 41h (F7) is the pause (`115EB`, `19CE8`, `11685`), 42h (F8) clears the messages (`13514`), 43h is ignored in some states, 23h (H) holds and 3Bh (F1) is the map; 1Fh (the S key) only sets `ds:6D1` bit 0 (used while touching things). The save prompt shows string 60 `THE GODS DEMAND A PRICE TO SAVE YOUR POSITION OF` + the price and string 62 `DO YOU WANT TO PAY. PRESS Y OR N.`; the price is `max(ds:246, 50 × (ds:708 + 1) − 45)` gold (the level 1 price is 5), and when the money is less string 61 `YOU CANNOT OFFER THE GODS ENOUGH TO SAVE A GAME.` appears. After Y the gold is paid, `ds:246` grows by `6 × level` and the game asks for a slot 0–9 (disk prompts via `1C810`: the digit keys 2–0Bh, 'D' / 'T' choose the drive); L asks for a slot and loads without charge. The other keys of the manual (PAYOFF.DOC: C clears the messages, P pause, F2 toggles fire / ENTER for the icons, F3 icons at contact / after fire, F4 four / eight directions; in the code of both games F7 / F8 do pause / clear, and **F2 / F3 / F4 change no byte of Cadaver 1's data segment when pressed in the running game** (diffs against a noise baseline), so they are not part of Cadaver 1's `game_step`; Payoff's handlers were not located — the port keeps F4 as the 4 / 8 direction switch) (the keyboard ISR `11F0` only maps the cursor keys to the movement bits; the others reach the game step as scan codes). The port implements S / L / C (`savemode_*`, slots `cadaverN.sav`, SaveHead version 2 stores the price).

**Placing a dropped or thrown thing, all eight facings (`1000:6C72`, table `cs:6C54`; verified by calling the original with a fake header of box 3 × 2 and the player record `x1, y1, x0, y0 = 24, 23, 18, 17`).** `ah = ds:608` is the movement key mask of the last move (1 up, 2 down, 4 left, 8 right; 0 and the impossible sums fail). With *xs, ys* the box of the placed thing: up `(x1, y0 − 3)`, down `(x1, y1 + 2 + ys)`, left `(x0 − 3, y1)`, right `(x1 + 2 + xs, y1)`, up-right `(x1 + 1 + xs, y0 − 1)`, up-left `(x0 − 2, y0 − 2)`, down-left `(x0 − 2, y1 + 1 + ys)`, down-right `(x1 + 1 + xs, y1 + 1 + ys)` — the emulator gave (24, 14), (24, 27), (15, 23), (29, 23), (28, 16), (16, 15), (16, 26), (28, 26) for them in this order, as the port's `front_spot` computes (`Walker.face` keeps the key mask; shots use it for their velocity too).

**The whole-rucksack grid navigation (`1000:125CF`, transcribed and verified: 2080 cursor moves through the original's key handler `2746 … 2825`, `tools/gen_grid_vectors.py`, `engine/ruckgrid.c`; ported as `grid_begin` / `grid_draw`).** State: column `ds:6AF`, absolute row `ds:6B0`, visible row `ds:6B1`, first visible row `ds:6B6`; `R = ds:6B4 = (n − 1) / 4` is the last row and `C = ds:6B5 = (n − 1) mod 4` the last column; the box shows `min(R, 3) + 1` rows (5 per 40-pixel row) of up to four things from row `ds:6B6`, the frame sits at the visible cell. **Left / Right** stay inside the row (wrapping to 3 / `C` and 0, the last row being `C + 1` wide), **Up / Down** change the row: Up from the top visible row scrolls up, wrapping to the last row; Down from the bottom visible row scrolls down. The code has quirks the port keeps: a wrap to the first row clamps the column to `C`, and Down past the end of a rucksack of more than 16 things sets the absolute row to 0 while the visible row and scroll stay (the frame then sits on an empty cell, the chosen thing is item `column`). The capacity is 32 things (message 53), 16 on the screen. The grid starts with the cursor on the thing that was looked at (the original always zeroes the state first, i.e. on item 0; not compared). Debug aid: `CAD_CARRYN=n` carries n things.

**The Roland driver `ROLAND.BIN` (Cadaver 2, RNC-packed on disk, 3258 bytes unpacked; `tools/roland_midi.py`).** Same calling convention as the other drivers (entry 0 = 50 Hz tick, entry 4 = init, `[8]` music request, `[0Ah]` effect request; it also needed the interpreters' new `LOOPE` / `LOOPNE`, `E1` / `E0`). It talks to an MPU-401: init resets (`FFh` to port 331h, waits for the `FEh` acknowledge on 330h), then the tick writes complete MIDI messages to **port 330h**, each preceded by the 'send data to track *n*' command `D0h`–`D7h` on port 331h (`B1 79 00` = controller reset on channel 2, `C3 72` = program change, `91 3B 7F` / `81 3B 7F` = note on / off, ...; the MT-32 channels 1–8 are the instruments, channel 10 rhythm). **Music requests 1 and 2 produce music** (1634 and 682 messages in 40 s, the first is the long tune; 3 and 4 are silent), **effects 1–8** produce short note sequences. `roland_midi.py music|effect N out.mid [seconds]` writes what the driver sends as a Standard MIDI file (25 ticks per quarter note at 500000 µs, i.e. one tick = 20 ms); play it with an MT-32 / General MIDI synth. The port does not play it (SDL has no MIDI output; the AdLib and Sound Blaster drivers are synthesised).

**Sound Blaster digital samples (`SBLASTER.BIN`, 11136 bytes; `beeper.c`, `cadaver_sound sblaster digital:N`).** Besides FM the driver can play 8-bit samples through the DSP: `105BC` (al = sample number) looks up `[2B25h + 4·n]` = (offset, length) in the BIN, programs DMA channel 1 (ports 02h address, 03h count, 83h page; 0Ch clears the flip-flop) with the physical address of that data, sends DSP **40h** (time constant, `1 MHz / (256 − tc)`) and **14h** (single-cycle 8-bit output, length − 1), and sets its busy flag `[538]`; the DSP interrupt (vectors 0Ah / 0Bh / 0Dh / 0Fh, common handler `073C`, ends with IRET) clears it (and, when the sample crossed a 64 K DMA page, starts the remainder). Two samples exist: **0 = 4922 bytes at `17E3`** (a quiet, noisy sweep, about 80 ± 25 around 128) and **1 = 8 bytes at `2B1D`** which the init plays as its Sound Blaster test (time constant 156 = 10 kHz). The driver starts a digital sample when the effect request `[0Ah]` has bit 7 set (`80h | n`, n ≤ 2; if `[537]`, the 'DSP present' flag, is set and `[538]` is clear) or when a music stream carries the sample command (`ah` → time constant, `al` = sample); **CAD.EXE strips bit 7 from its effect numbers (`115C5`), so the game never requests sample 0 through an effect**, and no tune of the intro uses it. The port's `Beeper` emulates the DSP / DMA side (nearest-neighbour resampling, IRQ call at the end) and `cadaver_sound` renders `digital:N`; the interpreters got `IRET` (`CF`) for the handler.

**Event 25 (19h) of a cast spell item.** `1000:6896` queues the **room event `0x4018` (24, argument = the spell number A[0]: 23 room handlers in the levels react to spells cast in their room)** and, at `1691B`, **event 19h (25) for the spell item itself** before the TAD routine runs; level scripts use it to script spells whose TAD routine is empty: level 3's thing 805 (spell 1Ah, POLTERGEIST) and Payoff level 1's thing 279 (it tests the spell number and teleports to a room: `37 21 04 04 64`, `37 09 08 08 64`, `37 20 03 03 64`). The port queues event 25 for every cast item (`cast_spell_item`), which also makes every spell with an empty TAD routine consume its charge, play sound 06h / 11h and run its script; headless: casting thing 805 plays the script's sound 3Ah.

**Three more ACL ops (read from `CAD.EXE`).** Op 07 `7A77` `id16` puts a thing on the active list `ds:B8` (the inverse of op 08: the port clears A[5] bit 7), op 4Ch (76) `825A` `n` is a **condition: bit n of the shield mask `ds:6AB` is set**, op 5Dh (93) `8469` `id16` sets bit 2 of the thing's A[3] (marks a potion bad). **Op 52h (82, `8404` `n`) is ported: it removes every carried thing whose sprite / object id equals the byte n** — the iteration is `1415C` over list 8, whose 4-byte entries `13F35` fills with the thing id and the word `[record + 6]` (the id of the thing's sprite header, section 2); `13EFD` drops the entry (letting go of a held thing first). Level 2 room 87 and level 5 room 27 (and Payoff 2/3/4) use it on entering, with many ids in a row (level 2: 115, 140, 51–53, 93; level 5: 36, 39–41, 47, 48, 56, 57): the player is stripped of the listed items — in the port the thing is marked deleted (`CAD_OPS` logs `OP82`). Still skipped in a headless scan: a script of Payoff level 2 room 7 that desynchronises the op lengths (ops 96, 128, 242). Both ops tables (94 entries) have the same layout in the two games. Ops 4Ah (74, `7ADF`) and 4Bh (75, `7AF7`) are the checked forms of ops 07 / 08 (id16 validated by `17D26`, then A[5] bit 7 cleared + `15F18` / set + `15F2F`); level 1 room 31 uses op 4Ah. A scan of every room of both games (`CAD_OPS`, 100 ticks each) now skips no op. The Payoff level 2 room 7 "desync" is a data quirk of the original: **thing 500's contact script is `2D FF FA` (energy −6) with no `17h` terminator** — its event length (12) ends right at the A structure (`01 F2 00 79 …`) and the original's dispatcher (`call cs:[7865h + 2·op]`, no bounds check) would read the table slot of op 80h (`cs:7965` = `9C80h`, the middle of other code) and keep going; the port executes the ops up to the first op ≥ 94 and stops. `CAD_OPTRACE=1` prints every event run and every op with its operand bytes.

**Room zones and event 10 (`1000:11EB5` → `2022 … 2108`; ported as `roomview_zone_events`).** A room record whose byte `17h` has **bit 1** carries at `rec[rec[0]]` a box count *n* (1–4) and then *n* boxes of six signed bytes `x0 y0 x1 y1 z0 z1` (level 3 room 40: four 4 × 4 plates `(26,26,30,30,2,3)`, `(50,26,54,30,…)`, `(50,50,54,54,…)`, `(26,50,30,54,…)`). Every 17 game steps (the room-timer unit) each tracked thing — the original uses the collision-list entries whose flag byte `[+27h]` has bit 0 and (bit 2 or the room's `17h` bit 4); the port takes the thing's record flag `04h`, which all the level 3 puzzle things (333–336: the skulls / orbs placed on the plates) have — is tested against the boxes *n … 1* with its box `(x1, y1, x0, y0, zt, zb)`: per axis the ranges overlap when `max ≥ box max ? min ≤ box max : max ≥ box min` (**verified**: 400 random cases through the original routine `1000:2022` — the entry for four boxes; `205B`, `2094`, `20CD` are those for 3, 2, 1 — `tools/gen_zone_vectors.py`, `src/tests/vectors_zone.h`). **An overlap queues event 10 (0Ah) for the thing, argument = the box number**; its handlers test `room == current` and `zone == argument` (condition table type 10, now compared on both bytes). Used by levels 3 (23 handlers), 4 (28) and 5 (3: things 53, 60, 364 with zone 2 of room 33). **Rooms whose byte `17h` also has bit 4 (10h) — all 143 rooms of both games that have room handlers of type 15 or 17 — raise the room's own events as well** (`12108` after the thing event): for the **player** (the thing whose record id `[bx+4]` is 0) room event **15** (`0xC00F`, argument = the box number, extra argument = the player) and for every other thing room event **17** (`0xC011`, box number, the thing becomes the current object `ds:9C`); every thing is tracked in such a room. Examples: level 1 room 0 box 3 (the lake's edge: a thing entering it is removed, `17` → op 2), room 12 box 1 (the player entering it spawns thing 1BDh: a trap), room 4 box 1 (a thing in the keyhole alcove moves thing 1Ch). Ported (`roomview_zone_events`: the player box is `x1, y1, x0, y0, z + 29, z`); headless check: `--sim tp:15:68` in level 1 room 12 raises event 15 and thing 445 (1BDh) appears at (23, 25) and walks. Debug aid: `CAD_ZONES=1` prints every overlap, `CAD_MOVE=thing:x:y:room`.

**The carried-thing verb list and the verbs 12, 15, 16 (`1000:12C72`, `3381`, `344D`, `2E40`; corrects the earlier reading of verb 15).** The list for a carried thing is built in this order: with a thing touched ahead (`ds:58C`): **16** when the carried thing is a spell with A[7] = 2 (cast on the touched thing); then by the touched thing's class (`ds:6D3` = header byte 16h): class 8 (container) → **12** when its A[4] has bits 4 *and* 0 (a container that is open and takes things), class 0Bh → **12**, class 0Ch → **15**; then the class verb (`ds:FA36`, not 3 = open), look (11, not for header 17h = 6 or 16h bit 7), **13 hold (always)**, **1 drop**, 14 when header byte 1Dh = 12h, and 6. So a potion shows `9 11 13 1 6`, the diary `8 13 1 6`, the pickaxe `11 13 1 6`. Handlers: **16** queues event 1 for the touched thing (argument: the carried thing) and then runs the spell routine `1689C` with `ds:6A8 = 1`; **15** queues event 26 (1Ah) for the touched thing with the carried thing as argument and removes the carried thing from the rucksack (`13509`); **12** (class 8: when the container's content word A[0..1] is non-zero message 27 `NO ROOM`, otherwise A[0..1] := the carried thing's id, the thing leaves the rucksack, the lid bit A[4] bit 0 is cleared and the sprite redrawn if A[4] bit 3) plays sound 1Eh and queues **event 18 (12h)** for the container / thing with the carried thing as argument (37 handlers in level 3: items put into receptacles). Ported in `ruck_show` / `ruck_step`.

**Event 4: a thing comes to rest on another (`1000:16B02`, read from the code; ported in `fly_step`, not verified in play).** After a movement step of a thing (a drop that falls, a throw, gravity) the routine plays the impact sound `ds:FEC6[sprite byte 1Dh]` and, for each thing it has come to rest on (its base at the support's top: `cmp ch, [bp+4]`), queues **event 4 (04h) for the supporting thing with the landed thing's id as argument** (condition type 4: operand word == argument: level 1 thing 108 reacts to thing 45 by placing things 110 and 371, then removing itself; levels 2–5 and Payoff 1–3 have 9 / 17 / 28 / 7 / 5 handlers), and event 9 (contact) for each pair that touches. The drop itself (`12F1C` → `16C72` → `116E0`) is refused when anything overlaps the spot, so event 4 comes from things that *fall* onto others. The port raises event 4 for every thing the landing test blocks on. **Event 11 (0Bh) is the fall reaching the floor**: `16D3A` is the jump / fall step (`ds:75C` walks a z-profile table ending in `'P'`, `add [bx+2], al` adds it to the thing's z): when z becomes exactly 0 while falling it queues event 11 for the thing (level 1 room 53's things 114 / 115 / 117 / 120 / 198 / 452 / 453, level 2 room 86's 461–470 …: they count, show a message or remove themselves); ported in `fly_step`. The verb 5 handler (`2EF2`) is not ported; verb 10 (`336A`, grab) is, see below.

**The ending of level 5 (action routine `02C4` of the thing 109 in room 50, `ROUTINE5.TAD`).** After some services it loops `call 072D` (flush and poll the keyboard until a key: services `2Eh` = `9D8E`, `30h` = `1043`, then `cmp [715], 0`), calls service `3Ah` (= `1187`, the last service of the table: the game ends) and repeats. The port answers `30h` with a pressed key and lets `3Ah` end the routine instead of spinning (no ending screen is shown). `tools/scan_tad_errors.sh` now reports no failing routine in any room of either game.

**Running `1689C` on real level 1 items (emulator), what it does per mode.** Mode 0 (projectile, thing 27): sound 11h, A[2] 14h → 13h, cooldown `ds:629` := A[6], **no event queued**. Mode 1 (TAD routine, thing 87 FREEZE: effect timer 22h := the power 15; thing 161 MIND BLAST; thing 259 MAP: `ds:6C9 := 81h`): the room event `0x4018` (24) and the thing event `0x19` (25) are queued, the charge drops and the cooldown loads (A[6]); a spell cast with no creature in the room (mind blast `099E`) queues message 47 `YOU WASTED THAT SPELL`; the last charge queues message 15. Mode 2 without a touched thing (thing 132, 304, 371 …): only message 41 and nothing changes. The port follows this (`cast_spell_item`).

**The cooldown `ds:629` (found as `byte_1F799`).** `1000:02BD` (once per game step in `game_step`): `if (ds:629) ds:629--`. `6896` / `16AC9` load it with the spell's A[6], `69CF` with the weapon's A[3] (emulator: 5 after every shot of the level 1 weapons 23, 462, 470, the ammunition's shots A[2] dropping by one each); the fire handling (`0687 … 0701`) skips a held class 1 / 4 thing while it is non-zero, and because it tests the *held* fire bit (not a press edge) **a held spell or weapon fires again as soon as the cooldown has run out while End stays down**. Ported (`fire_cool`).

**Climbing and the halving retry (`1000:05D8 … 0674`, read from the code; ported in `walker_tick`).** After the player's step has been tested (`116E0`) and found blocked, the game looks at the result list (`ds:3C`: count, flag): **when at most two objects were hit and one of them is solid (flag `FDh`), the first one is examined: if its sprite header byte `0Ch` has bit 3 set (climbable) and `top + 1` (its box byte `[+4]`, +1) is less than 20 above the player's z, the step is tested again at z = top + 1; if free the player moves there** (`ds:76E` = the rise is added to z). Otherwise, with the doubled step of a SUPER FAST potion (`ds:60E` bit 0 clear, both deltas even) the deltas are halved and the step retried. Stairs are rows of such objects 16 high (level 2 room 5 things 715 → 714: z 0 → 16 → 32 in a headless walk); walking off the top makes the player fall (`below_free`).

**Verb 10 (the pushable-thing icon, offered when sprite header byte 0Ch has bit 0, `1000:336A`) is "grab" — ported (read from the code, not compared in play).** The handler stores the touched thing's record in `ds:74` and the animation class `ds:FED9[header byte 1Dh]` in `ds:6E2`. In `game_step` the grab is only looked at on the *free-step* path (`10674`): when the fire key is down it is dropped (`10687`); otherwise the player's box is tested one step **behind** (`dl − 2·ds:76F`, `dh − 2·ds:770`, i.e. the old position minus the step), the first collision entry must not be a wall (`[ds:3C+1]` ≥ 0) and the grabbed record must be among the blockers (`16C3F`); then the blocker group is moved by the step with the ordinary push machinery (`sub_16570` → list `ds:80`, `sub_167DF` adds the step to the position records, strength `ds:59C`) and the player takes the step too, so the thing is dragged along while the player walks straight away from it. Anything else drops the grab (`1084E`): a step that is not free (the old push path `10AAE` also clears it), a sideways step, a failed pull, a fall (`1089C`), a room change. While `ds:74` is set the walking animation uses the script table `ds:FA93` instead of `ds:FA60`: the frames of the *opposite* facing played backwards (up key: PCMAN 8, 9, 10, 11 = the down frames reversed; down: 15 … 12; left: 3 … 0; right: 4 … 7), so the player keeps looking at the thing. `main.c`: `Walker.grab` / `grab_rev`, set in `menu_choose` (verb 10), the pull in the walk step; in level 1 room 7 thing 65 can be grabbed (`--scene walk --level 1 --room 7 --sim right:60,left:120,up:20,end:2,wait:4,end:2,wait:4,down:40` with `CAD_SNDLOG` prints `PULL`).

**The rucksack strip and the look descriptions (emulator with two carried things, `1000:125CF`, `84D9`, `30FE`).** Choosing look (verb 11) on a carried thing (or Return on the highlighted look in the small view) shows the **whole rucksack as a strip**: a scroll at (16, 10) with `5·min(n, 4)` fill columns and `5·rows − 1` middle rows (4 things per row, at most 16 shown), each thing in a 35 × 35 cell at (23 + 40·col, 25 + 40·row) holding its first sprite frame (the same window as the picture of a describe: rings of colours 1 and 3 around the interior 14 for the **current** thing, a single ring of colour 4 for the others), and the description at x = 208 as before. Left / Right move the frame, fire selects the highlighted thing and returns to the small view with its verbs, Space leaves. The text of the look is, from `84D9` (strings of the LNG table): spell items (class 1) `THIS IS A <spell name | UNKNOWN> SPELL. IT HAS n CHARGES REMAINING AND A POWER OF m.` (strings 5, 9, 10; name from the table at `ds:44`), potions (class 2) `… POTION. IT HAS n DOSES REMAINING AND A POWER OF m` (11, 12), weapons (classes 4 and 80h) `THERE ARE n SHOTS LEFT EACH WITH A POWER OF m` / `UNLIMITED` / `YOU NEED A <ammo> IN YOUR RUCKSACK TO USE THIS WEAPON` / `THIS WEAPON IS LOADED AND READY TO USE` (16–21), chests (class 8) `THIS IS OPEN` or `THIS IS CLOSED AND LOCKED.` / `… AND UNLOCKED.` plus `YOU NOTICE SOME SORT OF TRAP` when A[5] ≠ 0 (22–25, 51); any other thing on the floor gives `YOU FIND NOTHING` (3), but a carried thing without an event-16 handler shows `THIS IS A <name>` (5 + name; seen for the diary). The carried verb lists measured with two things: the diary `8 13 1 6` (class verb 8, hold, drop, joystick — no look), the pickaxe `11 13 1 6` (look, hold, drop, joystick); Return / fire on the diary's 8 shows the strip and `THIS IS A DIARY` (not the status dialog of the floor verb); while the strip is open the verb slots show the list of what was touched before the rucksack was opened (`2 8 10 6` of the diary on the floor) and the right panel returns to "DAY n" / the thing's name / the room name. The port draws the strip, composes the chest and carried-thing texts and navigates it; the spell / potion / weapon descriptions and the original's scrolling beyond 16 things are not ported.

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
* **Doors in the wall columns (verified on levels 2–4, `roomview.render_shell(door_sprites=…)`, `room_render_shell_doors`):** wall tile ids
  `>= 0xC0` mark a door; the low nibble `n` selects sprite block `n−1` of MP1 section 2 (blocks 0–7: 8×16 / 16×8 door-opening boxes, and the real door
  graphics 48×57 in blocks 0/1: grille, face door …). The first frame is drawn at `(x, y−26)` from the leading tile's place (`x − 16` for doors on the left-hand wall,
  `x` on the right-hand wall), **after the floor but before all wall columns** (so the neighbouring pillars cover its edge); first-room frames of levels 2/3/4 went from
  97.4/93.4/91.8% to 98.6/97.3/93.4% equal to the original. Tiles `>= 0x80` below `0xC0` stay invisible.
* **Initial state is not just the file:** thing byte `+3` bit `0x80` means "not placed", but the game clears it for some things at start
  (level 4 room 0: the six carpets 704–709 covering the pit tiles 50–55 are entries in the original — with the flag still set in the file —, so the
  first screen shows an orange carpet where the file-only view shows an open pit). That state is set by the start-up scripts (ACL / TAD), not decoded yet.
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
  halving retry / climbing onto low solid objects: now modelled, see "Climbing" below.
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
