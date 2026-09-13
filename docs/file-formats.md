# Ultima III (DOS) — File Formats

Sources: [Ultima Codex wiki — "Ultima III internal formats"](https://wiki.ultimacodex.com/wiki/Ultima_III_internal_formats)
and the [Exodus Project's PARTY.ULT page](https://sourceforge.net/p/ultima-exodus/wiki/U3%20PARTY.ULT%20File%20Format/)
(both fetched 2026-09-13), cross-checked against the real files in
`C:\games\ultima3` (sizes below) and, eventually, `ultima.asm`. Same
convention as `ultima2/docs/file-formats.md`: where an external source
and our own disassembly agree, that's a strong signal; where they
don't (yet) connect, that's a TODO. **Nothing below has been verified
against the disassembly yet** — treat every byte offset as an
unconfirmed hypothesis borrowed from external research until a specific
read/write site in `ultima.asm` is cited against it.

Every size in the tables below was cross-checked against the actual
file size in `C:\games\ultima3` and matches exactly unless noted
otherwise under "Discrepancies".

## `SHAPES.ULT` — tile graphics

80 tiles, 16×16 px, CGA 2bpp (2 bits/pixel: `00`=Black, `01`=Cyan,
`10`=Magenta, `11`=White). 64 (`0x40`) bytes/tile: first 32 bytes are
even scanlines (plane 0), next 32 are odd scanlines (plane 1). Total
`80 × 64 = 5120` bytes — **matches the real file exactly** (5,120
bytes).

## `CHARSET.ULT` — font

128 characters, 8×8 px. 16 (`0x10`) bytes/char: first 8 = plane 0, next
8 = plane 1. Total `128 × 16 = 2048` bytes — matches the real file
(2,048 bytes).

## `SOSARIA.ULT` — overworld map + live game state

| Offset | Length | Purpose |
|---|---|---|
| `0x0000` | `0x1000` | 64×64 world map, 1 byte/cell |
| `0x1180` | `0x20` | Monster tile numbers (stored value ÷4 = real tile ID — same "×4" encoding quirk as Ultima II's map tiles) |
| `0x11A0` | `0x20` | Floor tile underneath each of 32 monster slots |
| `0x11C0` | `0x20` | Monster X coordinates |
| `0x11E0` | `0x20` | Monster Y coordinates |
| `0x1200` | `0x20` | Monster movement flags |
| `0x1220` | `0x1` | Whirlpool X |
| `0x1221` | `0x1` | Whirlpool Y |
| `0x1222` | `0x1` | Whirlpool X delta |
| `0x1223` | `0x1` | Whirlpool Y delta |
| `0x1224` | `0x1` | Left moon phase (0-7) |
| `0x1225` | `0x1` | Right moon phase (0-7) |
| `0x1226` | `0x1` | Left moon sub-phase (0-0xB) |
| `0x1227` | `0x1` | Right moon sub-phase (0-3) |

Total `0x1228` = 4,648 bytes — matches the real file exactly. Note this
is a **live game state file**, not a static asset: monster
positions/moon phases are runtime state baked into the same file as
the static map, same "self-modifying map file" pattern Ultima II uses
for its `mapx??`/`monx??` split, except Ultima III apparently keeps
both in one file per location instead of two.

## Town/castle maps (12 files, same layout as `SOSARIA.ULT`'s map+NPC portion)

| Offset | Length | Purpose |
|---|---|---|
| `0x0000` | `0x1000` | 64×64 map, 1 byte/cell |
| `0x1000` | `n × 0x2` | Sign/dialog text offset table (each entry + `0x1000` = absolute offset of that string) |
| (variable) | variable | Null-terminated ASCII sign/dialog text, packed after the offset table |
| `0x1180` | `0x20` | NPC tile numbers (÷4 for real tile ID) |
| `0x11A0` | `0x20` | Floor tile underneath each of 32 NPC slots |
| `0x11C0` | `0x20` | NPC starting X |
| `0x11E0` | `0x20` | NPC starting Y |
| `0x1200` | `0x20` | NPC movement flag (high nibble: `0`=walks, `4`=stationary, `8`=merchant, `0xC`=attacks) + dialog sentence index (low nibble) |
| `0x1220` | `0x8` | Unused |

Total `0x1228` = 4,648 bytes, matching `SOSARIA.ULT`'s size exactly (as
expected — same base layout, town-specific fields swapped in for the
overworld-specific whirlpool/moon fields).

**Discrepancy, partially clarified 2026-09-13, not fully resolved**: the
external source lists this group as `BRITISH`, `DAWN`, `DEATH`, `DEVIL`,
`EXODUS`, `FAWN`, `GREY`, `LCB`, `MONTOR_E`, `MONTOR_W`, `MOON`, `YEW`
(12 names). Our actual `C:\games\ultima3` install has `BRITISH`, `DAWN`,
`DEATH`, `DEVIL`, `GREY`, `LCB`, `MONTOR_E`, `MONTOR_W`, `MOON`, `YEW`
**plus `AMBROSIA.ULT`**, and no separate `EXODUS.ULT` or `FAWN.ULT` file
on disk. `ultima_exodus.idb`'s string table (see
[overview.md](overview.md)) references **both** `aAmbrosiaUlt` and
`aFawnUlt`/`aExodusUlt` as literal filenames — so the executable's code
expects all of them to potentially exist, but this particular release's
data set only ships `AMBROSIA.ULT`. Best working hypothesis: `FAWN`
and `EXODUS` (the Castle of Exodus, the endgame location) are real
locations reachable in other releases/versions, and `AMBROSIA` is
either a renamed version of one of them or a location added in this
release — not yet confirmed which. Resolving this needs tracing
whichever function actually calls these filenames (not yet identified
— they weren't in the shared-runtime pass, so they're somewhere in the
95 still-unnamed game-specific functions).

## Dungeons (`DARDIN`, `FIRE`, `M`, `MINE`, `P`, `PERINIAN`, `TIME` — 7 files)

| Offset | Length | Purpose |
|---|---|---|
| `0x000` | `0x100` | Level 1, 16×16 cells, 1 byte/cell |
| `0x100` | `0x100` | Level 2 |
| ... | `0x100` each | Levels 3-8 (8 levels total, fixed allocation) |
| `0x800` | `0x10` | Sign text offset table (+`0x800` = absolute offset), max 1 sign per level (8 entries) |
| `0x810` | `0x80` | Null-terminated ASCII sign texts |

Total `0x890` = 2,192 bytes — matches the real file exactly. Same
8-levels-fixed pattern as Ultima I/II's dungeon format (that project
used 16 levels; this one appears to use 8 — worth confirming against
the disassembly's actual depth-limit check once found, don't take the
external source's level count as gospel without a corresponding
in-game check).

## Combat/conflict arenas (`CNFLCT_A/B/C/F/G/M/Q/R/S.ULT` — 9 files)

| Offset | Length | Purpose |
|---|---|---|
| `0x00` | `0x79` | 11×11 arena map, 1 byte/cell |
| `0x79` | `0x7` | Unknown |
| `0x80` | `0x8` | Monster starting X (up to 8 monsters) |
| `0x88` | `0x8` | Monster starting Y |
| `0x90` | `0x8` | (runtime scratch: tile underneath each monster) |
| `0x98` | `0x8` | (runtime scratch: monster HP) |
| `0xA0` | `0x4` | PC starting X (up to 4 party members) |
| `0xA4` | `0x4` | PC starting Y |
| `0xA8` | `0x4` | (runtime scratch: tile underneath each PC) |
| `0xAC` | `0x4` | (runtime scratch: each PC's base/home tile) |

Total `0xB0` = 176 bytes — matches the real file exactly. The 9
letter-coded files (`A/B/C/F/G/M/Q/R/S`) presumably each represent a
different arena backdrop or monster-type grouping, similar in spirit
to Ultima I's dungeon/combat room-layout reuse — which letter maps to
which encounter type is not yet known, worth tracing from the
encounter-trigger code once found.

## `PARTY.ULT` — active party save state (274 bytes)

| Offset | Length | Purpose |
|---|---|---|
| `0x00` | `0x1` | Transport mode (`0xA`=horse, `0xB`=ship, `0x3F`=foot) |
| `0x01` | `0x1` | Unknown |
| `0x02` | `0x1` | Location (`0`=Sosaria, `1`=dungeon, `2`=town, `3`=castle, `4`=shrine, `0x80`=combat, `0xF0`=merchant, `0xFF`=Ambrosia) |
| `0x03` | `0x4` | Move count (BCD) |
| `0x07` | `0x1` | Party size |
| `0x08` | `0x1` | X coordinate |
| `0x09` | `0x1` | Y coordinate |
| `0x0A` | `0x4` | Roster indices occupying party slots 1-4 |
| `0x0E` | `0x4` | Unknown |
| `0x12` | `0x40` | Character record, slot 1 (see below) |
| `0x52` | `0x40` | Character record, slot 2 |
| `0x92` | `0x40` | Character record, slot 3 |
| `0xD2` | `0x40` | Character record, slot 4 |

Total `0x12 + 4×0x40` = 274 bytes — matches the real file exactly.

## `ROSTER.ULT` — character roster (1,280 bytes)

**Confirmed against real code, 2026-09-13** (previously this section was
sourced externally only): `ultima_bootup.idb`'s `showCharacterDetails`,
`handleCreateCharacter`, and `showRegister` read/write every field below
at the exact offsets listed, via the `RosterEntry` struct
(`ida_scripts/create_roster_struct.py`). See
[overview.md](overview.md#session-2026-09-13-ultima_bootupidb-created-and-fully-swept-7373-named)
for the full evidence trail.

20 character records, 64 (`0x40`) bytes each, no separate header.
`20 × 64 = 1280` bytes — matches the real file exactly.

### Character record (64 bytes, shared by `ROSTER.ULT` and `PARTY.ULT`'s 4 embedded copies)

| Offset | Length | Field |
|---|---|---|
| `0x00` | `0xA` | Name, ASCII, null-terminated/padded |
| `0x0A` | `0x4` | Unknown |
| `0x0E` | `0x1` | Marks/cards bitmask (quest-item flags) |
| `0x0F` | `0x1` | Torch count (BCD) |
| `0x10` | `0x1` | Party membership (`0x00`=no, `0xFF`=yes) |
| `0x11` | `0x1` | Status (Good/Poisoned/Dead/Ashes — exact letter codes TBD from disassembly) |
| `0x12` | `0x1` | Strength (BCD) |
| `0x13` | `0x1` | Dexterity (BCD) |
| `0x14` | `0x1` | Intelligence (BCD) |
| `0x15` | `0x1` | Wisdom (BCD) |
| `0x16` | `0x1` | Race (Elf/Dwarf/Fuzzy/Hobbit/Bobbit?) |
| `0x17` | `0x1` | Class (Wizard/Ranger/Thief/Illusionist/Alchemist/Druid/Fighter/Lark/Cleric/Paladin?) |
| `0x18` | `0x1` | Gender (M/F/O) |
| `0x19` | `0x1` | Current magic points (BCD) |
| `0x1A` | `0x2` | Current HP (BCD, word) |
| `0x1C` | `0x2` | Maximum HP (BCD, word) |
| `0x1E` | `0x2` | Experience points (BCD, word) |
| `0x20` | `0x1` | Food fraction/sub-morsels (BCD) |
| `0x21` | `0x2` | Food (BCD, word) |
| `0x23` | `0x2` | Gold (word) |
| `0x25` | `0x1` | Gems |
| `0x26` | `0x1` | Keys |
| `0x27` | `0x1` | Powder (Sleep/other?) |
| `0x28` | `0x1` | Currently-readied armor index (0-7) |
| `0x29` | `0x7` | Armor quantities, 7 types (Cloth, Leather, Chain, Plate, +2 Chain, +2 Plate, Exotic) |
| `0x30` | `0x1` | Currently-readied weapon index (0-0xF) |
| `0x31` | `0xF` | Weapon quantities, 15 types (Dagger, Mace, Sling, Axe, Bow, Sword, 2H Sword, +2 Axe, +2 Bow, +2 Sword, Gloves, +4 Axe, +4 Bow, +4 Sword, Exotic) |

`0x00`-`0x3F` = 64 bytes exactly. The letter-code fields (status, race,
class, gender) need their exact encoding confirmed from the
disassembly (or from `NAME.DAT`/prompt-string cross-referencing) — the
labels above are from the external source's field *names*, not
confirmed byte values.

## `EXODUS.BIN` — overlay/data blob (not yet confirmed as executable vs. pure data)

External source lists several fixed data tables inside this file:

| Offset | Length | Purpose |
|---|---|---|
| `0x0` | 5600 | Unknown |
| `0x15E1` | `0x4` | Castle coordinates (LCB, Castle of Death — 2 castles × 2 bytes) |
| `0x15E5` | `0x14` | Town coordinates (10 towns × 2 bytes) |
| `0x15F9` | `0xE` | Dungeon coordinates (7 dungeons × 2 bytes) |
| `0x184D` | `0x8` | Moongate X coordinates (up to 8 gates) |
| `0x1855` | `0x8` | Moongate Y coordinates |
| `0x6566` | 335 | "Look" command description strings, null-terminated |
| `0x7445` | `0x2` | Exotic Weapon's map location |
| `0x7450` | `0x2` | Exotic Armour's map location |

Real file size is 44,234 bytes — well past the last documented offset
above (`0x7452` = 29,778), so most of the file is still undocumented
externally.

**Resolved, 2026-09-13**: this IS a chained executable, not a pure data
blob — `ultima_bootup.idb`'s `handleJourneyOnward` loads and chains
into it via the same FCB-read trick `ULTIMA.COM` uses for `BOOTUP.BIN`,
with one difference: execution reaches it via an **indirect** jump
through a 2-byte vector stored at file offset `0x1228` (absolute
`0x1328` once loaded), rather than falling straight through to offset
0. Since neither `ULTIMA.COM` nor `BOOTUP.BIN` contains any overworld/
combat/dungeon code, this is almost certainly where that lives — see
[overview.md](overview.md#session-2026-09-13-ultima_bootupidb-created-and-fully-swept-7373-named)
and [roadmap.md](roadmap.md) for the next-session plan to disassemble
it (`ultima_exodus.idb`).

## `DEMO.ULT` — attract-mode menu map

19×6 tiles, 1 byte/cell, per the external source. Real file is 114
bytes; `19 × 6 = 114` — matches exactly.

## `MOVES.ULT` — menu animation script (1,024 bytes)

| Offset | Length | Purpose |
|---|---|---|
| `0x000` | `0x200` | Command table |
| `0x200` | `0x200` | Data table |

Total `0x400` = 1,024 bytes — matches the real file exactly. Internal
structure of the command/data tables not documented externally — open
question for the disassembly.

## `.IMG` files (`BRAND`, `FOUNTAIN`, `SHRINE`, `TIME` — 121 bytes each)

11×11 tile images, 1 byte/cell (`11 × 11 = 121`) — matches exactly.
Presumably the special-location overlay art shown when standing on a
Shrine/Fountain/Time Lord/Brand tile (LairWare's `UltimaDngn.c` has
`DungeonStart` cases named `time lord`/`brand` at Apple II addresses
`$9076`/`$931C` — see [overview.md](overview.md) — a plausible
cross-reference for what triggers each, once traced on the DOS side).

## `NAME.DAT` (640 bytes) — a pixel-path animation script, NOT a name table

**Correction, 2026-09-13**: this doc previously guessed (from external
speculation only, with no disassembly evidence) that `NAME.DAT` is a
random name-generator table for character creation. Direct
disassembly evidence now contradicts that: `ULTIMA.COM` loads only the
first 533 (`0x215`) of its 640 bytes into `byte_14117`, and
`drawAnimatedPixelPath` (`ida_scripts/apply_renames.py`, formerly
`sub_15E68`) consumes that buffer as a stream of `(length, row)` pairs
— `length` and a row byte per entry, `0` terminates — plotting 2
adjacent pixels per entry via `plotPixel2bpp` with a wait between each,
called during the boot sequence. That's a coordinate-pair pixel-path
animation script (the classic hand-drawn logo/signature reveal
effect), not name-generation data. **Open**: why only 533 of 640 bytes
are read, and what the remaining 107 bytes hold — not yet traced.

## Files with no external documentation found (2026-09-13 search)

- **`DUNGEON.DAT`** (1,866 bytes) — **confirmed 2026-09-13**: loaded by
  `ultima_exodus.idb`'s `cmdEnter` as a *second*, separate file read
  (`0x800` = 2,048 bytes requested) whenever entering any dungeon,
  alongside — not instead of — the dungeon's own numbered map file
  (`0x890` = 2,192 bytes, matching the documented per-dungeon format
  above exactly). So it's a shared auxiliary data file common to all
  dungeons, not itself a map. Internal layout still undocumented.
- **`ANIMATE.DAT`** (5,888 bytes) — loaded into `byte_1432D` and
  consumed by `drawAnimationFrameRow`/`runBootFlagAnimation` (see
  `apply_renames.py`) as 16-row image-frame data for the boot logo/flag
  animation. Internal frame layout not yet decoded.
- **`BOOTUP.BIN`** (19,572 bytes) — **confirmed 2026-09-13**: the
  character-creation and party-management program, chained into from
  `ULTIMA.COM`'s title screen. Fully disassembled and named
  (`ultima_bootup.idb`, 73/73 functions) — see
  [overview.md](overview.md).
- **`EXODUS.BIN`** (44,234 bytes) — see its own section above:
  confirmed chained-to from `BOOTUP.BIN`, and by elimination (neither
  `ULTIMA.COM` nor `BOOTUP.BIN` has any overworld/combat/dungeon code)
  almost certainly the actual game-world engine. Not yet disassembled —
  see [roadmap.md](roadmap.md).

`DUNGEON.DAT`/`ANIMATE.DAT`'s full internal layouts still have to be
reverse-engineered from the disassembly and/or raw byte inspection —
no wiki/community documentation was found for them. Flagged in
[roadmap.md](roadmap.md) as open items.

## Open questions

- Every table above is sourced externally and cross-checked only by
  file *size* so far — no field has yet been confirmed against an
  actual read/write site in `ultima.asm`. Treat all field names/
  meanings as hypotheses to verify, not established fact, until cited
  against disassembly evidence (same bar the sibling projects hold
  their own file-format docs to).
- The `SOSARIA.ULT`/town-map discrepancy in file naming (`AMBROSIA` vs.
  `FAWN`/`EXODUS`) noted above.
- `EXODUS.BIN`'s true nature (overlay executable vs. data blob) and
  `BOOTUP.BIN`'s role, both undocumented externally.
- Exact letter/index encodings for character status, race, class, and
  gender fields in the character record.
