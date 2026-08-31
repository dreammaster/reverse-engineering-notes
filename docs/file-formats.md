# On-disk file formats

Every file in `C:\games\lota\` that the executables read or write, and
what's confirmed about each one's layout. Confirmed facts get cited by
the function/global that proves them; anything untraced is flagged as a
guess. See [overview.md](overview.md) for the code side and
[roadmap.md](roadmap.md) for what's still open.

Status (2026-08-31): the BSAVE container is confirmed, and the
**`.GLB` tile format + `.GMP` cell-map format are decoded** —
`decoders/glb_image.py` renders `TITLE` (the CGA title screen,
pixel-for-pixel), `SDMAP` (the SDEFENDR combat-arena screen frame), and
dumps the `SDOBJ.GLB` / `BJCHR.GLB` sprite atlases. `CHAR.DAT`'s
container (9 × 382-byte roster records) and its write/read path are
decoded; the record's scalar fields are partly identified. The
**dungeon data is decoded**: `DUNM1/2/3.BSV` tile maps fully (8 levels ×
16×16), and the `DUN.EXE` first-person **sprite/tile rendering model**
(the 4 LEGLIB primitives, the field-interleaved 8×8 cell, the
mask-list + BASIC-`PUT`-image sprite format) — so `DUNDATA.BSV`,
`DUNOBJ.BSV` and `DUNMONA/B.BSV` are understood structurally, with only
the last-mile `spriteBank` index arithmetic (needs a live dump) open.
The remaining `.BSV` files are still container-only.

## `.BSV` / `.GLB` / `.GMP` / `.BS1` / `.BS2` — Microsoft BASIC `BSAVE` images

Every one of these begins with the 7-byte Microsoft BASIC **BSAVE**
header:

| Off | Size | Field |
|---|---|---|
| 0 | 1 | `0xFD` magic (BSAVE marker) |
| 1 | 2 | segment (word, little-endian) — where the data was `BSAVE`d from / gets `BLOAD`ed to |
| 3 | 2 | offset (word) — usually `0x0000` |
| 5 | 2 | length (word) — bytes of payload following the header |
| 7 | … | raw payload, `length` bytes |

Verified: `file size == 7 + length` for `TITLE.GLB` (len `0x200A`),
`TITLE.GMP` (`0x0838`), `TOWN0.BSV` (`0x138E`), `CASTLE.BS1` (`0x32A0`),
`OUTDATA.BSV` (`0x3794`, offset `0x2B22`). So the game `BLOAD`s these
straight into fixed memory locations rather than parsing them — the
segment/offset in the header says where. Decoding each one is therefore a
matter of finding the `BLOAD` call site in the disassembly and seeing
what code then reads that memory region.

Naming convention (guessed): `.GLB` = "globals" (palettes / tile
graphics / object tables), `.GMP` = "game map", `.BSV` = generic BSAVE
(maps, monster/object tables, animations), `.BS1`/`.BS2` = paired
castle/fort layout banks.

## Inventory

| File(s) | Size | Used by (guess) | Kind (guess) |
|---|---|---|---|
| `TITLE.GLB` / `TITLE.GMP` | 8209 / 2111 | `MENU` | **fully decoded** — 512-tile 8×8 CGA sheet + 40×25 column-major cell map. See the dedicated section below; `decoders/title_screen.py` (or the generic `decoders/glb_image.py`) renders it. |
| `TOWN0.BSV`…`TOWNB.BSV` | ~5013–5123 | `TWNDR` | 12 town layouts |
| `CASTLE.BS1` / `.BS2`, `FORT.BS1` / `.BS2` | 12967 | `CASDR` | castle / fort layout banks |
| `TCASOBJ.BSV` | 4911 | `CASDR` | town/castle object table |
| `FORTANIM.BSV` | 263 | `CASDR` | fort animation |
| `OUTDATA.BSV`, `OUTOBJ.BSV`, `OUTM0.BSV`…`OUTM2.BSV`, `OUTDAT.DAT` | — | `OUT` | overworld map / objects / monsters |
| `DUNM1.BSV` / `DUNM2.BSV` / `DUNM3.BSV` | 2055 | `DUN` | **fully decoded** — dungeon tile maps. BSAVE → `0x2C07:0x0F3C`; payload 2048 B = **8 levels × 16×16 tiles, 1 byte/tile**. `0x00` floor, `0xFF` solid rock, `0x01`–`0x0F` features. `decoders/dun_map.py`. |
| `DUNDATA.BSV` | 5785 | `DUN` | **container mapped** — BSAVE → `0x2C07:0x173C`, i.e. **contiguous right after `DUNM*`** (`0x0F3C + 0x800`), so map + `DUNDATA` are one block in memory. 5778-B payload: header word (`0x0E94` = 3732) + a record/table area (`~0x000`–`0x4FF`) + a large `0x10`–`0x7F` byte region (`~0x500`–`0x167F`, ≈4.5 KB, likely the first-person view's wall/corridor tile-graphic bank) + an 18-B tail. Field meaning + its `BLOAD` site still open. |
| `DUNOBJ.BSV` | 9351 | `DUN` | **decoded** — dungeon objects + sprites. BSAVE → `0x140D:0x0DB6` into `spriteBank` (shared with `OUTOBJ`/`MUSOBJ`). ~5.6 KB real + zero pad: object records, then the `(maskSrc, screenDest)` pair table fed to `andSpriteMaskCell`, then the mask cells + `basPutSprite` image arrays. See the dungeon section below. |
| `DUNMONA.BSV` / `DUNMONB.BSV` | 14143 | `DUN` | **decoded** — dungeon monster sprites (two swappable sets). BSAVE → `0x140D:0x3236` into `spriteBank`. 14136-B payload = **exactly 6 records × 2356 B** = 6 monsters. Each = 20-B header (fixed 5-entry frame table) + 5 image frames (705/248/110/65/30 B = near→far, each a `basPutSprite` array) + ~5 mask frames + trailer. See the dungeon section below. |
| `DIS0.BSV`…`DIS15.BSV` (+ `DIS0A`, `DIS1A`) | 727–2055 | ? (`DIS9` → `CELDRV`) | "display" screens? ~18 of them. `celdrv_entry` BLOADs `DIS9.BSV` as one of the five ending image banks. |
| `CEL0.BSV`…`CEL3.BSV` | 1573 / 2597 | `CELDRV` | endgame-cinematic image banks — `celdrv_entry` BLOADs `CEL0`/`CEL1`/`CEL2` (loop), then `DIS9.BSV`, then `CEL3.BSV`, via `rt_FE07`, and relocates each one's internal offset table by its load segment. |
| `MUSDATA.BSV`, `MUSOBJ.BSV`, `MUSMSG.TXT` | 8055 / 12961 / 11229 | `MUS` | MUSEUM data / exhibit objects / message text |
| `SDMAP.GLB` / `.GMP` | 4113 / 2081 | `SDEFENDR` | **fully decoded** — 256-tile 8×8 CGA sheet + 41×25 column-major cell map = the combat-arena *screen frame* (ornate magenta viewport border around a black 3-D-view window, stippled background). Same format as `TITLE`; `decoders/glb_image.py SDMAP` renders it. |
| `SDOBJ.GLB` | 6161 | `SDEFENDR` | **tiles decoded** — 384 × 8×8 CGA tiles, same field-interleaved format as `TITLE`, **no `.GMP`** (bare sprite atlas). Content: approaching fireballs at ~6 scale steps, explosion / impact animation frames, cyan directional player-shot arrows, a horned enemy head. Per-sprite tile grouping TBD (needs the arena blit code). `decoders/glb_image.py SDOBJ` dumps the atlas. |
| `BJCHR.GLB` | 6161 | `GMB1` / `GMB2` | **tiles decoded** — card graphics ("BJ" = BlackJack). Same container as `SDOBJ.GLB` (6161 b, `{0x0A,6,1,0,1}`, bare atlas, no `.GMP`); 384 8×8 tiles, only 0–127 used. Holds a card **rank/suit glyph font** — `A 2 3 4 5 6 7 8 9 10 J Q K` + the four suit pips (♠♣ white, ♥♦ magenta), in **both upright and 180°-rotated** forms (the two opposite card corners) — plus card-back pattern + frame/corner tiles. `GMB1` BLOADs it into `seg004`; `GMB2` shares it. `decoders/glb_image.py BJCHR` dumps the atlas (use grid width 15 to line the font up). |
| `BIGNUM.DAT` | 420 | `GMB2` (at least) | large-digit font — `GMB2` (Flip-Flop Parlour) BLOADs it to render the GOLD / BET / winnings numbers |
| `D.BSV`, `R.BSV`, `PEGASUS.BSV` | 3527 / 3527 / 1159 | ? | ? |
| `CHAR.DAT` | 3444 | `MENU` / `SAVER` / all play modules | **container + write/read path decoded** — the character roster, which doubles as the in-progress save. 6-byte header + **9 records × 382 bytes**. Each record = 14-byte name + a 74-byte scalar block (a copy of resident LEGLIB DGROUP `ds:1AC0..1B08`) + 7 BASIC integer arrays. See the dedicated section below. `decoders/char_dat.py` lists the slots. |
| `LEGACY.DAT` | 2945 | `MENU` / `OUT` | **not a save/config file** — it's the game's **command / UI resource**: the A–Z keyboard-command name list (`Armor`, `Climb`, `Disembark`, `Fight`, `Gamespeed`, `Hold`, `Inventory`, `Leave`, `Magic`, `Open`, `Pass`, …) plus small 2 bpp CGA icon bitmaps. Same `05 06 xx xx 7E 01` header shape as `CHAR.DAT`. Field layout not yet mapped. |
| `OUTDAT.DAT` | 1012 | `OUT` | not checked yet |
| `DRCONFIG.DAT` | 1015 | `CONFIGUR` + all disk-loading code | **disk-drive layout**, not hardware config — which drive letter(s) hold the game floppies (or HD floppy / hard disk), so the loaders know where to look and can prompt for swaps. Written by `CONFIGUR.EXE` (`_main`); at offset near the start it holds a config-type byte (`'0'`/`'1'`/`'2'` — HD/hard-disk vs. 360K vs. 720K) and one or two drive-letter bytes. |
| `STDRVSCR.DAT` | 6192 | `STDRV` | "Stones of Wisdom" rules / instruction text — the walk-through the dealer narrates ("YOU AND THE DEALER BOTH RECEIVE FIVE DICE…", "THE LOSER OF A GAME GIVES UP ONE DIE…"). Read by `stonesOfWisdomMain`; **not** a story/cut-scene script. |
| `TWNMSG.TXT` / `MUSMSG.TXT` | 1911 / 11229 | `TWNDR` / `MUS` | plain-text message tables |

`LEGACY.BAT` (4 bytes: `menu`), `manual.txt`, and
`Passed.through.ANTiQUE.Shop` are not game data.

## `.GLB` + `.GMP` — tile sheet + cell map — **decoded 2026-08-31**

Fully decoded and rendered by `decoders/glb_image.py` (verified against
`TITLE` → the CGA title screen, and `SDMAP` → the SDEFENDR combat-arena
screen frame). `decoders/title_screen.py` is the earlier TITLE-only
prototype. Both files are BSAVE images; strip the 7-byte header, then:

**`.GLB`** — the tile sheet.
| off | contents |
|---|---|
| `0x00`–`0x09` | 5 header words. `word[0]` = `0x000A` (this header's size). The other four — `TITLE {8,1,1,1}`, `SDMAP {4,1,1,0}` — look like BASIC `DIM` bounds and are **not** needed to render. In particular `word[1]` is **not** the tile width (both sheets are 8-px tiles). |
| `0x0A`–end | a flat array of **8×8 CGA tiles, 16 bytes each**, 2 bpp / 4 colours (TITLE 512 tiles, SDMAP 256). Stored **field-interleaved**: the 8 words are scanlines 0,2,4,6,1,3,5,7 (in that file order). Within a word, byte 0 = the left 4 px, MSB pair = leftmost. |

**`.GMP`** — the cell map.
| off | contents |
|---|---|
| `0x00`–`0x05` | 3 header words `{0x1A, 0x11, 0x1A}` (`0x1A` = this header's size) |
| `word[3]` | **ROWS** — tile rows = words per column (`TITLE` 26, `SDMAP` 25) |
| `word[4]` | **COLS** — tile columns (`TITLE` 40, `SDMAP` 41); `COLS*8` slightly overruns the 320-px screen, so the last column is padding |
| `0x0A`–`0x19` | the BASIC source variable name (`"title"` / `"sdmap"`) as wide chars, zero-padded |
| `0x1A`–end | the cell array: **COLS columns × ROWS words, COLUMN-MAJOR**. Cell word `W` → tile index `W // 8` (the blitter does `src = tiledata + W*2`, tile stride 16; every `W` seen is a multiple of 8). |

Screen: CGA 320×200 mode 4, palette 1 (0 black / 1 cyan / 2 magenta /
3 white); even scanlines at `B800:0000`, odd at `B800:2000`.
`menu`'s `scrollTitleImage` re-blits `TITLE` column-shifted each music
tick (`titleScrollX`, step 40, wrap 160) for the slow horizontal drift.
`SDMAP` is an ornate magenta viewport border (hanging tab, top/bottom
latch details, side handles) around a black 3-D-view window, over a
stippled CGA background — tiles 17–20 are the intentional stipple.

`SDOBJ.GLB` and `BJCHR.GLB` ship without a matching `.GMP` — they are
raw **sprite atlases**, not screen layouts. Same 16-byte
field-interleaved 8×8 tile format (384 tiles each), but with no cell map
the client code (SDEFENDR's arena engine, GMB1/GMB2's card renderer)
indexes runs of tiles directly, and the per-sprite tile dimensions
aren't recovered yet. Their header is `{0x0A, 6, 1, 0, 1}` (`word[3]` =
0 vs. 1 on the two screen images — possibly the "has cell map" flag).
`decoders/glb_image.py <NAME>` dumps either as a tile grid (default 24
wide; pass a 4th arg for a different width).

- **`SDOBJ.GLB`** — approaching fireballs at ~6 scale steps, explosion /
  impact animation frames, cyan directional player-shot arrows, a horned
  enemy head.
- **`BJCHR.GLB`** — only tiles 0–127 are used. A card **rank/suit glyph
  font**: `A 2 3 4 5 6 7 8 9 10 J Q K` + the four suit pips (♠♣ white,
  ♥♦ magenta), in **upright and 180°-rotated** forms (for the two
  opposite corners of a card), then card-back pattern + frame/corner
  tiles. Grid width 15 lines the four suit rows up.

## `CHAR.DAT` — character roster / save — **container decoded 2026-08-31**

`CHAR.DAT` is the whole save system: the character roster *is* the
save-in-progress (there is no separate save file). The game `OPEN`s it
as a **BASIC random-access file, record length 382**, and `GET`/`PUT`s
record `rosterIndex + 1`.

| off | size | contents |
|---|---|---|
| `0x00` | 4 | `05 06 07 00` — purpose unconfirmed. `LEGACY.DAT` opens the same way with `05 06 04 0A`, so it's a shared BASIC-data-file marker, maybe a format/version tag. |
| `0x04` | 2 | `7E 01` = `0x017E` = **382**, the record length |
| `0x06` | 9 × 382 | the 9 roster records (`6 + 9*382 == 3444` = file size) |

**Each 382-byte record** (FIELDed by `SAVER.EXE` `saveRosterToDisk` /
`MENU.EXE` `enumerateRoster`):

| off | size | field |
|---|---|---|
| `+0x000` | 14 | **name**, space-padded. `"empty"` marks an unused slot (all 9 are `"empty"` in a fresh install). Matches the "up to 14 letters long" name prompt. |
| `+0x00E` | 74 | **scalar block** — 37 words copied verbatim from the resident LEGLIB DGROUP range `ds:1AC0`–`ds:1B08` (peeked word-by-word via `rtm_FE35`, written via `rtm_AB`). Identified so far (record offset): `+0x1C` dword ≈ experience (`ds:1AC2`); **`+0x20` dword `partyGold`** (`ds:1AD2`, = 20 for a new character); `+0x28` word `hitPoints` (`ds:1ADA`); `+0x3E` word `intelligence` (`ds:1AF0`, cap 28). `playerX`/`playerY` (`ds:1B02`/`1B06`) are also in this block. |
| `+0x058` | 294 | **7 BASIC integer arrays** — DGROUP array descriptors at `ds:1B0C`, `1B3A`, `1B68`, `1B96`, `1BC4`, `1BF2`, `1C20` (stride `0x2E`), each written/read element-by-element by `rtm_FE39` / `rtm_FE37`. Contents (inventory, spells known, quest / map-visited flags, museum coins, per-category stats …) not yet split — needs a populated save and the array `DIM`s traced in LEGLIB. |

Because the roster block lives in LEGLIB's **resident** DGROUP, it
survives the `OUT`↔`DUN`↔`TWNDR`↔… EXE chaining — which is why
`partyGold` / `hitPoints` / `playerX` sit at the same DGROUP offsets in
every play module (see [overview.md](overview.md)). Loading a saved
game = `MENU` `GET`s the record into `ds:1AC0`+ and chains to `OUT`;
saving = `SAVER` `PUT`s `ds:1AC0`+ back. The "is not on this / character
disk" / "empty" strings in `SAVER` are the removable-character-disk
handling.

`decoders/char_dat.py` reads the container and lists each slot
(name + the identified scalar fields).

## Dungeon data — `DUNM*` / `DUNDATA` / `DUNOBJ` / `DUNMON*` — **decoded 2026-08-31**

`DUNM1.BSV` / `DUNM2.BSV` / `DUNM3.BSV` (one per dungeon group) each
`BSAVE` to `0x2C07:0x0F3C`. The 2048-byte payload is **8 dungeon levels,
16×16 tiles, one byte per tile**, row-major (`8 × 256`):

| byte | meaning |
|---|---|
| `0x00` | open floor / corridor |
| `0xFF` | solid rock — the maze walls |
| `0x01`–`0x0F` | special-feature tiles: doors, up/down stairs, traps ("POISON GAS VENT", "FLOOR HOLE", "SLIME SPLOTCH" — names in `DUN.EXE`'s string pool), treasure, level links. The code→feature table is inside `DUN.EXE` and not mapped yet. |

`DUN.EXE` binds this array at DGROUP `ds:1E2A` and indexes
`base + level*0x100`; bytes `≥ 0x10` are walls/normal, `< 0x10` index the
feature table. `decoders/dun_map.py DUNM1` prints all 8 levels.

`DUNDATA.BSV` `BSAVE`s to `0x2C07:0x173C` — exactly `0x800` bytes after
the map, so `DUNM* + DUNDATA` load into **one contiguous DGROUP array**
(`dungeonMapArray`, `ds:1E2A`). `DUN.EXE`'s `blitViewCell` reads
wall/floor panels from this array: `dungeonMapArray[idx]` gives a base
offset + a `(rowCount << 8 | colCount)` word, and the panel is drawn as
a stack of horizontal 8×8-cell runs (see the rendering-model section
below). So `DUNDATA`'s ~4.5 KB `0x10`–`0x7F` region (`~0x500`–`0x167F`)
is the wall/corridor **tile-index lists** (one byte per 8×8 cell,
`0xFF` = transparent), and the 8×8 **tile bank** (16-byte
field-interleaved cells) is the rest of the payload. Exact sub-offsets
still need `blitViewCell`'s index math walked with live data.

### The dungeon-view sprite & tile rendering model — **decoded 2026-08-31**

`DUN.EXE`'s `bmDUNG` segment (`renderDungeonView`) draws the first-person
corridor by calling four LEGLIB primitives, all now identified:

| primitive | what it does |
|---|---|
| `sub_1FED8` | the atomic **8×8 CGA cell copy**: `movsw` ×8 with `+0x4E` steps and a `+0x1F0E` jump after the 4th word (even field → odd field). This is **byte-identical to the `.GLB` field-interleave** — 8 consecutive words = scanlines 0,2,4,6,1,3,5,7. *Every* 8×8 graphic in the game uses this layout. |
| `drawTileRun` (`rtm_FE2A`) | draw a horizontal run of `count` cells from a **byte tile-index list**; `0xFF` = skip (transparent); cell `N` is the 16 bytes at `srcBase + N*16`. Drives all wall/floor bands via `blitViewCell`. |
| `andSpriteMaskCell` (`rtm_FE2E`) | sprite **mask** pass: `and es:[di], word` for one field-interleaved 8×8 cell (8 words). `drawViewSprite` calls it once per masked cell, walking a list of `(srcOffset, destOffset)` word pairs. |
| `basPutSprite` (`rtm_61`, + `basPutSpriteXor` `rtm_60`) | stock Microsoft BASIC `PUT (x,y), array` — draws a **GET-array bitmap**: `dw widthPx ; dw heightPx ; planar CGA rows`. Honours `ds:250h/252h` origin, `ds:2D0h/2D1h` horizontal-flip, and an AND-vs-OR row-drawer. `drawViewSprite` uses this for the sprite **image** after `andSpriteMaskCell` lays the mask. |

`drawViewSprite` reads `viewObjectArray[8 + objType]` (`ds:1C7C`) → a
**depth band 0–4** (`−6` wraps if `> 5`), then per band uses hard-coded
`spriteBank` (`ds:1E58`) offsets + screen-Y / height / clip-rect
constants (the P-switch: mask-header word offsets `0x23/0x4B/0x73/0x9B/
0xC3` step `0x28`; clip rects like `0x717`,`0xE1F`). It then runs the
mask loop (`andSpriteMaskCell` × count) and the image (`basPutSprite`).

So each dungeon **sprite** = a mask (list of `(src,dst)` 8×8-cell pairs,
AND-blitted) + an image (BASIC `PUT` array, `[dw w][dw h][rows]`).

### `DUNOBJ.BSV` — dungeon objects + sprites

`BSAVE`s to `0x140D:0x0DB6` into `spriteBank` (`ds:1E58`) — the **same
array** `OUTOBJ.BSV` / `MUSOBJ.BSV` load into, plus `DUNMON*` at
`0x3236`. Payload 9344 bytes; only ~`0x1600` is real. Four regions:

| region | span | contents |
|---|---|---|
| A | `~0x000`–`0x3FF` | object-definition records (`dw 0x0110 ; dw V ; dw 0 ; dw V ; dw K`) |
| B | `0x400`–`0x8F1` | ~316 four-byte records = the **`(maskSrcOffset, screenDestOffset)` pairs** fed to `andSpriteMaskCell` (first of each pair steps by `0x10` = one 8-word cell) |
| C | `0x8F2`–`0x15FF` | the sprite data the pairs point at: field-interleaved 8×8 **mask cells** + `basPutSprite` **image arrays** (`[dw w][dw h]` + planar rows). Renders as noise only because it isn't a flat bitmap — it's these two indexed structures. |
| D | `0x1600`–end | zero padding |

### `DUNMONA.BSV` / `DUNMONB.BSV` — dungeon monster sprites

`BSAVE` to `0x140D:0x3236` (into `spriteBank`, after `DUNOBJ`). 14136-byte
payload = **exactly 6 records × 2356 (`0x934`) B** — 6 monsters.
`DUNMONA` / `DUNMONB` are two swappable sets (~74% of bytes differ).

Each record: a 20-byte header (`dw 9` + the 5-entry frame table
`0x2D5/0x3CD/0x43B/0x47C/0x49A` — identical across all 6, so frames are
fixed-size + `dw 0xA4` `dw 0x44`), then **5 image frames** of
705/248/110/65/30 bytes (the monster at 5 view distances, near→far —
each a `basPutSprite` array), then **~5 mask frames** (`~1158` B), then a
per-monster trailer. Pulling the exact `[dw w][dw h]` off each frame
needs `drawViewSprite`'s `spriteBank` index math resolved against a
live dump (some indices point at a runtime-built header at the array
start) — but the frame *format* is now known: BASIC `PUT` image +
`(src,dst)` mask-cell list.

## Screen-string pool (in the `.EXE`, not a file) — decoded 2026-08-30

Every module keeps its on-screen text as a **pool of string-constant
records**, each:

```
dw  length            ; 1..~200
dw  dgroup_ptr         ; near offset of the descriptor in the DGROUP segment
db  length bytes       ; the raw text
(db 00 or F4)          ; 0-1 byte, to word-align the next record
```

Once a module is unpacked (see [overview.md](overview.md#packing)) the
pool **is** the DGROUP segment — for both menu (`seg003:~0x2150`+) and
out (`seg003:~0x2150`+). `dgroup_ptr` is self-relative (the text is the
4 bytes right after the descriptor); code does
`mov ax,<descriptor_addr> / push ax / call basStrAssign`.

(In the *still-packed* image, DGROUP is BSS and the pool sits as an
initialiser list in the code segment's tail with `dgroup_ptr` = the BSS
destination — which is why the un-relocated far pointers and un-copied
DGROUP made the packed `out.idb` unreliable. Unpack first.)

`ida_scripts/dump_strings.py` walks the pool, maps each record to the
code sites that reference it, and (with `ANNOTATE = True`) writes the
text as an inline comment at each `mov reg,<descriptor_addr>`.

### In-string control codes (interpreted by `drawString` / `rtm_FE26`)

The text is not plain — a handful of punctuation characters are
directives:

| code | hex | meaning |
|---|---|---|
| `%` | `0x25` | newline. A leading run (`%%%`) positions the text N lines down; mid-string `%` breaks a line. |
| `@` | `0x40` | column / cursor-position marker (leading). |
| `!` `#` `$` `&` | `0x21` `0x23` `0x24` `0x26` | trailing paragraph / page-break / wait-for-key directives (exact split TBD). |

What looks like a 2-byte prefix in a hex dump (`N,` `j-` `B.` `F'`) is
just the low+high byte of the *previous* record's `dgroup_ptr` shown as
ASCII — not a code.

### What's in each pool

`MENU.EXE` `seg003:0x2150`+ (~4 KB): main-menu items, GAME CREDITS
(Doughertys / Al DeYoung / Tumanis / Stechow / Miller / Seelhoff /
Klonaris / Luzenski), SIMPLE INSTRUCTIONS / COMMANDS / CHARACTER
MOVEMENT, character-management prompts, the "poor peasant on the world of
Tarmalon" intro, MML music strings (`t120l4cl8ef…`).

`OUT.EXE` `seg003:~0x2150`+ (~140 records): overworld messages — terrain
("GRASSLANDS", "A FOREST", "THE MOUNTAINS", "THE WATER"), travel/raft
("THE RAFT MUST STAY IN THE WATER.", "YOUR RAFT SINKS."), combat
("ATTACKED BY ", "YOUR ATTACK MISSES.", "ENEMY HIT BY BLOW OF "),
shop ("DO YOU WANT TO BUY", "MUSEUM COIN FOR "), death ("YOU DIED!!!",
"THE POWERS OF THE MUSEUM / RESURRECT YOU FROM THE GRAVE!!"), the museum
access code ("World- / Stone- / Ring- "), and the chained-EXE names.
