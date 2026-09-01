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
| `OUTDATA.BSV` | 14235 | `OUT` | **fully decoded** — overworld graphics. BSAVE → `0x86AE:0x2B22`, contiguous with `OUTM*` in one array (`ds:1E2A`). `0x000`–`0x3FF` = 256 × 4-byte terrain-tile records (2×2 sub-cell indices); `0x400`–`~0xD1F` = the ~146-cell 8×8 sub-cell bank; `0x1402`+/`0x1F5C`+ = 64 × 124-byte object/creature sprite records = **32 image + AND-mask pairs**, each a 20×20 px CGA 2 bpp `PUT` array (rendered by `decoders/outdata.py`). See the overworld section. |
| `OUTOBJ.BSV` | 4395 | `OUT` | **structure decoded** — the overworld landmark-icon sprite bank. `loadOverworldData` `BLOAD`s it into `spriteBank` at **byte 0** (the BSAVE `0x0DB6` is ignored, like `DUNOBJ`). 9-word pointer table at byte 0 → 6 distinct CGA-2bpp sprite blocks (512/512/384/384/384/1222 B); a `dw 0x1560 + 2n` video-column address list + `0x0001` terminator in the tail. Word[4] (`0x0830`) is the slot `PEGASUS.BSV` (1152 B) overwrites when `combatPhase == 2`. Consumed via `identifyLocationObject` → `spriteBank` scratch slots → `refreshMapView` phase 2 → `rtm_FE69`/`rtm_FE6A`. See the overworld section. |
| `DUNM1.BSV` / `DUNM2.BSV` / `DUNM3.BSV` | 2055 | `DUN` | **fully decoded** — dungeon tile maps. BSAVE → `0x2C07:0x0F3C`; payload 2048 B = **8 levels × 16×16 tiles, 1 byte/tile**. `0x00` floor, `0xFF` solid rock, `0x01`–`0x0F` features. `decoders/dun_map.py`. |
| `DUNDATA.BSV` | 5785 | `DUN` | **decoded** — the first-person view's wall/floor/ceiling graphics. BSAVE → `0x2C07:0x173C` = **contiguous right after `DUNM*`** (`0x0F3C + 0x800`), one array. 5778-B payload: `word[0]` = the tile bank's array offset (`0x0E94` → payload `0x694`); `0x020`–`0x10F` = the projection record table (15 records = 5 depths × [10-word wall-band + 7-word left + 7-word right]); `0x110`–`0x693` = the flat `ncols×nbands` cell-index tile lists (incl. the 4×`0xCC` side-wall strip table at `0x1BC`); `0x694`–end = 255 × 16-B CGA cells. See the dungeon section. |
| `DUNOBJ.BSV` | 9351 | `DUN` | **decoded** — dungeon objects + sprites. BSAVE → `0x140D:0x0DB6` into `spriteBank` (shared with `OUTOBJ`/`MUSOBJ`). ~5.6 KB real + zero pad: object records, then the `(maskSrc, screenDest)` pair table fed to `andSpriteMaskCell`, then the mask cells + `basPutSprite` image arrays. See the dungeon section below. |
| `DUNMONA.BSV` / `DUNMONB.BSV` | 14143 | `DUN` | **fully decoded** — dungeon monster sprites (`A` = levels 0–3, `B` = 4–7). BSAVE → `spriteBank` word `0x1240`. 14136-B payload = **exactly 6 blocks × 2356 B** (one per monster-type slot). Each block = 6-word frame-offset table (`9/725/973/1083/1148/1178`) + 3 zero words + **5 back-to-back MS-BASIC `PUT` GET-arrays** for view-**depths** P0…P4 (82×68 / 48×41 / 32×27 / 24×21 / 16×14, linear 2 bpp) — not animation frames. **No palette animation** anywhere: static single-frame sprites, CGA palette 1 set once per view via `rtm_FE29`. See the dungeon section below. |
| `DIS0.BSV`…`DIS15.BSV` (+ `DIS0A`, `DIS1A`) | 727–2055 | `MUS` (+ `CELDRV`) | **decoded** — the **museum exhibit "display" illustration screens** (~18). `MUS.EXE` builds `"DIS"+n+".BSV"` to show the picture on display for an exhibit; `DIS9.BSV` doubles as frame 4 of `CELDRV`'s endgame cinematic. BSAVE → `0x13C2:0x0E4E`. Same container as `CEL*.BSV` (cell table + 8×8 cells; **no RLE**) — see the CEL section. |
| `CEL0.BSV` / `CEL2.BSV` (1573) · `CEL1.BSV` / `CEL3.BSV` (2597) | — | `CELDRV` | **decoded** — 4 of the 5 frames of the "AGAINST ALL ODDS!" endgame cinematic (the 5th is `DIS9.BSV`). `celdrv_entry` BLOADs `CEL0/1/2`, `dis9.bsv`, `cel3.bsv` into `spriteBank` at word-slot `bank·2000 + 1000` and relocates each `stripPtr`. Format: 8-word header + cell table + field-interleaved 8×8 CGA cells; **not RLE-compressed** (sparse changed-cell grid with dedup). See the CEL section. |
| `MUSDATA.BSV` | 8055 | `MUS` | **decoded** — the Tarmalon Museum data. BSAVE → `0x2C1C:0x0F58` (near the dungeon's `0x2C07:0x0F3C`). `0x000`–`0x07FF` = **3 exhibit floor maps, 16×16 tiles** (`0x80` wall / `0x00`–`0x03` floor / `0x10`–`0x43` wall-edge / `0xE0`–`0xEF` = the 16 display-case portals) + 5 empty slots; `0x800`+ = a near-copy of `DUNDATA.BSV`'s dungeon-view tile/graphic data (`bmMUSDUNG` ≡ `bmDUNG`). `decoders/dun_map.py MUSDATA`. |
| `MUSOBJ.BSV` | 12961 | `MUS` | **structure decoded** — the museum's `DUNOBJ`-family object bank. BSAVE → `0x1447:0x0DB6`, `BLOAD`ed into `spriteBank` at **byte 0**. 12954-B payload (no zero pad). Same container as `DUNOBJ` — region-A 5-word records, region-B `(videoDest, maskSrc)` pair lists, region-C 16-byte field-interleaved cells — but larger (14+ live records vs 5). **`MUS.EXE` never draws from it**: `renderExhibitView` has no `viewObjectArray` (`ds:1C7C`) reference — the exhibit halls render as plain corridors from `MUSDATA`, and the artifacts are shown via `DIS*.BSV` + per-exhibit `.BSV` files. `spriteBank[0x32A]` (= `0x329A`, the payload length) is where `loadExhibitData` `BLOAD`s each exhibit file. See the museum notes below. |
| `MUSMSG.TXT` | 11229 | `MUS` | plaque + exhibit message text |
| `SDMAP.GLB` / `.GMP` | 4113 / 2081 | `SDEFENDR` | **fully decoded** — 256-tile 8×8 CGA sheet + 41×25 column-major cell map = the combat-arena *screen frame* (ornate magenta viewport border around a black 3-D-view window, stippled background). Same format as `TITLE`; `decoders/glb_image.py SDMAP` renders it. |
| `SDOBJ.GLB` | 6161 | `SDEFENDR` | **tiles decoded** — 384 × 8×8 CGA tiles, same field-interleaved format as `TITLE`, **no `.GMP`** (bare sprite atlas). Content: approaching fireballs at ~6 scale steps, explosion / impact animation frames, cyan directional player-shot arrows, a horned enemy head. Per-sprite tile grouping TBD (needs the arena blit code). `decoders/glb_image.py SDOBJ` dumps the atlas. |
| `BJCHR.GLB` | 6161 | `GMB1` / `GMB2` | **tiles decoded** — card graphics ("BJ" = BlackJack). Same container as `SDOBJ.GLB` (6161 b, `{0x0A,6,1,0,1}`, bare atlas, no `.GMP`); 384 8×8 tiles, only 0–127 used. Holds a card **rank/suit glyph font** — `A 2 3 4 5 6 7 8 9 10 J Q K` + the four suit pips (♠♣ white, ♥♦ magenta), in **both upright and 180°-rotated** forms (the two opposite card corners) — plus card-back pattern + frame/corner tiles. `GMB1` BLOADs it into `seg004`; `GMB2` shares it. `decoders/glb_image.py BJCHR` dumps the atlas (use grid width 15 to line the font up). |
| `BIGNUM.DAT` | 420 | `GMB2` | **decoded** — the large-digit font for `GMB2` (Flip-Flop Parlour)'s GOLD / BET / winnings readouts. **No BSAVE header** — a raw **112 × 15 px CGA 2 bpp bitmap** (28 bytes/row × 15 rows = 420 B exactly; autocorrelation locks the stride at 28). One horizontal strip of ~10 digit glyphs (~11 px pitch), blitted a digit at a time by `drawBigNumberPanel`. `decoders/bignum.py`. |
| `D.BSV`, `R.BSV`, `PEGASUS.BSV` | 3527 / 3527 / 1159 | ? | ? |
| `CHAR.DAT` | 3444 | `MENU` / `SAVER` / all play modules | **fully framed + fields mapped** — the character roster, which doubles as the in-progress save. 6-byte header + **9 records × 382 bytes**. Each record = 14-byte name + 74-byte scalar block (image of resident LEGLIB DGROUP `ds:1AC0..1B08`) + **7 BASIC integer arrays with known `DIM` bounds** (8/8/30/17/38/42/4 words — sum = 294 = 382−14−74). Gold, HP, strength, experience, inventory count, overworld X/Y, compendium rank + the S5 shop price table identified; per-element split of S2/S4 still needs a populated save. See the dedicated section. `decoders/char_dat.py`. |
| `LEGACY.DAT` | 2945 | `MENU` (→ resident, all modules) | **fully decoded** — the game's **master font/string/data table** (`menuStartup` loads it once into resident LEGLIB DGROUP). 6-B header, then the **8×8 CGA software font** (`0x006`–`0x605`, 96 glyphs ASCII 0x20–0x7F, `decoders/legacy_font.py`), the **game-speed timing table** + **first-person movement table** (`0x606`–`0x647`), the **123-entry length-prefixed string pool** (`0x648`–`0xA02`, all command/weapon/armor/item/spell/town names), then the **382-B new-character template** (= one `CHAR.DAT` record) ending in the **shop price table**. `decoders/legacy_dat.py` + `decoders/legacy_font.py`. |
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

**Each 382-byte record** — framing fully pinned down 2026-09-01.
Write path: `SAVER.EXE` `saveRosterToDisk` (`rt_AB` for the name, then
`rtm_FE35` peeks `ds:1AC0`–`ds:1B08` word-by-word, then `rtm_FE39` ×7
over the array descriptors). Read path: `MENU.EXE`
`showEmptyCharacterSlots` / `readCharDat` (`rtm_FE36` poke loop, then
`rtm_FE37` ×7). The 7 arrays are `DIM …(bound) AS INTEGER` in
`MENU.EXE`'s module init (the `rt_AF` calls with params
`(desc, 0x101, elemSize 2, bound, 0, 0)`); the bounds sum to **exactly
294 bytes = 382 − 14 − 74**, which nails the split:

| off | size | field |
|---|---|---|
| `+0x000` | 14 | **name**, space-padded. `"empty"` marks an unused slot (all 9 are `"empty"` in a fresh install). Matches the "up to 14 letters long" name prompt. |
| `+0x00E` | 74 | **scalar block** — 37 words, a verbatim image of the resident LEGLIB DGROUP range `ds:1AC0`–`ds:1B08`. Survives the `OUT`↔`DUN`↔`TWNDR`↔`MUS`↔`CASDR` chaining. See the field table below. |
| `+0x058` | 16 | **S0** `ds:1B0C` `DIM(7)` — encounter / combat scratch (zeroed at the top of every `outInit`, so not real save data). |
| `+0x068` | 16 | **S1** `ds:1B68` `DIM(7)` — inventory slot data, 8 slots (cursor `ds:1AEA`). `rtm_FE50` clamps elements to `0..4` = the LEGACY "weapon condition" scale (Shoddy…Superb). |
| `+0x078` | 60 | **S2** `ds:1BC4` `DIM(29)` — world / quest state, 30 flags (cross-referenced against the overworld object array `ds:1C7C`). Template `[15]=1`, `[17]=2`. |
| `+0x0B4` | 34 | **S3** `ds:1B3A` `DIM(16)` — museum progress (`MUS` bumps `[15]` on every entry, sets `[14]=1`). Template all-zero except `[0]=-1`. |
| `+0x0D6` | 76 | **S4** `ds:1B96` `DIM(37)` — the main stat / map-state block, 38 words, ~150 read/write sites. `outInit` copies `S4[19]` (tmpl 200) → `hitPoints`, so S4 holds the persistent copy the `ds:1AC0` scalars are re-derived from. Template `[0..2]` = `1500/3099/31058` (RNG / world-hash junk from the dev dump), `[22]=32000`, `[25..29]=32767` sentinels, `[33]=3`. |
| `+0x122` | 84 | **S5** `ds:1BF2` `DIM(41)` — **shop price table**: `S5[0]=7`, then 41 prices (weapon / armor / item / food): `400,350,350,13,500,220,950,450, 150,170,200,170,250, … 2000,1500,1700,5,1300,2000,5,21`. Identical for every character — FIELDed per-record only for convenience. |
| `+0x176` | 8 | **S6** `ds:1C20` `DIM(3)` — reserved, 4 words, always `0`, no read/write site anywhere. Ends at `0x17E` = 382. |

**Scalar block** (`+0x00E`, 37 words). Record offset ← DGROUP word;
default from the LEGACY.DAT new-character template. `[C]` cross-checked
in play-module code, `[?]` inferred from the template value + usage:

| rec | ← ds: | default | field |
|---|---|---|---|
| `+0x10` | `1AC2` | 0 | `[C]` **experience** (dword; `TWNDR` `add`/`adc` accumulates) |
| `+0x16` | `1AC8` | 4 | `[?]` **game speed** (read by MENU + every module) |
| `+0x18` | `1ACA` | 0 | `[C]` chain-return / location context (`SAVER` `returnTarget`) |
| `+0x1A` | `1ACC` | 15 | `[?]` attribute / resource (potion-wizard `+5`, cap `0x24`) |
| `+0x20` | `1AD2` | **20** | `[C]` **party gold** (dword) |
| `+0x24` | `1AD6` | −99 | `[C]` dungeon return position (dword; −99 = "none") |
| `+0x28` | `1ADA` | **200** | `[C]` **hit points** |
| `+0x2C` | `1ADE` | 15 | `[?]` museum-adjustable stat (`MUS` `showGold` `+10`) |
| `+0x2E` | `1AE0` | 1 | `[C]` **compendium volumes / museum access rank** (1..7) |
| `+0x30` | `1AE2` | 0 | `[C]` dungeon position `level<<8 | cell` (`cmp 0x700`) |
| `+0x32` | `1AE4` | 0 | `[C]` dungeon / museum facing (0..3) |
| `+0x34` | `1AE6` | 0 | `[C]` dungeon light / step counter |
| `+0x36` | `1AE8` | 0 | `[C]` dungeon spell-effect timer (counts down to 0) |
| `+0x38` | `1AEA` | **5** | `[C]` **inventory count** (index into S1, guard `< 8`) |
| `+0x3A` | `1AEC` | 9 | `[C]` paired count / damage multiplier (`DUN` `imul`) |
| `+0x3E` | `1AF0` | 15 | `[C]` **strength** (cap `0x1C`=28; potion-wizard gate) |
| `+0x4A` | `1AFC` | 99 | `[C]` selected-item cursor (99 = none; guard `< 8`) |
| `+0x4C` | `1AFE` | 0 | `[C]` second-list count (paired with `1AFC`) |
| `+0x50` | `1B02` | 178 | `[C]` **overworld X** (new game sets 40) |
| `+0x54` | `1B06` | 106 | `[C]` **overworld Y** (new game sets 30) |
| `+0x56` | `1B08` | 15 | `[?]` attribute-like (`DUN` subtracts from it) |

`+0x0E`(`1AC0`), `+0x14`(`1AC6`), `+0x2A`(`1ADC`), `+0x3C`(`1AEE`),
`+0x4E`/`+0x52`(`1B00`/`1B04`) are transient scene/UI state that happens
to sit in the FIELDed range. `+0x1C`(`1ACE`), `+0x1E`(`1AD0`),
`+0x40`–`+0x48`(`1AF2`–`1AFA`) have **no reference in any module** —
dead words (`1AD0`'s `0x4270` default is stale dev-memory junk).

Because the roster block lives in LEGLIB's **resident** DGROUP, it
survives the `OUT`↔`DUN`↔`TWNDR`↔… EXE chaining — which is why
`partyGold` / `hitPoints` / `playerX` sit at the same DGROUP offsets in
every play module (see [overview.md](overview.md)). Loading a saved
game = `MENU` `GET`s the record into `ds:1AC0`+ and chains to `OUT`;
saving = `SAVER` `PUT`s `ds:1AC0`+ back. The "is not on this / character
disk" / "empty" strings in `SAVER` are the removable-character-disk
handling.

`decoders/char_dat.py` reads the container, lists each slot (name + the
identified scalar fields), and prints the LEGACY.DAT new-character
template split field-by-field.

## Dungeon data — `DUNM*` / `DUNDATA` / `DUNOBJ` / `DUNMON*` — **decoded 2026-08-31**

`DUNM1.BSV` / `DUNM2.BSV` / `DUNM3.BSV` (one per dungeon group) each
`BSAVE` to `0x2C07:0x0F3C`. The 2048-byte payload is **8 dungeon levels,
16×16 tiles, one byte per tile**, row-major (`8 × 256`):

| byte | feature — **decoded 2026-08-31** (`DUN.EXE` `moveHazards` / `doLookSearch` / `climbUp` / `climbDownOrExit` + the string pool at `DUN.EXE:0x881c`) |
|---|---|
| `0x00` | open floor / corridor |
| `0x01` | **POISON GAS VENT** — hidden trap → monster ambush |
| `0x02` | **FLOOR HOLE** — hidden trap → fall to the next level down; once sprung it stays as `0x0A` (an open down-passage) |
| `0x03` | **SLIME SPLOTCH** — hidden trap → ambush |
| `0x04` | **TRIP WIRE** — hidden trap → ambush |
| `0x05` | **CEILING HOLE** — hidden trap → ambush |
| `0x06` | **TREASURE CHEST** — hidden; Search/open (`openChest`), else ambush |
| `0x07` | **BOX** — hidden; Search/open, else ambush |
| `0x08` | a **visible** container (Search opens it without a trap check) |
| `0x0A` | **stairs DOWN** / open floor-hole — `climbDownOrExit` descends here |
| `0x0D` | **stairs UP** — `climbUp` ascends here; each climb toggles `0x0A ↔ 0x0D` |
| `0x09` / `0x0B` / `0x0C` / `0x0E` / `0x0F` | walkable "revealed" features (common in the shipped maps — probably pillars / doors / decoration; not individually confirmed) |
| `0x10`–`0xFE` | wall tiles (drawn on the auto-map as `CHR$(0x60 + b/16)`) |
| `0xFF` | solid rock (unlit / never drawn) |

`DUN.EXE` binds this array at DGROUP `ds:1E2A` and indexes
`base + level*0x100`; bytes `≥ 0x10` are walls, `1..7` are hidden traps
(springing one adds `8` → the `9..0x0F` "revealed" range). `Search`
springs a hidden trap; walking onto one triggers it. The **dungeon
monsters** are also in the `DUN.EXE` string pool, one set per file —
`DUNM1`: NERVE STREAKER / GNASHER TURTLE / TENDRO SNAPPER / NIGHT STALKER;
`DUNM2`: GRAPPLER / KNUCKLES / DANGLER / MR POTATO; `DUNM3`: RAKER BRUTE /
BLUE LION / GIANT SLUG / SLIME WART. `decoders/dun_map.py DUNM1` prints
all 8 levels with the feature legend.

`MUSDATA.BSV` is the museum's counterpart: 3 exhibit floor maps (16×16,
`0x000`–`0x07FF`) then, from `0x800`, its own copy of the dungeon-view
tile/graphic data — many 64-byte regions byte-match `DUNDATA.BSV` at a
~`-0x800` delta (the museum's `bmMUSDUNG` is the same renderer as
`bmDUNG`). Museum tile `0xE0`–`0xEF` = one of the 16 exhibit /
display-case portals `enterExhibit` routes on (to `TWNDR` / `DUN` /
`STDRV` / `CELDRV`).

`MUSOBJ.BSV` (12954-B payload, `BLOAD`ed to `spriteBank` byte 0) is the
museum's `DUNOBJ`-family object bank — `dw 0x0005`, then region-A 5-word
records `[type, endWord, count, startWord, K]` (`endWord = startWord +
count·2`; types `0x0110` / `0x015b` / `0x018a` / `0x010d`, arranged as
~5 depth-band groups of `[0x0110, 0x015b, 0x018a]` then a looser
`0x0110`-every-`0xF`-words run), then the `(videoDest, maskSrc)` pair
lists (from ~byte `0x732`), then the 16-byte field-interleaved
mask/bitmap cells (from ~`0xEBA`) out to the `0x329A` payload end. It is
**dormant**: `renderExhibitView` (mus `bmMUSDUNG` ≡ `bmDUNG`) never
touches `viewObjectArray` (`ds:1C7C`) or blits from `spriteBank` — the
exhibit halls are corridors only. `spriteBank[0x32A]` = `0x329A` is
just the offset where `loadExhibitData` `BLOAD`s each per-exhibit
`.BSV` (right after `MUSOBJ`).

`DUNDATA.BSV` `BSAVE`s to `0x2C07:0x173C` — exactly `0x800` bytes after
the map, so `DUNM* + DUNDATA` load into **one contiguous DGROUP array**
(`dungeonMapArray`, `ds:1E2A`) at array byte `0x800`.

Payload layout (5778 B, decoded 2026-08-31 — `decoders/dundata.py`). Every
offset word inside DUNDATA is an *array* offset (`0x800 + payloadOffset`),
used raw.

| span | contents |
|---|---|
| `0x000`–`0x01F` | header. **`word[0] = 0x0E94`** = the tile bank's array offset (`drawTileRun`'s `srcBase`) = payload `0x694`. Rest zero. |
| `0x020`–`0x10F` | **the projection record table** — exactly 15 records (see below), walked in sequence by a cursor `renderDungeonView` steps `0xA / 7 / 7` words per depth band. `240` B = `5` depths × (`10` + `7` + `7`) words. |
| `0x110`–`0x1BB` | wall-band tile lists — the ceiling / floor / front-wall cell-index bytes (more live at `0x4EC`–`0x693`). |
| `0x1BC`–`0x4EB` | **the strip table** — `4` columns × `0xCC` B = 4 render variants of the corridor side walls. Within a column the 10 side records' strips pack **contiguously**, each `ncols·nbands` bytes (`X` = `0x1BC`, `+0x2D, +0x2D, +0x1E, +0x1E, +0x10, +0x10, +6, +6, +5`). |
| `0x4EC`–`0x693` | more wall-band tile lists (front-wall blocks). |
| `0x694`–`0x1691` | **the 8×8 tile bitmap bank** — 255 cells × 16 B, CGA 2 bpp, **8 words per cell** which `sub_1FED8` writes to screen rows 0,2,4,6,1,3,5,7 (read-back order `w0,w4,w1,w5,w2,w6,w3,w7`). Cell 0 = blank. |

**The record table.** Per depth (0 = adjacent … 4 = farthest):

- **a wall-band record — 10 words** = three `(videoOff, packedDims, tileListPtr)` triples + 1 pad word. `packedDims = (ncols<<8) | nbands`. `drawViewWallBandNear` blits all three:
  - triple 0 = the **ceiling** strip (wide + short: `23×2` near → `5×0` far),
  - triple 1 = the **floor** strip (`17×2` → `3×0`, mostly the dark `0x4A`–`0x4C` cells),
  - triple 2 = the **front-wall** block — the wall the player faces head-on (`17×13` near → `3×5` far).
- **a left-side record and a right-side record — 7 words each** = `(videoOff, packedDims, p0, p1, p2, p3, padWord)`. `p0..p3` = the same corridor side wall in the 4 strip-table columns (deltas `0xCC`); `packedDims` shrinks `3×15` → `1×5` with depth. `drawViewFloorCeiling` picks **`p0` (col 0)** when that side is a solid wall, **`p1` (col 1)** when it's an open branching passage. `padWord` = `0x28·(depth+1)` (a screen-Y / horizon constant).

**Every tile list — wall-band strip or side strip — is a flat `ncols ×
nbands` row-major array of bank cell indices** (`0xFF` = skip) fed
straight to `drawTileRun`. **There is no marker / display-list layer.**
The bytes that look like delimiters (`0x00`, `0x4B`–`0x56`, `0x70`,
`0x7E`–`0x7F`, `0xC0`–`0xCC`) are just the near-black shadow / edge bank
cells every surface uses along its boundaries. Rendered, each strip is a
coherent dithered stone panel with a dark edge; composited they make the
first-person corridor view (arched dithered ceiling line, ragged floor
line).

**Blocked-view fallback** — same records, no new fields. When a wall
tile blocks the corridor at depth `k`, the near loop stops with the
cursor on depth `k`'s **left-side record**, then for each side:
`drawViewWallBandMid` (open side) draws that side wall from the record's
`p2`/`p3` (words 4/5 — the darker strip-table columns, chosen on the
blocking wall's `< 0x80` thickness), or `drawViewFloorCeiling` +
`rtm_FE2B` fill (solid side). If the corridor is open to the end,
`drawViewWallBandFar` draws depth 4's **front-wall triple** (words
6/7/8 of the depth-4 wall-band record), tile list `+0xF`.

The DUNDATA / `bmDUNG` first-person renderer is now **fully decoded.**

### The dungeon-view sprite & tile rendering model — **decoded 2026-08-31**

`DUN.EXE`'s `bmDUNG` segment (`renderDungeonView`) draws the first-person
corridor by calling four LEGLIB primitives, all now identified:

| primitive | what it does |
|---|---|
| `sub_1FED8` | the atomic **8×8 CGA cell copy**: `movsw` ×8 with `+0x4E` steps and a `+0x1F0E` jump after the 4th word (even field → odd field). This is **byte-identical to the `.GLB` field-interleave** — 8 consecutive words = scanlines 0,2,4,6,1,3,5,7. *Every* 8×8 graphic in the game uses this layout. |
| `drawTileRun` (`rtm_FE2A`) | draw a horizontal run of `count` cells from a **byte tile-index list**; `0xFF` = skip (transparent); cell `N` is the 16 bytes at `srcBase + N*16`. Drives all wall/floor bands via `blitViewCell`. |
| `andSpriteMaskCell` (`rtm_FE2E`) | sprite **mask** pass: `and es:[di], word` for one field-interleaved 8×8 cell (8 words). `drawViewSprite` calls it once per masked cell, walking a list of `(srcOffset, destOffset)` word pairs. |
| `basPutSprite` (`rtm_61`, + `basPutSpriteXor` `rtm_60`) | stock Microsoft BASIC `PUT (x,y), array` — draws a **GET-array bitmap**: `dw xBits (=pixelWidth*2)` `; dw yRows ; then yRows rows of ceil(xBits/8) bytes`, 2 bpp packed left-to-right, rows top-to-bottom, **linear (not CGA field-interleaved)** — `PUT` splits the even/odd fields itself. Honours `ds:250h/252h` origin, `ds:2D0h/2D1h` horizontal-flip, and an AND-vs-OR row-drawer; colour 0 = transparent. `drawViewSprite` uses this for the sprite **image** after `andSpriteMaskCell` lays the mask. |

`drawViewSprite` reads `viewObjectArray[8 + objType]` (`ds:1C7C`) → a
**depth band 0–4** (`−6` wraps if `> 5`), then per band uses hard-coded
`spriteBank` (`ds:1E58`) offsets + screen-Y / height / clip-rect
constants (the P-switch: mask-header word offsets `0x23/0x4B/0x73/0x9B/
0xC3` step `0x28`; clip rects like `0x717`,`0xE1F`). It then runs the
mask loop (`andSpriteMaskCell` × count) and the image (`basPutSprite`).

So each dungeon **sprite** = a mask (list of `(src,dst)` 8×8-cell pairs,
AND-blitted) + an image (BASIC `PUT` array, `[dw w][dw h][rows]`).

### `DUNOBJ.BSV` — dungeon objects + sprites

`BSAVE`s to `0x140D:0x0DB6`, but `loadDungeonData` **ignores that offset**
and `BLOAD`s the payload into `spriteBank` (`ds:1E58`) at **offset 0** —
so every file offset below **is** its `spriteBank` offset (no relocation),
and `DUNMON*` loads immediately after at word `0x1240` (= byte `0x2480` =
the payload length). Payload 9344 bytes; real data ends at `0x15A1`.

| region | span | contents |
|---|---|---|
| A — record table | `0x002`–`0x191` | **40 records**, `dw 0x0110 ; dw endWord ; dw count ; dw startWord ; dw K` (`endWord = startWord + count·2`), in **5 groups of 8** — one group per view depth. Records **7 / 15 / 23 / 31 / 39** (the 8th of each group) have `count > 0` and are the **live per-depth mask descriptors** `drawViewSprite` reads (`count` = `36/31/21/14/8`, `startWord` = the depth's region-B list). The other 35 records have `count = 0` and carry only a `startWord` marker into the *extended* region-B pair area plus a 5th word `K` (per monster, per depth-group: P0 group = `9,12,9,7,18,8`, P3 group all `1`; **purpose unknown** — `drawViewSprite` never reads it, and there is no sprite animation for it to count; possibly a per-monster stat or a vestigial editor field). `drawViewSprite` reads record `8·P+7` at word `P_idx` ∈ `{0x23,0x4B,0x73,0x9B,0xC3}` (= the tag word − 1), taking `[P_idx+3]` = count, `[P_idx+4]` = startWord. |
| A — DUNMON bank table | `0x190`–`0x1A3` | **6 word pointers** `0x1240 + k·0x49A` (`= 0x1240,0x16DA,0x1B74,0x200E,0x24A8,0x2942`) — overlaps record 39's `K` field. `spriteBank[0x190]` = `0x1240` is where `loadDungeonMonsters` `BLOAD`s `DUNMONA/B.BSV`; the 6 pointers subdivide it into the 6 monster-type blocks. |
| B — pair lists | `~0x247`–`0x8F1` | `(videoDest, maskSrc)` word-pair lists. The 5 that `drawViewSprite` uses start at words `0x2CF/0x36F/0x3EF/0x43F/0x469`. Words `~0x247`–`0x2CE` and `~0x480`–`0x59E` hold the count-0 records' per-object sub-lists in a **reversed `(maskSrc, videoDest)`** order — a separate consumer, never exercised by `DUN.EXE`. `videoDest` = a CGA **even-field byte offset** (`row·0x50 + colByte`), `maskSrc` = a **byte offset into region C** (`0x1212 + 16·n`). First word of a main-list pair = `andSpriteMaskCell`'s `arg_4` (dest), second = `arg_0` (src). |
| — object bitmaps | `0x8F2`–`0x1211` | ~2336 B of 4-colour CGA art (`0x66/0x99/0xAA/0xE6/0x55/0xFF` bytes) — **~146 field-interleaved 8×8 cells** of object/decoration graphics. **`DUN.EXE` never reads this**: `drawViewSprite` is the only `spriteBank` consumer and it touches only the mask descriptors, the DUNMON bank table, the 5 main region-B lists, region C, and the DUNMON GET-arrays (`≥ 0x1240`); the `rtm_FE2D` plain-cell-copy thunk is present but unreferenced. `MUSOBJ` has the same dormant region (its `renderExhibitView` doesn't draw sprites either) — the OBJ-family container just carries more than any single game module's renderer uses. Sprite boundaries / indexing unresolved (moot). |
| C — mask cells | `0x1212`–`0x15A1` | **57 contiguous 16-byte mask cells** (`0x1212 + 16·n`, n = 0–56). Each = one **field-interleaved 8×8 2 bpp `AND` stencil** (8 words, scanline order 0,2,4,6,1,3,5,7; bits 15-14 = leftmost pixel). `andSpriteMaskCell` `and`s each scanline into video: a pixel-pair of `11` keeps the video pixel, `00` forces it black. All nibbles are pair-aligned (`0x0/0x3/0xC/0xF`); the checkerboard mix (`0xF0`,`0x3C`,`0xC3`,…) is **ordered dither** → soft translucent edges. Composited per depth: a **rectangular aperture with a dithered top border** that shrinks P0→P4 — the lit niche the `DUNMON` sprite is `PUT` into. Shared pool: cells are reused within a list (P0 = 36 blits, 13 unique) and overlap across depths. |
| D | `0x15A2`–end | zero padding |

Per-depth region-C usage:

| P | count | region-B list (words) | distinct cells | cell span |
|---|---|---|---|---|
| 0 | 36 | `0x2CF`–`0x316` | 13 | `0x1212`–`0x12E2` |
| 1 | 31 | `0x36F`–`0x3AC` | 15 | `0x12A2`–`0x14A2` |
| 2 | 21 | `0x3EF`–`0x418` | 13 | `0x13B2`–`0x14A2` |
| 3 | 14 | `0x43F`–`0x45A` | 10 | `0x13B2`–`0x1512` |
| 4 | 8  | `0x469`–`0x478` | 8  | `0x1522`–`0x1592` |

`DUN.EXE`'s loader is now disassembled — **`loadDungeonData`** (was
`sub_12E9B`, called from `processTileFeature` on level entry;
`ida_scripts/fix_dun_loaddungeondata.py`). For each of
`DUNM<dungeonNo>.BSV` / `DUNDATA.BSV` / `DUNOBJ.BSV` it: pushes the
target BASIC array's descriptor (`rtm_11`), builds the name
(`"DUNM" + STR$(ds:1ACAh) + ".BSV"`), calls `resolveAndOpenGameFile`
(`rtm_FE63` — picks the drive via `DRCONFIG.DAT` and opens), then
`basBload` (`rtm_FE07` → `rtm_02`: read the 7-byte header + payload).
`DUNM<n>` → `dungeonMapArray` (`ds:1E2A`) offset 0; `DUNDATA.BSV` → the
same array `+0x800`; `DUNOBJ.BSV` → `spriteBank` (`ds:1E58`). **No
pointer-table relocation** — the offsets in region A / DUNDATA are used
as-is against the array base.

**Consumer chain** (traced — the loaders are decoded; a couple of
helpers in this `DUN.EXE` region are still tangled):

- **`drawViewSprite` (`bmDUNG`)** is region A/B/C's consumer: per depth
  band `P` (0-4), region A's descriptor at `spriteBank[P_idx·2]` gives
  `(count, listStart)`; it walks `count` `(videoDest, maskSrc)` pairs
  from region B calling `andSpriteMaskCell` (the dithered aperture
  stencil), then `rtm_FE46` (clear the clip box), `rtm_7A` (position),
  and `basPutSprite` on the `DUNMON` image. **The mask comes from
  `DUNOBJ`, the pixels from `DUNMON`.**
- The **DUNMON bank table at `0x190`**.
  `loadDungeonMonsters` (`DUN.EXE` `0x12F9F`, called from
  `processTileFeature` right after `loadDungeonData`) reads
  `spriteBank[0x190]` (= `0x1240` *words*) and `BLOAD`s
  `DUNMONA.BSV` (levels 0–3) / `DUNMONB.BSV` (levels 4–7, keyed on
  `ds:1AE2h >= 0x400`) into `spriteBank` at that word offset. So the 6
  pointers `0x1240 + k·0x49A` are the 6 `DUNMON` monster records
  (`0x49A` words = `0x934` bytes each — exactly the `DUNMON` record
  size), and region A tells `spriteBank` where the monster art lands.
- **Objects/monsters vs. the map** are tracked in `viewObjectArray`
  (`ds:1C7C`, DIM'd 63 words) as **8 slots**: `[idx]` = occupied,
  `[idx + 0x10]/[idx + 0x18]` = x/y, `[idx + 0x20]` = packed position,
  plus a facing word. `removeViewObject` clears a slot and strips it
  from its map cell (`map[pos] &= 0x8F`); `clearViewObjects` does all 8.
- **`rebuildLevelView`** (on level change) re-places the 8 slots for the
  new level: strip from old cell, recompute position, walk to the
  nearest free cell, stamp `map[pos] = (slotIdx<<4) | wallType | 0x10`,
  refresh the slot fields. So a map byte `> 0x0F` encodes
  `high nibble = slot/class`, `low nibble = wall under it`.
- **`renderDungeonView`** (`bmDUNG`) walks each view ray; a tile
  `> 0x0F` gives class `(tile>>4) - 1`; if `viewObjectArray[class-1] > 0`
  it calls **`drawViewSprite`**, which reads `viewObjectArray[8+class]`
  (facing), picks a `DUNMON` record, and blits mask + image
  (`andSpriteMaskCell` + `basPutSprite`).
- **`moveMonsters`** + `sub_139FC` relocate monsters on the map grid
  per turn (class in tile bits 4–6).

So region A's 40 records are **5 depth groups × 8** — 7 per-object
descriptors (`count 0` + `startWord` + frame count `K`) and one live
mask descriptor per group — followed by the DUNMON bank pointers
(`0x190`). It is **not** an object-*placement* table: the per-slot
`viewObjectArray` data (which objects are where) comes from the `DUNM`
map tiles, which already carry `(class<<4)|wall` for pre-placed objects.
The 7-per-group per-object descriptors and their extended region-B
sub-lists + the `0x8F2` bitmaps are the object draw path — wired up in
`MUS.EXE` but not `DUN.EXE`.

### `DUNMONA.BSV` / `DUNMONB.BSV` — dungeon monster sprites — **fully decoded 2026-08-31**

`BSAVE` to `0x140D:0x3236`, loaded into `spriteBank` (`ds:1E58`) at
**word offset `0x1240`** (right after `DUNOBJ`) by `loadDungeonMonsters`.
14136-byte payload = **exactly 6 blocks × 2356 B (`0x49A` words)** — one
per monster-type slot. `DUNMONA` = the monster set for dungeon levels
0–3, `DUNMONB` = levels 4–7 (`loadDungeonMonsters` picks on
`ds:1AE2h >= 0x400`); the two sets differ in ~74% of bytes.

Every block has the identical structure:

| words | contents |
|---|---|
| `0`–`4` | **frame-offset table** — word offsets, relative to block start, of the 5 pre-scaled sprites: `9, 725, 973, 1083, 1148`. Index = **view-depth band `P`** (`0` = adjacent … `4` = farthest). |
| `5` | `0x49A` — block length / end marker. |
| `6`–`8` | zero padding. |
| `9` … | the 5 sprites, back to back — each a **stock Microsoft BASIC `GET`/`PUT` array**. |

Sprite (GET-array) format — same as every other `basPutSprite` blit in
the game:

```
dw  xBits      ; width in BITS = pixelWidth * 2   (CGA SCREEN 1, 2 bpp)
dw  yRows      ; height in scanlines
db  yRows * ceil(xBits/8) bytes of pixels
```

The pixel rows are **2 bpp packed left-to-right (MSB pair = leftmost
pixel), row 0 at the top, byte-aligned per row, and stored linearly —
NOT CGA field-interleaved** (unlike the `.GLB` tiles and the `DUNOBJ`
region-C mask cells; `PUT` itself splits the even/odd fields). Colour 0
is transparent (PUT verb `0`).

The 5 depth frames in every block:

| `P` | xBits | px | rows | B/row | frame B |
|---|---|---|---|---|---|
| 0 (near) | 164 | 82 | 68 | 21 | 1432 |
| 1 | 96 | 48 | 41 | 12 | 496 |
| 2 | 64 | 32 | 27 | 8 | 220 |
| 3 | 48 | 24 | 21 | 6 | 130 |
| 4 (far) | 32 | 16 | 14 | 4 | 60 |

(Confirmed by rendering — DUNMONA block 0 P0 is an 82×68 coiled
serpent, block 1 a spiky-shelled turtle, etc.; each `P` is a hand-drawn
smaller redraw, not a scale-down.)

**`drawViewSprite` (`bmDUNG` `seg001:0x13BD3`)** — the consumer:

- args: `[bp+8]` → view-slot/class `(mapTile>>4)-1` (0–7); `[bp+6]` → depth `P` (0–4).
- `monType = viewObjectArray[8 + slotClass]` (`ds:1C7C`), wrapped `mod 6`.
- `bankBase = spriteBank[0x190 + monType*2]` = `0x1240 + monType*0x49A` (DUNOBJ region A's DUNMON bank table).
- `getArray = bankBase + spriteBank[bankBase + P]` — the `P`-th frame of that monster block.
- per-`P` constants baked into the function: sprite anchor `(X,Y)`, the DUNOBJ region-A mask descriptor index, and a clip-box:

  | `P` | X | Y | maskDescIdx | box TL | box BR |
  |---|---|---|---|---|---|
  | 0 | 179 | 56 | `0x23` | (7,23) | (14,31) |
  | 1 | 195 | 72 | `0x4B` | (9,25) | (13,29) |
  | 2 | 204 | 80 | `0x73` | (10,26) | (12,28) |
  | 3 | 208 | 82 | `0x9B` | (10,26) | (12,28) |
  | 4 | 211 | 86 | `0xC3` | (11,27) | (11,27) |

  `monType == 0` shifts `Y` up 6 px.
- draw order: AND-blit the region-C mask cells (region-B `(videoDest,maskSrc)` list, `andSpriteMaskCell` × count) → `rtm_FE46` zero the clip box (erase last frame) → `rtm_7A` set the graphics cursor to `(X,Y)` → `basPutSprite(getArray, verb 0)`.

So the split is: **`DUNOBJ` supplies the per-depth arch/frame mask,
`DUNMON` supplies the per-monster, per-depth image.**

**Palette / animation — there is none.** The monster sprites are **static
single-frame** CGA 2 bpp bitmaps; `drawViewSprite` runs once per player
action (`renderDungeonView` is a per-turn redraw, not a loop), picking
the depth frame from the monster's current distance — that distance
step is the only "animation". Each block holds 6 *distinct* monsters
(not frames of one — DUNMONA: streaker / two-headed turtle / octopoid /
one-eyed stalker / segmented worm / horned demon; DUNMONB: dragon /
horse-thing / horned swordsman / owlbear / caped antenna-beast / fanged
tentacle-thing). CGA colour is set **once**: `renderDungeonView` calls
`rtm_FE29` (leglib `seg007:0x27391`) with mode `0x0A` → port `0x3D8`,
colour `0x30` → port `0x3D9` (+ the matching BIOS `INT 10h,AH=0Bh` via
`sub_205CE`) — a fixed **palette 1** (black / cyan / magenta / white),
black border. No register cycling anywhere in the engine; the overworld
(`loadOverworldData`) does the same one-shot `rtm_FE29` but takes its
two bytes from the loaded map data (`ds:1E2A[+4]` mode, `[+2]` colour),
so different `OUTM*` regions *can* ship a different fixed palette — still
set once per load, never animated.

## Overworld data — `OUTM*` / `OUTDATA` — **terrain records decoded 2026-08-31**

Same architecture as the dungeon: `OUTM0/1/2.BSV` (`BSAVE` →
`0x86AE:0x0000`) are the map layers, and `OUTDATA.BSV` (`BSAVE` →
`0x86AE:0x2B22`) loads `0x2B22` bytes later in the **same segment**, so
they're one contiguous array (bound at `ds:1E2A`) — `OUTM` at array
byte 0, `OUTDATA` at `0x2B22`.

**`OUTM0/1/2.BSV`** — the overworld map. `loadOverworldData` builds the
name `OUTM0<combatPhase>.BSV`, so the three are alternate world states
(main / post-event / endgame — 9962 / 4184 / 2094 B). 8-byte header,
then 1 byte per tile. The base terrain grid is **95 tiles wide** (`0x5F`
— autocorrelation, and `resolveMoveTarget` does
`array[0x120 + Y·0x5F + X]` to copy a 13×13 window for move/collision
logic; this is the `imul ds:2444h, 0x5F` — *not* a record size). The
separate feature list packs its ids `128`-wide (`y = id/0x80`,
`x = id & 0x7F`).

**`OUTDATA.BSV`** (14228-byte payload):

| span | contents |
|---|---|
| `0x000`–`0x3FF` | **256 × 4-byte terrain-tile records.** Record `T` = the four **8×8 sub-cell indices** for terrain-tile-type `T`, in a **2×2 layout** — bytes `[0]`=top-left, `[1]`=top-right, `[2]`=bottom-left, `[3]`=bottom-right. Values `0x01`–`0x91` (+ `0xDB` = blank, for the unused `0xF0`–`0xFF` types) index the sub-cell bank below. 253 of 256 defined; tile `0x2C` = `3A 3A 3A 3A` (the plain ground quad). Transition tiles come in groups of four (the edge rotations). |
| `0x400`–`~0xD1F` | **the 8×8 sub-cell bitmap bank** — ~146 field-interleaved 16-byte CGA cells, cell `N` at `0x400 + N·16` (cell 0 = blank). Rendered = coastlines, forest/grass, mountains, water, roads. |
| `0xE00`–`0x13FF` | zeros |
| `0x1402`–`0x1AC9`, `0x1F5C`–`0x3793` | **object / creature sprites** — **64 records × 124 B** in two banks (14 + 50 records). Each is a **stock MS-BASIC `PUT` GET-array**, same convention as `DUNMON`: `dw 0x0028` (X extent = 40 **bits** → 20 px), `dw 0x0014` (20 rows), then `20 × ⌈40/8⌉ = 100 B` of linear 2 bpp pixels (5 B/row, MSB pair = leftmost, colour 0 transparent), then **20 B zero padding** to the 124-B stride. So each sprite is **20 × 20 px, one figure** — *not* "two side-by-side frames" (that was a wrong 10 B/row reading). Records pair up: even = colour image (`basPutSprite` verb 0), odd (= image `+0x7C`) = `AND`-mask silhouette (verb 1) → **32 pairs**. `chainExec`'s tail draws them for the travel events ("PEGASUS SETS YOU DOWN", "AMBUSHED BY BANDITS!"): a walking man, an armoured warrior, a sword-fighter, the pegasus, a centaur, angel wings, a bat, and ~24 overworld monsters (bug, scorpion, octopus, spider, serpent, eye-beast, …). Static single-frame — no animation. `decoders/outdata.py <file> <outdir>` writes `outdata_sprites.png`. |
| gaps + tail | zero padding |

**Renderer** — the overworld uses `drawTileRun` just like the dungeon.
`refreshMapView` walks the 9×9 map window around the player; for each
tile it looks up the 4-byte record at `OUTDATA + tile·4` and writes the
four sub-cell bytes into a `26`-wide tile-index buffer (TL/TR on one
row, BL/BR on the row `0x1A` below). `rtm_FE69` then blits that buffer
as a `26 × 17` grid of 8×8 cells via **`drawTileRun`** (`0x1A` cells per
row, video `+0x140`/row). `sub_28156` hands `drawTileRun` its source:
`srcSeg` = the `ds:1E2A` array segment (OUTM + OUTDATA), `srcBase` =
array byte `0x2F22` = **OUTDATA payload `0x400`** — the override taken
when `ds:2082h == 0x23CD` (the overworld tile-buffer word-base, byte
`0x479A`). So the terrain sub-cells live in `OUTDATA`, **not** `OUTOBJ`.
Partial edge tiles are clipped by `readTileObject` (`ds:2444h`–`ds:2452h`)
with `rtm_FE1B` (`rep stosb`) / `rtm_FE14` (`rep movsb`).

`OUTDATA`'s own graphics are **fully decoded** (`decoders/outdata.py`) —
terrain tiles, the sub-cell bank, and the 32 travel-event sprite pairs
(pegasus / bandits / creatures at `0x1400`+).

### `OUTOBJ.BSV` — overworld landmark-icon sprites — **structure decoded 2026-09-01**

`loadOverworldData` `BLOAD`s the 4388-byte payload into `spriteBank`
(`ds:1E58`) at **byte 0** — the BSAVE `0x0DB6` is ignored, exactly like
`DUNOBJ`.

| span | contents |
|---|---|
| `0x00`–`0x11` | **9-word pointer table** — `0x2B0, 0x4B0, 0x2B0, 0x6B0, 0x830, 0x830, 0x830, 0x9B0, 0xB30` → 6 distinct sprite blocks (entries 0/2 share `0x2B0`; entries 4/5/6 share `0x830`). |
| `0x12`–`0x2AF` | mostly zero — `identifyLocationObject` / `refreshMapView` fill parts of this at runtime as an object-slot scratch array (`spriteBank[0x120]` is the slot counter, `spriteBank[0x11E]` the enable flag `rtm_FE69` checks). |
| `0x2B0`–`0xFF5` | the **6 sprite blocks** — CGA 2 bpp bitmap data, `512 / 512 / 384 / 384 / 384 / 1222` B. |
| `0xFF6`–`0x1024` | a `dw 0x1560 + 2·n` **video-column address list** (~24 entries), `0x0001` terminator. |

`spriteBank[8]` (word 4 = `0x830`) is the slot **`PEGASUS.BSV`** (1152 B,
raw CGA bitmap, no table) overwrites when `combatPhase == 2` (the
endgame pegasus travel).

**The blit pipeline** (traced 2026-09-01, not fully resolved).
`identifyLocationObject` walks the current tile's location-type list and
appends a 3-word slot per landmark to the scratch array
(`spriteBank[0x120]` = slot count, `[0x11E]` = enable). Each slot holds
a sprite pointer (`spriteBank[0x10]` = the block-8 pointer `0xB30`) and
a tile-buffer cell position computed as `0x34·row + 2·col + 0xDC`.
`refreshMapView` phase 2 shuffles the slot table
(`spriteBank[i·2+0x94] → [i·2]`), then `rtm_FE69`:

1. `sub_282FD` — for each slot, **punches `0xFFFF` (transparent) into
   the 26-wide tile buffer** at the landmark's cell (2 rows), saving the
   displaced terrain tiles into `spriteBank` scratch.
2. `rtm_FE0E` — copies `OUTOBJ` bytes `0..0x1B8` into the tile buffer at
   `tileBuf[0x1D6]`, **remapping every byte through a 256-entry table at
   `tileBuf[0x1D8]`**.
3. the terrain `drawTileRun` pass (`26 × 17` cells) paints the merged
   buffer.
4. `rtm_FE6A` clears the slot scratch.

**Still open:** how a block's bytes become drawable cells. The pipeline
is a tile-cell merge (via the `rtm_FE0E` remap), not a straight bitmap
blit, and the blocks don't render coherently at any fixed stride. Needs
a live memory dump — the tile buffer plus the `0x1D8` remap table with a
landmark on screen.

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

## `LEGACY.DAT` — master string / data table — **decoded 2026-09-01**

Loaded once by `MENU.EXE`'s `menuStartup` (`decoders/legacy_dat.py`):

| span | contents |
|---|---|
| `0x000`–`0x005` | header: `dw 0x0605` (`w0`; `w0/2 − 3` sets the font-load count), `dw 0x0A04`, `dw 0x017E` (= 382, the `CHAR.DAT` record length). |
| `0x006`–`0x605` | **the software font** — 1536 B / 768 words → resident LEGLIB array `ds:1EC0`. **96 glyphs, ASCII `0x20`–`0x7F`, 16 bytes each = 8×8 px, CGA 2 bpp, field-interleaved** (words → screen scanlines 0,2,4,6 then 1,3,5,7 — the engine's universal tile-cell layout). `drawStringInner` → `sub_28B90` → `rtm_FE34` renders **all in-game text** from it: `glyph = fontBase + (ord(ch) − 0x20)·16`, then `sub_1FED8` blits the cell, advancing 2 bytes (8 px) per character. `decoders/legacy_font.py`. |
| `0x604`–`0x627` | 18 words → resident array `ds:1E8E` `DIM(5,2)` (3×6) — the **game-speed timing table**: `1E8E[speed][phase]` → a message / animation delay, indexed by `ds:1AC8` (game speed 0–5), min-clamped to `0x3C`. Values `0,5,10,15,20,25 / 0,8,12,16,20,24 / 0,4,10,20,34,52`. Read by `rtm_FE4B`. (Overlaps `0x604`–`0x605` with the font's last word.) |
| `0x628`–`0x647` | 16 words → resident array `ds:1C4E` `DIM(3,3)` (4×4) — the **first-person movement table**: `1C4E[(key−1)·4 + facing]` → a packed value, `v>>6` = turn quadrant, `v & 0x3F` = step delta. Values `−24,64,24,192 / 1,64,−1,192 / 24,64,−24,192 / −1,64,1,192`. Read by `DUN`/`MUS` `doMovement`. |
| `0x648`–`0xA02` | the **string pool** — 123 strings, each `db len ; db chars`, loaded into 8 BASIC string arrays (`ds:1CAE`…`ds:1DF0`) via `rtm_FE68`, in fixed order: `[0..18]` the A–Z command names (`Armor`, `Climb`, `Disembark`, `End`, `Fight`, `Gamespeed`, `Hold`, `Inventory`, `Leave`, `Magic`, `Open`, `Pass`, `Rob`, `Speak`, `Take`, `Use`, `" "` (Q — unused), `Weapon`, `Xamine`); `[19..23]` weapon condition (`Shoddy`…`Superb`); `[24..32]` weapons (`bare hands`…`Compound bow`); `[33..37]` armor (`Studded hide`…`Mythan plate`); `[38..54]` per-item "use" verbs; `[55..78]` items (`nothing`, `Gold armband`, … the 7 gem coins); `[79..84]` spells (`Magic flame`…`Seek spell`); `[85..88]` directions; `[89..100]` menu responses; `[101..110]` digits `"0"`–`"9"`; `[111..122]` the 12 **town names** (`Isle City`, `Cobbleton`, `Alanville`, `Grand Ledge`, `Big Rapids`, `Thornberry`, `Mazelton`, `Thompson Crossing`, `Merchant Square`, `Laingsburg`, `Holy Point`, `Eagle Hollow`). |
| `0xA03`–end | a 382-B **new-character template** = one complete `CHAR.DAT` record copied into empty slots. Split field-by-field in the [`CHAR.DAT` section](#chardat--character-roster--save--container-decoded-2026-08-31): 14-B name + 74-B scalar block (gold 20, HP 200, strength 15, 5 items, …) + the 7 arrays, of which **S5 = the shop price table** (`7, 400, 350, 350, 13, 500, 220, 950, 450, 150, … 2000, 5, 21`). `decoders/char_dat.py` prints the whole split. |

The string-pool ordering is the game's canonical index for weapons,
armor, items, spells and towns — the `CHAR.DAT` equipment scalars and
the shop code index into it.

`MENU.EXE` is the only module that opens LEGACY.DAT; the font and the
two tables land in **resident LEGLIB DGROUP** (`ds:1EC0` / `ds:1E8E` /
`ds:1C4E`, the latter two just past the `CHAR.DAT` array descriptors) so
they ride the `OUT`↔`DUN`↔… EXE chain. (The `0x006` region was
previously mis-called "command-menu icon bitmaps" — it is the font, and
`1C4E` is the nav table, not icons. the `readLegacyDat` name on
`MENU.EXE`'s text line-parser is misapplied — that routine parses a
line-oriented file testing for CR / ESC / `'0'` / `'H'` and never
touches this binary layout.)

## `CEL*.BSV` / `DIS*.BSV` — cel animation + exhibit illustrations — **decoded 2026-08-31**

`CEL0`–`CEL3` + `DIS9` are the 5 frames of `CELDRV`'s "AGAINST ALL
ODDS!" endgame cinematic; `DIS0`–`DIS15` (+ `DIS0A`/`DIS1A`) are the
museum exhibit illustration screens (`MUS.EXE`). One container
(`decoders/cel_image.py`):

| span | contents |
|---|---|
| `0x00`–`0x0F` | 8-word header: `[id, 0x0010, W, H, 0x0020, 0x000A, mode(0x10/0x30), stripBase(0x0220)]`. `W`/`H` in pixels (`CEL0`/`DIS9` = 272×120). |
| `0x10`–`0xDF` | mostly zero (some files put a tiny extra table at `0x20`). |
| `0xE0`–`stripBase` | **the cell table** — up to 80 entries `dw videoDest ; dw stripPtr`, ending exactly at `stripBase`. `videoDest` = CGA even-field byte offset — bands step `0x140` (8 screen scanlines), columns step `+2` (8 px) → a 10-col × 8-row grid of 8×8 cells (fewer for small frames). `CELDRV` relocates `stripPtr` by the BLOAD word-offset. |
| `stripBase`–EOF | **the strip data** — 16-byte **field-interleaved** 8×8 CGA cells (word order → screen rows 0,2,4,6,1,3,5,7), packed 16-B-aligned. A cell-table entry whose gap to the next distinct `stripPtr` is `k·0x10` owns a horizontal run of `k` cells (drawn from `videoDest`, `+2` per cell). |

**There is no RLE.** The "compression" is that a frame stores only its
*changed* 8×8 cells and de-duplicates identical ones (several table
entries can share one `stripPtr`). The 5 cinematic frames are painted in
sequence over the same ~80×64-px region — a blast, a standing figure,
the aftermath.

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
