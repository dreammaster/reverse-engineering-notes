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

## 6. Level files — *unknown / partial*

`LEVn.MP1` (~110 KB unpacked), `MP2` (~25–40 KB), `MP3` (~55–63 KB),
`ROUTINEn.TAD`, `ANDATA.DAT` (animation data, 20 454 bytes), `PCMAN.DAT` (the
player sprite set, 41 936 bytes).

Facts established so far:

* All graphics are stored **EGA-planar**: the game keeps its sprite and tile
  bitmaps in VGA RAM (pages at `A000h/A200h/A800h`) and blits them with the
  latch-copy write mode (`rep movsb` with GC mode 1) and AND/OR masks —
  hence the planar layout of the on-disk bitmaps.
* The screen is isometric, 320×200, 16 colours, mode 0Dh, with a status panel
  (6 inventory slots, "DAY n", location name) drawn in the lower part.
* `ROUTINEn.TAD` (after the `mjm0` header; 16 zero bytes, then a small x86 stub
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
* The game also contains a bytecode interpreter (the in-house "ACL" language;
  INTRO's credits: *"cadaver uses an internal language … 'acl'. acl is copyright
  the bitmap brothers"*). Likely opcode handlers (small routines that fetch
  operands with `lodsb`/`lodsw` from SI and poke object fields via `DI`) sit in
  `CAD.EXE seg000` around `1000:7AE0–7C50`; **the link between the `.TAD` data
  and the interpreter is not yet confirmed.**
* `LEVn.MP2` starts with two big-endian dwords, then an array of
  `(size:u16be, offset:u16be)` records where `offset[n+1] = offset[n] + size[n]`
  (e.g. 0x20@0x68, 0x40@0x88, 0x2A@0xC8, …): a table of sub-blocks.
* After loading, the game places `MP1` at `2917:0000`, `ENG1.LNG` at
  `6C28:000A`, parts of `MP2` and `ANDATA` at other segments (measured in the
  emulator harness; see `docs/tools.md`).

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
