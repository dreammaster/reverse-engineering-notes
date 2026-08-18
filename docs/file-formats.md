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

  Note these are all multiples of 0x10 in the high nibble, confirmed
  bitfield encoding — the low nibble **is used**, not unused: `attack`'s
  dungeon-combat code (traced via `loc_11451`/`map_get_monster_at?`,
  see `docs/overview.md`) checks the low 3 bits (`al & 7`) of the cell
  in front of the player to detect **monster presence**, and clears
  those bits (`and 0F0h`) on a kill while preserving the terrain type.
  So dungeon monster tracking has two independent layers: the
  `_mapMonsters`-style per-slot fields (position, HP, type — see
  `monx??` below) *and* a presence flag baked directly into the
  dungeon map's own tile bytes, specific to dungeons/towers (overworld
  combat uses only the per-slot fields, via `find_target_monster`).

## `monx??` — Monster/NPC data

**No wiki page exists.** The summary page links to "Ultima II Monster
Format" under `monx??`, but the link is a redlink
(`.../w/index.php?title=Ultima_II_Monster_Format&action=edit&redlink=1`)
— the page has never been created. Checked 2026-08-17; same is true of
the "Ultima II Save Game Format" page linked for `player`. This format
has to come entirely from our own disassembly work — no external
source to cross-check against.

- Same `nn` map-number suffix scheme as `mapx??`, patched identically —
  see `save_game1` (asm ~6700-6708) patching both `aMapxff_0+4/+5` and
  `aMonxff+4/+5` from the same two `player` digit fields in the same
  routine.
- Loaded/saved as a companion file alongside the matching `mapx??` —
  always the pair (map file, then monster file) in both `load_map` and
  `save_game1`.
- **Fixed size: 256 bytes** (`load_map` reads it with `mov cx,100h`
  into `_mapMonsters`, same size confirmed on the `save_game1` write
  side).
- **Confirmed against real files, 2026-08-19**: real `monx??` files on
  disk are actually 384 bytes, same story as `tlkx???` above — bytes
  `[256:384]` are a stale duplicate of part of `[0:256]`, not new
  data, and the game's 256-byte read is the correct logical size.
  Checked `MONX00` specifically: `[257:384] == [129:256]` exactly.
- **Layout: struct-of-arrays, not one struct per monster.** The game
  supports up to 32 monster/NPC slots per map (`byte_1742F` in the
  movement-AI loop, asm ~2851-2870, counts down from `1Fh`=31 to 0).
  Rather than one N-byte record per monster, each *field* is its own
  32-byte array (one byte per monster slot), and the monster's slot
  index (0-31) is used as a common array index across all fields
  (`di`).
- **All 8 fields decoded, IDB fully split (2026-08-18,
  `ida_scripts/split_map_monsters.py`) — the entire 256-byte buffer is
  now accounted for, no gaps.** `_mapMonsters` was originally declared
  as a single flat `0xBC` (188)-byte array, undersized vs. the true
  256-byte file; each field below is now its own named 32-byte array,
  so code renders as `_monsterType[di]` instead of
  `(_mapMonsters+60h)[di]`:

  | Offset | Field name | Evidence |
  |---|---|---|
  | `+0x00` | `_mapMonsters` (kept — already the established name) | Map X position. `find_cursor_target_monster` (`cast`'s monster-finder) matches it against `_mapX + _mapLeft` (viewport→absolute X) when locating "the monster under the cursor" |
  | `+0x20` | `_monsterMapY` | same routine, `_mapY + _mapTop` against this field |
  | `+0x40` | `_monsterSpellHP` | `cast` subtracts a computed spell-damage value from it and checks for "borrow" (death) — same borrowless-subtract idiom as `player._hp`, a separate pool from the runtime melee HP |
  | `+0x60` | `_monsterType` = **`TileId × 4`** | `attack`'s post-kill dispatch *and* `transact`'s NPC flavor-line dispatch both confirm exact matches: `"A GUARD SAYS: PAY YOUR TAXES!"`=`0x60`, `"A JESTER SINGS..."`=`0x64`, `"A MERCHANT SAYS..."`=`0x68`, plus `0xF0`/`0xF4`/`0xF8`/`0xFC` (Fighter/Cleric/Wizard/Thief) — `TILE_MINAX×4`, `TILE_GUARD×4`, `TILE_THIEF×4`, `TILE_FIGHTER×4` from the `TileId` enum. Same ×4 encoding as `mapx??` tile bytes — monster identity *is* its overworld tile ID |
  | `+0x80` | `_monsterGlyphTile` | written directly onto the map as a tile byte after a non-lethal hit; also compared against a caller-supplied value in `find_cursor_target_monster` (a "match this specific glyph" parameter for `cast`'s detection). Likely a companion/duplicate of `_monsterType` rather than something distinct — both get zeroed together on death — not fully distinguished yet |
  | `+0xA0` | `_monsterOfferFlag` | **dual-use, confirmed via 2 independent call sites**: `offer` reads it as an offer-type (`0x81`/`0x82`/`0x83`, high bit + 1-3 index) gating a location-specific gold-offer script; `transact` checks the same high bit (`cmp al,80h`) to decide "is this NPC a shopkeeper", falling through to the `_monsterType` flavor-line dispatch if not. One field, two addressing idioms in different code paths — see the correction note below |
  | `+0xC0` | `_monsterTempX` | **not persistent monster data** — temporary scratch holding a saved copy of `_playerX` during a position swap, later copied into `_mapMonsters[di]` |
  | `+0xE0` | `_monsterTempY` | same swap pattern as `_monsterTempX`, for `_playerY` |

  **Correction to an earlier session's notes (2026-08-18)**: this used
  to be documented as "6 on-disk fields (`+0x00`-`+0xA0`) plus 8
  separate *runtime-only, not saved to disk* fields at
  `_mapMonsters+0x137` onward" — that framing was a misunderstanding.
  `_mapMonsters` itself sits at **segment-relative offset `0x137`**
  (segment `sg08e3` starts at linear `0x17410`; `_mapMonsters` is at
  `0x17547`; difference = `0x137`). Every one of those "runtime"
  offsets is exactly `_mapMonsters + {0x00, 0x20, 0x40, 0x60, 0x80,
  0xA0, 0xC0, 0xE0}` — the **same 256-byte buffer**, just accessed via
  raw segment-literal displacements (`[di+1D7h]`) in some code paths
  instead of the symbolic `(_mapMonsters+A0h)[di]` form IDA renders
  elsewhere. There is no separate unsaved region, and `+0xC0`/`+0xE0`
  (previously "unmapped, no code found referencing") are real fields,
  just reached via that raw-literal path. Two other offsets that used
  to be claimed (`+0x177` "AI-wander cooldown", `+0x1B7` "cached
  terrain tile") don't appear anywhere in the current `.asm` and were
  dropped as unverifiable rather than carried forward.
- **`attack`'s full monster-kill drop table, traced end to end
  (2026-08-18)** — dispatches on `_monsterType`, one BCD-encoded drop
  (or none) per branch, then falls through to shared cleanup that
  zeroes the monster's slot (`_monsterGlyphTile`/`_monsterSpellHP`/
  `_mapMonsters`/`_monsterMapY`/`_monsterType`):

  | `_monsterType` | Monster | Drop |
  |---|---|---|
  | `0x40` | Minax | special — routes into `minax_death_sequence` instead of a drop |
  | `0x60` | Guard | `_keys` +2, unconditional |
  | `0xFC` | Thief | `_thievesTools` +1 on a 25% roll (`rand_byte()<0x40`), **then unconditionally** a 15/16 chance (`rand_byte()&0xF != 0`) to increment a *random* slot of `player._ringOwned[]` (any of the 16 treasure items) |
  | `0xF0` | Fighter | `_helmsOwned` +1 on a 25% roll, **then unconditionally** `_torches` `+(rand_byte()&3)+1` (1-4) |
  | `0xF8` | Wizard | unconditional 50/50 (`rand_byte()&1`) between `player._ringOwned[1]` (Wand) or `player._ringOwned[2]` (Staff) — matches the barkeep hint "MAGES CARRY WANDS OR STAFFS!" from the treasure-item block investigation |
  | anything else (e.g. Cleric `0xF4`) | — | no drop, straight to cleanup |

  This also resolves an old open item: what earlier notes described as
  "the Thief/Fighter kill branches touch a small array at
  `[idx+0D6h]`, not traced" is simply `player._ringOwned[]` (the
  16-element treasure array — see the treasure-item block writeup),
  addressed via `player._ringOwned[di]` in the actual code, not a
  separate/mysterious array.

  All the flat-amount drops (`_torches`/`_keys`/`_thievesTools`) are
  BCD-encoded (`daa` after the add), same style as `_hp`/`_food`.
  `_helmsOwned` (Fighter, 25% chance) was the first of these traced,
  consumption site found once `view`'s IDB inline-data gap was fixed:
  `view` requires it nonzero ("VIEW WHAT?" otherwise) and decrements it
  to show the world map ("VIEW WITH MAGICAL HELM!") — see
  [overview.md](overview.md#text_strings-treasure-item-block-traced--a-16-element-inventory-array-unifying-8-prior-findings).

  `_monsterGlyphTile` (`+0x80`) vs. `_monsterType` (`+0x60`) are
  confirmed genuine companions, not redundant: `_monsterType` is the
  identity/dispatch value (compared against `0x40`/`0x60`/`0xFC`/etc.
  throughout), while `_monsterGlyphTile` is the value actually written
  onto the map as a display tile byte — a separate copy for rendering,
  cleared together with the rest of the slot on death.

  **The "killed Minax" transposition oddity, fully traced (2026-08-18)
  — resolved as far as static analysis can settle it.** `attack` has
  **two entirely separate "you killed Minax" code paths**, not one:
  - **Melee-death path** (`loc_12CA4`, reached when a regular hit
    drops `player._hp`-style melee HP below 0): does one
    `get_player_tile` lookup at the monster's true position, writes
    `_monsterGlyphTile` there (restoring the terrain glyph), then
    checks `_monsterType==0x40` and calls the full
    `minax_death_sequence` — "MINAX IS DEAD!! ALL HER WORKS SHALL
    DIE!", the dramatic multi-line victory animation.
  - **Spell-death path** (`loc_12C38`, reached only via the separate
    `_monsterSpellHP` borrow branch — i.e. killing Minax with a spell
    instead of melee): has its own simpler, self-contained handling,
    *not* shared with the melee path and *not* calling
    `minax_death_sequence` at all. It restores the glyph at Minax's
    true position (same as the melee path), **then deliberately swaps
    `_mapMonsters[di]` and `_monsterMapY[di]`'s values with each
    other**, does a *second* `get_player_tile` lookup at the
    now-swapped (transposed) coordinates, and writes `_monsterType`
    there — then prints the shorter "SHE'S GONE!!!" and sets
    `_monsterOfferFlag[di]=0x84` (an unusual value, likely a
    dead/gone marker).

  The swap is structured, deliberate code — two distinct
  `get_player_tile` calls with genuinely different coordinate pairs,
  different payloads written to each — not a disassembly artifact or
  copy-paste mistake. Whether the *design intent* was really to leave
  a marker at the transposed cell (a strange but real choice), or
  whether swapping the fields was itself an authoring slip in the
  original 1983 code, can't be settled from static analysis alone —
  it would take either external documentation (none exists for this
  port) or watching it happen live in-game to know for sure. Treating
  this as closed for now: the mechanism is fully understood even
  though the "was it a bug" verdict is inherently unknowable without
  runtime observation. Worth noting as a fun side effect: spell-killing
  Minax gives a *shorter* victory message than melee-killing her — an
  asymmetry that may itself be intentional (different flavor per kill
  method) rather than an oversight.

## `MONSTERS` — dungeon corridor monster-marker position table (traced 2026-08-19)

Distinct from `monx??` (the per-map monster/NPC instance data above) —
this is a single master file, loaded once at `play_game` startup (asm
~10807), name literally `MONSTERS` (no digit suffix). **Despite the
generic name, this is not a monster stat/type/name table at all — it's
a small, fixed lookup of screen positions for drawing monster markers
in the dungeon 3D corridor view.** Full consumer chain traced:

- Read with `mov cx,800h` (2048 bytes) into scratch buffer
  `monsters_ptr` (`0x2900`, DATA segment). Real file on disk is 2176
  bytes — same inert-tail pattern as every other format on this page
  (2176 − 2048 = 128 bytes unread).
- **Sole consumer: `draw_dungeon_monster`** (asm ~12856), called once
  per frame from `render_dungeon_view` right after the corridor walls
  are drawn. For each of up to 8 look-ahead corridor depth steps
  (`di` = 1..however far the corridor is currently visible, tracked in
  `byte_17892`):
  1. Checks `[di+4AFh]` (DATA:0x4AF, an 8-entry per-frame scratch
     array) — nonzero means "draw a monster marker at this depth."
     **This flag is not new data** — it's the same dungeon-tile
     "monster presence" sub-field already documented above (the low 3
     bits of the tile byte), re-extracted fresh every frame by
     `precompute_dungeon_corridor` (asm ~11809, which also fills the
     parallel left/straight/right wall-type caches at `[di+48Fh]`
     `[di+497h]` `[di+49Fh]` that the corridor-wall-segment drawer
     uses) for the 8 tiles directly ahead of the player. So dungeon
     monster rendering and dungeon monster *combat* (`attack`,
     `map_get_monster_at?`) both key off the exact same tile bitfield,
     just cached separately.
  2. For `di >= 2`: looks up `cs:byte_1697E[di]` (asm ~12934), a table
     with exactly 6 nonzero entries for `di`=2..7 — `0x80, 0xC0, 0xE0,
     0xF0, 0xF8, 0xFC` — each a **fixed byte offset into the
     `MONSTERS` buffer**, one per visual depth band. (`di == 1`, the
     nearest step, takes a different addressing path whose input
     register state comes from the caller and wasn't traced further —
     flagged open below.)
  3. Reads a **2-byte record** from `MONSTERS` at that fixed offset:
     byte 0 is used almost directly as an X screen-position value;
     byte 1 is packed — low 5 bits = Y position, top 3 bits (extracted
     via 5x `rcr` with `clc` first, i.e. a plain `>> 5`) = a
     facing/type value 0-7.
  4. Feeds `(X, Y, facing)` into `draw_dungeon_monster_sprite` →
     `draw_sprite_row`, which reuses the **same shared low-level CGA
     pixel-plot tables** (`cs:48D0h`/`48D4h`/`48DCh`/`48DEh`) as
     `plot_point`/`erase_point` (the space-flight starfield/line-draw
     primitives) to paint a small 4-pixel-wide marker at a screen
     position fixed for that depth band.
- **Confirmed against the real file**: decoded the 6 addressed
  `(X, Y, facing)` triples from Paul's actual `MONSTERS` file — all
  small, plausible screen-offset-shaped values (X≈30-31, Y≈14-16,
  facing 0-2), consistent with this theory rather than character/stat
  data.
- **Conclusion**: `MONSTERS` supplies fixed marker positions for up to
  6 dungeon-corridor depth bands, nothing more. There is **no
  per-monster-type stat/name table anywhere in this DOS port's data
  files** — monster-type behavior (HP formulas, offer rewards, etc.)
  is hardcoded in code branches keyed on the type-value byte
  (`0xFC`=Thief, `0xF0`=Fighter, `0xF8`=Wizard, ... — see `monx??`
  above), not loaded from disk. **For the C++ port**: model monster
  *types* as a hardcoded table mirroring those branches, not as
  something read from a data file; `MONSTERS` only needs to be
  understood at all for implementing dungeon-view rendering
  specifically.
- **Left genuinely open** (not force-resolved): the `di == 1` special
  case's addressing, and why only ~256 of the file's 2048 logical
  bytes (really only a 126-byte window within that) have any confirmed
  reader — the remaining ~1.8KB has no reader found anywhere in the
  binary. Could be reserved headroom or a carryover from a richer
  original-platform format; left as an honest unknown rather than
  guessed.

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

**Update, 2026-08-19, confirmed against real game data files** (Paul's
own install at `c:\games\ultima2`, not just the EXE): every `tlkx???`
file on disk actually *is* 384 bytes — the wiki's figure is physically
correct after all. But the DOS binary's 256-byte read is still right:
checked byte-for-byte, bytes `[256:384]` of every real `TLKXFF` file
are not new content, they're a stale duplicate of a chunk of bytes
`[0:256]` (the exact overlap offset varies per file — e.g. `TLKX81`
has `[257:384] == [129:256]`, `TLKX03` has a similar but not
byte-identical overlap). That's consistent with leftover
scratch-buffer content baked in by whatever tool wrote these files,
not meaningful game data — and it means the disassembly-only
conclusion above was correct even though it had no real files to
check against at the time. **The same 384-byte-container /
256-byte-logical / 128-byte-inert-tail pattern was independently
confirmed in `player` and `monx??` too** (see their sections below) —
this isn't a `tlkx???`-specific quirk, it's how every fixed-size
record file in this DOS port was packaged. For the C++ port: read
(or just allocate for) the full on-disk size if it matters for file
I/O parity, but only the first 256 bytes (first `N` bytes generally —
see each format's own logical size) are ever meaningful.

A standalone, non-IDA reference decoder implementing this (confirmed
against every real `tlkx???` file in the install) lives at
[tools/decode_talk_file.py](../tools/decode_talk_file.py).

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
- All 7 read exactly `0x4000` (16384) bytes (`ah=27h` random-block read,
  `cx=4000h`). Traced 2026-08-18 while re-running
  `ida_scripts/fix_access_file_calls.py` (surfaced these 7 filenames as
  a byproduct — none previously catalogued except `PICDRA`): they're
  the title/demo attract-mode slideshow in `start_` (asm ~100-422), each
  screen shown with a matching two-line caption via `write_string`,
  looping back to the title:

  | File | Caption |
  |---|---|
  | `PICDRA` | (title screen, shown before "BY LORD BRITISH") |
  | `PICOUT` | "BATTLE STRANGE CREATURES / ACROSS THE FACE OF THE EARTH" |
  | `PICTWN` | "SEARCH FOR CLUES IN CARELESS WORDS / SPOKEN AT THE LOCAL PUB" |
  | `PICCAS` | "PLEAD WITH MEDIEVAL KINGS / FOR ASSISTANCE" |
  | `PICDNG` | "TRAVERSE DEEP DARK DEADLY DUNGEONS / AND TALL TERRIFYING TOWERS" |
  | `PICSPA` | "TRAVEL THROUGHOUT THE GALAXY / TO THE PLANETS OF OUR SOLAR SYSTEM" |
  | `PICMIN` | "AND CONQUER TIME ITSELF TO BATTLE / MINAX THE ENCHANTRESS" |

## `player` — Save game

- Single fixed file, format = the in-memory `Savegame` struct
  (`sizeof 0x100` = 256 bytes) dumped directly — see
  [overview.md](overview.md#savegame-asm-line-22-sizeof-0x100--256-bytes)
  for known fields. No separate wiki page found/fetched yet for this
  format specifically (ModdingWiki calls it "Ultima II Save Game
  Format").
- **Confirmed against a real file, 2026-08-19**: the real `PLAYER`
  file on disk is 384 bytes, same pattern as `tlkx???`/`monx??` above
  — bytes `[256:384]` duplicate part of `[0:256]` (`[257:384] ==
  [0:127]` exactly), not new data. The `Savegame` struct is still
  correctly 256 bytes; ignore the trailing 128 on disk.

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
  - Tiles 20, 40, 63 — matched against the full wiki legend below once
    fetched: 20 = Rocket (the diagonal branching shape reads as a
    rocket silhouette), 40 = "I / Door" (an exact match — solid
    top/bottom bars with a thin stem is a textbook serif capital "I"),
    63 = Thief (the humanoid figure).
- **Applied**: `tile_00`..`tile_63` renamed in the IDB via
  `ida_scripts/label_tile_graphics.py` (`DRY_RUN = False`). Paul
  deliberately kept these as `tile_NN`, not semantic names — see below
  for where the semantic names live instead.

### Full tile ID legend (0-63) and the `TileId` enum

Fetched verbatim from the wiki (no gaps guessed/filled in — confirmed
with Paul that the alphabet gaps below are original-game-intentional,
not a fetch error: the in-town sign-lettering font never needed the
full alphabet):

| ID | Name | ID | Name | ID | Name | ID | Name |
|---|---|---|---|---|---|---|---|
| 0 | Water | 16 | Minax | 32 | A | 48 | Moongate |
| 1 | Swamp | 17 | Horse | 33 | B | 49 | R |
| 2 | Grass | 18 | Ship | 34 | C | 50 | S |
| 3 | Forest | 19 | Airplane | 35 | D | 51 | T |
| 4 | Mountain | 20 | Rocket | 36 | E | 52 | U |
| 5 | ? | 21 | Shield | 37 | F | 53 | V |
| 6 | Town | 22 | Sword | 38 | G | 54 | W |
| 7 | Tower | 23 | Forcefield | 39 | H | 55 | X |
| 8 | Castle | 24 | Guard | 40 | I / Door | 56 | Y |
| 9 | Dungeon Entrance | 25 | Jester | 41 | J | 57 | Z |
| 10 | Signpost | 26 | Shopkeep | 42 | K | 58 | Counter End, Right |
| 11 | Sea Monster | 27 | ? | 43 | L | 59 | Counter End, Left |
| 12 | Orc | 28 | Road | 44 | M | 60 | Fighter |
| 13 | Daemon | 29 | Empty | 45 | N | 61 | Cleric |
| 14 | Devil | 30 | Wall | 46 | O | 62 | Mage |
| 15 | Balron | 31 | Empty Counter / Space | 47 | P | 63 | Thief |

Note IDs 5 and 27 have no name on the wiki page (marked `?` there too
— not a gap in our transcription).

This is where the **semantic** tile names live, as an IDA enum
(`TileId`, e.g. `TILE_TOWN = 6`) rather than as renames of the
`tile_NN` graphics-array symbols — deliberate choice, so the enum is
available anywhere a raw tile-ID literal shows up in code (movement/
collision checks, etc., see `docs/overview.md`'s `canMoveToTile`
notes) without conflating "the graphics data for tile 6" with "the
numeric ID 6". **Applied** via `ida_scripts/create_tile_id_enum.py`
(idempotent, safe to extend and re-run for future finds). Remember the
on-disk byte in `mapx??` files is the ID **×4**, not this raw 0-63
value — see the "divide by 4" note earlier in this doc.
