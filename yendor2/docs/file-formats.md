# File formats

On-disk data formats used by "Yendorian Tales Book I: Chapter 2"
(`SW.EXE`), cross-referenced against the disassembly as they're decoded.
**Nothing below is confirmed against the actual IDB/code yet** — this is
still a placeholder with the observational starting points from the
initial setup session (2026-09-14), now supplemented with two reference
documents Paul added the same day: `docs/manual.txt` (the official game
manual) and `docs/Hex Hacking Item Guide.txt` (a community-written 2004
guide by Josh Hines on hex-editing savegame items — freely
redistributable per its own footer). See [overview.md](overview.md) for
the full session notes and [roadmap.md](roadmap.md) for priority order.

## `CURGAME` / `SAVGAME1` (and presumably `SAVGAMEn`)

77,509 bytes each, identical size. `SAVGAMEX` appears in `SW.EXE`'s
string table as a templated filename (`X` = slot digit), and the
in-EXE error string `"Problem with CURGAME."` / `"Problem with a SAVED
GAME file."` confirms `CURGAME` is the active/working copy distinct from
the numbered save slots.

First 32 bytes of `CURGAME` (hex-decoded):
```
53 4D 49 54 48 57 41 52 45 20 50 41 52 54 59 00   SMITHWARE PARTY\0
57 41 52 45 00 20 20 20 00 00 00 00 00 00 00 00   WARE\0   \0...
```
`"SMITHWARE PARTY\0WARE\0"` — identifies the developer as **SmithWare**
(matches `SW.EXE`'s name) and suggests a header/magic string rather than
game data proper.

**A fourth fixed `FileEntry` reads it directly**: `bx=0x8FFB` (distinct
from `0x9043`=`WORLD.DAT`, `0x902C`=`SAVGAMEX`, `0x9011`=`PICTURES.VGA`
— see `PICTURES.VGA`'s section below for how the filename-at-`+0xE`
convention was found). `LoadCurgameRecord` (`0x1770C`,
`ida_scripts/name_curgame_reader.py`) reads a record from it via EMS
paging, indexed by `word_32DBC*4 + 0x1A*_val9` — the `0x1A`-word (26)
row stride is a plausible per-character record size, worth checking
against the item-slot layout above once the struct is actually located.
Splits one field by 100 into quotient/remainder (currency or time value,
not confirmed which).

**Autosave on movement**: `start`'s main loop writes a small, *fixed-
offset* record (`ax=0x556D`, via the resource stub `sub_27DE5`) back to
this same `FileEntry` whenever `TryInteractAtPosition` (was `sub_216F0`
— validates an interaction at a map position via `FindObjectAtPosition`,
branching on the target's type flags into a weight/capacity check,
`LoadCurgameRecord`, or a specific failure code) returns certain outcome
codes — not indexed by character, so plausibly global state like the
player's world position. `CURGAME` is also explicitly zeroed and closed
cleanly (`FileEntry_Write` with `_blockSize`/`_blockOffset` all zero,
then `FileEntry_Close`) on the quit-to-DOS path. `sub_27DE5`'s target
record itself isn't named yet.

**Party-member record** (in-memory, currently selected one pointed to
by `word_328D4` — plausibly backed by this same `CURGAME` data once
loaded): lives in a **confirmed fixed array**, `g_partyRecords` (base
`0x95F3`, stride `0x1F4` = 500 bytes/record, up to 9 slots per
`SelectDefaultPartyRecord`'s scan bound) — not a linked list.
**Correction**: earlier documentation here claimed `word_328D4` was
"traversed via a `+0x10` 'next' link"; that was wrong (a comment had
been misattached to the wrong call site — see
`ida_scripts/fix_party_record_next_claim.py`). `ApplyMapTriggerEffect`
independently confirms the same base/stride via a separate 4-slot
`g_partySlotAssignment` index table. `word_328D4` itself is set by
several different mechanisms depending on context (a direct selector
struct in some callers, `SelectDefaultPartyRecord`'s "first record
with `+0xE`==0" scan as a fallback in `ShowPartyMembers`) — exactly how
"the current member" gets chosen isn't fully mapped yet. Confirmed
fields so far — `+0x0`: name (13 chars max, see `EditCharacterName`,
`ida_scripts/name_char_rename.py`); `+0xE`: a time-of-day-like value
(reduced mod 10 in `RestCharacter`; `SelectDefaultPartyRecord` treats
`0` here as its scan target, though whether that means "empty slot" or
something else isn't confirmed); `+0x10`: gender/type (compared
against `2` in `ShowCharacterEquipment`); `+0x16`: plausibly a
level/skill stat — used in `FailsSavingThrow`'s save-chance formula
(`5*([+0x16] - threshold) + resistance bonus`), higher beats a higher
threshold; `+0x1C`: a status/condition flags word, tested throughout
(`RunTitleScreen`'s `E` handler, `ShowCharacterSkills`,
`RunConversation`, `UseAbilityOnTarget`'s `0xDFBB` table, and now a
"resting" bit in `RestCharacter`); **`+0x20`–`+0x30`: 9 contiguous
2-byte equipment/bonus resistance values** (`+0x20`, `+0x22`, `+0x24`,
`+0x26`, `+0x28`, `+0x2A`, `+0x2C`, `+0x2E`, `+0x30`), found via
`RollEffectResistance` — each one is conditionally summed into a trap/
status effect's resistance-check total, selected by one of 9 matching
high bits (`0x8000`..`0x80`) in the effect-definition record's cost
flags; individual fields not yet matched to specific resistance types
(fire/poison/etc. is a guess, not confirmed); `+0x52`/`+0x92`: HP
current/max, confirmed by `CastSpell`'s heal codes
(`0x12`/`0x13`: +25%/+50% of missing; `0x14`: full heal directly);
`+0x54`/`+0x94`: MP current/max (`0x1D`: +50% of missing; `0x17`: full
restore), regenerated by `RestCharacter` the same way as HP. `+0x58`,
`+0x64`, `+0x66`: three more fields, each averaged across the valid
party by `UpdatePartyAverageStatTiers` and each feeding a *different*
tiered gameplay system — `+0x64` → `word_36CA5`, compared against 5
ascending thresholds to set bits in `word_36C7F` that
`DrawMinimap`/`BuildMinimapTileData` read directly (bit `0x1000` blanks
the dungeon view entirely) — plausibly a light-source/torch-fuel level,
not confirmed; `+0x66` → `word_36CA7`, gating a 4-tier area size in
`RevealMapRegion` (**correction**: previously guessed "plausibly
weather" — traced further and it's a `Locate`/`Scout`/`Magic-Mapping`-
style special ability that reads `WORLD.DAT`/`CURGAME` directly and
reveals a `word_36CA7`-sized box of the map around the player, not a
visual weather effect); `+0x58` → `word_36CA9`, gating progressively-
revealed detail icons in `sub_234D3` (plausibly a bestiary/identify
mechanic, not traced). Only the light/torch-fuel identity (`+0x64`)
and the bestiary identity (`+0x58`) remain unconfirmed guesses; the
`+0x66` → area-reveal-size link is now solid. `+0xB4`: a bitmask of
which of (at least) 4 special abilities this character has learned;
`+0xB6`/`+0xB8`/`+0xBA`/`+0xBC`: per-ability charge or level values,
each checked against a threshold in a small table at `0x77C6` before
`RevealMapRegion` (and presumably its 3 sibling abilities, `ax`=2..5)
will fire.
`0x18` dispels/cures — clears bits 13-15 of `+0x1C`, a *different* 3-bit group
than the one `TickStatusEffects`/`ApplyStatusEffect` manage (bits
10/11/13 — bit 13 appears in both groups). Not yet mapped: the 6
attribute values (`ShowCharacterStats`), the 8 item slots
(`ShowCharacterInventory`), or the skill values (`ShowCharacterSkills`).

### Global material counters and BCD arithmetic

Three **global** (not per-party-member) crafting-material counters at
`0x94B3`, `0x94B7`, `0x94BB` — confirmed **exactly consecutive**,
4-byte packed-BCD stride, by `ShowResourceDepletedOverlay`'s scan of
all three in one loop. Individual identities: `0x94BB`/`0x94B7` are
used by `CastSpell`'s `0x1C` alchemy ability (converts 10 units of one
into the other — `NUORE`/`MAGIC ORE`); `0x94B3` is a third, sibling
counter used the same way by `ApplyEffectCost`'s cost dispatch but not
otherwise identified yet. All three are manipulated via the packed-BCD
bignum library (`ConvertWordToBCD4`, `CompareBCD4`/
`IsBCDCounterAtLeast`, `AddBCD4`/`AddToBCDCounter`, `SubBCD4`/
`SubtractFromBCDCounter`) — 4 bytes (8 decimal digits) per counter,
most-significant-digit-first, `DAA`/`DAS`-adjusted arithmetic. This
same library is called from many unrelated places throughout the
executable, so it's very likely also the engine behind gold/currency,
not just these three material counters — not confirmed.

### Item-slot encoding (from `Hex Hacking Item Guide.txt`, not yet cross-checked against the IDB)

Not independently verified against `SW.EXE`'s code yet, but internally
consistent and cross-confirmed against in-EXE strings (see below) enough
to treat as a strong starting hypothesis rather than a guess — a much
better starting point than `ultima1`'s `Savegame` struct work had, which
was derived from scratch.

Per-character section: appears roughly 15 text-rows below each
character's name in a hex view; a 4-character party gives 4 such
sections. Each character has **8 item slots** (matches the manual's
equip diagram: helmet, armor, gloves×2, rings×2, leggings, boots, plus
projectile/container/weapon/shield — the manual actually lists 10 named
body slots, so the "8 item slots" in the guide likely refers specifically
to the inventory-grid/backpack slots the guide's reset procedure empties,
not literally every equip slot; needs reconciling once the struct is
actually located).

Each item slot is **4 bytes**: `[item_id] [modifier] [uses] [unknown]`
- **Byte 0 (`item_id`)**: 0x00-0xFF, indexes a ~256-entry item table (see
  full table below).
- **Byte 1 (`modifier`)**: selects which "page" of the item table
  `item_id` is read against — `00` (weapons/armor/consumables page 1),
  `01` (armor/weapons page 2), `02` (armor/weapons/misc page 3) are all
  the guide documents; implies **at least ~768 distinct item
  definitions** across 3 pages, not just 256. Whether more pages exist
  past `02` isn't stated in the guide.
- **Byte 2 (`uses`)**: charge/use count for consumables (potions,
  scrolls, wands, rods) — `00` and `01` both mean 1 use, `02` = 2 uses,
  etc., up to `FF` (~255, described as "almost infinite"). **Does not
  apply to food** per the guide (tried and confirmed not to work).
- **Byte 3**: unknown/unconfirmed — guide author never observed a
  non-zero value and warns changing it may cause instability. Worth
  checking against the IDB once the struct is located (padding? a second
  modifier bit? unused?).

**Cross-confirmation against `SW.EXE`'s own strings** (from this
session's string survey in overview.md): the item table's door-key run
(`0x28`-`0x2E`: `Brass/Bronze/Copper/Iron/Steel/Silver/Gold door key`)
matches the exact 7-tier key hierarchy found in-binary
(`BRASS KEY`/`BRONZE KEY`/`COPPER KEY`/`IRON KEY`/`STEEL KEY`/
`SILVER KEY`/`GOLD KEY`), and the "Key of \<town\>" cluster
(`0x30`-`0x34`: Port Hope, Pariah, Numagik, Stony Peak, Tracking) lines
up with the town/password strings found in the binary (`PORT HOPE`,
`NUMAGIK`, `STONY PEAK`). This is good evidence the guide's table is
accurate for at least the shareware chapter's item set, not just
inferred/reconstructed after the fact by its author.

**Shareware relevance**: the guide notes the shareware version has a
blocked portal that can be bypassed by giving a character the "Key of
Pariah" (item `0x31`, modifier `00`) — directly explains the registration
nag string found this session (`"Thank You for playing... Please
register your copy today."`) and confirms `SW.EXE` (Share**w**are) is
content-limited by design, not just nagging.

**Full item ID table**: reproduced in full in
[`Hex Hacking Item Guide.txt`](Hex%20Hacking%20Item%20Guide.txt) — 3
pages of ~256 entries each (weapons, armor by material/enchant tier,
potions, scrolls/wands/rods/parchments of each skill, quest items, key
items, and a block of intentionally-broken "dummy"/crash items flagged
`*(3)` at the end of page 2). Not duplicated here; treat that file as the
source of truth and link back to it rather than copying the table, since
it's long and Paul may update it.

Next real step here: locate the actual per-character struct in the IDB
(candidate: the already-present but unidentified `Struc1`, per
roadmap.md) and verify slot byte offsets/count directly against this
guide rather than assuming it's exactly right.

## `WORLD.DAT`

1,761,397 bytes. Referenced by (unverified names, see overview.md)
`loadWorldDat1` through `loadWorldDat5` and `WorldDat_setBlock1` through
`WorldDat_setBlock6`. First 8 bytes read as repeating
`00 00 00 00 01 00 00 00` pairs in a quick raw peek — consistent with a
table of `(flag/type, count-or-offset)` pairs or similar, but this is a
guess from 64 bytes, not a traced format. Error strings suggest it holds
at least: text data, NPC data, conversation data (per `"Problem
retreiving text/NPC/conversation data."`), and presumably maps.

**One resource confirmed**: offset `0x8270A`, 768 bytes, is the game's
master 256-color VGA palette (see `PICTURES.VGA`'s "Palette" section
below) — loaded via `LoadMasterPalette` (`0x27CB0`), one of the ~27
`FileEntry`-block-setup stub functions at `0x27B42`-`0x2801A`
(`ida_scripts/document_resource_stubs.py`, `ida_scripts/extract_resource_stubs.py`
has every stub's own offset/size if more need identifying the same way).

### In-memory dungeon map grid (source file not yet identified — plausibly loaded from `WORLD.DAT`)

Found via `GetMapCellPtr` (`0x16F64`), the address computation the
fog-of-war reveal system (`RevealCellsAroundPlayer`
`ida_scripts/name_map_reveal.py`) uses: a 2D grid, **8 bytes per cell**,
rows **78 cells wide** (row stride `0x270` = `78*8`), segment
`word_2E562`, with the grid's own origin held in `word_2E564`
(row/y)/`word_2E55C` (column/x) — i.e. addressing is relative to
whatever sub-region of the full map is currently loaded, not the map's
absolute origin. Confirmed fields: **`+0`/`+2`: two tile-type indices**
(used by `BuildMinimapTileData` — `ida_scripts/name_minimap.py` — as
lookups into two small tables, 12 bytes/entry at `0xE551` and 10
bytes/entry at `0xE175`, giving the two picture ids `DrawMinimap`
draws per cell — plausibly floor/base tile and a wall or object
overlay); **`+6`, a flags word, bit `0x8000` = "already explored"** —
the automap's "cells become known as you walk near them" mechanic
(matches the manual's "M uses the party map"). The reveal action itself
(`PersistExploredCell`) writes the explored bit into `CURGAME` — the
automap survives save/load because it's part of the savegame, not just
in-memory state.

**The dungeon "view" is a small tile-grid minimap, not a full-screen
first-person render**: `DrawMinimap` (`0x21588`) draws a 7×9 grid of
8×8-pixel tiles at a fixed on-screen position (base tile + optional
overlay per cell, from `BuildMinimapTileData`'s buffer) — this pair is
what `start`'s main loop calls after every movement/state change to
refresh the view. No separate "3D corridor" renderer has turned up;
this minimap widget appears to be the game's primary way of showing the
dungeon layout.

**The `0xE551`/`0xE175` tile-type tables have a confirmed layout and a
second consumer.** Entry stride: 12 bytes (`0xE551`) / 10 bytes
(`0xE175`). Confirmed fields on `0xE551`: `+0xA` = `g_pictureDir` byte
offset (used by `DrawMinimap`/`DrawCellIconPair`/`DrawWallTypeLegendRow`
to pick the drawn picture); `+0` and `+2` hold two further per-type
values read only by the still-unnamed `sub_20D2F`/`sub_20CEC`/
`sub_20E12` cluster (see below) — not picture offsets, their exact
meaning is unconfirmed. `0xE175`'s confirmed field is `+8` (same role
as `0xE551`'s `+0xA`, for the overlay picture).

Checked whether `word_32926` (a parameter `BuildMinimapTileData` sets
per-cell before drawing) is a color/remap value, since `DrawMinimap`
itself keeps its picture index fixed at `g_pictureDir` entry 9
throughout its loop rather than varying it per cell — read the
candidate consumer `sub_2A53C` directly and ruled this out, it never
touches `word_32926` at all. **Still an open question** what
`word_32926` actually controls; not worth another guess without more
evidence.

A second, separate screen also reads these two tables:
`RunMapEditorScreen` (`0x20070`, reached from an ordinary keyboard
command slot in `start`'s main dispatch — not a debug/dev-only hook)
draws two scrollable 17-icon horizontal legend strips, one per table
(`DrawWallTypeLegendRow`/`DrawFloorTypeLegendRow`), then previews the
current map cell's own floor+overlay icon pair at full size
(`DrawCellIconPair`, the same floor/overlay composite `DrawMinimap`
uses, just unscaled). **Correction**: first pass concluded this was a
read-only legend screen ("nothing writes back to map data") and named
it `ShowTileLegend` — wrong. Its `A` key (`FillVisibleAreaWithSelectedTile`)
loops over the whole visible 40×24 cell grid and, per cell
(`PaintCellAndPersist`), writes the selected legend tile into a
`WORLD.DAT`-backed record and calls `FileEntry_Write` — a real,
persisted bulk edit. `B`/`F` jump the legend strips to a per-level tile
palette read from `WORLD.DAT` (`sub_205C0`/`sub_27FE0`, FileEntry
`bx=0x9043`; record layout not fully traced yet). This is a
debug/level-editor screen left reachable in the shipped binary, not a
passive legend. A sibling cluster
(`sub_20C8E`/`sub_20CEC`/`sub_20D2F`/`sub_20E12`/`sub_29FF6`, called
from the same `sub_20C1E` master-redraw dispatch the minimap uses)
draws two small "current cell class" preview boxes using `g_pictureDir`
entries 4 and 5 as fixed panel graphics, and scans candidate lists
(reading the `0xE551` table's `+0`/`+2` fields, compared via the new
`IsPairedValueMatch` fuzzy-equality helper) to highlight the matching
legend icon — structure understood, but the driving inputs
(`word_328E6`..`word_328F2`, seven consecutive words that are read in
several places but have no literal write site anywhere in the
disassembly — almost certainly filled by an indirect/computed pointer
write rather than a `mov word_328E6, ax`) remain unidentified, so no
name was forced onto that cluster.

**Open question — how the two tile-type lookup tables actually work**:
dumped both (`ida_scripts/dump_tile_tables.py`) and the picture-id-like
values they yield (`0x16`-`0x50` range) are far outside `g_pictureDir`'s
10 valid entries, while `DrawMinimap` keeps `word_2E532` (the actual
`g_pictureDir` byte offset `DrawPicture` reads) fixed at `0x90` — entry
9, the small 8×8 icon — for the whole 7×9 loop. So every cell likely
draws the *same* base glyph, and the varying table value instead feeds
`word_32926`, a parameter `DrawPicture` passes (as `[bp+var_21]`) to
`sub_2A53C` first thing. Checked whether that's a per-cell color/remap
parameter by reading `sub_2A53C` — **ruled out**: it never reads
`[bp+var_21]` at all (it's a local stack-buffer init/copy routine keyed
off different globals, `word_328C6`/`word_2E48E`/`word_2E490`). So
`word_32926`'s actual role in `DrawPicture` — and by extension what the
`0x16`-`0x50` table values mean — is still unknown; genuinely open,
not a working theory.

## `PICTURES.VGA`

**Decoded 2026-09-15.** 12,550,618 bytes, raw 8bpp indexed pixels (VGA
Mode 13h palette), no per-image header or compression — a directory
table elsewhere (`g_pictureDir`, in `SW.EXE`'s own data segment, not in
this file) says where each picture starts and how big it is; the file
itself is just a flat blob of pixel bytes back to back.

**Directory** (`g_pictureDir`, linear `0x3508E`, i.e. `DS:0x782E` with
`DS` fixed to paragraph `0x2D86` — see `fix_ds_segreg.py`): an array of
16-byte entries, indexed as `g_pictureDir + picture_id*0x10`:

```
+0x0  word   unused/reserved in the entries examined (always 0)
+0x2  word   unused/reserved in the entries examined (always 0)
+0x4  word   unconfirmed (varies per entry, not yet decoded)
+0x6  word   unused/reserved in the entries examined (always 0)
+0x8  word   width, pixels
+0xA  word   height, pixels
+0xC  word   file offset into PICTURES.VGA, low word
+0xE  word   file offset into PICTURES.VGA, high word
```

**The table has exactly 10 entries** (confirmed by
`ida_scripts/enumerate_pictures.py`: scanning for plausible
width/height/offset values, the pattern holds for 10 entries then
breaks down completely) — this is a small fixed set of splash/UI
graphics, not a general asset catalog. Dungeon views, portraits etc.
must be generated or stored some other way (not yet investigated).

All 10 extracted and rendered with `ida_scripts/extract_pic.py`
(grayscale — real VGA palette not recovered yet, but shape alone was
already unambiguous for most):

| # | size | offset | content |
|---|------|--------|---------|
| 0 | 318×198 | `0x0` | **"SmithWare" splash-screen logo** (matches the developer name from `CURGAME`'s header string) |
| 1 | 210×105 | `0xE694C` | `GameDialog_draw*` background panel — button-label text fragments (`SAVE`/`LOAD`/`MUSIC`/`SOUND FX`/`DOS`/`RETURN`) baked into the bitmap |
| 2 | 140×155 | `0x3064B6` | two-figure **combat/fighting scene** silhouette |
| 3 | 190×110 | `0x779552` | **wolf/monster** silhouette |
| 4 | 224×74 | `0xAB3F1A` | light gradient panel — indistinct in grayscale, possibly sky/background |
| 5 | 224×62 | `0xAFCC9A` | sky/cloud gradient |
| 6 | 56×136 | `0xB2579A` | male character silhouette — plausibly the character-creation body template (`docs/overview.md`'s string survey found `"MALE"`/`"FEMALE"`/`"PICK A PORTRAIT"` nearby in the string table right after this same directory) |
| 7 | 32×32 | `0xB8BBDA` | icon — indistinct in grayscale (mostly two flat index values, needs the real palette) |
| 8 | 16×16 | `0xBCF3DA` | **mouse-cursor arrow** |
| 9 | 8×8 | `0xBEF1DA` | small **scroll-arrow icon** (matches `UpdateScrollArrows`' two-glyph indicator from earlier this session — likely one of its actual glyphs) |

Loading path, fully traced in `ida_scripts/name_picture_system.py`:
`DrawPicture` (`0x29878`, called from `start` and 8+ other functions)
looks up `g_pictureDir[id]`, calls `LoadPictureIntoEms` (`0x2A68D`) to
ensure the picture's bytes are mapped into a small LRU cache of LIM EMS
4.0 pages (evicting the oldest entry on a cache miss and reading fresh
bytes from `PICTURES.VGA` — the fixed `FileEntry` at `bx=0x9011`, opened
once in `InitGame`), then blits `width`×`height` pixels from the EMS
page frame to the video buffer at `(x, y)`, with the blit mode selected
by `_font_bgTransparent` (0–5, different transparency/color-key
branches).

`sub_23874` (called repeatedly from `start`) indexes the same
`g_pictureDir` table the same way (`g_pictureDir + word_2E532`,
`word_2E532` = `picture_id*0x10`) — it's one shared directory, not a
separate table per caller. One observed call used entry 8 (the mouse
cursor), so this is more likely a cursor-draw/update path than an intro
animation as first guessed — not confirmed either way.

### Palette

Colors are 8bpp indices into a 256-entry VGA DAC palette, set via direct
port I/O rather than the BIOS (`ida_scripts/name_palette_io.py`):
`SetPaletteRange` (`0x25A3B`) writes a starting register to port `0x3C8`
then RGB triples to port `0x3C9` (optionally waiting for vertical
retrace first); `GetPalette` (`0x25A5B`) reads all 256 back via BIOS
`INT 10h/AX=1017h`. `FadePaletteStep` (`0x11953`, `ida_scripts/name_intro_picture.py`)
nudges each of a range of DAC registers one step toward a target buffer
and calls `SetPaletteRange` — called once per animation frame by
`ShowIntroPicture` (`0x1177C`, called directly from `start` and
`InitGame`: shows a picture via `DrawPicture`, fades its palette via
`FadePaletteStep`, waits for a keypress) to fade a picture's palette in
or out smoothly.

**Found: the master palette.** `ShowIntroPicture` calls
`LoadMasterPalette` (`0x27CB0` — one of the resource-block-setup stub
family from the `WorldDat` section above, the first one actually
confirmed) right before building the fade buffer. It configures a
`FileEntry` read of **`WORLD.DAT` offset `0x8270A`, 768 bytes** — 256
RGB triples, already valid 6-bit VGA DAC values (0-63 per channel, no
transform needed) — verified by reading those bytes directly and
re-rendering the whole `PICTURES.VGA` catalog in true color
(`extract_pic.py --palette game/WORLD.DAT 0x8270A`). The dialog panel's
`SAVE`/`LOAD`/`NEW GAME`/`DOS`/`MUSIC`/`SOUND FX`/`ANIMATION`/`RETURN`
labels are now fully legible in color, and the wolf and character
silhouettes render with entirely plausible natural colors — strong
confirmation this is the right palette (the splash-screen logo, entry
0, renders with some rainbow banding at higher indices — either a
second, not-yet-found palette region for that one image, or an
intentional effect; not fully explained).

`ShowIntroPicture`'s own copy loop (`al=[si]; al-=0x3F; [di]=al`,
copying the just-read `WORLD.DAT` bytes into the `0x475A`-based fade
buffer) subtracts `0x3F` per byte — since the source bytes are already
in 0-63 range, this looks like it's building a *signed delta* or
some other derived form for `FadePaletteStep`'s interpolation, not a
second encoding layer on the base palette; not fully traced.

Not yet decoded: the real VGA palette (so images render in true color,
not grayscale), and the directory's `+0x4` field's meaning (varies per
entry, didn't fit an obvious role from the entries examined so far).

## Not yet examined

- `SBFMDRV.COM` — third-party(?) Sound Blaster FM driver, likely not
  worth reverse-engineering in detail (not game logic).
