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

## 4. `FONT2.SPR` — *partial*

992 bytes = 31 glyphs × 32 bytes; each glyph is 8 rows × 4 bytes
(one byte per bit-plane, bit 7 = leftmost pixel). Glyph order: `A`–`Z`
followed by punctuation (`. , ? -` …). The planes encode fill/outline/shadow;
exact colour assignment still to be pinned down from the text-drawing routine.

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

### 6.3 `ANDATA.DAT` — *container verified, contents unknown*

`u16be count` (6), `count × u16be` block sizes (1904, 15840, 576, 920, 1104, 96),
then the blocks back to back (the sizes tile the file exactly; `tools/mpsections.py`).
Block 0 is a table of 8-byte entries (e.g. `80EE 0040 7F30 80FE`, with
`FFFE 0000 0000 FFFE` as the empty entry); the 15 840-byte block is the largest.
Identical in Cadaver 1 and 2.

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
CELL 3`). Whether list *n* is room *n* is not established. `tools/things.py` dumps
them with names.

**Thing record layout** (compact form, big-endian, 16–132 bytes, mostly 16–44):

| Offset | |
|---|---|
| `+0`, `+1` | two bytes that differ per thing (`+1` is constant within groups of similar things — 55 for every thing in level-1 list 12 — so probably a type/class number; `+0` unknown) |
| `+2` | `u16` flags (values like `0020`, `1000`, `2040`, `4002` …; meaning unknown) |
| `+4` | `u16` **thing id** (= the record index = the name string id) — all levels |
| `+6` | `u16`, varies per thing (BOAT 41, STONE 89, SILVER COIN 82, WELL 46 …); unknown |
| `+8` | `u16` **slot offset = 70 × (position of the thing in its room list)** (verified for every thing of 8 of 9 levels; level 1 list 39 restarts at 0 for its 4th entry). So the loader gives each room an array of 70-byte object slots |
| `+10` | byte **room list number** the thing belongs to — verified for every thing in all nine levels (BOAT → 29, SILVER COIN → 27, DIARY → 68) |
| rest | variable tail (e.g. `16 16 16 00 06 90 48 …` on BOAT) |

(An earlier guess that `+6/+8/+10` were x/y/z coordinates was wrong: they are
bookkeeping — slot and room links — so positions must be in the tail or elsewhere.)

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
| 1 | 100 | 32 008 | per-room records of byte ids (e.g. `06 00 37 38 39 00 00 00 18 17 16 …`, 42–122 bytes); sizes track the floor map sizes — probably walls/room furniture; not decoded |
| 2 | 255 | 255 000 (255 × 1000) | 255 variable-size blocks (120 … 29 424 bytes) of big-endian words, e.g. `00 00 00 06 00 01 00 02 00 03 FF FF …`; not decoded |

Directory entries are `(size, offset)` with offsets contiguous in sections 1 and 2 of MP2; in
MP1 section 0 of level 1 the last entry is `(320, 3002)`, past `data_len` (so use `data_len`
for the section length). The trailer is a table of 8-byte records (`u16be id` + six
spaces, then zeros) followed by `u16be id, u32be value` pairs ending with
`0x4F 40000`, `0x50 45000`, `0x51 50000`, `0x52 60000` — probably per-object attributes
(prices/scores?) or a script-compiler symbol table; not decoded.

**Floor drawing** (verified; `room_draw_floor`, `CAD.EXE 1000:D1FA`): for each cell
`(col, row)` of the room, tile id `t = map[row*w + col]`; if non-zero the 32×18
tile `t` is drawn at screen position `(x0 + 16*(col-row), y0 + 8*(col+row))`
(video offset `col*0x142 + row*0x13E`), rows top to bottom, OR-blitted with its
mask. Rendering rooms this way reproduces the running game's floor
(`tools/roomview.py`, `src/engine/rooms.c`; identical output for every room of all 14
level directories, unit-tested).

### 6.6 `LEVn.MP3` — *floor tiles, verified*

```
u32be tile_bytes (17 280 in level 1 = 60 tiles)
tile_bytes / 288 tiles of 288 bytes; tile 0 is empty
rest of the file (~41 KB)   not decoded
```

A tile is 32×18 pixels: 18 rows of 16 bytes; each row = two 16-pixel groups of four
big-endian plane words (Atari-ST interleaved, plane 0 = colour bit 0 — like `.PC1`).
The level loader (`CAD.EXE 1000:B7B5…B86D`, reading 288 bytes per tile with `mjm_read`)
derives the mask as the OR of the four plane words and stores `mask + 4 planes`
(5 × 72 = 360 bytes per tile, the same layout as the player sprites) at
`36EF:000A + 360 * id`. Verified by finding the file bytes of tiles 1–3 in the RAM tiles
and by rendering all 60 tiles (isometric diamonds with an edge: water, stepping
stones, cave rock; in the Payoff paving and so on).

### 6.7 `ROUTINEn.TAD` and the script language

`ROUTINEn.TAD` (after the `mjm0` header; 16 zero bytes, then a small x86 stub
at offset `0x10`, then data): the stub is
`nop nop; push ds; mov ax,cs; mov ds,ax; mov si,0E5Dh; mov cx,10h; rep movsw;
pop ds; mov bx,27h; mov ax,2Ch; retf` followed at `+0x27` by
`call word [078Ch]; retf`. i.e. the blob is loaded as a code segment, the game
far-calls offset `0x10` to initialise it (it copies a 16-word table from the
end of the blob and hands back BX=entry thunk, AX=data-table offset) and later
calls the thunk, which dispatches through a pointer inside the blob. The bulk
of the file after the stub is **not** x86 (looks like 16-bit tables and
0xFFFF-terminated lists) — presumably the per-level ACL scripts / object
behaviour tables.

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
