# On-disk file formats

Every file `C:\games\ULTIMA1\*` the five executables read or write, and
what's confirmed about each one's internal layout. Confirmed facts are
cited by the function/global that proves them (all in `OUT.EXE` unless
noted); anything not traced is flagged as such rather than guessed.
See [overview.md](overview.md) for the functions/structs named while
tracing these, and [roadmap.md](roadmap.md) for what's still open.

## Inventory

| File | Size (bytes) | Kind | Status |
|---|---|---|---|
| `player1.u1` … `player4.u1` | 820 | savegame record | fully decoded |
| `inuse.u1` | 820 | savegame record (session hand-off) | fully decoded |
| `map.bin` | 13,104 | overworld terrain, nibble-packed | fully decoded |
| `tcd.bin` | 6,840 | 10 reusable town/castle/dungeon-room layouts | fully decoded |
| `castle.4` | 16,384 | title-screen image, CGA (2bpp) | format confirmed (raw framebuffer dump) |
| `castle.16` | 32,000 | title-screen image, EGA/VGA (4bpp) | format confirmed (raw framebuffer dump) |
| `cga`/`ega`/`t1k` + `tiles.bin` | 3,328 / 6,656 / 6,656 | overworld tile graphics | loaded whole; pixel packing not decoded |
| `cga`/`ega`/`t1k` + `town.bin` | 816 / 1,632 / 1,632 | town/castle tile graphics | loaded whole; pixel packing not decoded |
| `cga`/`ega`/`t1k` + `fight.bin` | 3,192 / 3,192 / 3,192 | combat-screen graphics (`SPACE.EXE`) | loaded whole; not decoded |
| `cga`/`ega`/`t1k` + `mond.bin` | 1,216 / 2,432 / 2,432 | Mondain-encounter graphics (`MONDAIN.EXE`) | loaded whole; not decoded |
| `cga`/`ega`/`t1k` + `space.bin` | 7,296 / 7,296 / 7,296 | space-combat graphics (`SPACE.EXE`) | loaded whole; not decoded |
| `nif.bin` | 6,720 | unknown | **not referenced by name in any of the 5 executables** |
| dungeon levels | — | *not a file* | procedurally generated at runtime, see below |

Filenames are matched case-insensitively by DOS; the executables spell
them in a mix of cases (`"CgaTiles.Bin"`, `"tcd.bin"`, `"map.bin"`) —
normalized to lowercase above.

## Savegame record (`playerN.u1`, `inuse.u1`) — 820 bytes

The `Savegame` struct, synced byte-for-byte across all four IDBs that
reference it (`ULTIMA.EXE`/`GEN.EXE`/`OUT.EXE`/`SPACE.EXE` — see
[overview.md](overview.md#cross-idb-struct-cleanup--savegame-brought-up-to-date-and-synced)
for how that sync was done and what several of the less-obvious fields
turned out to mean). This is the single character-save format used
everywhere in the game; `player1.u1`–`player4.u1` and `inuse.u1` are
all identical 820-byte records in this same layout, differing only in
which file holds which copy (see below).

| Offset | Size | Field | Notes |
|---|---|---|---|
| `0x000` | 16 | `_name` | `STR15` — 15 chars + implicit terminator |
| `0x010` | 2 | `_race` | |
| `0x012` | 2 | `_class` | |
| `0x014` | 2 | `_sex` | |
| `0x016` | 2 | `_hits` | HP. Index 0 of a 7-word attribute array — see `_strength` below |
| `0x018` | 2 | `_strength` | |
| `0x01A` | 2 | `_agility` | |
| `0x01C` | 2 | `_stamina` | |
| `0x01E` | 2 | `_charisma` | |
| `0x020` | 2 | `_wisdom` | |
| `0x022` | 2 | `_intelligence` | |
| `0x024` | 2 | `_coins` | |
| `0x026` | 2 | `_experience` | |
| `0x028` | 2 | `_food` | |
| `0x02A` | 2 | `_readyWeapon` | index into the weapon inventory table below |
| `0x02C` | 2 | `_readySpell` | index into the spell inventory table below |
| `0x02E` | 2 | `_readyArmor` | index into the armor inventory table below |
| `0x030` | 2 | `_transportType` | current transport (`TRANSPORT_WALKING`, etc.) |
| `0x032` | 2 | `_randomSeed` | |
| `0x034` | 4 | `_position` | `Point` (x, y) on the overworld map |
| `0x038` | 2 | `_soundOn` | |
| `0x03A` | 18 | `_questStatus` | 9-word array indexed by `_castleIndex*2`: `-1`=not offered, `1`=accepted, `0`=reward claimed |
| `0x04C` | 2 | `_redGems` | |
| `0x04E` | 2 | `_greenGems` | |
| `0x050` | 2 | `_blueGem` | |
| `0x052` | 2 | `_whiteGem` | |
| `0x054` | 2 | `_skin` | armor inventory[0] — "nothing worn" baseline, see below |
| `0x056`–`0x05E` | 10 | `_leatherArmor`, `_chainmail`, `_plateMail`, `_vacuumSuit`, `_reflectSuit` | armor inventory[1..5], one word each |
| `0x060` | 2 | `_hands` | weapon inventory[0] — "unarmed" baseline |
| `0x062`–`0x07E` | 30 | `_dagger` … `_blaster` | weapon inventory[1..15], one word each (15 weapons) |
| `0x080` | 2 | `_prayer` | spell inventory[0] — the always-available baseline "spell" |
| `0x082`–`0x094` | 20 | `_open` … `_kill` | spell inventory[1..10], one word each (10 spells) |
| `0x096` | 2 | `_foot` | transport inventory[0] — "on foot" baseline |
| `0x098`–`0x0A6` | 16 | `_horse` … `_enemyVessels` | transport inventory[1..7], one word each |
| `0x0A8` | 2 | `_signMarker` | |
| `0x0AA` | 2 | `_overworldEntityCount` | mirrors the live `_creaturesCount` global across save/load |
| `0x0AC` | 4 | `_moveCount` | mirrors the live `_moveCtr` global |
| `0x0B0` | 2 | `_shipFuel` | (space combat) |
| `0x0B2` | 2 | `_shipShield` | (space combat) |
| `0x0B4` | 640 | `_overworldEntities` | 40 × 16-byte `Creature` records — persistent overworld monsters *and* ships/markers left on the map, see below |

**`_hits`/`_strength`/…/`_intelligence` inventory-array note**: these
7 fields are addressed as one contiguous word array in the disassembly
(`decreaseAttribute`/`increaseAttribute`/`updateAttribute` all index
from `_hits` — see
[overview.md](overview.md#character-creation-point-buy-mechanic-decoded)),
mirroring the exact same "index 0 is a real baseline value" pattern as
the item categories below — `ATTRIBUTES[0]` (a pre-existing OUT.EXE
string table) is literally `"Hit Points"`.

**The 4 inventory categories all follow one convention**: index 0 is
the "nothing owned in this category" baseline (`_skin` = bare skin,
`_hands` = unarmed, `_prayer` = the one always-castable non-item
spell, `_foot` = walking) rather than an item you can run out of;
`_readyWeapon`/`_readySpell`/`_readyArmor` pick which index is
currently equipped. Confirmed via `OUT.EXE`'s pre-existing
`ARMOR`/`WEAPONS_LOWERCASE`/`SPELL_NAMES`/`TRANSPORTS` name tables and
`dropArmor`'s menu loop, which explicitly starts at index 1 (you can't
drop your own skin) — full writeup in
[overview.md](overview.md#item-slot-0-and-creature-padding-resolved).
Each inventory word is a *count* (how many of that item you own), not
a boolean, except the index-0 baseline which is set to the sentinel
`-1` at character creation (confirmed in `GEN.EXE`'s
`generateCharacter`) rather than a real count.

**`Creature`** (16 bytes, the element type of `_overworldEntities`):

| Offset | Size | Field |
|---|---|---|
| `0x0` | 2 | `_type` | tile ID (compared against `TILE_FIRST_MONSTER` to decide "is this a monster") |
| `0x2` | 2 | `_data` | the terrain tile that was underneath, before this entity was placed |
| `0x4` | 2 | `_x` | |
| `0x6` | 2 | `_y` | |
| `0x8` | 2 | `_hits` | |
| `0xA`–`0xE` | 6 | `_unused1`/`_unused2`/`_unused3` | confirmed genuinely unreferenced by any instruction in `OUT.EXE` or `SPACE.EXE`; the array is indexed via `shl ax, 4` (×16) rather than `imul` by the real 10-byte struct size, so this is likely just padding to a power-of-2 stride for fast indexing — see [overview.md](overview.md#item-slot-0-and-creature-padding-resolved) |

## Roster and session hand-off

Three tiers of files share the 820-byte `Savegame` layout, distinguished
only by *when* each gets written/read:

- **`player1.u1`–`player4.u1`** — the permanent 4-slot character
  roster. `GEN.EXE`'s `readSavegameList` opens all 4 (mode `"rb"`) and
  reads just the first 15 bytes of each (the `_name` field, into an
  array of `STR15`) to build the character-select menu — it doesn't
  need the rest of the record for that. Written on an explicit
  "quit and save"; read in full when a slot is chosen to actually play.
- **`playerx.u1`** — not a real file on disk, a *template string*.
  `OUT.EXE`'s `readSavegame(saveNum, param3)` patches the `'x'` byte
  (offset 6 of the 10-character string `"playerx.u1"`) with the ASCII
  digit `saveNum`, turning it into `"player1.u1"` etc. before opening
  it — this is how a single fixed string constant serves all 4 roster
  slots without 4 separate hardcoded filenames.
- **`inuse.u1`** — the transient hand-off file used specifically when
  chaining between executables (`OUT.EXE` ↔ `SPACE.EXE` ↔
  `MONDAIN.EXE`). `readSavegame`'s `param3` flag selects `inuse.u1`
  instead of a numbered roster slot; every chain-exit path (`writeInUseAndExit`,
  `SPACE.EXE`'s `exit()`, `MONDAIN.EXE`'s `mondainMainLoop` startup)
  writes/reads this file immediately before/after the overlay jump.
  Confirmed the same way in every executable that chains.

`saveNum` itself is a *raw ASCII character*, not a small int — valid
values are `'1'`–`'4'` (a real roster slot); anything else (e.g. the
literal `"S"` `SPACE.EXE` passes on its way back to `OUT.EXE`) falls
through to a reset path and relies on `param3`/`inuse.u1` instead.
This is the same argv-passing convention documented for `GEN.EXE` →
`OUT.EXE` (`OUT C 0` — video-mode letter + save-slot digit).

## Overworld map (`map.bin`) — 13,104 bytes

**168 × 156 tiles, 4-bit nibble-packed (2 tiles/byte), row-major, 84
bytes/row.** Decoded directly from `getMapTile2`'s index arithmetic:
`byteOffset = y*84 + x/2`; the tile occupies the low nibble of that
byte if `x` is even, the high nibble if `x` is odd. `84 * 156 =
13,104`, matching the file size exactly, and `84` bytes/row matches
the pre-existing `MapLine` struct's size exactly too.

The raw 4-bit value (0–15) then gets adjusted before use as a real
tile ID (the `TileNum` enum, already defined in this IDB):

```
raw <= 4  ->  tile = raw                      ; OCEAN..CASTLE1 unchanged
raw > 4   ->  tile = raw + 1                  ; skips CASTLE2 (5)
              if tile == CITY2 (8): tile += 1 ; skips CITY2 (8) too
```

So `CASTLE2` and `CITY2` are never actually stored in `map.bin` — the
static terrain only ever encodes `OCEAN`/`GRASS`/`WOODS`/`MOUNTAINS`/
`CASTLE1`/`SIGNPOST`/`CITY1`/`DUNGEON` (raw 0–7) directly. Transport
tiles (`HORSE`/`CART`/`RAFT`/`FRIGATE1`/`FRIGATE2`/`AIRCAR`, tile IDs
11–16) are reachable through the same decode arithmetic but are never
present in the *on-disk* file — they only ever appear in the in-memory
working copy of the map when a `Creature`/widget entity's tile
temporarily overwrites the terrain underneath it (see `_data` above).
`PLAYER` (10) and `SHUTTLE`/`TIME_MACHINE` (17–18) are likewise
runtime-only overlay states, never on-disk terrain.

Loaded via `readFile("map.bin", &_map, size Map)` in `main`'s startup
(`size Map` = 13,104, confirmed exact match).

## Town/castle/dungeon room layouts (`tcd.bin`) — 6,840 bytes

**10 reusable 38×18-tile layouts, one byte per tile (not packed),
684 bytes each** (`38 * 18 = 684`; `684 * 10 = 6,840`, matching the
file size exactly). Decoded from the index arithmetic shared by every
`_townCityMap[bx]` access in `OUT.EXE`:

```
bx = _mapStyle * 684 + _locationPosX * 18 + _locationPosY
```

i.e. `_locationPosX` ranges `[0, 37]`, `_locationPosY` ranges `[0,
17]`, and `_mapStyle` (0–9) selects which of the 10 pre-baked layouts
the *currently entered* location uses. Every named castle/town/dungeon
location on the overworld reuses one of these 10 shared floor plans —
a location's specific identity (name, NPCs, quest state) is tracked
separately from its physical layout. Tile values are the pre-existing
`CityTileNum` enum (`CTILE_GROUND`, `CTILE_CELL_DOOR`, `CTILE_POND`,
`CTILE_KING`, `CTILE_GUARD_MB`, etc. — up to `0x37`+, hence one byte
per tile rather than nibble-packed like the overworld).

Loaded via `readFile("tcd.bin", &_townCityMap, 6840)` in `main`'s
startup, right after `map.bin`.

## Dungeon levels — not a file, generated at runtime

`_dungeonMap` (11×11 grid of 8-byte `DungeonCell` records —
`_tileNum`/`_monsterId`/`_itemId`/`_monsterHp`, using the pre-existing
`DTILE_*`/`DITEM_*`/`UMONS_*` enums) is populated entirely through
direct cell-by-cell writes throughout `OUT.EXE`'s dungeon-navigation
code (`dungeonAttackAt` and friends) — there is no `readFile` call
anywhere that loads it. Ultima 1's dungeons are procedurally generated
at runtime, not stored data; tracing the actual generation algorithm
(seeded presumably from `_randomSeed`/`_moveCount`) is a separate,
substantial undertaking not attempted here — flagged in
[roadmap.md](roadmap.md) as future work, not a file-format question.

## Title-screen image (`castle.4` / `castle.16`)

Raw, uncompressed CGA/EGA framebuffer dumps — `ULTIMA.EXE`'s
`loadLogo` does a single `_fread(buffer, 1, count, fp)` straight into
a screen-sized buffer, no decompression step. `castle.4` (CGA, 2bpp):
`_fread` count `0x4000` = 16,384 bytes, matching the file size exactly
(a 320×200 2bpp CGA frame is 16,000 bytes; the extra 384 is likely
buffer padding/alignment, not file content). `castle.16` (EGA/VGA,
4bpp): matches this project's earlier "raw CGA/EGA framebuffer dumps,
not compressed" finding from the ULTIMA.EXE session; `320*200*4/8 =
32,000` bytes, exactly the file size.

## Tile/sprite graphics files — loaded whole, pixel format not decoded

Every remaining `.bin` file (`*tiles.bin`, `*town.bin`, `*fight.bin`,
`*mond.bin`, `*space.bin`) follows the same pattern: 3 video-mode
variants (`Cga`/`Ega`/`T1K` prefix, selected by `_videoMode`/
`word_163EC`), each read wholesale into a fixed buffer with a single
`readFile`/`_fread` call and no further parsing — `loadTiles`
(`OUT.EXE`) is the clearest example, reading `0xD00`/`0x1A00` bytes
(matching `CgaTiles.Bin`/`EgaTiles.Bin` exactly) straight into `_tiles`.
The data is then blitted directly to video memory (segment `0xB800`
CGA / `0xA000` EGA) by tile-drawing routines like `drawTile`/
`drawCityTile`, confirming it's raw per-video-mode bitmap/sprite data
— but the exact pixel packing (tile pixel dimensions, bit-plane
layout) wasn't traced during this pass; whoever builds the
reimplementation's renderer will need to work that out from the
blit code directly (`drawTile`, `drawCityTile`, and their `SPACE.EXE`/
`MONDAIN.EXE` counterparts) or by cross-referencing against a
screenshot/emulator run, not from this document.

One quirk worth flagging: `_townBin`'s `readFile` always requests
1,632 bytes regardless of video mode, even though `cgatown.bin` is
only 816 bytes on disk (exactly half — consistent with CGA's 2bpp vs.
EGA/Tandy's 4bpp doubling every other `*tiles.bin`/`*town.bin` pair
too). A short `_fread` isn't treated as an error (no `_doserrno` set,
no `insertDisk` retry), so this is presumably harmless — the CGA
rendering path is assumed to only ever touch the first half of the
buffer — but that assumption wasn't verified against the actual CGA
blit code.

## `MONDAIN.EXE`'s startup file loads — corrected

**Note**: the original MONDAIN.EXE session's writeup (see
[overview.md](overview.md#mondainexe--findings-log)) described
`mondainMainLoop`'s 3 startup `readFile` calls as loading "the special
map/room data". Re-checked while writing this document and that
description was wrong — none of the 3 calls load room-layout data:

1. `readFile("inuse.u1", buffer, size Savegame)` — loads the current
   savegame (same 820-byte format as everywhere else), into an
   untyped scratch buffer since this IDB has no `Savegame` struct
   instance (see [overview.md](overview.md#cross-idb-struct-cleanup--savegame-brought-up-to-date-and-synced)
   for why that's correct, not a gap).
2. `readFile("CgaTiles.Bin"/"EgaTiles.Bin"/"T1KTiles.Bin", buffer, 704/1408/1408)`
   — a *partial* read of the shared overworld tile-graphics file (same
   files `OUT.EXE` reads in full) into a scratch area, offset by an
   incoming parameter whose exact role wasn't pinned down.
3. `readFile("CgaMond.Bin"/"EgaMond.Bin"/"T1KMond.Bin", buffer, wholeFile)`
   — the dedicated Mondain-encounter graphics (portrait/dialog art),
   same per-video-mode pattern as every other graphics file above.

The 19×9 encounter room's actual tile layout is therefore **not
loaded from any file** — with only one Mondain encounter in the whole
game (unlike the 10 reusable `tcd.bin` layouts), it's presumably
hardcoded directly in `MONDAIN.EXE`'s own data segment. Not traced
further; low priority since the room's game-logic behavior is already
fully documented in the MONDAIN.EXE findings.

## Open questions

- `nif.bin` — present in the game directory, not referenced by name
  anywhere in the 5 executables traced by this project. Possibly used
  by `MOSLO.COM` (a separate loader/decompressor present in the same
  directory, not part of this reverse-engineering effort) or an
  intro/unused asset. Not investigated.
- Exact pixel/bit-plane format of every `*tiles.bin`/`*town.bin`/
  `*fight.bin`/`*mond.bin`/`*space.bin` graphics file — sizes and load
  paths are confirmed, internal packing is not.
- Dungeon-generation algorithm (seed, layout rules, monster/item
  placement) — confirmed procedural, not traced in detail.
- `map.bin`'s second `readFile` overload target — worth double-checking
  whether anything besides `main`'s startup ever re-reads `map.bin`
  mid-session (not checked).
