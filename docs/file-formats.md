# Ultima II (DOS) — File Formats

Sources: [ModdingWiki](https://moddingwiki.shikadi.net/wiki/Ultima_II:_Revenge_of_the_Enchantress)
pages (fetched 2026-08-17), cross-referenced against `ultima2.asm`. Where
the wiki and the disassembly agree, that's a strong signal the format
notes are correct; where they don't (yet) connect, that's a TODO.

All files are accessed through the shared `access_file` routine
(asm line 17318) — see [overview.md](overview.md#the-inline-data-after-call-trick)
for the inline-filename calling convention. `access_file` builds a
standard DOS FCB (`_picData` in segment `sg08e3`) and drives it with FCB-style
`int 21h` calls — `AH=0Fh` open, `AH=1Ah` set DTA, `AH=27h` random block
read, `AH=28h` (asm shows as decimal `40`) random block write, `AH=10h`
close. So every format below is read/written in fixed-size blocks via
FCB random-block I/O, not stream I/O.

## `mapx??` — Planet/Town/Dungeon maps

- **No header.** Raw byte grid, no metadata.
- Planet/town maps (file number ends in 0-3): **64×66 cells**, 1 byte per
  cell, **4224 bytes** total. Rendered as 1024×1056 px at 16×16 px/tile,
  "CGA Linear" tile encoding.
  - **Tile ID encoding quirk**: the byte stored in the file is the real
    tile ID **× 4** — divide by 4 to get the index into the tile lookup
    (e.g. tile 4 = mountain is stored as `0x10`). Worth confirming this
    against whatever code in `canMoveToTile`/`draw_map` reads map bytes —
    look for a `shr`/`and` by 2 bits.
  - The map file does **not** encode which specific NPC/monster/signpost
    occupies a cell, only terrain — the game patches monster/NPC positions
    directly into the file at runtime (see `monx??` below, and note the
    wiki's summary conflates "positions saved into map files" with the
    separate monster files we can see in the disassembly — `load_map` and
    `save_game1` both access `mapx??` *and* `monx??` as two distinct
    files each time. Treat the wiki's single-file claim as imprecise;
    trust the disassembly: two files).
- Dungeon/tower maps (file number ends in 4-5): see Dungeon Format below,
  same base filename scheme.
- 64 known tile types (IDs 0-63): terrain, structures (castle/tower/town),
  creatures, objects (ship/sword/shield/etc).
- Filename on disk: `MAPXnn` where `nn` are the two ASCII digits of the
  map number, e.g. `MAPXFF` seen throughout the disassembly is a
  placeholder that `load_map` (asm 6618) patches in-place from
  `player._mapNum1`/`_mapNum2` before each `access_file` call:
  ```
  clc
  mov al, player._mapNum1
  adc al, '0'                    ; digit -> ASCII
  mov byte ptr cs:aMapxff+4, al  ; patches the 'F' at offset 4
  mov al, player._mapNum2
  adc al, '0'
  mov byte ptr cs:aMapxff+5, al  ; patches the 'F' at offset 5
  ```
  i.e. the inline filename literal is a template that gets self-modified
  before the following `call access_file` executes.

## Dungeon/tower format (`mapx??`, number ends 4-5)

- **256 bytes per level** (16×16 cells, 1 byte/cell).
- Levels stored **sequentially** in one file; level 0 = top for dungeons,
  bottom for towers, depth increases with level number.
- Minimum file size 4096 bytes = 16 levels (towers/dungeons apparently
  always have a fixed 16-level allocation regardless of actual depth
  used).
- Rendered as 17×17 with implied outer walls (not stored).
- Per-cell tile encoding (single byte):

  | Byte | Meaning |
  |---|---|
  | `0x00` | Floor |
  | `0x10` | Ladder up |
  | `0x20` | Ladder down |
  | `0x30` | Ladder up+down |
  | `0x40` | Chest, or tri-lithium on the final level |
  | `0x80` | Wall |
  | `0xC0` | Door |
  | `0xE0` | Secret door |

  Note these are all multiples of 0x10 in the high nibble — looks like a
  bitfield/category encoding (low nibble likely unused or a sub-variant).
  Worth checking dungeon-rendering code for a `and 0F0h` or `and 0E0h`
  mask to confirm.

## `monx??` — Monster/NPC data (ModdingWiki: "Ultima II Monster Format",
not yet fetched in detail — TODO)

- Same `nn` map-number suffix scheme as `mapx??`, patched identically —
  see `save_game1` (asm ~6700-6708) patching both `aMapxff_0+4/+5` and
  `aMonxff+4/+5` from the same two `player` digit fields in the same
  routine.
- Loaded/saved as a companion file alongside the matching `mapx??` —
  always the pair (map file, then monster file) in both `load_map` and
  `save_game1`.
- Byte-level layout not yet pulled from the wiki — fetch
  `Ultima_II_Monster_Format` next session if it exists as its own page
  (the summary page links to it under "monx??" but we haven't fetched
  that specific page yet).

## `tlkx???` — shop/NPC response text (wiki: "NPC dialogue")

- **Actual size for this DOS port: 256 bytes, confirmed against the
  disassembly** (the wiki's 384-byte figure does not match this port —
  see discrepancy writeup below, settled, not still open).
- Byte 0: null marker/header byte.
- Then a run of **null-terminated ASCII strings**, indexed and printed
  one at a time by `print_indexed_shop_string` — see the consumer trace below for how
  many and what selects which one; likely four given the wiki's
  framing, not independently confirmed in the disassembly.
- **Encryption**: every byte is ROT-128, i.e. `stored = plaintext + 128
  (mod 256)`; decrypt with `stored - 128`. (Wiki phrases this as "rotated
  by 128, the length of the ASCII set" — functionally XOR/add with the
  high bit, so `stored = plaintext | 0x80` for 7-bit ASCII plaintext,
  equivalently `plaintext = stored & 0x7F`.) **Located**: there's no
  dedicated decrypt step — see the consumer trace below, it's folded
  into the universal character-output routine.
- Display constraint: max 3 rows × 30 chars; wraps at 30 chars (CR/`0x0D`
  implied if not explicit); CR is the line terminator, `0x00` ends the
  whole string.
- **Loaded by**: `load_talk_file` (asm 6631-6649, formerly `sub_122D5`).
  Called from `enter` (the `E` command) only when walking onto a
  VILLAGE, TOWN, or CASTLE map feature — *not* a standalone command; the
  earlier guess that this was reachable via a dedicated Talk key was
  wrong (there is no Talk key — `T` is `transact`, i.e. shop/trade). See
  [overview.md](overview.md#resolved-a-z-command-jump-table-and-the-tlkxff-loader).

### 256-vs-384-byte discrepancy: traced and settled

The wiki says `tlkx???` is a fixed 384 bytes; the disassembly reads only
256. Three independent pieces of evidence confirm the disassembly's 256
bytes is deliberate, not a bug or partial read:

1. **The read itself.** `load_talk_file` does `mov ah,27h` (FCB random
   block read) / `mov cx,100h` / `call access_file`. Inside
   `access_file` (asm 13162), `_picData.record_size` is hardcoded to `1`
   for every file type it handles (not just this one) — so for `27h`
   reads, `cx` is a literal byte count, not records of some other
   implicit size. `cx=100h` really does mean "read 256 bytes," full
   stop.
2. **No second read exists.** A `27h` random-block read starts at
   whatever the FCB's `random_record` field says. That field is never
   written anywhere in the code (only declared in the `FCB` struct) —
   and `access_file` zeroes the entire FCB before every call — so every
   read always starts at record/offset 0. There is no code path
   anywhere that comes back and reads a further 128 bytes to complete a
   384-byte file.
3. **The destination buffer is only 256 bytes wide.** `load_talk_file`
   reads into `word_17886` (value `0x2800`), one of a small set of
   hand-managed buffer-pointer variables alongside `map_ptr` (`0x1800`)
   and `monsters_ptr` (`0x2900`). The gap from `0x2800` to the next
   buffer, `monsters_ptr`, is exactly `0x100` = 256 bytes — the game
   only ever reserved 256 bytes of scratch space here. A 384-byte read
   into this slot would silently corrupt the adjacent monster-data
   buffer; nothing in the code guards against or expects that.

**Conclusion**: for this 1983 DOS port, `tlkx???` files (or at least the
part of them the game actually reads) are 256 bytes, not 384. The
wiki's 384 most likely describes the 1982 Apple II original (different
platform/memory model) rather than this port, or is simply inaccurate.
Treat 256 as the confirmed figure for DOS `TLKXFF`-pattern files going
forward.

### Consumer traced: read out by `transact`, not a walk-up "talk"

Followed `word_17886`'s only other reader, `print_indexed_shop_string` (asm 3383-3435),
all the way through, and it closes out both "where's the decrypt loop"
and "where's the display code" at once:

- `print_indexed_shop_string(al = index)`: sets `si = index` (zero-extended), `di =
  word_17886` (the buffer `load_talk_file` filled in), then walks
  forward counting null (`0x00`) terminators, decrementing `si` each
  time, until it reaches the start of the `index`-th string in the
  buffer. It then prints that string byte-by-byte by calling
  `print_char` (a one-line wrapper: `call write_character; retn`) until
  the next null terminator.
- **There is no separate ROT-128 decrypt loop anywhere in the binary**
  (confirmed by grepping for `sub al,80h`/`xor al,80h`/`sbb al,80h`
  patterns — exactly one hit in the whole `.asm`, and it's unrelated,
  see below). Instead `write_character` (asm 12639, the single
  character-output primitive used *everywhere* in the game, not just
  here) unconditionally does `and al, 7Fh` on every character it prints
  (asm 12645), before anything else — mechanically identical to the
  wiki's decode (`plaintext = stored & 0x7F`). The "decryption" is an
  incidental side effect of the normal display routine, not a dedicated
  step, which is exactly why no decrypt code turned up in the earlier
  search: there isn't any to find.
- `print_indexed_shop_string` is called from exactly one place: `transact` (asm 10545,
  the `T` command), in the branch handling a shopkeeper-type monster
  (`[di+1D7h] >= 0x80` on that monster's record, where `di` points at
  the monster/NPC record for whatever's at the transacted-with tile).
  The high bit of `[di+1D7h]` gets stripped (`sbb al, 80h`, asm 10701 —
  the one `80h`-subtraction hit from the grep above, an item-index/flag
  mask, *not* a text decrypt) to get a small integer, which becomes the
  `index` argument to `print_indexed_shop_string`.

**Revised understanding**: the `TLKXFF`-loaded buffer isn't consumed by
a dedicated "walk up and talk to an NPC" interaction — there's no such
command (see the jump-table correction above). It's consumed when you
`transact` (`T`) with a shopkeeper-type NPC, as an indexed list of
null-terminated strings (item names / shop patter) picked out by that
NPC's monster-record byte. The wiki's "four null-terminated strings, the
four dialogue responses for that NPC" description is structurally
correct (index into a run of null-terminated strings) but its framing as
walk-up dialogue doesn't match this port — here it's shop dialogue,
loaded once per map at `enter` time and read back out during `transact`.
Full chain: `enter` (asm 9136/9159/9206, VILLAGE/TOWN/CASTLE) →
`load_talk_file` (loads 256 bytes into `sg08e3:0x2800`) → later,
`transact` on a shopkeeper NPC → `print_indexed_shop_string` (string lookup) →
`write_character` (prints, incidentally stripping the ROT-128 high bit).

## `pic???` — Full-screen CGA art

- Interlaced 320×200 CGA, used for the intro/demo sequences.
- Loaded through the same `access_file`/FCB path as everything else —
  `_picData` (the FCB instance) is literally named after this format,
  from earlier tentative work, even though it's reused for every file
  type. `access_file`'s special-casing of `_picData.filename == 'IP'`
  (asm ~17375, checking for "PI" reversed — i.e. filenames starting
  `PIC`) switches DS to `0B800h` (the CGA framebuffer segment) before the
  `int 21h` read, i.e. picture files are read **directly into video
  memory**, no intermediate buffer.

## `player` — Save game

- Single fixed file, format = the in-memory `Savegame` struct
  (`sizeof 0x100` = 256 bytes) dumped directly — see
  [overview.md](overview.md#savegame-asm-line-22-sizeof-0x100--256-bytes)
  for known fields. No separate wiki page found/fetched yet for this
  format specifically (ModdingWiki calls it "Ultima II Save Game
  Format").

## `ULTIMAII.EXE` — embedded overworld tiles

- Wiki: overworld tile graphics start at **file offset `0x7C43`**, **0x40
  (64) tiles**, consistent with the "64 distinct tile types" in the map
  format doc above.
- **Located and confirmed — found a better path than the wiki's file
  offset.** The IDB already has a fully-resolved `TILE_OFFSETS` table
  (asm ~19157, a 64-entry word array, one pointer per tile, xref'd from
  `draw_map_content` and `animate_water`) that gives the real per-tile
  addresses directly — no `ida_loader.get_fileregion_ea` translation
  needed (that approach was tried first; it landed 3 bytes into tile
  0's *pixel* data rather than its start, and assumed the wrong
  per-tile stride, so the resulting visualization looked like noise
  past the first tile — see git history on
  `ida_scripts/label_tile_graphics.py` for that dead end).
- **Confirmed layout** (counted `byte_17A40`'s declared bytes by hand,
  spot-checked headers on the first, middle, and last of the 64 table
  entries, all consistent): each tile is a **66-byte record**, tiles
  contiguous from `byte_17A40` (0x1080 bytes total for all 64):
  - Byte 0: `04h` — row width in bytes.
  - Byte 1: `10h` — row count (16).
  - Bytes 2-65: 64 bytes of packed pixel data, CGA Linear 2bpp,
    matching byte 0/1 exactly (16px/row × 2 bits ÷ 8 = 4 bytes/row × 16
    rows = 64 bytes) — i.e. the original "64 bytes/tile, CGA Linear
    16×16px" inference was right for the *pixel* portion, it was just
    missing this 2-byte header in front of each tile.
- `ida_scripts/label_tile_graphics.py` re-derives all 64 addresses from
  `TILE_OFFSETS` itself (working around a second IDA quirk found along
  the way — see the script's docstring: `idautils.DataRefsFrom` only
  resolves a stored xref for the *first* sub-word of a multi-value `dw
  offset a,b,c,...` declaration, not the other 63, despite IDA
  rendering `offset labelname` text for all of them; fixed by reading
  the raw word + segment base instead), verified the 66-byte-stride/
  header pattern holds for all 64 (not just the hand-spot-checked
  ones), and confirmed all 64/64 resolved cleanly.
- **Visually confirmed against real tiles**, not just the header/stride
  pattern. Dry-run output for `[0, 1, 6, 20, 40, 63]`, unpacked 2bpp and
  eyeballed:
  - Tile 0 (wiki: water) — a diagonal dash pattern repeating every 4
    rows, plausible stylized ripples.
  - Tile 1 (wiki: swamp) — a similar but distinct diagonal dash motif,
    consistent with another liquid/marsh terrain sharing the general
    "water-family" look but a different specific pattern.
  - **Tile 6 (wiki: town) — unambiguous**: two square towers with
    crenellated tops/bottoms (`#####  #####`), solid connecting wall
    bands (`############`), paired vertical walls between — a clear
    stylized twin-tower town/castle icon. This alone confirms the
    66-byte/`[04h,10h]`-header decode is correct.
  - Tiles 20, 40, 63 — not yet matched to wiki names, but all render as
    coherent small sprites (a bar/pole shape for 40, a branching
    diagonal for 20, a humanoid-ish figure for 63), not noise.
- Not yet applied (renamed) as of this pass — `DRY_RUN` still `True` in
  the script; flip it once ready to commit `tile_00`..`tile_63` names.
