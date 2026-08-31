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
decoded; the record's scalar fields are partly identified. The other
`.BSV` files are still container-only.

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
| `DUNDATA.BSV`, `DUNOBJ.BSV`, `DUNM1.BSV`…`DUNM3.BSV`, `DUNMONA.BSV` / `DUNMONB.BSV` | — | `DUN` | dungeon map / objects / monsters |
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
