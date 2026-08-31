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
mask-list + BASIC-`PUT`-image sprite format). The **overworld** files
`OUTM0/1/2.BSV` + `OUTDATA.BSV` have their architecture and region
layout mapped (they mirror the dungeon's `DUNM* + DUNDATA`); open there
is the last-mile bitmap-record packing. The **12 town layouts**
(`TOWN0..B.BSV`, 80×40), the **castle / fort floor maps**
(`CASTLE.BS1/2` 90×91, `FORT.BS1/2` 112×73), and the **museum**
(`MUSDATA.BSV`, 3× 16×16 exhibit maps) are decoded. What's left is
container/structure-only: the per-file tile-graphic banks, `TCASOBJ`
sprite-cell dims, and the monster/object sprite pixel packing.

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
| `TOWN0.BSV`…`TOWNB.BSV` | ~5013–5123 | `TWNDR` | **decoded** — the 12 town layouts. BSAVE → `0x9259:0x0000`. Each = an **80×40 tile map** (`0x000`–`0xC7F`, 1 byte/tile) + object/feature records + a `0x1A` slot table + the town's shop-name strings. `decoders/town_map.py`. See the town section below. |
| `CASTLE.BS1` / `.BS2` | 12967 | `CASDR` | **decoded** — the two castle floors. BSAVE → `0x86AE:0x0000`. Each = a **90×91 tile map** (`0x000`–`0x1FFD`, 1 byte/tile) + a per-floor table + a shared CGA 2 bpp tile-graphic bank (`~0x2400`+). `.BS1`/`.BS2` differ only in `0x000`–`0x20BF` (map + table); the graphics bank is shared. `decoders/town_map.py CASTLE.BS1`. |
| `FORT.BS1` / `.BS2` | 12967 | `CASDR` | **decoded** — the two Warlord's-fort floors, same layout as `CASTLE.BS*` but a **112×73 tile map** (`setViewport` mode 2). |
| `TCASOBJ.BSV` | 4911 | `CASDR` | **structure mapped** — castle/town animated-object sprites. BSAVE → `0x8537:0x0000` into `spriteBank` (`ds:1E58`) via `loadCastleObjects` — same array/role as `DUNOBJ`/`OUTOBJ`/`MUSOBJ`. See the castle section below. |
| `FORTANIM.BSV` | 263 | `CASDR` | fort-specific animation — BSAVE → `0x8537:0x1228`, i.e. it **overlays `TCASOBJ`'s last `0x100` bytes** (`0x1228`–`0x1327`), swapping the castle's animated-object frames for the fort's. |
| `OUTM0.BSV` / `OUTM1.BSV` / `OUTM2.BSV` | 9969 / 4191 / 2101 | `OUT` | the 3 overworld map layers (picked by `combatPhase` in the filename `OUTM0<phase>.BSV`). BSAVE → `0x86AE:0x0000`. Shared 8-byte header `62 24 0A 00 11 00 3A 23`; 1 byte/tile terrain codes (`0x2C` = default, `0x2D`–`0x32`, …); logical map is 128 tiles wide (`OUT`'s feature scanner does `idiv 0x80` / `and 0x7F`). |
| `OUTDATA.BSV` | 14235 | `OUT` | **structure mapped** — the shared overworld graphics + tables. BSAVE → `0x86AE:0x2B22`, i.e. **contiguous with the `OUTM*` layers in one array** (bound at `ds:1E2A`, exactly parallel to the dungeon's `DUNM* + DUNDATA`). See the dedicated section below. |
| `OUTOBJ.BSV` | 4395 | `OUT` | overworld object sprites — BSAVE → `0x13A8:0x0DB6` into `spriteBank` (same `0x0DB6` offset as `DUNOBJ` / `MUSOBJ`). Not separately decoded (same shape as `DUNOBJ`). |
| `DUNM1.BSV` / `DUNM2.BSV` / `DUNM3.BSV` | 2055 | `DUN` | **fully decoded** — dungeon tile maps. BSAVE → `0x2C07:0x0F3C`; payload 2048 B = **8 levels × 16×16 tiles, 1 byte/tile**. `0x00` floor, `0xFF` solid rock, `0x01`–`0x0F` features. `decoders/dun_map.py`. |
| `DUNDATA.BSV` | 5785 | `DUN` | **container mapped** — BSAVE → `0x2C07:0x173C`, i.e. **contiguous right after `DUNM*`** (`0x0F3C + 0x800`), so map + `DUNDATA` are one block in memory. 5778-B payload: header word (`0x0E94` = 3732) + a record/table area (`~0x000`–`0x4FF`) + a large `0x10`–`0x7F` byte region (`~0x500`–`0x167F`, ≈4.5 KB, likely the first-person view's wall/corridor tile-graphic bank) + an 18-B tail. Field meaning + its `BLOAD` site still open. |
| `DUNOBJ.BSV` | 9351 | `DUN` | **decoded** — dungeon objects + sprites. BSAVE → `0x140D:0x0DB6` into `spriteBank` (shared with `OUTOBJ`/`MUSOBJ`). ~5.6 KB real + zero pad: object records, then the `(maskSrc, screenDest)` pair table fed to `andSpriteMaskCell`, then the mask cells + `basPutSprite` image arrays. See the dungeon section below. |
| `DUNMONA.BSV` / `DUNMONB.BSV` | 14143 | `DUN` | **decoded** — dungeon monster sprites (two swappable sets). BSAVE → `0x140D:0x3236` into `spriteBank`. 14136-B payload = **exactly 6 records × 2356 B** = 6 monsters. Each = 20-B header (fixed 5-entry frame table) + 5 image frames (705/248/110/65/30 B = near→far, each a `basPutSprite` array) + ~5 mask frames + trailer. See the dungeon section below. |
| `DIS0.BSV`…`DIS15.BSV` (+ `DIS0A`, `DIS1A`) | 727–2055 | ? (`DIS9` → `CELDRV`) | "display" screens? ~18 of them. `celdrv_entry` BLOADs `DIS9.BSV` as one of the five ending image banks. |
| `CEL0.BSV` / `CEL2.BSV` (1573) · `CEL1.BSV` / `CEL3.BSV` (2597) · `DIS9.BSV` (1221) | — | `CELDRV` | **structure mapped** — the 5 frames of the "AGAINST ALL ODDS!" endgame cinematic. `celdrv_entry` BLOADs each (`CEL0/1/2` built as `"CEL"+n+".BSV"`, then `dis9.bsv`, then `cel3.bsv`) into `spriteBank` at word-slot `bank·2000 + 1000`, then **relocates the file's pointer table** by `+2·(bank·2000+1000)`. Each file: an 8-word header (`{v, 0x10, W, H, 0x20, 0x0A, 0x10, 0x220}` — `CEL0` W=`0x110` H=`0x78`), a relocatable pointer table from `~0x100` (`(stripPtr, ?)` pairs; `stripPtr` steps by `0x140` = 320 = a 4-even-scanline band, and consecutive column-groups step `+2` = +8 px), then RLE-packed CGA bitmap strips from `~0x300` (798 B of data for `CEL0`'s ~272×120 area ⇒ ~10:1, so compressed). `celFrame` cycles the 5 as an animation. |
| `MUSDATA.BSV` | 8055 | `MUS` | **decoded** — the Tarmalon Museum data. BSAVE → `0x2C1C:0x0F58` (near the dungeon's `0x2C07:0x0F3C`). `0x000`–`0x07FF` = **3 exhibit floor maps, 16×16 tiles** (`0x80` wall / `0x00`–`0x03` floor / `0x10`–`0x43` wall-edge / `0xE0`–`0xEF` = the 16 display-case portals) + 5 empty slots; `0x800`+ = a near-copy of `DUNDATA.BSV`'s dungeon-view tile/graphic data (`bmMUSDUNG` ≡ `bmDUNG`). `decoders/dun_map.py MUSDATA`. |
| `MUSOBJ.BSV` | 12961 | `MUS` | **decoded** — museum exhibit object sprites. BSAVE → `0x1447:0x0DB6` into `spriteBank`. Byte-for-byte the **same 3-region format as `DUNOBJ.BSV`**: object records (`0x000`–`0x3FF`), the `(maskSrc, screenDest)` pair table for `andSpriteMaskCell` (`0x400`–`0xFFF`, 768 pairs), then the sprite bitmap bank (`0x1000`–`0x329A`). A different, larger sprite set — otherwise identical structure and renderer (`drawViewSprite`). |
| `MUSMSG.TXT` | 11229 | `MUS` | plaque + exhibit message text |
| `SDMAP.GLB` / `.GMP` | 4113 / 2081 | `SDEFENDR` | **fully decoded** — 256-tile 8×8 CGA sheet + 41×25 column-major cell map = the combat-arena *screen frame* (ornate magenta viewport border around a black 3-D-view window, stippled background). Same format as `TITLE`; `decoders/glb_image.py SDMAP` renders it. |
| `SDOBJ.GLB` | 6161 | `SDEFENDR` | **tiles decoded** — 384 × 8×8 CGA tiles, same field-interleaved format as `TITLE`, **no `.GMP`** (bare sprite atlas). Content: approaching fireballs at ~6 scale steps, explosion / impact animation frames, cyan directional player-shot arrows, a horned enemy head. Per-sprite tile grouping TBD (needs the arena blit code). `decoders/glb_image.py SDOBJ` dumps the atlas. |
| `BJCHR.GLB` | 6161 | `GMB1` / `GMB2` | **tiles decoded** — card graphics ("BJ" = BlackJack). Same container as `SDOBJ.GLB` (6161 b, `{0x0A,6,1,0,1}`, bare atlas, no `.GMP`); 384 8×8 tiles, only 0–127 used. Holds a card **rank/suit glyph font** — `A 2 3 4 5 6 7 8 9 10 J Q K` + the four suit pips (♠♣ white, ♥♦ magenta), in **both upright and 180°-rotated** forms (the two opposite card corners) — plus card-back pattern + frame/corner tiles. `GMB1` BLOADs it into `seg004`; `GMB2` shares it. `decoders/glb_image.py BJCHR` dumps the atlas (use grid width 15 to line the font up). |
| `BIGNUM.DAT` | 420 | `GMB2` | **decoded** — the large-digit font for `GMB2` (Flip-Flop Parlour)'s GOLD / BET / winnings readouts. **No BSAVE header** — a raw **112 × 15 px CGA 2 bpp bitmap** (28 bytes/row × 15 rows = 420 B exactly; autocorrelation locks the stride at 28). One horizontal strip of ~10 digit glyphs (~11 px pitch), blitted a digit at a time by `drawBigNumberPanel`. `decoders/bignum.py`. |
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

`MUSDATA.BSV` is the museum's counterpart: 3 exhibit floor maps (16×16,
`0x000`–`0x07FF`) then, from `0x800`, its own copy of the dungeon-view
tile/graphic data — many 64-byte regions byte-match `DUNDATA.BSV` at a
~`-0x800` delta (the museum's `bmMUSDUNG` is the same renderer as
`bmDUNG`). Museum tile `0xE0`–`0xEF` = one of the 16 exhibit /
display-case portals `enterExhibit` routes on (to `TWNDR` / `DUN` /
`STDRV` / `CELDRV`).

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

## Overworld data — `OUTM*` / `OUTDATA` — **structure mapped 2026-08-31**

Same architecture as the dungeon: `OUTM0/1/2.BSV` (`BSAVE` →
`0x86AE:0x0000`) are the map layers, and `OUTDATA.BSV` (`BSAVE` →
`0x86AE:0x2B22`) loads `0x2B22` bytes later in the **same segment**, so
they're one contiguous array (bound at `ds:1E2A`).

**`OUTM0/1/2.BSV`** — the overworld map. `OUT`'s `loadOverworldData`
builds the name `OUTM0<combatPhase>.BSV`, so the three are alternate
world states (main map / post-event / endgame — sizes 9962 / 4184 /
2094 B). All share the 8-byte header `62 24 0A 00 11 00 3A 23`; the body
is 1 byte per tile, terrain codes clustered around `0x2C` (`,`, the
default) through `0x32`. The logical map is **128 tiles wide** — the
feature scanner unpacks a tile id as `y = id / 0x80`, `x = id & 0x7F`.

**`OUTDATA.BSV`** (14228-byte payload) — the shared overworld graphics
and tables:

| span | contents |
|---|---|
| `0x000`–`0x3FF` | terrain-type tables — bytes `0x10`–`0x6D` in ~4 rows of `0x100` (per-terrain-code data: appearance / movement / encounter). The `+0x200` row is all `0x41`–`0x4B`. |
| `0x400`–`0xDFF` | **terrain tile graphics** — a run of **95-byte (`0x5F`) records** (`OUT`'s tile renderer does `imul ds:2444h, 0x5F`); renders as coherent dithered forest/water/mountain texture. Each record is a small `basPutSprite`-style bitmap. |
| `0xE00`–`0x13FF` | zeros |
| `0x1400`–`0x1AFF`, `0x1F00`–`0x36FF` | **object / creature sprite banks** — a regular array of **124-byte (`0x7C`) records**, each `dw 0x28 ; dw 0x14` (40 × 20 extent) + 120 bytes of CGA 2 bpp data (a BASIC GET-array; the 120-byte body = 6 bytes/row × 20, so the stored bitmap is ~24 px wide — the `40` is a scaled/padded extent). |
| gaps + tail | zero padding |

`OUT` never calls `drawTileRun` — it scrolls a working tile buffer
(95-byte tiles, 13-unit row stride) with `rtm_FE1B` (`rep stosb`) /
`rtm_FE14` (`rep movsb`) and paints tiles/sprites via `basPutSprite`
(`rtm_61`). The exact 95-byte tile-record layout and the terrain-table
field meanings need `OUT`'s overworld draw path traced.

## `TOWN0.BSV` .. `TOWNB.BSV` — town layouts — **decoded 2026-08-31**

`TWNDR.EXE`'s `loadTownData` builds the name `TOWN<n>` and `BLOAD`s it
into the shared map array (`ds:1E2A`). `setViewport` mode 0 reads the
map with `mapStride = 0x50` (80) and `mapHeight = 0x28` (40); the map
size `0xC80` = 3200 is hard-coded (`ds:253Ah` / `ds:256Ah` / `ds:25D0h`).

| span | contents |
|---|---|
| `0x000`–`0xC7F` | **the town map — 80 wide × 40 tall, one byte per tile**, row-major. Tile codes (from `TOWN0`): `0x00` out-of-bounds, `0xA9` open ground, `0x0E`–`0x12` street/path, `0x48`–`0x4B` building walls, `0x67`–`0x6F` building interior, `0x3A` water, `0x3B`–`0x51` roofs, `0x5B` / `0x9C`–`0xD0` shop features, `0x70`–`0x89` / `0xB2`–`0xB6` decorations. The code→graphic mapping lives in `TWNDR` + `TCASOBJ.BSV`. |
| `0xC80`–`0xFFF` | object / feature records — short multi-tile runs (`2D B5 B6 0B 98` etc.) for the shop-front art |
| `0x1000`–`0x10BF` | a 192-byte table, all `0x1A` in `TOWN0` — likely door / NPC / shop-entrance slots (192 = 3 × 64) |
| `0x10C0`–`0x12FF` | zero padding |
| `0x1300`–end | the town's **shop / service names** as wide chars (`XX 00` per char), control-byte separated. This is the only part that varies in size between towns. |

The names are the game's flavour: `TOWN0` = FLUID MOTION / ICE TOUGH /
LIQUID ILLUSIONS / SAIL AWAY / CAPTAIN GREED'S (a port); `TOWN4` = AL'S
ARMS / ANTHONY'S ARMOR / HUBERT'S HOUSE OF HEX / FLOYD'S FLOATAWAY /
SOOTHING TOUCH / FELICIA'S FORTUNES / ED'S EASY MONEY; `TOWNA` = SAINTLY
SWORDS / ARMAGEDDON ARMOR / MIRACLE MAGIC / SAIL SALES / HOLY HEALERS /
BIZARRE BAZAAR / PROPHET FOR PROFIT (a temple town). Each town has the
standard set — weapons, armour, magic, healer, bank, fortune-teller,
tavern, boat sales.

## `CASTLE.BS1/2` + `FORT.BS1/2` — castle / fort layouts — **decoded 2026-08-31**

Same `bmTNCALB` renderer as the towns, bigger maps. `CASDR.EXE` sets
`mapStride` = `0x5A` (90) for the castle, `0x70` (112) for the fort
(`setViewport` modes 1 / 2). BSAVE → `0x86AE:0x0000`; 12960-byte payload:

| span | contents |
|---|---|
| `0x0000`–`0x1FFD` | **the floor map** — castle **90 × 91**, fort **112 × 73**, 1 byte per tile, row-major (autocorrelation confirms stride 90 for `CASTLE.BS1`). Same tile-code vocabulary as the towns. |
| `~0x1FFE`–`0x20BF` | a per-floor table (this is exactly where `.BS1` and `.BS2` stop differing) |
| `0x20C0`–`0x23FF` | small tables + zero padding |
| `~0x2400`–end | **the castle/fort CGA 2 bpp tile-graphic bank** (~3.6 KB) — *shared* between `.BS1` and `.BS2` (they're two floors of the same building). `CASTLE` vs `FORT` graphics are almost entirely different. |

`decoders/town_map.py CASTLE.BS1` prints the map (it auto-selects the
90×91 / 112×73 layout from the filename).

## Castle/town data — `TCASOBJ.BSV` + `FORTANIM.BSV` — **structure mapped 2026-08-31**

`CASDR.EXE`'s `loadCastleObjects` `BSAVE`-loads `TCASOBJ.BSV` to
`0x8537:0x0000` into `spriteBank` (`ds:1E58`) — the same array and role
as `DUNOBJ` / `OUTOBJ` / `MUSOBJ`, so the same sprite-format family
(mask + `basPutSprite`, field-interleaved 8×8 cells). 4904-byte payload:

| span | contents |
|---|---|
| `0x000`–`0x0FF` | object records — groups of **four `(offA, offA+0x80)` word pairs** followed by a `(0,0)` terminator (~10 objects). The `offA` values (`0x0230`–`0x0CF0`) index the sprite bank; `+0x80` is the paired mask/second-plane. The 4 pairs are the object's 2×2 cell layout (col step `0x40`, row step `0x100`). |
| `0x100`–`0x2FF` | small tables / mostly zero |
| `0x300`–`0x0EFF` | **CGA 2 bpp sprite / animation bank** — ~12 blocks of `0x100` bytes, several byte-identical: the animation-loop frames (castle banners / torches / gate). `FORTANIM.BSV` overwrites the `0x1228`–`0x1327` block for the fort. |
| `0x0F00`–`0x12FF` | 4 word-index tables, each `base + i·2`, table-to-table stride `0x63C` (bases `0x02B2`, `0x08EE`, `0x0F2A`, `0x1566`) — blit column/row address lists |
| `0x1300`+ | a short tile-index tail (`E3 E4 E5 …`) |

The exact sprite-cell dimensions and how `bmTNCALB` walks the record /
table structure still need the castle-view renderer traced (it uses
`rtm_FE19`, the shared single-tile blit).

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
