# File formats

On-disk data formats used by "Yendorian Tales Book I: Chapter 2"
(`SW.EXE`), cross-referenced against the disassembly as they're decoded.

**Status (last updated 2026-09-19)**: the three major formats below
(`CURGAME`/`SAVGAME*`, `WORLD.DAT`, `PICTURES.VGA`) are all decoded to
the level of "confirmed against the actual code, traced record layouts
and field meanings" — not placeholders. Cross-referenced throughout
against two reference documents Paul added early on: `docs/manual.txt`
(the official game manual) and `docs/Hex Hacking Item Guide.txt` (a
community-written 2004 guide by Josh Hines on hex-editing savegame
items — freely redistributable per its own footer). Only
`SBFMDRV.COM` (the third-party Sound Blaster driver, not game logic)
remains genuinely unexamined — see "Not yet examined" below. See
[overview.md](overview.md) for the full session narrative and
[roadmap.md](roadmap.md) for current priorities.

## `CURGAME` / `SAVGAME1` (and presumably `SAVGAMEn`)

77,509 bytes each, identical size. `SAVGAMEX` appears in `SW.EXE`'s
string table as a templated filename (`X` = slot digit), and the
in-EXE error string `"Problem with CURGAME."` / `"Problem with a SAVED
GAME file."` confirms `CURGAME` is the active/working copy distinct from
the numbered save slots.

### Byte layout (confirmed 2026-09-19)

The file is **7 contiguous sections**; each section's start is the
previous start plus the previous size, and Chapter 2's sizes sum to
exactly 77,509. Section starts come from the seven "record-setup" stubs
(`PrepareMasterHeaderBlockRead` `0x27DA8`, `PrepareGameDialogSizedBlockRead`
`0x27E20`, `PrepareGroundItemSlotBlockRead` `0x27E3A`,
`PrepareRecordAtIndexDC6` `0x27DE5`, `PrepareRecordAtIndexDCA` `0x27DC6`,
`PrepareGameDialogIndexedBlockRead` `0x27E04`,
`PrepareGameDialogLargeBlockRead` `0x27D8A`), each of which loads a
32-bit base offset from a static table at `DS:0xCDD3`-`0xCDEB` and sets
the `FileEntry`'s size field; sizes come from `InitGlobals` constants.
`FileEntry` (`FileEntry_Seek`, `0x184E7`): `+0` DOS handle, `+2`/`+4`
buffer far pointer, `+6` byte count, `+8` block index, `+0xA`/`+0xC`
32-bit base offset, `+0xE` ASCIZ filename; seek position =
`count * index + base`.

| # | Ch2 offset | Ch2 size | Ch3 offset | Ch3 size | Contents |
|---|---|---|---|---|---|
| 1 | `0x0000` | 5000 | `0x0000` | 5000 | 500-byte game-state block + 9 x 500-byte party records; in-memory `DS:0x93FF` (Ch3 `0xCEDD`) |
| 2 | `0x1388` | 144 x 100 | `0x1388` | 168 x 100 | fog-of-war bitmap (`PersistExploredCell`, `RevealMapRegion`, `ShowLocalAreaMap`, `RefreshDungeonMapWindow`) |
| 3 | `0x4BC8` | 1296 x 34 | `0x5528` | 1296 x 34 | item instances: ground slots and container contents (`LoadGroundItemSlotRecord`, `LoadContainerContents`, `SaveAndCloseContainer`, ...) |
| 4 | `0xF7E8` | 644 | `0x10148` | 1059 | shared lock+curgame-record "already unlocked/triggered" bitmap, bit-packed (`LoadCurgameRecord`, `LoadLockState`, `UnlockDoorCommand`, `HandleSearchCommand`, `UseAbilityCommand`, `ApplyEncodedItemEffect`, `start`'s autosave; see `src23/interact.h`) |
| 5 | `0xFA6C` | 608 | `0x1056B` | 1008 | byte-addressed lock/shop state (`LoadLockState`, `RunShopScreen`, `TriggerShopExitSoundAndPersist`) |
| 6 | `0xFCCC` | 313 | `0x1095B` | 626 | monster-spawned flag bitmap (`Set`/`Clear`/`TestCellMonsterSpawnedFlag`) |
| 7 | `0xFE05` | 80 x 156 | `0x10BCD` | 80 x 156 | `g_levelMonsters`; in-memory `DS:0x0F26` (Ch3 `0x122C`) |
| | total | **77,509** | total | **81,037** | |

Sections 1 and 7 are in-memory snapshots: `SaveCurrentGameToSlot` writes
them from memory to both `CURGAME` and the slot, then copies sections 2-6
file-to-file (the game edits those in `CURGAME` on demand). A save slot is
therefore a byte-for-byte copy of `CURGAME`, and loading (the
`RunGameDialog` path) is the reverse. Section 3's 44,064 bytes are copied
in 16 chunks of `0xAC2`; section 5 in chunks of `0xBB8` (3000). Chapter 3
grows only sections 2, 4, 5 and 6 (the larger world); its offset chain was
verified end to end, but no Chapter 3 save file exists locally, so it has
not been checked against real data.

**Save slots**: six, `SAVGAME1`-`SAVGAME6` (the `SAVGAMEX` template's `X`
is patched with the slot digit from a 27-byte-per-entry UI table at
`DS:0x6CBE`: `+0` slot char, `+1` bit `0x80` = slot in use, `+2` the
displayed name, initially `- EMPTY -`). The in-use flag is runtime UI
state set at startup, not stored in the file.

**Section 1 game-state block** (offsets from the block start; verified
against real Chapter 2 files unless noted): `+0x00` the save's name, a
NUL-terminated string of at most 24 characters copied with a plain
string copy, so a shorter name leaves the tail of the previous one
behind — this is why `CURGAME` starts `SMITHWARE PARTY\0WARE\0` and the
real `SAVGAME1` starts `DAN\0HWARE PARTY\0` (**correction**: the string
was earlier read as a "header/magic string"; it is just the default save
name plus stale bytes, and there is no magic number). `+0x96` facing
(`0x8000` north, `0x4000` south, `0x1000` east, `0x2000` west, from
`ShowCompassDirection`), `+0x98`/`+0x9A` world X/Y, `+0x9C`/`+0x9E`/
`+0xA0`/`+0xA2` day/month/year/clock minutes, `+0xA4`... five party-role
assignments, `+0xB4` gold (packed BCD4), `+0xB8`/`+0xBC` the two ore
counters (packed BCD4), `+0x10E`/`+0x112`/`+0x116` flag words set by the
ore purchase, `+0x1EC` the 4-entry party-slot table (1-based party
record id, 0 = empty). Gold, ore and the three flag words were
independently confirmed at the same offsets in Chapter 3. The 9 party
records follow at `+0x1F4`, stride 500. Reimplemented in `src23/savegame.c`.

### Party record layout (consolidated 2026-09-19)

500 bytes, little-endian words. This table supersedes the piecemeal
prose below wherever they disagree; every row was checked against the
four real Chapter 2 characters (`SQUIRE`, `DIANA`, `YENDOR`, `JOSEPHINE`)
unless marked otherwise. Reimplemented in `src23/party.c`.

| Offset | Field |
|---|---|
| `+0x00` | name, NUL-terminated, at most 13 characters |
| `+0x0E` | class id: `tier*10 + base`, base 1-9, tier 0-2; valid ids 1-9, 11-19, 21-29 (`GetClassNameString` gives garbage for 10 and 20). Names: FIGHTER MERCHANT ROGUE MONK ALCHEMIST PALADIN MAGE DRUID MARKSMAN / WARRIOR TINKERER THIEF CLERIC TRANSMUTER CAVALIER WIZARD ENCHANTER RANGER / CHAMPION BLACKSMITH ASSASSIN PRIEST HEALER HERO SORCERER SAGE KNIGHT |
| `+0x10` | gender: 1 or 2 (the two real female characters are 2) |
| `+0x12` | unidentified (20-33 in real data) |
| `+0x14` | portrait icon id |
| `+0x16` | level |
| `+0x18` | experience, packed BCD4 |
| `+0x1C` | status flags. `0x40` dead, `0x80` cursed, `0x100` hexed, `0x200` jinxed, `0x400` stoned, `0x800` frozen, `0x1000` paralyzed, `0x2000` diseased, `0x4000` poisoned, `0x8000` sick. Low 6 bits = secondary-class reached, `0x20 >> (class-4)` for class ids 4-9 (real: MONK `0x20`, MAGE `0x04`, DRUID `0x02`) |
| `+0x20`..`+0x30` | 9 protection values: disease, poison, sickness, stoning, frozen, paralyze, cursing, hexing, jinxing |
| `+0x3C`..`+0x70` | 27 **current** stat values, indexed by the game's 27-entry name table |
| `+0x7C`..`+0xB0` | the same 27 stats' **maximum** values (current + `0x40`) |
| `+0xB4` | learned-abilities bitmask (`0x8000`..`0x1000`); `+0xB6`..`+0xBC` one charge counter per ability bit |
| `+0xBE`/`+0xC0`/`+0xC2` | wear counters for equipment slots `0xA`/`0xC`/`0xD` |
| `+0xCA`..`+0xE9` | 256-bit flag bank (known spells/abilities) |
| `+0x10C`..`+0x117` | 96-bit flag bank (per-character one-time events) |
| `+0x118`..`+0x139` | main inventory group (below) |
| `+0x13A`..`+0x15B` | equipment slots (below) |
| `+0x15C` | UI flags word set by the slot lookup: `0x80` main group shown, `0x400`/`0x200`/`0x100` bag 1/2/3, `0x40` last lookup was a 2-byte slot |
| `+0x17C`, `+0x1A2`, `+0x1C8` | three open-bag records of `0x26` bytes (below) |

**Stat name table** (`DS:0x7DC7`, 13-byte stride; entry *i* is the stat at `+0x3C+2i` current, `+0x7C+2i` max — from `UseAttributeBoostItem`'s `(offset-0x3C)/2` index and `ShowArmorAttributeBonusList`'s `0x7C` scan):
0 STRENGTH, 1 DEXTERITY, 2 STAMINA, 3 INTELLIGENCE, 4 WISDOM, 5 CHARISMA,
6-10 unnamed (five equipment-derived ratings, `+0x48`..`+0x50`), 11 HIT POINTS,
12 MAGIC POINTS, 13 unnamed (carry capacity, `+0x56`, exactly 10 x Strength in
real data), 14 SURVIVAL, 15 PROJECTILE, 16 SLASHING, 17 BASHING, 18 POLEARM,
19 CASTING, 20 MAPPING, 21 NAVIGATION, 22 BARTERING, 23 REPAIR, 24 THIEVERY,
25 LINGUISTICS, 26 CHEMISTRY. **This resolves several earlier guesses**: the
`+0x58`..`+0x70` block is the 13 *skills*; `+0x68` is BARTERING (what
`ComputeBarterPricingPreview` reads), `+0x64` is MAPPING (not "light-source
fuel"; it drives the minimap tiers), `+0x66` NAVIGATION (map-reveal size),
`+0x58` SURVIVAL (monster-detail reveal), `+0x70` CHEMISTRY (alchemy yield).
The six attributes' name-to-offset order is therefore confirmed, not a guess.

**Inventory group** (34 bytes): `u16` total carried weight, then 8 slots of
4 bytes (`u16` item id, `u16` extra; id 0 = empty). Slot *n* is at
`group + 2 + (n-1)*4`. **The same layout is a save file's item-instance record**
(section 3, 1296 of them). Carry capacity `+0x56` is checked against the main
group's weight. **Open bags**: at each marker `M`, `[M+0]` is the container's
item id (0 = closed), `[M+2]` is the item-instance record number where its
contents are saved (`SaveAndCloseContainer` writes the 34 bytes at `M+4` to
that record; the older text calling this a "count" was wrong), `[M+4]` the
contents. `GetInventorySlotPtr` acts on the first open bag (bag 1, 2, 3
priority) else the main group.

**Equipment slots** by command code (`GetInventorySlotPtr`): codes `0xA`-`0xF`
are 4-byte slots at `+0x13A`, `+0x13E`, `+0x142`, `+0x146`, `+0x14A`, `+0x14E`;
codes `0x10`-`0x14` are 2-byte id-only slots at `+0x152`, `+0x154`, `+0x156`,
`+0x158`, `+0x15A`. (Code 9 is not a slot; the docs' "slot 1-9" was wrong.)

**Correction**: the `+0xCA` "16-word skill array" below is really the 256-bit
flag bank above (`TestRecordFlag_CA`/`SetRecordFlag_CA`; index *n* is 1-based
and MSB-first, word `(n-1)/16`, mask `0x8000 >> ((n-1)%16)`), and it *does*
live on the party record (each real caster has exactly two bits set, the two
grants `ApplySecondaryClassTierFlags` makes per class).

First 32 bytes of `CURGAME` (hex-decoded):
```
53 4D 49 54 48 57 41 52 45 20 50 41 52 54 59 00   SMITHWARE PARTY\0
57 41 52 45 00 20 20 20 00 00 00 00 00 00 00 00   WARE\0   \0...
```

**The actual save operation is now traced**: `SaveCurrentGameToSlot`
(was `sub_1F5FF`, called once from `RunGameDialog`'s SAVE option)
creates or opens `0x902C` (`SAVGAMEX`), writes matching header
records to it and to `0x8FFB` (the live `CURGAME` file), then copies
the game data record-by-record from live into the save slot across
several typed-record loops.

**A fourth fixed `FileEntry` reads it directly**: `bx=0x8FFB` (distinct
from `0x9043`=`WORLD.DAT`, `0x902C`=`SAVGAMEX`, `0x9011`=`PICTURES.VGA`
— see `PICTURES.VGA`'s section below for how the filename-at-`+0xE`
convention was found). `LoadCurgameRecord` (`0x1770C`,
`ida_scripts/name_curgame_reader.py`) reads a record from it via EMS
paging, indexed by `word_32DBC*4 + 0x1A*_val9` — the `0x1A`-word (26)
row stride is a plausible per-character record size, worth checking
against the item-slot layout above once the struct is actually located.
Splits one field (`word_32DD0`) by 100 into a quotient/remainder pair
(currency or time value, not confirmed which; the quotient/remainder
globals themselves are reused as generic scratch parameters elsewhere,
so left unnamed). `LoadLockState` also freshly loads the queried lock's
bit-packed "previously unlocked?" mask into `g_lockUnlockedMask`, and
consistently OR's/tests it against `g_lockUnlockedAccumulator` (written
back to `CURGAME`) — a running per-lock-id-group record of which locks
have been unlocked.

**Autosave on movement**: `start`'s main loop writes a small, *fixed-
offset* record (`ax=0x556D`, via the resource stub `sub_27DE5`) back to
this same `FileEntry` whenever `TryInteractAtPosition` (was `sub_216F0`
— validates an interaction at a map position via `FindObjectAtPosition`,
branching on the target's type flags into `LoadLockState` — a lock/
door persisted-state loader, corrected from an earlier "weight/capacity
check" guess — `LoadCurgameRecord`, or a specific failure code) returns
certain outcome codes — not indexed by character, so plausibly global
state like the player's world position. `LoadLockState` itself reads
this same block (`0x556D`) back when examining a lock, alongside a
bit-packed "previously unlocked?" array in block `0x556C`. `CURGAME` is
also explicitly zeroed and closed
cleanly (`FileEntry_Write` with `_blockSize`/`_blockOffset` all zero,
then `FileEntry_Close`) on the quit-to-DOS path. `sub_27DE5`'s target
record itself isn't named yet.

**Party-member record** (in-memory, currently selected one pointed to
by `g_currentPartyRecord` — plausibly backed by this same `CURGAME` data once
loaded): lives in a **confirmed fixed array**, `g_partyRecords` (base
`0x95F3`, stride `0x1F4` = 500 bytes/record, up to 9 slots per
`SelectDefaultPartyRecord`'s scan bound) — not a linked list.
**Correction**: earlier documentation here claimed `g_currentPartyRecord` was
"traversed via a `+0x10` 'next' link"; that was wrong (a comment had
been misattached to the wrong call site — see
`ida_scripts/fix_party_record_next_claim.py`). `ApplyMapTriggerEffect`
independently confirms the same base/stride via a separate 4-slot
`g_partySlotAssignment` index table (base `0x95EB`, confirmed exactly
by `SelectClickedRosterPortrait`, was `sub_1930E` — it maps 4
screen-clicked portrait slots directly to `0x95EB`/`0x95ED`/`0x95EF`/
`0x95F1`). `SelectAndDrawPartyStatusRow`
uses the same table to map a party record back to its slot number
(1-4), fakes that digit as a keypress to reuse the main loop's
existing panel-select routine (`HandlePartyStatusPanelInput`), then draws that member's
status-bar row: portrait icon, name, level (`+0x16`), packed-BCD XP
(`+0x18`) — called both from a `UseItemType_400` path and from the
F1-F4/click-portrait party-member selection handler. The main
mechanism setting `g_currentPartyRecord` is now confirmed:
`SelectPartyRecordById` (a foundational, extremely widely-called
function) takes a 1-based record id and sets `g_currentPartyRecord` =
`(id-1)*0x1F4 + 0x95F3` (or 0 for id 0), also caching the id itself in
`g_currentPartyRecordId` — this is the function behind `g_partyRecords`'
base/stride confirmation above. Other mechanisms (a direct selector
struct in some callers, `SelectDefaultPartyRecord`'s "first record
with `+0xE`==0" scan as a fallback in `ShowPartyMembers`) still apply
in their own contexts. `ShowCreateCharacterPrompt` (was `sub_25544`,
also called from `ShowPartyMembers`) is built directly on that
empty-slot scan: if `SelectDefaultPartyRecord` finds no empty slot
(roster full), it does nothing; otherwise it wipes the slot
(`ClearPartyRecord`, was `sub_243C3` — zeroes exactly one
`g_partyRecords` stride, 500 bytes, confirming the stride from yet
another angle), draws a full-screen picture
(`DrawFullScreenPictureAndCacheToEMS`, was `sub_22387` — a generic
"draw picture + cache to EMS" utility reused by ~11 different screens
including `InitGame` and `RunDungeonGameLoop`), and writes
"CHARACTER CREATION" — the entry point into character creation from
the party roster screen. Confirmed
fields so far — `+0x0`: name (13 chars max, see `EditCharacterName`,
`ida_scripts/name_char_rename.py`, which — confirmed by one of its
call sites falling inside `EditCharacterName`'s own address range —
uses the generic `EditTextField` (was `sub_1D1D4`) single-line text
input editor, reused across at least 6 different text-entry screens:
draws a `-` cursor, handles Enter/Backspace/Escape/printable chars via
`PollKeyboardInput`); **`+0xE`: confirmed class id**
(**correction**: documented since early in the session as "a time-of-
day-like value" — wrong, or at least a worse fit. `RestCharacter`'s two
branches settle it: the HP-regen branch never touches `+0xE` at all,
but the MP-regen branch reduces it the same way `UseTrainingItem` does
`(cmp 9 / -0xA / cmp 9 / -0xA)` and, if <4, skips MP regen entirely
instead of gating on time; `UseTrainingItem` reduces the identical
field to pick between class-specific MP-growth formulas blending two
stat tables in different proportions. Read together: ids 0-3 are
plausibly non-caster classes with no MP pool to regenerate or grow —
far more consistent than a clock value correlating with class-specific
formulas in an unrelated function. `SelectDefaultPartyRecord` treats
`0` here as its scan target — whether that's "class 0" specifically or
something else isn't confirmed. **Now fully confirmed**: `GetClassNameString`
(was `sub_19768`) takes this exact field through the identical
`cmp 9 / -0xA / cmp 9 / -0xA` dispatch and returns a pointer into a real
27-entry class-name string table (11-byte stride, two contiguous blocks)
— `1 FIGHTER, 2 MERCHANT, 3 ROGUE, 4 MONK, 5 ALCHEMIST, 6 PALADIN, 7
MAGE, 8 DRUID, 9 MARKSMAN, 10 WARRIOR, 11 TINKERER, 12 THIEF, 13 CLERIC,
14 TRANSMUTER, 15 CAVALIER, 16 WIZARD, 17 ENCHANTER, 18 RANGER, 19
CHAMPION, 20 BLACKSMITH, 21 ASSASSIN, 22 PRIEST, 23 HEALER, 24 HERO, 25
SORCERER, 26 SAGE, 27 KNIGHT` — the game's full 27-class list, leaving no
doubt `+0xE` is the class id); `+0x10`: gender/type (compared
against `2` in `ShowCharacterEquipment`); `+0x16`: **confirmed
character level** — used in `FailsSavingThrow`'s save-chance formula
(`5*([+0x16] - threshold) + resistance bonus`), higher beats a higher
threshold, and incremented (capped at 90) by `UseTrainingItem`, which
also recalculates max HP/MP from it — a level-up/training item; also
the value `DrawCharacterClassAndLevel` (was `sub_2504F`, shared by
`ShowCharacterSkills` and the still-untraced `sub_23C18`) draws next to
the character's class name on the character sheet, which is what
upgraded this from "plausibly" to confirmed;
`+0x1C`: a status/condition flags word, tested throughout
(`RunTitleScreen`'s `E` handler, `ShowCharacterSkills`,
`RunConversation`, `UseAbilityOnTarget`'s `0xDFBB` table, and now a
"resting" bit in `RestCharacter`). **Fully mapped** via
`DrawAfflictionsList` (was `sub_25D82`, called from `HandlePartyStatusPanelInput`), which
draws the header "AFFLICTIONS:" then tests every individual bit and
shows the matching name (or "NONE"):
`0x2000`=**DISEASED**, `0x4000`=**POISONED**, `0x8000`=**SICK**,
`0x400`=**STONED**, `0x800`=**FROZEN**, `0x1000`=**PARALYZED**,
`0x80`=**CURSED**, `0x100`=**HEXED**, `0x200`=**JINXED**; bit `0x40`
separately confirmed as **DEAD** via `DrawThreeStatBars` (see below).
**Correction/extension**: "fully mapped" above referred only to the
affliction bits (`0x40` and up) `DrawAfflictionsList` itself tests.
The low 6 bits (`0x1`-`0x20`) are a *separate* sub-field: found via
`ApplySecondaryClassTierFlags` (was `sub_25456`, called from
`ShowCharacterSummary`) to encode which "secondary class" (ids 4-9 of
the 27-class table — MONK/ALCHEMIST/PALADIN/MAGE/DRUID/MARKSMAN) the
character has reached, one bit per class (`0x20` down to `0x1`),
gated on an unconfirmed `[+0x94]` marker and paired with up to 2
class-specific ability-flag grants via `SetRecordFlag_CA`. This one
function resolves several previously-separate findings at
once: `TickStatusEffects`/`ApplyStatusEffect`'s `0x400`/`0x800`/
`0x1000` group is STONED/FROZEN/PARALYZED (the *timed* ailments);
`CastSpell`'s `0x18` dispel bits (`0x2000`/`0x4000`/`0x8000`) are
DISEASED/POISONED/SICK (the *dispellable-only* group);
`CheckPartyWipeAndReinitLevel`'s `0x1C40` incapacitation mask is
DEAD+STONED+FROZEN+PARALYZED — literally "can't act"; and
`TickPartyAilmentIconBar`'s two effect-id groups are DISEASED/
POISONED/SICK (id `2`, all characters) vs. CURSED/HEXED/JINXED (id
`0xE`, MP-gated characters only); and `PickRandomActivePartyMember`
(was `sub_22A35`, called from `TriggerSideTrapForRandomPartyMember` —
**correction**: previously cited by its pre-naming address
`sub_22989`) uses the same `0x1C40` mask
to retry-pick a random party slot until it lands on one that's
occupied and not incapacitated — a classic "pick a valid random
target" utility. `TickStatusEffects` itself has two call sites: directly
from `HandleGameCommand`, and via `CheckAndTickAvailableAilment` (was
`sub_1A5A6`, called 3 times from `TickTravelResourceAilments`, was
`sub_1A582`, called from `TravelToDestination` to check 3
resource/consumable item types during travel) — a loop that calls
`IsItemRangeAvailable` and ticks the status effect whenever an item
turns up in range.

**`+0x20`–`+0x30`: 9 contiguous 2-byte protection/resistance values**
(`+0x20`, `+0x22`, `+0x24`, `+0x26`, `+0x28`, `+0x2A`, `+0x2C`, `+0x2E`,
`+0x30`), found via `RollEffectResistance` — each one is conditionally
summed into a trap/status effect's resistance-check total, selected by
one of 9 matching high bits (`0x8000`..`0x80`) in the effect-definition
record's cost flags. **Now identified by name** via
`DrawCharacterProtectionsList` (was `sub_25FCD`, called from
`HandlePartyStatusPanelInput`), which draws "PROTECTIONS:" and pairs each value with its
exact affliction name, in order: `+0x20`=**DISEASE**,
`+0x22`=**POISON**, `+0x24`=**SICKNESS**, `+0x26`=**STONING**,
`+0x28`=**FROZEN**, `+0x2A`=**PARALYZE**, `+0x2C`=**CURSING**,
`+0x2E`=**HEXING**, `+0x30`=**JINXING** — the resistance-value
counterpart to `+0x1C`'s active-flag bits above, matching one-to-one;
`+0x52`/`+0x92`: HP
current/max, confirmed by `CastSpell`'s heal codes
(`0x12`/`0x13`: +25%/+50% of missing; `0x14`: full heal directly);
`+0x54`/`+0x94`: MP current/max (`0x1D`: +50% of missing; `0x17`: full
restore), regenerated by `RestCharacter` the same way as HP —
independently confirmed by `DrawAlchemyStatusPanel`, whose bar for
this exact field pair is labeled "MAGIC:". `+0x58`,
`+0x64`, `+0x66`: three more fields, each averaged across the valid
party by `UpdatePartyAverageStatTiers` and each feeding a *different*
tiered gameplay system — `+0x64` → `word_36CA5`, compared against 5
ascending thresholds to set bits in `word_36C7F` that
`DrawMinimap`/`BuildMinimapTileData` read directly (bit `0x1000` blanks
the dungeon view entirely) — plausibly a light-source/torch-fuel level,
not confirmed. **A concrete lighting mechanism now found**:
`ApplyDistanceShadingToFloorOrCeiling` (was `sub_29FF6`, called from
`DrawDungeonFloorAndCeiling`) re-shades the floor/ceiling viewport
buffer band-by-band using a fixed 7-entry shade-delta gradient table
(`word_328E6`-`word_328F2`) — a distance-based brightness falloff,
plausibly fed by `+0x64`. **Confirmed shared with wall/monster
rendering too**: `RenderDungeonViewport` copies the same 7 globals
into `g_shadeShiftDelta` (the shade delta `DrawPicture`/
`ShiftPaletteShadeClamped` apply per pixel) before each of its 7
`RenderDungeonViewRow` calls — so this one gradient table lights
walls, monsters, floor, and ceiling consistently by depth row (see
the `RenderDungeonViewport` correction below for detail). Where the
gradient values themselves get computed isn't traced yet; `+0x66` →
`g_mapRevealAreaTier`, gating a 4-tier area size in
`RevealMapRegion` (**correction**: previously guessed "plausibly
weather" — traced further and it's a `Locate`/`Scout`/`Magic-Mapping`-
style special ability that reads `WORLD.DAT`/`CURGAME` directly and
reveals a `g_mapRevealAreaTier`-sized box of the map around the player, not a
visual weather effect). `RevealMapRegion` also calls
`TryTravelToClickedMapCell`: click a cell within the revealed area
(gated on that cell's "explored" bit) to instantly travel/teleport the
party there — a scry-then-teleport interaction consistent with a
Locate/Scout/Magic-Mapping ability; `+0x58` → `g_monsterDetailRevealTier`, gating progressively-
revealed detail icons in `DrawMonsterInfoPanel` (**correction, round
2**: the "3 fixed addresses" turned out to be `g_monsterSlots` — 3
active-combat monster records, found via `BuildCombatTurnOrder` — so
the *original* "identify"-style hypothesis was right after all, just
for the wrong reason at first: `+0x58` is plausibly a perception/
identify stat that reveals more monster detail as it rises, not a
bestiary browser — now further confirmed as a *derived* stat by
`ComputeDerivedCharacterStats`, computed from a weighted blend of the
6 base attributes plus a class-dependent bonus, not a raw rolled
value). Only the light/torch-fuel identity (`+0x64`) remains
an unconfirmed guess; `+0x66` (area-reveal-size) and `+0x58`
(monster-detail-reveal) are now on solid ground. `+0xB4`: a bitmask of
which of (at least) 4 special abilities this character has learned;
`+0xB6`/`+0xB8`/`+0xBA`/`+0xBC`: per-ability charge or level values,
each checked against a threshold in a small table at `0x77C6` before
`RevealMapRegion` (and presumably its 3 sibling abilities, `ax`=2..5)
will fire. **Confirmed by `UseAbilityScroll`**: a scroll/tome item type
that teaches a new ability sets the matching bit in `+0xB4` and zeroes
the matching charge field (`+0xB6`/`+0xB8`/`+0xBA` for bits `0x8000`/
`0x4000`/`0x2000`) — resetting that ability's charge to 0 the moment
it's learned, cross-confirming both fields' roles.
`AccumulateLearnedAbilityFlags` (was `sub_29040`, called from `UseItem`
and `UseAbilityScroll`) reads `+0xB4` to gate which entries of an
unidentified `word_2E54C` catalog table (stride `0x3A`) qualify to OR
their own `[+0x16]`/`[+0x18]` flags into accumulators
`word_2E40C`/`word_2E40E` — plausibly determining which item-use
options are available given the character's learned abilities.
`0x18` dispels/cures — clears bits 13-15 of `+0x1C`, a *different* 3-bit group
than the one `TickStatusEffects`/`ApplyStatusEffect` manage (bits
10/11/13 — bit 13 appears in both groups).

**The 6 core attributes are now mapped** (found via `RollCharacterAttributes`,
character creation's stat roller — resolves the "not yet mapped" note
that stood since early in the session): 6 base/derived field pairs,
each rolled `RandomInRange(15)+45` (**45–60**: that routine's range is
inclusive of its bound — see the note under "Trap and status effect
definitions" — so real characters do roll 60s) into the base field, copied
to the derived field 0x40 higher: `+0x3C`/`+0x7C` (also ×10 into a
weight-like derived stat at `+0x56`/`+0x96` — plausibly **Strength**,
carry capacity); `+0x3E`/`+0x7E` (**secondary use found**:
`RefreshCarryCapacityAndAttributeBonuses`, was `sub_1AA9B`, scales 20%
of the value above 72 into `+0x3A`/`+0x7A` as a threshold-gated
bonus, run whenever an icon-bar effect changes carry-relevant stats —
the same threshold/scaling treatment `+0x3C`/`+0x7C`'s own excess
above 72 gets into `+0x38`/`+0x78`);
`+0x42`/`+0x82` (one component of `UseTrainingItem`'s MP-growth blend —
plausibly **Intelligence**); `+0x44`/`+0x84` (the other MP-growth
component — plausibly **Wisdom**); `+0x46`/`+0x86` (feeds a separate
`UseTrainingItem` growth calculation — **correction, 2026-09-24: this
value, `word_2E38E`, feeds `RunItemServiceRecipientLoop`'s UI flow
control, not a character stat; see "UseTrainingItem" below**);
`+0x40`/`+0x80` (25%-scaled to
set both current and max HP, `+0x52`/`+0x92` — plausibly
**Stamina/Constitution**). Exact name-to-offset assignment for all 6
isn't independently confirmed — the roll order doesn't obviously match
the manual's STR/DEX/STA/INT/WIS/CHA listing — but the *pairing*
(base ↔ derived, and which pair feeds HP vs. MP) is solid. Independent
confirmation that these 6 base offsets (`0x3C`/`0x3E`/`0x40`/`0x42`/
`0x44`/`0x46`) form one coherent, evenly-spaced set: `UseAttributeBoostItem`
(was `sub_1B4C2`, a one-time-use "tome of attribute" consumable)
takes its target stat as a literal field offset from its own item
data, uses `(offset-0x3C)/2` to index a 13-byte-stride display-name
string table, and adds its boost amount to both that field and
field+`0x40` (the base↔derived pair) — the same base/derived stride
documented here.
**Correction**: `DrawThreeThresholdStats` — called right after
`RollCharacterAttributes` in `ShowCharacterSkills`, at `+0x4C`/`+0x8C`,
`+0x4E`/`+0x8E`, `+0x50`/`+0x90` — was first guessed to be 3 of these
6 attributes; it isn't, since those 6 are confirmed at the entirely
different `+0x3C`–`+0x86` range above. What `+0x4C`/`+0x4E`/`+0x50`
actually are (drawn alongside the attributes on the same screen, so
presumably related) is still open — though `+0x50` picked up a second,
independent data point this session via
`TriggerSideTrapForRandomPartyMember`/`RollTrapAvoidanceMagnitude`
(was `sub_22989`/`sub_227F5`): a party member's `+0x50` field is used
as a save-vs-trap avoidance stat (higher value, smaller/less-likely
trap effect) for a wall/door-embedded "side trap" trigger — an
unrelated system from `ComputeAlchemyRefinementYield`'s use of the
neighboring `+0x70` field, reinforcing that these are general
character stats reused across many systems rather than single-purpose
fields — though `DrawCharacterStatSheet`
(was `sub_24D30`, the character sheet's main stat renderer, called
from `ShowCharacterSkills` and `sub_23C18`) draws them in the *same
screen column* as the 6 attributes, immediately following (visual
slots 7-9 of the same list), reinforcing that they're a related but
distinct trio rather than coincidental neighbors. `ComputeDerivedCharacterStats`
(called right after `RollCharacterAttributes`, before
`DrawThreeThresholdStats`) computes a family of derived stats from
the 6 attributes — each a weighted percentage blend of 2-3 attributes
plus a class-dependent bonus, mirrored into current/max pairs
`+0x58`/`+0x98`, `+0x5A`/`+0x9A`, `+0x5C`/`+0x9C`, `+0x5E`/`+0x9E`,
`+0x60`/`+0xA0`, and more — but it starts at `+0x58`, so it isn't the
source of `+0x4C`/`+0x4E`/`+0x50` either.

`DrawCharacterStatSheet`'s right-hand column draws 13 contiguous
derived stats, `+0x58` through `+0x70` — confirming `ComputeDerivedCharacterStats`'s
block extends that far — and the *last 5* of them (`+0x68`/`+0x6A`/
`+0x6C`/`+0x6E`/`+0x70`) get a highlight color specifically when this
character's own roster-slot number matches one of 5 global "assigned
role" slots (`g_partyRoleAssignment1`/`g_partyRoleAssignment2`/`g_partyRoleAssignment3`/`g_partyRoleAssignment4`/
`g_partyRoleAssignment5`) — consistent with practical party-role skills (a
designated navigator, mapper, barterer, etc., per the earlier
attribute/skill string survey) where the game highlights whichever
character currently holds that role. Individual field-to-skill-name
assignment isn't confirmed yet, but the "5 assignable roles" shape is
a solid new lead for pinning them down. **One data point**:
`ComputeAlchemyRefinementYield` (was `sub_2AD32`, called from
`CastSpell`) gates a divisor (better yield for a higher stat) on the
last field in this group, `[+0x70]`, in what looks like an
alchemy/ore-refining calculation — consistent with `[+0x70]` being an
"alchemist"-flavored role, though not confirmed against the other 4
fields or the string survey's exact role names.

**A caution about reusing these offsets in combat code**:
`TryResolveAttackAgainstTarget` (was `sub_2D171`, part of the
`ApplyEncodedItemEffect`/`ApplyAttackToTarget` combat-dispatch tree)
reads `[di+0x4E]` and
`[di+0x58]` from a `di`-pointed record of unconfirmed type (attacker?
target? monster?) — *not* assumed to be the same party-record fields
documented above, since combat code may operate on a differently-
shaped monster or scratch struct at the same numeric offsets. Its
sibling in the same sequence, `ApplyTargetResistancesToAttack` (was
`sub_2D1C2`), reinforces this: it reads target immunity/resistance
*bitflags* at `[di+0x96]`/`[di+0x98]` (not weight-capacity-like
numbers) and drains an elemental resource counter at `[di+0x10]`/
`[di+0x54]`/`[di+0x56]`/`[di+0x58]`/`[di+0x5A]` — clearly a different
field layout than the party record's HP/MP/capacity fields at those
same numeric offsets. Filed as an explicit non-finding to avoid
conflating the two if `di`'s type is pinned down later (a monster
record struct is the leading candidate, still unlocated).

**The full per-target attack pipeline is now named end to end**:
`ApplyAttackToTarget` (was `sub_2D4B6`, called twice from
`ApplyEncodedItemEffect`, was `sub_2C0FE`, since named — one of its
~19 effect-type branches drives a direct attack/damage effect, not
just icon-bar status effects) calls `TryResolveAttackAgainstTarget`
to get a base damage/status result, `ApplyTargetResistancesToAttack`
to filter it by the target's resistances, then commits the surviving
damage/status directly into the target's
`[di+0xC]`/`[di+0x10]`/`[di+0x1C]`/`[di+0x1E]`/`[di+0x96]` fields.
This is a solid, fully-traced combat sub-pipeline.

**`di`'s record type is now confirmed**: `ApplyAttackAlongCorridorLine`
(was `sub_2D470`, a sibling of `ApplyDamageAlongCorridorLine` driving
this fuller pipeline) moves `GetMonsterAtViewportRow`'s result
directly into `di` before calling `ApplyAttackToTarget(di)` — so `di`
throughout this whole cluster is a **monster record**, not the party
record. The `[di+0xC]`/`[di+0x96]`/`[di+0x98]`/`[di+0x1C]`/`[di+0x1E]`/
`[di+0x4E]`/`[di+0x58]` etc. fields referenced by these functions are
therefore a *separate, still-unlocated monster-record struct* that
happens to share some numeric offsets with the party record — the
caution notes above were warranted, and none of those field identities
should be merged with the party-record documentation elsewhere in this
file.

**`ApplyAttackAlongCorridorLine`'s follow-up call, now resolved**:
`ReapplyDamageWithCompoundedResistance` (was `sub_2D428`, left unnamed
for two rounds pending this) runs only when `ApplyAttackToTarget` left
damage/status pending — which, since every one of that function's
early-return paths re-checks both are 0 first, only happens after it
has already run its own complete commit. So this is a genuine
**second** commit pass on the same monster target: it re-filters
status flags by immunity (idempotent, bits stay set) but recomputes a
*compounded* resistance halving (once per matching bit among the same
7 resistance-category bits `ApplyTargetResistancesToAttack` checks
with only a single first-match halving) and subtracts that from
`[di+0x10]` again — a real double-application of damage. *Why* the
game does this specifically for corridor-line area attacks
(compounding elemental damage as a deliberate design choice, vs. an
artifact of the original code) remains an open question.

**Skill values found**: `ShowCharacterSkills` clears a **16-word array
at `+0xCA`–`+0xE9`** (`and es:[si+0xCA]... rep stosw cx=0x10`) before
drawing 3 category headers with 3/4/8 skill-name lines respectively
(15 total — matches a pre-existing comment noting "15 total" skill
lines). Individual skill names/offsets within that array aren't mapped
yet (the line-drawer, `WriteStringWithHighlightedChar` (was
`sub_23AF2`) — draws a string with exactly one character in a
highlight color, called once per category header — only draws label
strings; the numeric skill values themselves must be drawn by an
untraced call in the same function). Both `ShowCharacterSkills` and
`ShowCharacterInventory` also call `DrawQuitOrReturnLabel` (was
`sub_25595`) for their bottom-left exit button: `QUIT "CREATE"`
(highlighting the `Q`) when `g_uiScratchFlags4` bit `0x8000` is clear, or
`RETURN` when set — these screens are shared between viewing an
existing character and the character-creation flow, and the button
text/behavior switches accordingly. **This also sharpens an earlier hedge**: since
`+0xCA` here holds plain word values (not a bitmask), it's now fairly
confident that `GetRecordFlagBitAndWord_CA` (the per-object flag bank
at the same relative offset, from several rounds ago) operates on a
*different* record type than the party record, not this one — that
hedge said "unconfirmed record type," which now reads more like
"confirmed not the party record." **Follow-up**: found the 8 item
slots too, via `ShowCharacterInventory`'s item-selection handler
(`HandleInventoryGridClick`, was `sub_26415`, since named — also
shared by `RunPartyInventoryScreen`, the equipment/bag grid click
handler behind the portrait-click "open inventory" screen) down into
the new `GetInventorySlotPtr`. Each character
has up to **4 separate 8(-ish)-slot inventories** — 1 main plus 3
"alternate bags" — not stored directly on the base record:
`GetInventorySlotPtr(slot 1-9)` picks a group base (`+0x118` by
default, or `+0x180`/`+0x1A6`/`+0x1CC` if the matching marker field —
`+0x17C`/`+0x1A2`/`+0x1C8` — is populated) and returns
`group_base + 2 + (slot-1)*4`, i.e. 4 bytes per slot. Individual slot
content (item id vs. quantity split) not decoded yet. This also
explains why the slots weren't visible from `ShowCharacterInventory`'s
own drawing code — that reads generic catalog ids for its labels, not
per-character storage directly.

**The group base's leading 2 bytes (the ones `GetInventorySlotPtr`
skips with `+2`) are now confirmed: a running total-weight-carried
counter for that inventory group**, not padding. Three independent
functions agree: `PickUpItemFromSlot`/`PlaceItemInSlot` add/subtract
an item's value at exactly `+0x118` (main inventory) or `+0x180`/
`+0x1A6`/`+0x1CC` (the 3 alternate bags) whenever an item enters/leaves
that group; and `DrawThreeStatBars` (was `sub_25F10`) draws `+0x118`
labeled **"WEIGHT:"** against `+0x56` as the max — and `+0x56` was
already suspected as a carry-capacity stat (derived from Strength×10
via `RollCharacterAttributes`). So the full picture: each inventory
group is `[2-byte running weight total][8 slots × 4 bytes]`, and
`+0x56`/`+0x96` is Strength-derived max carry weight, checked only
against the *main* inventory's `+0x118` counter (the 3 alternate bags'
weight isn't shown on this panel, at least).

**Container search** (`FindItemInInventoryRange`, `FindItemInsideContainer`/`Level2`/`Level3`,
`partyFindItemDeep`): a container item's extra word is the 0-based item-instance record number
holding its 8 slots (`FileEntry_Seek` offset = base + record * 34, no decrement); nested containers
are searched to three levels, a hit inside a container is returned before the next main slot is
examined, and the equipped slot at `+0x13E` is searched last (only as a container).

**The "3 alternate bags" are literal container items**, confirmed via
`HandleInventoryGridClick`'s (was `sub_26415`) open/close branches: clicking an unopened container item
assigns it to the first free marker (`+0x17C`/`+0x1A2`/`+0x1C8`,
priority order) and calls the new `LoadContainerContents`, which reads
that container's own saved inventory contents from **`CURGAME`**
(`FileEntry` `bx=0x8FFB`, `errorCode=0xB`) into the matching bag slot
area. Clicking an open container again calls `SaveAndCloseContainer`,
which writes the bag's contents back to `CURGAME` (same `FileEntry`)
if nonempty, then clears the marker — unloading it.
`CloseAllAlternateBags` (was `sub_266A9`, called from
`RestorePortraitAreaAtPosition`) calls it for all 3 marker offsets at
once, e.g. when leaving the inventory screen. So each bag's
contents persist independently in the savegame, swapped into the
character's inventory groups only while open.

**`IsContainerTypeCompatible`** (called from `sub_2621C`, a 230-line
main-input-loop handler not traced this round) checks whether the
currently-open alternate bag matches an allowed-type bitmask before
letting an item be placed into it, rejecting with
`FlashStatusWarning` otherwise. `sub_2621C` also calls
`IsItemEligibleForCommand` (was `sub_26A75`) right after loading the
clicked item's catalog record — a generalized eligibility gate for the
current command code (`g_currentCommandCode`) against the item's flags, gating
whether an inventory action is allowed on this item at all (a superset
of the pattern already seen in `IsItemEligibleForEnhance`/
`IsItemEligibleForRepair`). `sub_2621C` also uses
`LoadNextContainerInChain`, which walks a linked chain of container/
world-object records via `CURGAME` (each record's own `[+8]` field
points to the next id) — e.g. multiple containers found together —
loading each via `LoadContainerContents` in turn. `TryLoadNextContainerLink`
(was `sub_18FC5`, called 9 times from `sub_18C79`) is a small guard
wrapping this: only calls `LoadNextContainerInChain` if the item is
itself a container ([+0xC] bit `0x2000`) and a caller-supplied flag
allows it. Both
`LoadNextContainerInChain` and `sub_2621C` also call
`CommitContainerWrite` (was `sub_26C0E`) — a minimal write-commit
(`FileEntry_Write(errorCode=0xB)` + `ErrorCheck`) that, unlike the
similarly-shaped `SyncContainerContents`, does no descriptor setup of
its own and assumes the caller already configured the container-write
descriptor.

**A shared scratch buffer at `0xAFA8`** is reused across many unrelated
subsystems (item records, conversation-topic text — see `WORLD.DAT`
notes below — and more) as the landing spot for whatever a resource-
stub helper (`PrepareRecordAtIndexDC6`/`PrepareRecordAtIndexDCA`/
`PrepareGroundItemSlotBlockRead`/etc.) is currently configured to
read into or write from. **`ConsumeItemChargeResource`** (was
`sub_274B4`, called from 21 sites including `CheckAndPaySpecialItemCost`
and `HandleSearchCommand`'s failed-trap-search) is the shared "spend
one use of an item-based resource" engine: driven entirely by
caller-configured globals, `g_uiScratchFlags3` bits `0x8000`/`0x4000`/
`0x2000` select the consumption mode — recharge-and-reset-wear (also
zeroing the matching equipped-item durability counter,
`+0xBE`/`+0xC0`/`+0xC2`, the same fields `TickEquippedItemDurability`
increments), full discard, swap-effect-then-discard
(`SwapItemMultiStatEffect`), or the default decrement-with-auto-
discard. It deducts the consumed item's weight from the owning party
member's carry capacity, then syncs bags, redraws the portrait, and
reapplies stat effects. Immediately before spending the charge, both
known callers of this pattern first call `ApplyEncodedItemEffect`
(was `sub_2C0FE`, the largest function in the binary at 4,210 bytes,
also called from `RunAlchemyScreen`): a flat bitmask switch over two
flag words (`word_33302`/`word_33306`, ~19 distinct effect types, one
handled per call) that applies the item's/container's coded magical
effect — confirmed via `InteractWithContainer`'s own sequence
(`ConfirmContainerInteraction` -> `ApplyEncodedItemEffect` ->
`ConsumeItemChargeResource`). The two branches read closely apply a
single-target or whole-party icon-bar status effect
(`ApplyEffectAndDrawIconBar`); the rest (world-state timers, etc.)
weren't individually traced given the function's size.

**`ConsumeItemChargeResource`'s default mode partially reimplemented,
2026-10-01**: confirmed the "default decrement-with-auto-discard" mode
is what every caller except `RepairItemCommand` gets -- it's the only
site in either game that ever sets any of the 3 special mode bits, and
it always clears them again immediately after its own one call, so
`RestPartyAndAdvanceClock` (the caller this round actually needed --
see the "R rest" regen-rate section below) and every other caller
always hit this mode. Reimplemented both of its branches: the
party-inventory case as `partyConsumeItemCharge` (`src23/party.c`/`.h`):
if the item's own target-entry flags (`itemTargetWord(entry, 1)` bit
`0x1`, not previously named -- real food items in both games all have
it clear) are set, decrements the slot's own `itemSlotExtra`, keeping
the item if charges remain; otherwise clears the whole slot and
subtracts the item's `ItemFieldWeight` from the *main* inventory
group's weight total -- unconditionally the main group, matching the
original's own hardcoded offset regardless of which group the slot
actually belongs to. The 6-entry global-table case (the same decrement-
or-discard logic, no weight to deduct) is `itemSlotConsumeGlobalCharge`,
reimplemented the same day. Not reimplemented: the 3 special modes
(recharge/discard/swap -- `TickEquippedItemDurability`'s own
equipped-item durability write-back, `SwapItemMultiStatEffect`), and
the portrait-redraw/bag-sync UI side effects. Tests in `test_party.c`
cover a single-use item (discarded immediately, weight deducted), a
multi-use item losing one charge without discarding, and a multi-use
item discarding on its last charge; all 23 suites pass.

**Both status-effect branches reimplemented, 2026-09-26**: unlike the
search/lockpicking trap's own `ApplySavingThrowEffect`, neither branch
here rolls anything — the caller supplies an already-resolved
inflictedStatus/magnitude pair that `RollEffectMagnitude`/
`RollEffectResistance`'s own "already set"/"already resolved"
early-outs (see the "Turn-based combat" section's effect-pipeline
writeup) exist to accommodate; this is the concrete call site those
early-outs were built for. A shared gate
(`ResetOrCopyTargetPositionFields`, `yendor2.asm:53238`, instruction-
identical in Chapter 3) zeroes both values instead of applying them
when a global flag is set and the *acting* character
(`g_currentPartyRecord`) is Cursed. The single-target branch's own
icon-slot-reuse search turned out to be pure UI bookkeeping — the
record it actually affects is unconditionally the acting character,
regardless of which slot the search finds. **A real Chapter 2 vs.
Chapter 3 difference found while confirming the whole-party branch is
instruction-identical**: Chapter 3 adds a per-recipient skip for any
Cursed party member (not even icon-slot-populated) that Chapter 2
altogether lacks — see `engine-diffs.md`. The whole-party branch also
reproduces the same "stops dead at the first unoccupied
`SaveHeaderPartySlots` slot" quirk `ApplySavingThrowEffect`'s own
whole-party branch has, but — confirmed by reading both directly, not
assumed from that similarity — does *not* skip incapacitated members
the way that other mechanism does. Reimplemented as
`combatResolveEncodedItemEffectValue`/`combatApplyEncodedItemEffectSingle`/
`combatApplyEncodedItemEffectParty` in `src23/combat.c`/`.h`. Tests in
`tests/test_combat.c` cover the curse-gate zeroing (active+cursed,
active+not-cursed, inactive+cursed), single-target application, an
out-of-range effect id, the whole-party stop-at-empty-slot quirk, and
the Chapter 2/Chapter 3 cursed-skip difference directly. Still open:
the surrounding dispatch itself (which of `word_33302`'s ~19 bits
fires; whether `word_33300`'s own `0x800`/`0x1000` bits — read from an
untraced caller context — select this path at all versus skipping the
icon-bar entirely) and the other ~17 branches (world-state timers, a
corridor/ranged-attack path, held-item cursor updates, weather
effects).

**Surveyed several more branches, 2026-09-29, and found one more
tractable one**: the remaining branches split cleanly into "genuinely
entangled with a separate, not-yet-touched subsystem" and one clean
win.
- **Bit `0x80`** (world-state timers): sets one of 6 duration counters
  (`word_36C93`..`word_36C9D`) and a matching bit in `word_36C79` —
  confirmed as the same 6 counters `TickWorldAilments` sums for
  ambient-lighting darkening, and the same overloaded bitfield the
  timed-status-ailment system (Diseased/Poisoned/etc.) and the
  day/night ambient-lighting table builder both also read/write for
  unrelated purposes. Genuinely tangled with a whole not-yet-scoped
  "world ailments / weather / lighting" subsystem
  (`TickWorldAilments`, the ambient-lighting table builder, the
  day/night cycle) — not a good target in isolation. **A real hazard
  found while reading it**: the index dispatch (`bx` = `word_332E4`,
  compared against `1`..`6`) has no `else` arm — an index outside
  `1..6` falls through into an infinite loop (`cmp bx,6 / jnz
  loc_2C2B0`, which re-enters the `cmp bx,3` test with `bx` unchanged).
  Presumably unreachable in practice (real item/spell data always
  encodes a valid index), not reproduced.
- **Bit `0x10`** (create/conjure an item onto the cursor): touches
  `g_heldItemType`/`word_31948`/`word_3194A`/`word_3194C`, the "item
  currently held on the mouse cursor" state — a pervasive UI
  drag-and-drop system read/written by ~40 other functions across
  inventory, container and shop screens. Not a self-contained piece;
  needs that whole system as a prerequisite, not a good target either.
- **Bit `0x4`** (a corridor/ranged-attack path): confirmed to match
  `roadmap.md`'s existing scoping exactly — a 4-direction dispatch on
  `g_partyFacing` feeding `ApplyDamageToMapMonster`, which still needs
  `ApplyAttackToTarget` traced first.
- **Bit `0x2`** ("rest here"): just calls the already-named, not-yet-
  reimplemented `RestPartyAndAdvanceClock` (a whole separate UI-heavy
  top-level command) with a couple of flag/wait wrappers around it —
  nothing to add here ahead of that function's own pass.
- **Bit `0x1`, reimplemented**: a "Knock"-style effect. Probes the
  party's own cell, then the cell one step ahead in their facing, via
  a new shared primitive, `ProbeFacingTile` (`yendor2.asm:30915`,
  instruction-identical in Chapter 3) — reimplemented as
  `worldObjectProbeFacingTile` in `src23/worldobjects.c`/`.h`, reusing
  `worldObjectFind` for the lookup itself and `movementApply`'s own
  forward-direction delta rather than re-deriving the same
  North/South/East/West table a third time. Whichever cell has a world
  object there gets classified via the already-existing
  `interactClassify`, and if the outcome is `InteractOutcomeLockMagical`
  or `InteractOutcomeCurgameFallbackB`, the matching bit in the shared
  "already unlocked/triggered" bitmap is set — the exact same write
  `UnlockDoorCommand` performs on an ordinary key-based unlock.
  Resolving this also cleared up a small worry from tracing it: the
  original's own `g_lockUnlockedMask`/`g_lockUnlockedAccumulator`
  scratch globals (seen throughout `UseAbilityCommand` and elsewhere)
  turned out to be nothing more than a *cached single byte* of the
  same `SaveSectionEventState` bitmap `interactBitmapTest`/`Set`
  already model — set by `LoadLockState`/`LoadCurgameRecord` themselves
  as part of loading a record — not a separate mechanism, so no new
  bitmap semantics were needed. Reimplemented as `interactKnock` (plus
  a small reusable `interactWorldObjectBitIndex` helper) in
  `src23/interact.c`/`.h`. A sibling branch, bit `0x40`
  (`yendor2.asm:51852`), shares this exact probe-then-classify-then-mark
  shape but targets a different outcome set
  (`LockFlag40`/`LockPriced`/`CurgameFlag40`) and adds a UI text-column
  choice on top (`g_lockStatusFlags` bit 0x80 selecting between 2 text
  IDs via `ShowAbilityDescriptionColumn`, pure UI, not modeled). Since
  the only *decision-logic* difference between the two branches is
  which outcomes qualify, `interactKnock` was refactored onto a shared
  primitive, `interactResolveIfOutcome(save, game, object, lock,
  lockAlreadyUnlocked, curgameFlags, curgameAlreadyTriggered,
  monsterAlreadySpawned, qualifying, qualifyingCount)`, that bit 0x40
  can call directly with its own outcome set — deliberately not given
  its own named wrapper the way bit 0x1 got `interactKnock`, since this
  project doesn't have a confident read on what narratively unifies an
  unconfirmed lock flag, a priced lock, and an unrelated curgame flag
  into one spell/item effect (unlike "Knock," which reads cleanly).
  Tests in `tests/test_worldobjects.c` (all 4 probe outcomes, including
  that the returned coordinates are the *facing* cell's, not the
  party's own, when the first probe misses) and `tests/test_interact.c`
  (both door and curgame-record cases for both outcome sets, the
  already-resolved case, and no object at all).

**`ApplyTargetResistancesToAttack` fully decoded and reimplemented,
2026-09-29** (while resolving the equipment-corrosion write-back —
see the "Attack resolution" section for the full context of how this
surfaced): filters a pending player-triggered item/spell attack
against a map monster (`g_levelMonsters`) before
`ApplyAttackToTarget`/`ApplyDamageToMapMonster` commit it. Confirmed,
by cross-referencing every field offset this function touches against
`monster.h`'s own already-decoded fields (from an earlier session's
clue-book UI work), that `di` is a full `MonsterRecordSize` record —
resolving the original's own "di's record type here is unknown"
uncertainty (its own contemporaneous IDA comment). `word_33304`'s high
bits (`0x400`-`0x8000`) are candidate inflicted-status bits, filtered
by `MonsterFieldImmunities`; its low bits (`0x1`-`0x10`, same enum)
instead zero the attack's damage entirely on a match — a clean resist,
not a partial filter. `word_33306` (`0x200`-`0x8000`,
`MonsterFieldResistances`' own `MonsterResistMagicMask`/`PhysicalMask`)
halves damage on the first matching bit, checked in a fixed priority
order — with a genuine asymmetry reproduced exactly: requesting bit
`0x200` specifically returns immediately even on a *miss*, since it's
the last bit checked and the original simply falls out of the chain
rather than continuing, unlike the other 6 bits which keep checking on
a miss. Only when `word_33306` didn't request bit `0x200` at all does
a **drain effect** get a chance to run: `word_33304` bits `0x20`-`0x200`
(checked in priority order `0x200 > 0x100 > 0x80 > 0x40 > 0x20`) select
one of the target's own combat-stat fields —
`MonsterFieldHealth`/`Accuracy`/`Dexterity`/`Absorption`/`Damage`
respectively — and subtract a caller-supplied amount from it, floored
at 0. A genuine "permanently weaken this monster's own stats" effect
category, distinct from (and reached via a completely different code
path than) ordinary HP damage. Reimplemented as
`combatApplyTargetResistances` in `src23/combat.c`/`.h`, instruction-
identical in Chapter 3. Tests in `tests/test_combat.c` cover every
branch: status filtering, full negation (and that an already-filtered
status bit survives it), resistance halving, the bit-`0x200`-miss
asymmetry, and all 5 drain fields including their priority order.

**Still not composed with its own callers**: `ApplyAttackToTarget`
(base damage via `combatResolveAttack`, using the target's own
`MonsterFieldAbsorption` as defense and the acting party member's own
`PartyStatCasting` as accuracy — confirmed directly from
`TryResolveAttackAgainstTarget`'s own register sourcing, `[di+0x58]`/
`[si+0x62]`) and `ApplyDamageToMapMonster` (adds death/reward handling,
already fully reimplemented elsewhere as
`monsterGrantRewards`/`monsterPoolRemove`) both still depend on more
untraced caller-context globals (`word_33300`, the tick-timer
amount/countdown sources that arm `monsterTickTimer` on a successful
status hit, etc.) that `ApplyEncodedItemEffect`'s own bits
`0x4`/`0x2000` — the two branches that reach this whole family, per
the "Attack resolution" section — haven't been traced far enough to
supply. Left for a future pass rather than composed against
unconfirmed inputs; see `roadmap.md` candidate 8.

`SyncItemChargeFieldToCurgame`
(was `sub_2778D`, called 3 times from `ConsumeItemChargeResource`)
reads a `CURGAME` record into it (`errorCode=0xA`),
applies the same charge/transfer/swap-category logic
`ConsumeItemChargeResource` applies to its own in-memory item fields
to a chosen field
(`[0xAFA8+dx]`), then writes it back — except one category/`dx`
combination instead adjusts generic scratch variable `word_38808` and
skips the write. Reinforces that `0xAFA8` is a true scratch landing
zone, not itself a stable record layout — any field offsets within it
are only meaningful for the specific record currently loaded there.

**Dropping a held item**: `TryDropHeldItem` (checks `IsItemDroppable`,
warns and bails if not; shows a confirm prompt; calls
`PlaceItemOnGround` on confirmation, else restores the held item)
handles the "drop" action. `PlaceItemOnGround` recurses into a
dropped container's 8-slot contents (catalog `[+0xC]` bit `0x2000`,
the same container flag) so a dropped bag's full contents are placed
too, not just the container item itself. Its low-level slot I/O:
`ReadGroundItemSlot` (was `sub_1A34C`, called 3 times) reads a ground/
world-object slot record; `PrepareGroundItemSlotWrite` (was
`sub_1A320`) clears a scratch buffer and transplants the read count
into place before calling `CommitGroundItemWrite` (was `sub_1A36A`) —
a minimal write-commit, the same shape as `CommitContainerWrite` but
for the ground slot. `IsItemDroppable` itself, for
a held container, calls `HasDroppableItemInInventory` (base case
`HasDroppableItemInContainer`) — a container is only droppable if it
holds at least one directly-droppable item somewhere inside it.

A related but distinct action, `FinishPlacingHeldItem` (was
`sub_2BA62`, called from `sub_271DC` and the still-untraced,
container-related `sub_2621C`), also clears the held-item cursor
(`UpdateCursorForHeldItem(0)`) after loading the held item's catalog
record and OR-ing a value derived from one of its flag bytes into
`g_heldKeyFlags`. **Resolved**: this is the player's held-key flags
accumulator, confirmed by `HandleGameCommand`'s unlock-door handler,
which compares it against `g_lockStatusFlags`' key-tier bits to decide
whether the party is carrying a high-enough-tier key to open the
current door — so `FinishPlacingHeldItem` placing a key-type item
updates the carried-key tier available for unlocking, alongside
whatever container/slot placement it performs.

A third item-manipulation action, `SwapHeldItemWithSlot` (was
`sub_268A0`, also called from `sub_2621C`): a classic drag-and-drop
swap, done via a careful save/restore dance around the held-item triple
(`word_31948`/`word_3194C`/`word_3194A`) across two calls — one that
presumably picks up a target slot's item (`sub_26B4F`, not traced) and
one that places the original held item into that slot (`sub_266D4`,
not traced) — finishing with a portrait redraw and cursor update. A simpler sibling,
`PlaceHeldItemIntoEmptySlot` (was `sub_2687B`, also called from
`sub_2621C`): the same "swap-family" sound, but calls `PlaceItemInSlot`
(was `sub_266D4`) directly with no pickup step (the slot is already
empty). `PlaceItemInSlot` and its exact mirror image,
`PickUpItemFromSlot` (was `sub_26B4F`), are now both confirmed and
named: `PlaceItemInSlot` ADDS the placed item's value to one of 3
equipment-section running totals (`+0x180`/`+0x1A6`/`+0x1CC`, the same
3 offsets `WriteContainerSubBlock`'s fields sit next to, selected by
condition-icon flag bits `[+0x15C]` `0x400`/`0x200`/`0x100`) or the
general total `+0x118`, skipping a section whose "type" field
(`+0x17C`/`+0x1A2`/`+0x1C8`) equals `0x11`; `PickUpItemFromSlot`
SUBTRACTS the same way when removing an item. `+0x118`/the 3 section
totals are plausibly weight or quantity counters, not fully confirmed.
`PickUpItemFromSlot` also calls `RemoveMultiStatEffect` (was
`sub_1AC2F`) — the removal counterpart to the already-named
`ApplyMultiStatEffect`: walks the item's multi-stat-effect table
(`g_itemStatEffectTable`) subtracting each entry's bonus from the matching party
stat field (floor-clamped at 0 for one sub-range of fields), then
recalculates via `UpdatePartyAverageStatTiers` — i.e. taking off a
magic item correctly reverses whatever stat bonuses putting it on
applied. Its exact ADD-side mirror, `ApplyMultiStatEffectForItem` (was
`sub_1AA06`, called 6 times incl. from `HandleIconBarItemExpiry`), is a
distinct, lower-level primitive from the higher-level command handler
`ApplyMultiStatEffect` — same table-walking core, capped at `0x3E7`
instead of floored at `0`, but without that command's target
confirmation, incapacitation check, or full redraw sequence.
**`g_itemStatEffectTable` reimplemented, 2026-09-29**: turned out to
already be `item.h`'s own `itemEffectEntry` (this project just hadn't
connected the two) — see the `HandleIconBarItemExpiry` note further
below for the full writeup, including the "type id is a raw
party-record byte offset" confirmation and both add/remove
asymmetries. Reimplemented as `partyApplyMultiStatEffect`/
`partyRemoveMultiStatEffect` in `src23/party.c`/`.h`.
`SwapItemMultiStatEffect` (was `sub_276C5`, called 3 times from
`ConsumeItemChargeResource`) ties the whole cluster together: if the current item's
category matches a `word_2E548` sub-flag, it removes the current
item's effect, swaps in a new item id from `word_2E548`'s `+4`/`+8`
field (the exact fields `GetClassifiedItemStatField` selects between),
and applies the new item's effect — replacing one equipped item's stat
effect with a different item's, per category.
A fourth sibling, `PickUpHeldItemFromSlot` (was `sub_26864`, called
from `sub_2621C`), is the simple "pick up only" action (no placement
step) — the pickup counterpart to `PlaceHeldItemIntoEmptySlot`. All
four actions (`FinishPlacingHeldItem`, `SwapHeldItemWithSlot`,
`PlaceHeldItemIntoEmptySlot`, `PickUpHeldItemFromSlot`) are called from
`sub_2621C`, reinforcing that it's the main container-interaction
input handler (still not traced as a whole — 230 lines).

**`SyncAllContainers`** (found via `RepairItemCommand`'s opening call)
writes every open bag's contents back to `CURGAME` across the *whole
party*, without closing them (`SyncContainerContents`, the same
write-back as `SaveAndCloseContainer` minus the marker-clear) — a
"commit everything to the savegame" step called before risky actions
like the repair minigame, presumably so an in-progress bag's state
isn't lost if the action fails.

A related low-level primitive: `WriteContainerSubBlock` (was
`sub_2776F`) writes a data block (address + count) via the same
`FileEntry_Write(errorCode=0xB)` pattern. Called 3 times from
`SyncAlternateBagsToSave` (was `sub_2772C`, itself called from
`ConsumeItemChargeResource`) for the 3 "alternate bag" inventory groups at
`+0x17E`/`+0x180`, `+0x1A4`/`+0x1A6`, `+0x1CA`/`+0x1CC` — the same
group-base fields `GetInventorySlotPtr` already established (count
field followed by its slot data), written only when each bag is
populated (count nonzero). Confirms those 3 sub-blocks are exactly the
3 alternate bags, resolving the earlier "identity not confirmed" note.

### Party travel / fast-travel

`TravelToDestination` (called from `start` and `ExamineTarget`) is the
party teleport/fast-travel handler: given a destination id, looks up
a destination table (`0xD40B`) for the new position/facing, an
optional message, and a music/mode flag; gated by
`IsDestinationUnlocked` (a separate eligibility table, `0xDFBB`) when
the destination requires it, rejecting with a message if not yet
unlocked. Reveals cells around the new position and redraws the
screen/minimap on success.

### Travel keys (`ExamineTarget`)

An item whose consumable-target entry (`item.h`) has word0 flag 0x4 (and word1
& 0x7E00 clear) is handled by `ExamineTarget`; word 2 of the entry is a travel
destination id. Real data: Chapter 2 KEY OF PORT HOPE (dest 12), PARIAH (4),
NUMAGIK (9), STONY PEAK (19), TRACKING (91), all word0 0x000C; Chapter 3 ATHANEUM
KEY (2) and ANKH OF PORTALS (3). The key travels at once unless the destination
has a gate-table row (see above) with its flag clear -- NUMAGIK and STONY PEAK --
when it only shows a description. Chapter 3 adds a story gate on global flag 0x9D
and items 0x17F-0x181 (SWORD/HAMMER/TRIDENT OF LIGHT). `travelExamineKey`.

### Map tiers and the local area map (`UpdatePartyAverageStatTiers`, `ShowLocalAreaMap`)

**Field identities settled** (the notes under the party record's stat block called
these unconfirmed): `UpdatePartyAverageStatTiers`'s three fields `+0x64`, `+0x66`
and `+0x58` are stat indices 20, 21 and 14 of the array at `0x3C` -- the
**MAPPING**, **NAVIGATION** and **SURVIVAL** skills. The averages are over members
in slot order (stopping at the first empty slot, skipping Dead/Stoned/Frozen/
Paralyzed). Mapping's average sets cumulative tier bits in the minimap word
`word_36C7F` (>= 45: 0x400, >= 50: 0x8000 large-minimap capable, >= 60: 0x200 the
local map works, >= 70: 0x800 it prints coordinates, >= 80: 0x100 the full-screen
overview works); the same word's 0x1000/0x2000/0x4000 are the minimap's hidden/
small/large display modes (so the "light-source fuel" guess was wrong -- it is the
Mapping skill). Navigation sizes `RevealMapRegion`'s box; Survival gates the
monster panel's detail tiers (0x37/0x4B/0x50).

`ShowLocalAreaMap` (action 0x1E) is refused with "YOUR SKILL IS NOT HIGH ENOUGH!"
unless the 0x200 tier is set (the map editor's call, `g_uiScratchFlags1` bit 1,
bypasses; Chapter 3 also bypasses on bit 0x8000). It shows the 40 x 24 *page* of the
world containing the party (page index `(y / 24) * 20 + x / 40`, the clue book's
location index), one 8-pixel tile per cell: unexplored (fog-bitmap bit clear) cells
draw the fixed tile 0x13, explored ones their own two tile layers; the party's arrow
(picture 0 north / 2 south / 1 east / 3 west) is drawn at `(x % 40, y % 24 + 1)`.
`ToggleMapViewMode` (0x1F) needs the 0x100 tier and draws the full-screen overview
picture with `DrawPlayerPositionMarker` for world x 160..639, y 48..239. All
instruction-identical in both games apart from the one bypass bit. `mapview.c`.

### Mounts in use, the per-page table and the fog-of-war reveal (`mount.c`, `explore.c`)

**Correction: `RevealMapRegion` is the mount-flight command, not a "Locate/Scout/Magic-Mapping" spell.** The
four special abilities (PEGASUS, GIANT EAGLE, FLYING RUG, MAGIC DRAGON -- the table at `DS:0x77C6` that the
transport teachers sell from) each have a daily charge count (1, 2, 4, 4; `ResetDailyAbilityCharges` zeroes
`+0xB6..` every new day) and a time-of-day word (bit 1 = night, bit 2 = day; 07:00-19:00 inclusive is day: the
RUG flies only by day, the DRAGON only by night). The command shows an 11x7 to 27x17 box of the map (size from
the party's average Navigation: 65/80/95) and the player clicks an explored cell to fly there
(`TryTravelToClickedMapCell`: refused for unexplored, impassable, trap/trigger cells; a charge is spent on
success). **The per-page table**: WORLD.DAT `0x70800` (Chapter 2, 120 records) / `0x83400` (Chapter 3, 140),
6-byte records indexed by page `(y / 24) * 20 + x / 40`; byte 5 is a page attribute 0/1/2 (Chapter 2: 48/25/47
pages) that decides which other pages' cells the box shows (only from an attribute-1 page, never into a 2) and,
in Chapter 3 only, forbids flights to another page when either end is attribute 2.

The fog-of-war bitmap (MSB-first, one record per row) and `RevealCellsAroundPlayer` (each step marks the party's
row/column and the one ahead, three cells wide, in the order -1, +1, 0) are `explore.c`.

### Tile-type legends, both games (`worldmap.c`)

The wall/base type (tileA) and floor/overlay type (tileB) of a map cell index two legend tables of raw picture data. Chapter 2:
flat tables at DS:0xE551 (58 wall entries of 6 words) and DS:0xE175 (68 floor entries of 5 words). **Chapter 3 pages them**
(`sub_1BC98` / `sub_1BCDB`, which earlier notes had called EMS-paged -- they are data-segment tables): a type is
`page * 100 + index`, with a page table (4 bytes per page: entry pointer, highest valid index) at DS:2 (walls: pages 0-3 with
10/4/9/43 entries) and DS:0xC8E7 (floors: pages 0-3 with 12/72/55/2). All 800 x 168 cells of Chapter 3's real map resolve
to a wall and a floor entry (checked in `test_worldmap.c`). Wall words (corrected 2026-10-04 once the renderer was
decoded): 0 floor picture id (PICTURES.VGA category 4), 1 ceiling picture id (category 5), 2 wall picture id (category 1;
0 = open cell), 3 far-wall picture id (category 6), 4 its frame offset (which 14-column strip), 5 local-map picture; floor
words 0-3: overlay picture per party facing (N/S/E/W), 4 local-map picture. Chapter 3 additionally draws the entry's word 0
as the overlay for flag 0x2000 cells where Chapter 2 always draws picture 5.

### The first-person viewport cells and their occlusion (`BuildDungeonViewportCells`, `ComputeDungeonCellVisibility`; `viewport.c`)

The view is rendered from a 51-cell buffer: two 17-wide far rows, a 5-wide row, then four 3-wide rows ending with the
party's own row (cell 49 = the party), copied from the dungeon grid with a facing-dependent column/row step (start offsets
per facing in `viewport.h`). The occlusion pass sets bit 0 of a copied cell's flags: the first fully solid row of 3/3/3/5/17
cells hides everything beyond it, eight corner rules hide a list when two adjacent cells are solid, and every visible solid
cell from 50 down to 18 hides the cells it shadows. The static tables (identical in both games; the originals store buffer
addresses, `viewport.c` uses cell indices) sit at DS:0x0E/0xE0 (Chapter 2) and DS:0x30A/0x3DC (Chapter 3). "Solid" is wall type
2-5 in Chapter 2, 2-99 in Chapter 3.

### Drawing the first-person view (`RedrawDungeonScreen`, `DrawViewportSprite`; `viewrender.c`)

Decoded 2026-10-04 and verified by rendering real positions of both games (`src23/tools/render_view.c` writes a PNG; the
frames in `test_viewrender.c` were inspected). Nothing is scaled at run time: each of the 51 buffer cells has its own cut of a
pre-drawn picture, described by one 0x499C-byte block of WORLD.DAT (offset 0x19C7A5 in Chapter 2, 0x40BF71 in Chapter 3;
**byte-identical in both games**; `ida_scripts/dump_view_tables_offset.py`) that the original loads as "monster stats"
(`PreloadMonsterStatsTable` is misnamed). The viewport is 224 x 136 at (8, 8) of the 320 x 200 screen.

Order (`RedrawDungeonScreen`): base ceiling (category 5, 224 x 62, at y = 8) and floor (category 4, 224 x 74, at y = 70) pictures
chosen from the party cell's legend entry (ceiling id 0 becomes 1 unless facing north/south), each re-shaded in seven horizontal
bands by the lighting gradient (ceiling bands from the top use gradient 6..0 over 10/11/10/10/9/9/3 rows, floor from the top
0..6 over 4/9/9/10/10/11/21); floor patches then ceiling patches per cell (Chapter 2 skips a cell whose id is the base id or
its even/odd partner; Chapter 3 always draws, id = legend id with the bit 0 of the party cell's); then the six rows of cells
far to near: each cell's wall (legend word 2; layer 0), a side wall when its neighbour on the centre side is open (layers 3 left /
4 right), and its floor-type feature (layer 7 or 8; Chapter 3 draws floor types <= 99 as category 2 object sprites at layer 13);
within a row the cells run left edge inward, right edge inward, centre last. The last three cells (48-50) draw the far-wall strips
beside the party (layer 6) and the feature on the party's own cell. Monsters are drawn by `viewDrawMonster` (`DrawMonsterAndUpdateAttackState`): frame `[+8]` of the record at layer `[+0xA]` (10 =
category 3, 13 = category 2), recoloured by `[+0x72]` when flag 4 is set; monsters in cells 17-48 draw after their cell, up to three in
combat on the party cell last. The damage splash (state bit 8) and the hit colour effect (record `[+0x18]`: 44 per-pixel effects -- dissolve, shade, recolour to a hue group, recolour + shade) are drawn.

Tables, 6-byte entries {x, y, ptr} indexed by cell number (x = 0: nothing there): `val11`@0 (front walls, layers 0 and 8),
`val12`@0x386 (floor patches), `val13`@0xBF2 (ceiling patches), `val14`@0x13B6 (side walls and far strips),
`ptr1`@0x3B76 (layer 7), `ptr2..7`@0x3CA8/0x3DF2/0x41D2/0x4316/0x4460/0x4858 (layers 9-14, monsters at ranges). Layers 0/7/8/9-14
point at {group list, run records}: a run record {count, run, skip} copies `run` source pixels then skips `skip`, `count` times,
per scanline; the group list {repeat, rows, skip} repeats "draw `rows` scanlines, skip `skip` source scanlines" -- so a far
cell is the near picture with columns and rows dropped (nearest-neighbour decimation, pre-computed). Layers 1/2 are polygons
{run, shift} cut from the 224-wide floor/ceiling picture; layers 3/4 draw columns downwards with per-column vertical
decimation patterns, stepping one row after each column set; layer 6 copies 113 rows x 7 pixels at (x, y) from a 14-column
strip of a 56-wide picture. Colour 0xFF is transparent; shading is `ShiftPaletteShadeClamped` (delta added to the low nibble of
a colour < 0xD0, clamped inside its 16-colour block). Exact algorithm and layer map: `viewrender.h`.

### The main screen: frame, minimap, fonts and click regions (`minimap.c`, `font.c`, `uiregions.c`)

Decoded 2026-10-04 (render with `RENDER_HUD=1 render_view ...`). The main dungeon screen is `RunDungeonGameLoop`'s
`DrawFullScreenPictureAndCacheToEMS` of **category 0 picture 1** (318 x 198 at (1, 1)): the stone frame with a black 224 x 136 viewport
at (8, 8), the minimap well at (240, 8), the four-button icon row at y 67-82, a text panel below it, the six-arrow movement pad and four
party panels along the bottom (each 32 x 32 portrait at (8 + 58n, 148) -- category 7 faces -- three 8 x 8 slot icons and a name bar).
Other full-screen pictures (category 0): 0 SmithWare logo, 2 title menu, 3 character stat sheet, 4 party roster, 5 chapter title card
(15 pictures in Chapter 2, 23 in Chapter 3), 6 castle gate, 7 automap background, 8 scroll in hand, 9 castle, 10 throne room,
11 "Tyrants of Thaine", 12 Dark Union / clue book, 13-14 stone panels. All draw at (1, 1).

The **minimap** (`BuildMinimapTileData` / `DrawMinimap`, identical in both games): a 9 x 7 grid of 8 x 8 category-9 tiles at (240, 8),
the party's cell the fifth column of the fourth row. An explored cell (flag 0x8000) shows its wall legend word 5, plus an overlay (floor
legend word 4, if nonzero, 0xFF transparent); unexplored cells show tile 0x13. Each tile is shaded by its entry of the lighting
module's 63-cell table (the party cell is entry 31). The facing arrow is category-9 picture 0/2/1/3 (north/south/east/west) at
(272, 32).

The **party panels** (`DrawPartyMemberStatusPanel`, `statuspanel.c`; layout from the `PartyPanels` region table, 8 entries per panel):
face = category 7 picture `[+0x12]` (a dead/stoned/frozen member also gets an overlay picture), three 38 x 5 bars -- HP `[+0x52]` of
`[+0x92]` (colour 0x59; 0 when dead), MP `[+0x54]` of `[+0x94]` (0xCA) and **carried load** `[+0x118]` of `[+0x56]` (0x86; the third
bar's meaning was previously unidentified) -- plus an abilities icon, a 'T' when a level-up is pending (`[+0x1E]`), three affliction
icons and a protection icon (category 9), and "DEAD" over a dead member's bars. Bar width is `3800 / (100 * max / cur)` (min 1).

The **paper dolls** (`DrawPartyMemberPortrait`, `paperdoll.c`; render with `RENDER_DOLLS=1`): on the inventory screen the four members
stand side by side, 56 pixels wide at x = 8/64/120/176, y = 8. Body = category 6 picture `[+0x14]`; equipment icons are category 8
(`item [+8]`) at the `InventoryGrid` region entries (weapon `[+0x13A]` entry 9, `[+0x13E]` x3 entries 10-12 or, with status flag
0x1000, `[+0x142]` x2 plus the held pair's icon, the open bag's or main inventory's first 8 slots entries 0-7, rings `[+0x14A]` as 8 x 8
pictures at entries 13-14); worn clothing `[+0x152..+0x15A]` is drawn on the body as category 7 pictures numbered by **word 2 of the
item's wearable entry** (+1 for a non-male wearer) -- the field earlier documented as "break replacement item" doubles as the
worn/ring picture id.

The **monster panels** (`DrawMonsterInfoPanel`, `monsterpanel.c`): three at x = 241, y = 87/123/159 -- the two name lines, then, by
the party's reveal tier (55 / 75 / 80; Chapter 3's first threshold is 60), a 45 x 8 health bar, three status icons and a one-line
detail (POISONED/DISEASED, PARALYZED/FROZEN, HEXED/CURSED or HEALTH: cur/max, chosen by which quality bit 0x200/0x100/0x80/0x40 of
the monster's state a spell set; the bits are cleared after one showing).

The **character sheet** (`DrawCharacterSheetPanel`/`DrawCharacterStatSheet`, `statsheet.c`; `RENDER_SHEET=n`): full-screen picture category 0 / 3 (it
carries all labels), title at (107, 6), face (category 7 `[+0x12]`) at (116, 19), paper doll at (116, 60), name (156, 26), class and level
(156 / 256, 38), then every number in its slot: attributes `[+0x3C..+0x46]`, `[+0x4C..+0x50]`, HP/MP, comma-grouped experience, skills
`[+0x58..+0x66]` and the role skills `[+0x68..+0x70]` (colour 0xCB for the character holding the role; Chapter 3 has no 5th/Chemistry);
a value above its natural maximum (`+0x40`) is drawn in 0x8A; a level-1 character gets POOR/AVERAGE/GOOD/GREAT from the attribute
average.

The **shop item grid** (`DrawShopItemSlotGrid`, `shopgrid.c`): the stock of the shop is eight (item id, extra) slots shown as category 8 icons in the
`CatalogSlots` boxes (two rows of four at x = 241 + 18k, y = 160 / 179) over a colour-4 background, or the word EMPTY at (259, 179).

The **character-creation steps** (`charcreate.c`; `RENDER_PICK=class|portrait|items|roll`): the functions earlier named `ShowCharacterSkills` and
`ShowCharacterEquipment` are the class and portrait pickers (over the character-sheet backdrop, category 0 / 3). Classes 1-9 in three groups with
hotkeys F M R O A P G D K; nine portraits per gender as a 3 x 3 grid of category 7 faces, portrait k giving body `[+0x14] = 2(k-1)` (+1 female)
and face `[+0x12] = [+0x14] + 0x13`. The item pick (`ShowCharacterInventory`, also misnamed) lists up to eight items under "TAKE UP TO FOUR /
ITEMS" (0x8A at (8, 25) and (8, 31)): category 8 icon at (8, 42 + 16 row), label (name field 1 + space + name field 2, trailing spaces trimmed) at
(25, y + 5); "NAME CHARACTER" (N highlighted) at (8, 176) unless the screen is reused for viewing a hero, and `DrawQuitOrReturnLabel` at (8, 185):
`QUIT "CREATE"` (Q highlighted) or `RETURN` (E highlighted). Highlight colour 0x7B, text 0xF. String addresses: Chapter 2 DS:0x7A28 headers,
0x79E5 / 0x7A84 exit labels; Chapter 3 0x7D5A, 0x7D17 / 0x7DB6 (`dump_creation_item_strings.py`). The roll screen (`ShowCharacterStats`) is the new
hero's character sheet with "SELECT AN" / "OPTION" (0x8A at (8, 25) and (8, 31); DS:0x7A11, Chapter 3 0x7D43), "ROLL ATTRIBUTES" (R highlighted) at
(8, 51), "PICK ITEMS" (I highlighted) at (8, 69) (0x8572 / 0x8899) and the `QUIT "CREATE"` label; R rolls again, I accepts, Q quits creation.
The summary menu (`ShowCharacterSummary`, the hub after the steps) lists KEEP CHARACTER (K, y 51), CLASS (C, 69), PORTRAIT (P, 78),
ROLL ATTRIBUTES (R, 87), PICK ITEMS (I, 96), NAME CHARACTER (N, 105) at x = 8 under the same "SELECT AN / OPTION" header; K applies the secondary-class
tier flags and writes the hero out, Q discards. The name prompt (`EditCharacterName`) shows "ENTER THE NAME" and `EditTextField` (`textfield.c`):
a 13-byte buffer that really holds 12 characters (the 13th is typed into the terminator's slot, beeps and is lost), '-' as the cursor,
backspace (beep on empty), Enter / Escape; the name is trimmed of trailing spaces and asked again if empty. Strings (Chapter 2 / 3): 0x7A4D / 0x7D7F
"ENTER THE NAME", 0x7A5C / 0x7D8E "KEEP CHARACTER", 0x7A22 / 0x7D54 "CLASS", 0x853C / 0x8863 "PORTRAIT".

The **pause dialog** (`RunGameDialog`, `gamedialog.c`; `RENDER_DIALOG=<ui flags>`): the panel is PICTURES category 1 id 0 at (24, 23) with every label already
baked into the picture; an option that is not enabled is overwritten with its label in the dull colour 6. Click regions 1-6 are the save-slot rows, 7-14 the
options (SAVE 7, LOAD 8, NEW GAME 9, DOS 10, MUSIC 11, SOUND FX 12, ANIMATION 13, RETURN 14; hotkeys S L N D M F A R / Escape). A UI flag bit *set* means
enabled (0x80 save, 0x40 load, 0x20 new game, 0x10 DOS, 0x08 animation, 0x04 return); music / sound need driver flags 1 / 4 (devices present) and toggle
flags 2 / 8, shown as check boxes (category 9 picture 0x12 ticked / 0x11 clear) at (97, 106) and (170, 106). The animation option cycles the speed
1 (FAST) -> 9 (SLOW) -> 5 (MEDIUM) -> 1 and shows the name at (97, 119). Strings: 0x78F2... (Chapter 3 0x7C20...; `dump_game_dialog_strings.py`).

The **title menu** (`RunTitleScreen`, `titlemenu.c`): PICTURES category 0 picture 2 at (1, 1) with five commands baked in (regions 1-5 = C party roster,
A world map, E enter the game, R intro picture, I create a hero; Return toggles music, Ctrl+S sound effects, Ctrl+Q quits). E beeps unless a hero exists
(sum of the four party slot assignments non-zero). Chapter 3 adds an idle attract: 75 periodic ticks without input run its intro sequence. The Chapter 2
boot sequence (`PlayTitleScreenSequence`) fades in category 0 picture 0, plays music track 3 and waits 216 ticks or Escape.

The **clue book item page** (`ShowClueBookItemDetail` and its rows, `clueitem.c`; `RENDER_ITEMPAGE=<item id>`): colour-0 screen with category 0 picture 13, the item name
at (6, 4) in 0xD, the navigation bar, the item icon at (68, 41), BASE VALUE / WEIGHT (one decimal) / FITS IN- rows, then armor (ABSORPTION-, PROTECTIONS:, ADDS:
lists built from the effect pairs: field offsets 0x20-0x30 are protections, 0x7C and up the 27 attribute / skill names), weapon (DAMAGE:, SKILL:, 2-HANDED:),
healing (Chapter 3 only the magic variant) and duration rows. The page text is read from the executable (`exedata.c`, addresses in `clueitem.c`;
`dump_clue_item_strings.py`). The sub-icon selector row's table at DS:0x6976 (Chapter 3 0x6CA4) is empty in the file and filled at run time.

The **clue book monster page** (`ShowClueBookMonsterDetail`, `cluemonster.c`; `RENDER_MONPAGE=<monster type id>`): the same backdrop as the item page (Chapter 2 picture 13, Chapter 3
picture 6) with the monster's name and the heading MONSTER STATISTICS; 23 rows with labels at fixed per-game positions: four loot rows (EXPERIENCE `+0x8A`, GOLD `+0x7E`,
the ore `+0x86`, NUORE `+0x82`), seven u16 stats (`+0x50 0x54 0x56 0x58 0x5A 0x64 0x66`), ten immunity rows (bits of `+0x96`) and two resistance rows (`+0x98`). The category headings
of every clue page sit at the top right (`clueHeadingX`; Chapter 3 right aligns to x = 313). Not drawn: the animated sprite and the attack-effects line.

The **alchemy status panel** (`DrawAlchemyStatusPanel`, `alchemyStatusPanelDraw`): in the text panel (cleared first) the character name over a blank filler at (241, 87), MAGIC: at y 96
(0xCA, 0xCC when MP is above the maximum), current/max MP in 0xF at y 102, then the ore labels (0x8A at y 114 and 132) each with its counter below in 0xF. Chapter 3 shows only NUORE.

The **clue book transportation page** (`cluetransport.c`; `RENDER_TRANSPORT=1`): the mounts are 26-byte records in the executable's data segment (Chapter 2 `DS:0x77C6`, Chapter 3 `0x7AF4`:
name 12 + NUL, price Bcd4 at `+0xE`, uses at `+0x16`, mask | time word at `+0x18`; PEGASUS 10,000 / GIANT EAGLE 30,000 / FLYING RUG 50,000 / MAGIC DRAGON 70,000). The page lists
three of them (not the rug) with VALUE, USES and TIME; the time reads ANYTIME when the time word has bit 2, else "BETWEEN 7P.M. AND 7A.M." (so only the dragon).

The **local area map** (`ShowLocalAreaMap`, the M key; `localmap.c`; `RENDER_LOCALMAP=1`): the world is cut into blocks of 40 x 24 cells, 20 blocks per row; the party's
block is shown full screen as 8 x 8 tiles (category 9, the minimap's pictures) from y = 8, unexplored cells as the blank tile 0x13, the party as the compass arrow. The clue book's
map pages (F1) use the same drawing for any block.

**Sound events** (`TriggerSoundEvent`, yendor2.asm:43714): the argument is simply the 1-based effect id of WORLD.DAT's VOC table (`audio.h`; `LookupSoundEffectBlockOffset(id)`).
With no digital sound device only id 3 does anything (a PC speaker beep). Call-site census of the ids with a literal argument (Chapter 2): **1** opening
a screen or dialog (pause dialog, inventory, alchemy, rest, portrait click, clue book selection); **2** opening the member detail screen; **3** the error / refusal beep
(`FlashStatusWarning`, `EditTextField` overflow, locked door, search failure, title screen); **4** a confirm / click (toggles in the pause dialog, travel, placing a held item);
**5** and **17-27** the character creation steps; **6** moving an item between slots; **7** spending or collecting money and ore; **8** `HandleMovementInput`; **9** a status
tick and the cursor; **10** being hit (monster attacks, trap effects); **11** the alchemy screen; **12** damage effects; **19** monster spawn and the party wipe.
Higher ids (42-59, 71-72, 100, 157 ...) are the item-effect sounds `ApplyEncodedItemEffect` picks per effect. The census script is a few lines of Python over the asm
(track `mov ax, N` before each call).

**Fonts** (`writeChar`, `font.c`): 6 x 6 glyphs, 6 bytes each, indexed by character - 0x20, four fonts selected by `fontOffset` (0/2/4/6);
font 0 is the text face, 1-3 the unreadable-script faces; Chapter 3's ':' and ';' are thinner. The pen advances 6.

**Click regions** (`HitTestRegionTable`, 10-byte entries xMin, xMax, yMin, yMax, result; `uiregions.c` has all 24 tables of both games):
main screen viewport (8,8)-(231,143) = 1, minimap well = 2, icon row = 3, right-hand monster panels = 4, lower panel = 5, portrait
strip = 6; the party-panel table gives each panel's portrait (codes 1, 11, 21, 31), six slot squares and name bar.

### Ambient lighting (`ComputeAmbientLightingTable`, `lighting.c`)

The 7-entry shade-delta gradient (`word_328E6`-`328F2`, previously "where the gradient values get computed isn't
traced") is built from a base level -- a fixed cave/indoor gradient selected by a place flag, else the time of day from a
37-entry table (dark -10..-4 at night, 0 by day; Chapter 2 adds +1 at midday; a clock past minute 1443 is reset to 0) --
then lifted by the party's light sources (a tier chosen from the carried candle/torch/lantern bits, the spell timers, or a
wall torch in the 3x3 cells around the party facing the right way; the adjustment table has 7 rows x 6 tiers and only ever
raises a NEGATIVE delta, capping at 0). A second step expands the gradient into the 63-cell (7 x 9) viewport shade table
(centre of the far row = the last delta, a diamond of nearer bands around it). Chapter 3 splits the flag word in two
(place/travel flags vs light flags), forces noon while travel flag 0x4000 is set and drops one cave gradient.
Tables dumped by `dump_lighting_tables.py` (both games).

### The party roster screen (`ShowWorldMap`)

`ShowWorldMap` (the world-map screen, called from `RunTitleScreen`)
does more than draw the map: it also iterates the full 9-slot
`g_partyRecords` array (base `0x95F3`, stride `0x1F4`) and, for each
occupied slot (`+0x16 != 0` — the already-documented level/skill
field, used here purely as an "is there a character here" check),
draws a roster row via `DrawPartyRosterEntry` (was `sub_2BFBC`): an
icon at `[+0x12]` (a field not otherwise identified yet), the
character's name (`+0x0`), and — via the newly-named
`GetClassNameString` (was `sub_19768`) — the character's class name as
text. **Correction**: this loop previously carried a comment guessing
it placed "up to 9 small markers... at per-location positions,
skipping locations not flagged discovered" — that reading doesn't
survive `DrawPartyRosterEntry` showing a name+class label is drawn for
each row; these are player characters, not towns.

Digit keys `1`-`9` select a roster slot by index (recomputing
`g_currentPartyRecord` the same way `SelectPartyRecordById` does) and call
`RunCharacterDetailOverlay` (was `sub_23C18`, since traced — the
roster screen's character-detail popup) to open a detail/interaction
screen. A second, differently-routed key range reaches the same slot
math but first tests a flag (`+0x15C` bit `0x800`); when set, it
clears the bit and removes the slot's index from two small lookup
tables (`0x95EB`, 5 slots at `0x94A3`) before falling through —
plausibly a recruit/dismiss mechanic (adding/removing a character
from the active adventuring group), but not confirmed. The same
5-slot table `0x94A3` is independently reached from
`RunPartyMemberDetailScreen` (was `sub_19553`, the interactive
party-member info screen entered by F1-F4/portrait-click from the
main dungeon loop): while viewing one of the 4 active members, the
player can toggle that character's id into/out of one of the 5
`0x94A3` slots via a dedicated hit-test region — consistent with, but
not conclusively tying together, the recruit/dismiss reading above.
Separately, `ShowWorldMap`'s exit path (`D` key or an
equivalent mouse click, both leading straight to a `retf`) calls
`CompactPartyRosterSlots` (was `sub_2BF3C`) as a cleanup-on-exit step:
it cascades non-empty roster entries down to fill gaps, across not
just the 4 active slots but **3 more "reserve" slots**
(`g_partyReserveSlot1`/`g_partyReserveSlot2`/`g_partyReserveSlot3`) —
confirming the roster extends beyond the 4 active party members into
at least 3 reserve slots.

### Character creation wizard (`RunCharacterCreation`)

`RunCharacterCreation` (called from `InitGame` and from
`RunTitleScreen`'s `I` key, per its own pre-existing comment) is a
3-step wizard, each step ESC-cancelable: `ComposeCharacterPortrait`
(step 1), `PlayCharacterCreationIntroAnimation` (was `sub_15429`, step
2 — **correction**: not a wizard step at all, but an opening animated
sequence run before the wizard's actual steps: builds a palette fade
buffer — see the `ShowIntroPicture`/palette section below for the
matching transform — plays music, and runs several staged
sub-animations built from a small moving wipe-effect primitive,
`SetWipeEffectPixel`/`RestoreWipeEffectPixel` (was `sub_1619F`/
`sub_1618E`, using `ComputeVgaOffsetFromRowCol`'s `row*320+col`
mode-13h offset math), plus a multi-step palette fade,
`RunPaletteFadeSequence` (was `sub_160D6`, a sibling of
`FadePaletteStep` sharing the same `0x4D5C`/`0x442A`/`0x475A`
current/target/output buffer trio), each ESC-abortable),
`RunCharacterCreationSelectionStep` (was `sub_1559A`, step 3 — a
large 2218-byte function, partially traced: continues the intro
animation through several more ESC-cancelable staged loops with
palette fades between them, then draws a shadowed 5-item text list —
consistent with presenting the class-selection menu, though not
fully disentangled) — it also calls `SetPaletteToWhite`, was
`sub_16244`, a full-palette white flash, with a byte-for-byte
duplicate `SetPaletteToWhiteAlt`, was `sub_11EBE`, called from
`PlayStudioCreditsIntro` (the game's separate studio/publisher
credits cinematic, `start`'s own intro sequence, since named)), then
always `FinalizeCharacterCreation` (was
`sub_15267`, runs regardless of which step was reached). Matches the
manual/string-survey's `CHARACTER CREATION`/`PICK A CLASS`/`MALE`/
`FEMALE`/`PICK A PORTRAIT` cluster. Step 3
(`RunCharacterCreationSelectionStep`) calls
`DrawShadowedText` (was `sub_161D0`) — a drop-shadow text/list draw
(background-color pass, then a foreground-color pass shifted 1 pixel
up-left), with a byte-for-byte identical duplicate,
`DrawShadowedTextAlt` (was `sub_11E4A`), used elsewhere by
`PlayStudioCreditsIntro`.
`FinalizeCharacterCreation` loads a
transition palette, reads file entry `#3`, frees a temp memory block if
one was allocated, clears the screen, and stops the
character-creation music before returning — the wizard's common
cleanup/exit path. The entry point *into* this wizard from the party
roster screen is `ShowCreateCharacterPrompt` (see above), which first
finds and wipes an empty `g_partyRecords` slot.

### Character stat generation (`RollCharacterAttributes`, `ComputeDerivedCharacterStats`; `chargen.c`)

Fully decoded and reimplemented; the formulas and class tables are in `chargen.h` and `chargen.c` (the
tables were generated from the disassembly and cross-checked against an emulation of it, 759 derived
values and 36 roll vectors over both games and all classes). Highlights, correcting earlier notes:
the six rolls (45-60) are drawn in the order **Strength, Dexterity, Intelligence, Wisdom, Charisma,
Stamina**; HP = 25 % of Stamina; carry capacity = 10 x Strength; MP = a class-dependent base / 4 and
the CASTING skill (`+0x62`) = base + a class bonus (MONK, ALCHEMIST, DRUID, MAGE use Intelligence or Wisdom
blends; classes 1-3 have no magic); the equipment baselines `+0x32`/`+0x34` (and their maxima) that
`party.h` flagged as "nothing writes them" are zeroed here. The derived skills `+0x58`-`+0x70` (Survival,
Projectile, Slashing, Bashing, Polearm, Mapping, Navigation, Bartering, Repair, Thievery, Linguistics,
Chemistry) are each a blend of attributes plus a class bonus, or for some classes a flat 0 or 40. Chapter 3
retunes most class bonuses and flat values and never computes Chemistry. The scaling helper
(`ScaleByPercentRounded`) does its multiply and +50 in 16 bits, so it wraps for large inputs.

### Character creation flow and the new-game template (`chargen.c`, `newgame.c`)

The roster's **"ShowCharacterSkills" is the class-selection screen** (keys F/M/R/O/A/P/D/K and one more
pick class base 1-9): entering it clears the six secondary-class bits (low six of `+0x1C`) and the ability flag
bank (`+0xCA`, 16 words); choosing a class sets `+0xE` = class, `+0x16` (level) = 1 and, during creation, rolls
the attributes, derives the skills and recomputes the equipment ratings; the summary screen's R key repeats
that roll. Accepting the summary runs `ApplySecondaryClassTierFlags`: for a character with magic points, the
class's one or two starting ability flags (MONK 1,3; ALCHEMIST 1,2; PALADIN 1; MAGE 2,3; DRUID 1,2;
MARKSMAN 2 -- the same in both games) and, in Chapter 2 only, the secondary-class status bit.

**New game** (`InitializeNewGameWorldState`): the starting `CURGAME` section 0 (500-byte game-state block + nine
500-byte party records, 5000 bytes) is a template at the END of `WORLD.DAT` (Chapter 2: offset `0x1ACCED`, the
last 5000 bytes; Chapter 3: `0x41D72F`). It carries the opening position (Chapter 2: (166, 36) facing west,
4 Nov 546, 07:00; Chapter 3: (460, 46) facing north, 20 Mar 547, 09:00) and **four ready-made heroes in roster
slots 6-9** (SQUIRE, DIANA, YENDOR, JOSEPHINE; Chapter 3's template names its header "PRE-CREATED PARTY" and
lists them as the active party, which the initializer then empties). The initializer also clears bit `0x0800` of
every record's UI flags (`+0x15C`) and zeroes the remaining sections.

### The on-line clue book (F8)

`ShowClueBook` (the manual's "F8 On-line clue book") opens by calling
`SaveClueBookBackgroundToEMS` (was `sub_150B8`) to back up the current
VGA screen, and closes by calling
`RestoreClueBookBackgroundFromEMS` (was `sub_14DFC`) — a full-screen
restore from its own dedicated EMS page (`0x5616`, distinct from the
`0x55D8` page the portrait/dungeon-screen cluster uses) — bringing back
whatever was on screen before the book opened. It drives an
interactive, categorized clue-entry browser: `RunClueEntryMenu` (the
per-category menu loop) → `ShowClueCategoryEntries` (init+draw one
category, reading its entry count from a table at `0xF3F4` indexed by
`g_clueBookCategory`, the category selector) → `DrawClueEntryList` (the
scrollable entry list itself, positions from a table at `0x68D2`) →
`BuildClueEntryText` (composes one entry's display text, dispatching
on `g_clueBookCategory` to different lookups per category) → for at least
category 1, `BuildClueLocationSuffix` (reads a clue record from
`WORLD.DAT` and appends ` LEVEL X` or ` MAP X` when the clue is tied to
a specific level/map, else no suffix). **Correction**: this whole
chain was first named as a save-game slot-selection menu — wrong; all
13 call sites into it trace to `ShowClueBook` alone, confirmed by that
function's own pre-existing comment citing the manual, with no other
caller anywhere.

For the monster-statistics category specifically,
`LoadClueBookMonsterEntry` reads `WORLD.DAT` block `0x32` ("MONSTER
STATISTICS", per its own pre-existing comment) into a fresh buffer, and
`BuildMonsterDisplayName` (was `sub_14B85`, called from
`BuildClueEntryText` and `ShowClueBookMonsterDetail`) uses the same
`WorldDat_setBlock5`/`FileEntry_Read(errorCode=9)` pattern to build a
single space-joined display string from two `WORLD.DAT`-sourced text
fields — plausibly a monster's name and its type/category label, though
the exact field semantics aren't independently confirmed. The item-detail
sibling, `BuildItemDisplayName` (was `sub_14B24`, called from
`BuildClueEntryText` and `ShowClueBookItemDetail`), does the same thing
for items: `LoadItemCatalogRecord` then join 3 text fields (`+0x13`,
`+0x20`, `+0x2D`) with the same single-space separator — per-field
semantics (name/material/type?) likewise not confirmed.

Some clue entries are **registration-locked**: `RunClueEntryMenu` shows
`ShowClueBookRegistrationNag` ("REGISTER YOUR COPY OF THE CLUE BOOK
TODAY!") instead of an entry's detail when the global "registered"
flag (`g_uiScratchFlags4` bit 1) is clear and that entry's own flag
(`[+2]` bit `0x8000`) marks it as requiring registration — a shareware
limitation.

`HandleClueEntryScrollInput` (was `sub_12DD8`) is `RunClueEntryMenu`'s
entry-list scroll handler — the same "I"/"Q" hotkey convention as
`HandlePagedEntryNavigation` elsewhere, plus a mouse hit-test
(`HitTestRegionTable` table `0x6960`) as an alternate input, updating
current index `g_clueEntrySelectedIndex` from candidate `g_clueEntryScrollOffset` ('I') or
`g_clueEntryPageUpperBound` ('Q') and signaling which via `errorCode`
(1/2/0=unchanged) — unless a `g_clueBookNavFlags` bit (`0x100`/`0x80`) defers
to a full-page jump instead: `ScrollClueEntryListPageUp`/
`ScrollClueEntryListPageDown` (was `sub_13014`/`sub_12FED`, also
shared with `HandleClueEntryRowScrollInput` below), which move
`g_clueEntryScrollOffset` by a fixed page size of `0x38` (56) entries/rows, clamped
against `g_clueEntryLowerBound+2`/`g_clueEntryPageUpperBound` respectively, then call
`RecomputeClueEntryPageBounds` (was `sub_12FC1` — **correction**: not
a redraw as first speculated; it's pure bounds arithmetic, recomputing
`g_clueEntryPageUpperBound`/`g_clueEntryVisibleRowCount` from the new `g_clueEntryScrollOffset`, with a `/4` in
its last-partial-page math confirming a 4-entries-per-row grid). The
list's other scroll input, `HandleClueEntryRowScrollInput` (was
`sub_12D5C`), handles 'H'/'P' for a single-row (step 4) scroll,
falling through to the same two page-jump functions only at the
current page's boundary — together these five functions cover the
entire clue-entry-list scrolling mechanism end to end.

`HandleClueCategorySelection` (was `sub_14D26`) is `RunClueEntryMenu`'s
category-switching input handler: keyboard (`ESC`/digit keys, plus
`K`/`P` hotkeys gated on the same `g_clueBookNavFlags` `0x40`/`0x20` bits
`DrawClueBookNavBar` uses for its "d) LIST"/"c) MAP" hints — not the
registration lock, a different flag) and mouse (region table `0x6876`,
categories 1-9) both funnel into a shared "apply new category" block
that walks a 7-bit category mask in `g_clueBookNavFlags` and plays a sound cue
on change.

`RunClueEntryMenu` also calls `DrawClueBookNavBar` (twice) to draw the
book's top bar: conditional "d) LIST" / "c) MAP" hotkey hints
(`g_clueBookNavFlags` bits `0x40`/`0x20`), then a row of 7 category-tab icons
(fixed base picture ids, each swapped to a highlighted +1 variant when
its bit in `g_clueBookNavFlags`, `0x8000` down to `0x200`, is set).

`ShowClueBook` also calls `ShowClueBookHelpScreen` (bound to TAB,
per its own title text), which lists the clue book's categories: F1
Maps (world/towns/mines), F2 Monster Statistics, F3 Spells, F4 Magic
Users (spells by class), F5 Inventory Items, F6 Complete Walk Through,
ESC Return to Game — very likely (order not yet matched bit-for-bit)
the identities of (at least 6 of) `DrawClueBookNavBar`'s 7 tabs.

**`ShowClueBook`'s full F-key dispatch**, traced directly from its own
`g_currentCommandCode` (`PollKeyboardInput` result) switch: F1 (`g_clueBookCategory=1`,
Maps) → `RunClueEntryMenu` + `RunClueBookMapCategory` (loads the map
via `LoadClueBookMapEntry`, draws a row/col grid of per-cell location
labels via `DrawClueBookMapGrid`, and dispatches cell clicks to an
untraced `sub_14122`). F2
(`g_clueBookCategory=2`, Monster Statistics) → `RunClueEntryMenu` +
`RunClueBookMonsterCategory`. F3 (`g_clueBookCategory=3`, Spells) →
`RunClueEntryMenu` + `RunClueBookSpellCategory`, which (along with
`BuildClueEntryText`) reads each spell's data via `LoadClueBookSpellEntry`
(was `sub_1D198`) — an 80-byte record from its own dedicated EMS page
(`0x5610`), the spell-data equivalent of `LoadClueBookMonsterEntry`'s
`WORLD.DAT` read. F4
(`g_clueBookCategory=4` lists classes, then `g_clueBookCategory=[selected class]+4`,
Magic Users) → two chained `RunClueEntryMenu` calls (class picker,
then that class's spell list) + `RunClueBookSpellCategory` again —
`ShowClueBookSpellDetail` draws "CLASS:"/"LEVEL:" plus "MP:"/
"NUORE:"/"ORE:" cost fields (spells cost MP and the same two alchemy
ore counters used elsewhere) and "AFFECTS:"/"WHEN:"/"EFFECT:"
description sections with a 6-class eligibility marker row. Each cost
field row is drawn via `DrawLabeledNumberRow` (was `sub_13C86`) — a
generic "label, formatted number, label again, next line" primitive.
The "LEVEL:" field itself is resolved by `DrawSpellLevelForCurrentClass`
(was `sub_13C4B`): searches a 20-level × 2-class-slot table for a
match against the current class id (`g_clueBookClassId`) and draws the
matched level. Each cell of the eligibility row is drawn by
`DrawClassEligibilityMarker` (was `sub_13C1D`): a fixed value of `1`
in a highlight color when a 2-entry candidate array matches the same
class id. F5
(`g_clueBookCategory=0xB`,
Inventory Items) → `RunClueEntryMenu` lists **8 item subtypes**
(`g_clueEntrySelectedIndex[0]` 1–8), each with its own sub-loop and now fully
identified by title dump: **1 "ARMOR/RINGS"** →
`RunClueBookItemCategory` (**correction**: previously described below
as "the F5 category's own loop" — it's actually only item subtype 1's
loop within F5's subtype selector); **2** → just `WaitForKeypress`
(an empty/placeholder subtype, no title); **3 "JEWELS/ARTIFACTS/
UNIQUE ITEMS"**, **4 "MAGIC SCROLLS/QUARTZ"**, **5 "POTIONS"**, **6
"SUPPLIES/FOOD"** (`g_clueBookCategory=0xD/0xE/0xF/0x10`) → all four route
through `RunClueBookItemDetailWithAbilityInfo` (**correction**: has 4
call sites here, not "two other sites" as first counted); **7
"TRANSPORTATIONS"** →
`RunClueBookTransportCategory` (PEGASUS/GIANT EAGLE/MAGIC DRAGON —
ties to `IsItemRangeAvailable`'s "boat/horse-style transport gate" and
to `ShowTransportUsagePreview`, `ShowItemUsagePreview`'s preview for
actually using one of these mount items: name, cost, and a
flight-time-restriction line, e.g. "CAN FLY ANYTIME DAY OR NIGHT"; the
clue-book detail row itself, `DrawTransportDetailRow`, shows "VALUE:",
"USES:", and "TIME:" — "BETWEEN 7P.M. AND 7A.M." or "ANYTIME");
`ShowItemUsagePreview` and `FinishItemUse` both call
`BuildItemUseMessage`, which builds the confirmation/preview text for
using an item — either a generic message by tier, or item-specific
message entries copied from an EMS-backed segment. For special items
it first calls `CheckAndPaySpecialItemCost`: pays gold, NUORE, or
MAGIC ORE (selected by a tag on the item), or checks/consumes a
specific inventory item otherwise; `UseItem` and `FinishItemUse` also
call `DrawEligibleItemList` (was `sub_1B8EE`), which iterates the item
catalog and draws a 2-column x 5-row list of every entry
`CheckItemEligibilityAndCopyName` (was `sub_1B818`) approves — a
category-flag match (`[+0x16]`/`[+0x18]` against `word_2E40C`/
`word_2E40E`) plus 6 prerequisite flag ids (`[+0x22..+0x2C]`, each
checked via `TestGlobalFlag`) — recording each match's catalog index
for later selection;
**8 "WEAPONS"** (`g_clueBookCategory=0x11`) → `RunClueEntryMenu` +
`RunClueBookWeaponCategory`. F6 (Complete Walk Through) →
`ShowPagedEntryScreen` (already-named, generic paginated text), whose
page-turn input is handled by `HandlePagedEntryNavigation` (was
`sub_133EB`): `I`/previous-page and `Q`/next-page keys or mouse hits
(region table `0x6960`), the latter gated past page 5 by the same
registration check `ShowClueBookRegistrationNag` guards elsewhere. ESC →
cleanup and `LoadMasterPalette` back to the normal palette (the
reverse of `PlayClueBookOpenAnimation`'s swap). `sub_13278`/
`sub_13216`/`sub_1334E`/`sub_1318D` are confirmed as clue-book category
loops by this trace but not yet individually named/traced.

`ShowClueBookItemDetail` is confirmed as item subtype 1's per-entry
screen (F5): an item's icon plus "BASE VALUE:" and "WEIGHT:" fields,
each drawn via `DrawLabeledBCDIfNonzero` — a small reused helper
(shared with `ShowClueBookMonsterDetail`'s stat fields) that draws a
label then the BCD4 value only if it's nonzero.
`DrawRecordFieldBCDIfNonzero` is its sibling, taking a record pointer
instead of a direct value pointer.
The F2 "MONSTER STATISTICS" category follows the same pattern (and is
plausibly preloaded wholesale at startup: `PreloadMonsterStatsTable`,
was `sub_124EC`, called once from `InitGame`, allocates a large
~18.4KB block and reads into it with the same `errorCode=9`
`LoadClueBookMonsterEntry` uses for this data — immediately preceded in
`InitGame` by an identically-shaped sibling, `PreloadWorldDataTable`
(was `sub_12449`, ~25.9KB, via a different resource-setup stub whose
`WORLD.DAT` block isn't confirmed)):
`RunClueBookMonsterCategory` (called from `ShowClueBook`) calls
`LoadClueBookMonsterEntry` once (reads `WORLD.DAT` block `0x32` for
the current entry into a fresh buffer) then loops redrawing
`ShowClueBookMonsterDetail` plus `DrawClueBookNavBar` whenever dirty,
until ESC — no region-table hit-testing, unlike the item category's
loop. `ShowClueBookMonsterDetail`'s labels (dumped from its message
table) give a full monster stat sheet: `EXPERIENCE:`, `GOLD:`,
`MAGIC ORE:`, `NUORE:` (loot — consistent with `GrantMonsterRewards`'
4 staged loot fields), `HEALTH-`, `ACCURACY-`, `DEXTERITY-`,
`ABSORPTION-`, `DAMAGE-`, `RANGED ACC.-`, `RANGED DAM.-` (combat), and
`POISON:`/`DISEASE:`/`PARALYSIS:`/`FREEZING:`/`HEXING:`/`CURSING:`/
`FIRE:`/`COLD:`/`ELECTRIC:`/`POWER:` (resistances or vulnerabilities)
— individual field offsets into the loaded record not traced yet.

`RunClueBookItemCategory` is item subtype 1's own interactive loop
(draw entry, poll input, hit-test a region table so the player can
click a sub-icon to jump entries, until ESC), adding `ShowArmorDetailRow`
("ABSORPTION-", matching `ShowClueBookMonsterDetail`'s own field) after
the generic fields; `RunClueBookWeaponCategory` (subtype 8) similarly
adds `ShowWeaponDetailRow` ("DAMAGE:" and "2-HANDED: YES/NO"); both
end with `DrawSubIconSelectorRow`, drawing the clickable sub-icon
indicator strip (region table `0x6976`, the same table
`RunClueBookItemCategory` hit-tests, toggled by `g_clueBookIconSelectionMask` bits). Both
also call `ListCompatibleClueBookItems` (was `sub_1472A`, using the
same `0x6976` table/position): checks the current entry's usability
flags, and if eligible, scans up to 9 more catalog ids re-checking the
same eligibility test and drawing each match — a filtered
compatible-items list for the category view.

**Major reference find**: `ShowArmorDetailRow` also calls two
bonus-list drawers, each iterating up to 4 `(type id, amount)` pairs
at `g_itemStatEffectTable` and printing `"+<amount> <name>"`:
`ShowArmorProtectionsList` ("PROTECTIONS:", type id `<= 0x30`, a
9-entry table — DISEASE/POISON/SICKNESS/STONING/FROZEN/PARALYZE/
CURSING/HEXING/JINXING) and `ShowArmorAttributeBonusList` ("ADDS:",
type id `>= 0x7C`, a 27-entry table at `0x7DC7` that is the
**canonical index order of the game's attribute/skill system** —
`STRENGTH`, `DEXTERITY`, `STAMINA`, `INTELLIGENCE`, `WISDOM`,
`CHARISMA`, 3 blank slots, `HIT POINTS`, `MAGIC POINTS`, a blank,
`SURVIVAL`, `PROJECTILE`, `SLASHING`, `BASHING`, `POLEARM`,
`CASTING`, `MAPPING`, `NAVIGATION`, `BARTERING`, `REPAIR`,
`THIEVERY`, `LINGUISTICS`, `CHEMISTRY`, and more beyond the 150 bytes
dumped so far — previously this skill list was only known piecemeal
from a raw string scan; this table gives its actual in-engine index
order, which future work can cross-reference against the party-record
skill array (`+0xCA`–`+0xE9`) and `ShowCharacterSkills`'s 15-entry
array. — the more complex
`RunClueBookItemDetailWithAbilityInfo` (item subtypes 3–6, 4 call
sites; simpler than `RunClueBookItemCategory` in that it has no click
navigation) adds an extra ability-info overlay when the item's id
falls in `CastSpell`'s or `RestCharacter`'s dispatch range — i.e. some
clue-book items (plausibly the "MAGIC SCROLLS/QUARTZ" subtype) grant a
spell/ability when used, and the clue book shows what it does via
`ShowHealingItemPercentInfo` (for the `RestCharacter` range —
"HEALTH-"/"MAGIC-" plus a percentage, e.g. "HEALTH- 25 PERCENT",
matching `UseHealingItem`'s own HP/MP flag convention),
`ShowItemEffectDuration` ("DURATION- `<n>` MINUTES") and
`ShowItemAbilityEffectInfo` (a percent-chance or effect-amount line —
confirmed to use the *exact same* damage constants as
`ResolveAbilityEffect`'s own dispatch, i.e. it shows the real combat
numbers). Both use `DrawLabeledNumberIfNonzero`, the plain-integer
sibling of `DrawLabeledBCDIfNonzero`.

**Temple/healer paid services**: `UseHealingItem` (4 sites),
`UseItemType_400`, and `UseTrainingItem` all call
`ShowHealingCostPrompt` to compute and display a gold cost for a
specific service — "IT WILL COST `<total>` GOLD TO REPLENISH YOUR
HEALTH POINTS." / "...TO REMOVE YOUR CONDITIONS." / "...TO RETURN YOU
TO LIFE." / "...TO COMPLETELY RESTORE YOU." (or a dynamically-built
message for the other two callers) followed by "IS THAT PRICE
AGREEABLE?" — drawn via `DrawIndentedTextColumn` (was `sub_28A76`), a
word-wrapped text-column mode dispatcher with a caller-selectable
hanging-indent style. Its per-word helper `DrawWordToken` (was
`sub_28B94`) shows the underlying text buffers are pre-wrapped into
lines at load/format time, with NUL bytes marking line boundaries —
these functions consume that pre-wrapping rather than computing
word wrap from on-screen width themselves. All 6 traced call sites `retf` immediately after the
call — none poll Y/N or deduct gold there, so the actual confirm+pay
step (if it exists) happens on some later, separate re-entry not yet
found. The total is a per-unit base cost (varies by call site,
sometimes built from `word_2E40C` condition bits) times an
item-catalog quantity field (`0xBCE+0x18`), added in a loop running
once per `[g_currentPartyRecord+0x16]` — plausibly once per afflicted/eligible
party member. The "REMOVE YOUR CONDITIONS" base cost itself is
computed by `ComputeAfflictionHealingCost` (was `sub_1BA35`, called
twice from `UseHealingItem`): a flat per-affliction price summed over
the confirmed `+0x1C` bitfield (SICK `+5` through STONED `+60`).

A 4th shop-mode bit, `g_uiScratchFlags2` `0x200`, gates a mouse-click-driven
shop purchase path: `HandleShopCatalogSlotClick` (was `sub_17032`, a
catalog-click handler reached from the main input loop via a hit-test
against region table `0x5AC0` — its own click gate is
`HitTestCatalogSlot`, region table `0x63C8` plus an 8-entry exclusion
check) calls
`PayGoldAndAcquireItem` for its "quick buy" branch, and a sibling
branch, `SellClickedCatalogItem`, credits gold back for the clicked
item instead — the click counterpart to `TrySellItemForGold` — pays
`g_partyGold` against a price at `0xB30`
(`CompareBCD4`/`SubBCD4`, bailing if unaffordable, with a special-case
`ShowResourceDepletedOverlay` when the purchase exactly drains gold to
zero) and stages the acquired item the same way
`TrySellItemForGold`/`TryEnhanceItemForGold`/`TryRepairItemForGold`
stage theirs.

Both `PayGoldAndAcquireItem` and `SellClickedCatalogItem` open with a
call to `ComputeBarterPricingPreview` (was `sub_1CCBC`), which computes
a tiered discount/markup percentage from a party record field, `+0x68`
(not otherwise identified — plausibly the **BARTERING** skill, given
the shop context and the already-found attribute/skill list that
includes it) via 7 descending thresholds, then scales a price at
`0xB30` (the same `0xB30` `PayGoldAndAcquireItem` charges against
`g_partyGold`) using the newly-named `MulBCD4ByWord` (was `sub_19CA1` —
a `MulBCD4`-style sibling of the existing `ConvertWordToBCD4`/
`CompareBCD4`/`AddBCD4`/`SubBCD4` library: multiplies a packed-BCD4
value digit-by-digit by a 16-bit word). It also previews scaled enhance/
repair costs when `IsItemEligibleForEnhance`/`IsItemEligibleForRepair`
say the clicked item qualifies.

All four shop actions (sell, enhance, repair, buy) are hosted under
one umbrella screen, `RunShopScreen` (reached from `UseAbilityCommand`,
not `UseItem`): it calls the main input loop `RunPartyInventoryScreen`
(was `sub_1869D`, since named) directly (twice, enabling the Space-bar
cluster) and `HandleShopCatalogSlotClick` (was `sub_17032`, since
named — enabling the click-to-buy path, reaching
`PayGoldAndAcquireItem`), redraws
`ShowMaterialCounterHud` repeatedly, and
writes state back via `FileEntry_Write` near an exit. Its many
internal helpers aren't individually traced yet, though a few now
are: `BuildShopCategoryTabList` (was `sub_179AE`, called near the
start) sets up the screen's category tab list — an 8-entry table
gated by a
per-shop-type bitmask (`byte_32DCC`), pairing each enabled category id
with a value (fixed globals for 3 special currency-like tabs, else
pulled from the category's own catalog record); and
`TryHandleCatalogSlotClick` (was `sub_17270`, called from both
`RunShopScreen` and `RunPartyInventoryScreen`, distinct from
`HandleShopCatalogSlotClick`'s own buy handler) gates a click on an
occupied catalog slot before dispatching to `ShowItemPurchaseConfirmPrompt`
(was `sub_219FA`, since named) — a select/preview interaction
separate from the purchase flow.

A separate, likely shop/vendor "buy" `UseItem` handler,
`UseItemType_800` (was `sub_1BBED`, gated on `UseItem`'s
`word_2E410` bit `0x800` — the direct structural sibling of
`UseItemType_400`, dispatching on the identical
`SelectItemUseRecord` `[si+0x10]` bit pattern) spends `g_partyGold`
via `CompareBCD4`/`SubBCD4` against a price table at `0x512A`, gated
by inventory-capacity checks against the same `0xBCE` range table
used elsewhere, and redraws the gold readout afterward via
`RedrawPartyGoldDisplay`. Named for its role in `UseItem`'s outer
dispatch, but it has several branches (a single-item purchase path
and a quantity-loop path that repeatedly adds the unit price to both
`g_partyGold` and a second counter `0xB30`) not yet disentangled in
detail.

A specific, fully-traced instance of a quantity-purchase flow:
`PromptBuyOreQuantity` (was `sub_1AF49`, called from `UseItem`) shows
"ORE COSTS 10 GOLD PER UNIT.", the current gold balance ("GOLD COINS:"),
and "ENTER QUANTITY TO BUY", reads the quantity via
`PromptForBCD4Quantity` (was `sub_19BE6` — parses a digit string typed
through `EditTextField` backward into a packed-BCD4 value), then
validates affordability via `CompareBCD4` against `g_partyGold`. This
is the purchase flow for using an Ore-type item from the inventory —
plausibly connected to `UseItemType_800`'s quantity-loop path above,
though that link isn't confirmed.

### Music and sound effects in `WORLD.DAT` (`audio.c`)

Decoded 2026-10-04. The sound lives inside `WORLD.DAT`, found through two pairs of data-segment tables (`LookupMusicTrackBlockOffset`,
`LookupSoundEffectBlockOffset`: 32-bit file offsets plus 16-bit lengths, ids 1-based, 0 = silence): **music tracks are Creative
Music Files** (`CTMF` header + FM instrument block + MIDI-style events; played through `SBFMDRV.COM`) -- 21 in Chapter 2 (the
first at 541383, 5357 bytes) and 24 in Chapter 3 -- and **sound effects are Creative Voice Files** (`Creative Voice File\x1A`, 8-bit PCM)
-- 80 in Chapter 2 (the table has an 81st slot pointing at unrelated text) and 141 in Chapter 3. All blocks are contiguous (music,
then effects). The 2493-byte block in front of the first track (Chapter 2 offset 538890, Chapter 3 618714) is the Creative
`CT-VOICE.DRV` binary, not game data. Which music track plays where is `music.c`. Sound events (`TriggerSoundEvent`, the effect
id in `ax`; when no driver is present only event 3 beeps on the PC speaker) are fired from the UI and game code; the effect
ids by call site (Chapter 2 / Chapter 3 numbering differs, Chapter 3's effect set is larger):

| use | Chapter 2 | Chapter 3 |
|-----|-----------|-----------|
| clicks on portraits, clue-book categories, dialogs, resting | 1 | 1, 15 (portraits) |
| error / refusal (text field, warnings, failed unlock, searching) | 3 | 3, 11 |
| confirm / placing a held item, music and sound toggles | 4 | 4 |
| picking up, swapping or dropping items in inventory | 6 | 9 |
| gold, ore, loot, shopping, repair, ability scrolls | 7 | 7, 8 |
| mouse cursor / status-effect ticks | 9 | 40 |
| map trigger effects, credits | 10 | 44, 84 (story events) |
| monster spawns, party wipe | 19 | 50 (wipe), 6/10 (ranged/combat) |
| movement | -- | 13 |
| alchemy screen | 1, 3, 11, 52 | 1, 3, 44, 87 |
| character creation | 5, 17-19, 22, 24-27 | 51, 54, 59, 71, 83, 130-141 |

### Palette blocks, the dawn/dusk fade and the colour cycle (`palette.c`)

Decoded 2026-10-04. `WORLD.DAT` holds consecutive 768-byte palettes from the master palette on (Chapter 2 `0x8270A`, Chapter 3 `0x95BDA`): block 0 is
the game palette; `loadWorldDat5` also loads **block 2**, a sunrise/sunset ramp, for two effects. The dawn/dusk fade (started at 06:00 and 18:00) steps
113 times, each time writing a 32-colour window of block 2 into DAC entries 0xE0-0xFF and recomputing the ambient lighting: dawn starts at colour
0 and moves up, dusk starts at colour 111 and moves down (its last window starts one colour before the table). The colour cycle (every 5 timer ticks)
writes block 2 colours 144-159 to DAC 0xD0-0xDF in four phases (phase 0 as stored, 1-3 rotate each group of four left by 1/2/3). The algorithms and
constants are identical in both games.

### Ambient music by map region

`UpdateAmbientMusicForRegion` computes a coarse map-region index from
the party's position and, when it changes, reads that region's
`WORLD.DAT` record and plays its associated music track
(`PlayMusicTrack`) — background music changes as the party crosses
between zones. It configures that `WORLD.DAT` read via
`PrepareAmbientMusicBlockRead` (was `sub_2801A`), the same shape as
the codebase's other resource-stub helpers, keyed by global
`_blockSize5`. `g_currentMusicTrack` caches the currently-playing/
forced track id: `RefreshDungeonMapWindow` compares it against two
region-specific track ids on entry to decide whether to call
`StopMusicAndResetTimer` before a region change, and the driver-reset
path clears it alongside stopping playback — distinct from
`g_forcedMusicTrack` (the "0 = let the ambient system choose" override
flag `RunTitleScreen` and character creation toggle).

**Reimplemented as `music.c`**: the region track is a u16 per 40 x 24 map page in a WORLD.DAT table (Chapter 2 `0x71048`,
120 entries, ids 0-17; Chapter 3 `0x83DD0`, 140 entries, ids 0-23), found next to the per-page label table. A travel
destination's day and night tracks (`TravelDestination.rawA/rawB` in Chapter 2) take over via `UpdateAmbientMusic`, which
picks the forced track, else the day (07:00-19:00) or night track, if ambient music is allowed.

### The EMS page-mapping call cache

`MapUnmapPages` (the central `int 67h` LIM EMS 4.0 page-mapping
primitive, called from every `loadWorldDatN`-style resource loader)
caches its `bx` parameter — a pointer to the mapping-array descriptor —
in `g_lastEmsMappingArrayPtr`, and skips the actual EMS call entirely
if called again with the same pointer. A simple call-memoization guard
against redundant remaps when consecutive loaders want the same page
mapping.

### The map legend editor

`RunMapEditorScreen` (name pre-existing from an earlier session; not
otherwise documented) hosts a wall/floor legend editor:
`EditWallLegendTypeNumber` and `EditFloorLegendTypeNumber` are a
symmetric pair of numeric-entry fields (via `ReadTypedInteger`, was
`sub_1D146` — a generic typed-integer prompt built on `EditTextField`,
reused 7 times) storing a wall/floor type number into
`g_mapEditorWallType`/`g_mapEditorFloorType`, then
redrawing the corresponding legend row (`DrawWallTypeLegendRow`/
`DrawFloorTypeLegendRow`). One error path in the floor field falls
through into the wall field, suggesting Tab-style navigation between
the two. Each legend row also has its own animated scroll position,
`g_mapEditorWallScrollIndex`/`g_mapEditorFloorScrollIndex` — reset to 0
alongside the actual selected type, then "nudged" by 1 per tick toward
the true `g_mapEditorWallType`/`g_mapEditorFloorType` value, an easing
effect for the 17-icon scrollable strip rather than a value in its own
right.

### Mouse input: the click-position latch cluster

Read `seg073`'s INT 33h mouse-callback handler directly (the routine
`ClampDragCursorPosition` feeds into): it's a standard MS Mouse user
callback registered via function `0Ch`, where `ax` on entry is the
condition mask and `cx`/`dx` are the cursor position. This handler
converts the raw per-call motion counters (function `0Bh`, `int 33h`)
through `ClampDragCursorPosition` — which accumulates them into
`g_dragCursorX`/`g_dragCursorY` and returns the clamped result in
`cx`/`dx` — then, based on which event bit the (separately preserved)
condition mask has set, latches that clamped position into one of four
dedicated globals, one pair per button-transition type (the classic MS
Mouse mask bits: `2`=left-down, `4`=left-up, `8`=right-down,
`0x10`=right-up):
- **`g_mouseLeftDownX`/`g_mouseLeftDownY`**: the left-click position,
  read throughout the game's click/hit-test handlers (catalog-slot
  clicks, portrait drag-and-drop, map editor cell painting, and more)
  — by far the most-referenced of the four pairs, matching its role as
  the general-purpose "where did the player just left-click" position.
- **`g_mouseLeftUpX`/`g_mouseLeftUpY`**: the left-button-release
  position.
- **`g_mouseRightDownX`/`g_mouseRightDownY`**: the right-click
  position — used by `RunMapEditorScreen`'s overlay/wall-tile paint
  handler.
- **`g_mouseRightUpX`/`g_mouseRightUpY`**: the right-button-release
  position.

`g_dragCursorX`/`g_dragCursorY` (the accumulated position these are
all derived from) are themselves clamped to
`g_dragCursorMinX`/`g_dragCursorMaxX`/`g_dragCursorMinY`/
`g_dragCursorMaxY` — bounds `RunMapEditorScreen` temporarily overrides
and restores around its own editing session (the map editor evidently
needs a different valid drag region than the normal game screens).

Separate from all of the above (which record *where a click/motion
event happened*): `g_mouseCursorX`/`g_mouseCursorY` track the cursor
*sprite's* current on-screen draw position — written by the cursor-draw
routines (including a fixed default of `(0xE6,0xB4)` at one call site)
and read by `RestoreCursorBackground` to erase the cursor from its
previous position before redrawing it at the new one.

### The main pause/options dialog

`RunGameDialog` drives the pause dialog with its 8 `GameDialog_draw*`
icons (Animation, Dos, Return, Load, Music, NewGame, Save, SoundFx).
`SelectGameDialogOption` is its input handler: polls keyboard
(`'1'`-`'6'`, or `'L'`/`'S'` shortcuts in one input mode) and mouse
(region table `0x5CD0`) to pick one of the 8 options, storing the
1-based selection in `word_3291E`.

### Quest-item and party-inventory range checks

`IsItemRangeAvailable` (**correction**: named `CheckTransportAvailability`
several rounds ago on a first-seen use that looked transport-related —
too specific a guess) is actually a generic primitive: given an item-id
range (a single id if the range's min/max are equal), it first checks a
fixed 6-entry table (`0x9519`) for a direct match — the same table
`HandleStatusPanelItemSlotClick` (was `sub_271DC`, called from
`start`/`HandleDungeonInput`) lets the player drop/retrieve/swap
items into via 6 clickable slots next to the resource-counter panel,
tracing where that table's contents actually come from — then falls
back to
`FindItemInInventoryRange` (search every party member's main inventory,
recursing into open containers via `FindItemInsideContainer`) until
someone qualifies. The container recursion is a **fixed 3-level-deep**
chain, confirmed structurally identical at each level:
`FindItemInsideContainer` → `FindItemInsideContainerLevel2` (was
`sub_1CF50`) → `FindItemInsideContainerLevel3` (was `sub_1CFC8`,
terminal — it does not recurse further). Each level loads a
container's 8-slot contents and scans for an item id in range, and if a
non-matching slot's item catalog record has flag `[+0xC]` bit `0x2000`
set, recurses one level deeper into that nested container. It's reused
for at least two different purposes:
a boat/horse-style transport gate (its original use), and — found via
`CheckQuestItemsCompleted` (an item-icon-dispatch handler,
`g_currentActionId==0x2C8`) — a **quest-item-completion check**: 4 specific
item ids (`0x254`-`0x257`) are each checked for being *absent* from
every party member's inventory; if all 4 are gone, it plays a success
sound and runs an animated sequence re-checking those 4 plus a 5th
(`0x2C8`) in reverse order. Reads as "the party has used/given away all
N required quest items," a completion reward sequence — exact narrative
(which items, what they unlock) not identified.

**One of the 5 items identified**: item `0x258` — immediately adjacent
to the `0x254`-`0x257` completion range — is `UseLocationBoundPotion`
(`g_currentActionId==0x258`, another item-icon-dispatch handler): a potion
that only works at one specific map cell, confirmed by its own message
strings (`THE POTION WORKED SUCCESSFULLY` there, `YOU CAN NOT USE THAT
HERE!` elsewhere). Using it there sets global quest flag `0x48`. The
adjacency to the completion-check range (`0x254`-`0x258` consecutive,
plus `0x2C8`) strongly suggests these 5-6 items are one themed quest
item set.

**`IsItemRangeAvailable` partially reimplemented, 2026-10-01**: the
6-entry-table-then-party-inventory half (`partyFindItemInRange`/
`itemRangeAvailable`, `src23/party.c`/`.h`) -- the global table check
and the fallback 8-slot main-inventory scan across
`SaveHeaderPartySlots`, including the already-documented-elsewhere
"stops dead at the first unoccupied slot" whole-party quirk. **The
3-level container recursion described above is deliberately not
included** -- it's a genuine, separate, CURGAME-backed "ground item
container" subsystem (`FindItemInsideContainer`/`Level2`/`Level3`) this
project has no reader for yet; a slot holding a container-type item is
simply reported as not matching rather than partially modeled. This
reimplementation is what unblocks `ApplyRestEffectsToCharacter`'s own
regen-rate dependency, documented further down this file -- composing
that derivation itself (converting an item-availability count into a
percentage) is a separate, still-open step. Tests in `test_party.c`.

**Also part of this cluster**: item `0x253` (immediately
before the range) is `ShowVisionAtLocation` — saves the current view,
jumps it to a fixed coordinate (340,99) using the same redraw sequence
`ApplyMapTriggerEffect` uses for teleports, shows it briefly, then
restores the original view (the player doesn't actually move). A
scrying/vision effect revealing a fixed, presumably story-significant
location — not followed further this round, but a promising thread
into the main-quest structure.

**The rest of the cluster confirmed by message strings**: items
`0x246`-`0x249`, all gated on `TestGlobalFlag(0xB1)` (shows `PATIENCE
IS A VIRTUE.` if not yet available — some kind of daily/periodic
recharge), are powerful relic-tier effects:
- `0x246` `CollectNuoreCache` — `+5,000 NUORE` (adds to counter `0x94BB`)
- `0x247` `CollectMagicOreCache` — `+5,000 MAGIC ORE` (adds to counter `0x94B7`)
- `0x248` `PartyMassHealAndOverheal` — `2 X HEALTH` / `2 X MAGIC`:
  cures every ailment and sets the *whole party's* current HP/MP to
  **2× their max** (an overheal exceeding the normal cap), with a heal
  icon shown on each member
- `0x249` `InstantKillActiveMonster` — zeroes the active monster's HP
  directly, no roll

Together with `ShowVisionAtLocation`/`UseLocationBoundPotion`/
`CheckQuestItemsCompleted`, this reads as a themed set of quest/relic
items central to the main story — exact narrative (what they are,
where they come from) still unidentified, but now a well-scoped thread
for a future round. The recharge flag itself, `g_globalFlags` index
`0xB1`, has **no literal `SetGlobalFlag`/`ClearGlobalFlag` call**
anywhere in the disassembly — it must be flipped through
`ApplyItemEffectFlags`'s data-driven flag-index fields (an item's own
catalog data, not a code literal), so pinning down exactly what
recharges it needs `WORLD.DAT` item-data inspection rather than more
static tracing.

Before any of the three render passes runs, `RedrawDungeonScreen`
first calls `BuildDungeonViewportCells`, which builds the local
scratch cell buffer (`0x6D60`) every pass actually reads from:
computes a facing-dependent row stride/side-step (the same
`g_partyFacing` tier bits used throughout) from the current position,
then calls `CopyDungeonRowCells` 7× (matching `RenderDungeonViewport`'s
row-count pattern) to copy the visible cells out of the level's map
data. `RedrawDungeonScreen` then calls `ComputeDungeonCellVisibility`,
which identifies the origin of the `[+6]` bit-0 "hidden" flag every
render-pass function checks (`DrawDungeonCellWallTexture`,
`ExtendDungeonFloorTexture`, `ExtendDungeonCeilingTexture`, etc. all
skip a cell when it's set): walks progressively closer rows to find
the nearest wall-blocked boundary, then marks side-passage cells past
it as hidden — dungeon line-of-sight occlusion, using
`IsDungeonRowFullyBlocked` (is every cell in a given row a solid-wall
type — a dead-end/closed-wall test) to find that boundary row.

**Attacking a monster in the corridor before it's engaged in turn-based
combat**: `ApplyDamageToMapMonster` (called from `ApplyEncodedItemEffect`,
was `sub_2C0FE`, since named) applies damage to a `g_levelMonsters`-pool monster, sets
wound/display flags, redraws, then resolves death (`GrantMonsterRewards`
+ `RemoveMonsterFromMap` + `RedrawDungeonScreen`) or survival
(`RefreshDungeonScreen`) based on its HP — the corridor-encounter
counterpart to the turn-based `g_monsterSlots` combat flow documented
below. `HandleRangedOrCombatAction` also uses `SaveCorridorBackgroundToEMS`
(the mirror image of `RestoreCorridorBackgroundFromEMS` — video
buffer → EMS, caching the corridor before an animated overlay draws
over it) and `SaveActionIconPanelToEMS` (same idea for the action-icon
panel area, also used by `HighlightSelectedAbilityIcon`, which also
calls `ClearActionIconHighlightMask` (was `sub_2BBD7`) to reset the
mask/overlay buffer backing that highlight effect.

**`ApplyEncodedItemEffect` (was `sub_2C0FE`) is now named**, resolving
the last unnamed function in the binary. This ~4,200-byte function
turned out to be a flat bitmask switch (`word_33302`/`word_33306`,
~19 distinct effect types) that applies an item's or container's
coded magical effect — see the `ConsumeItemChargeResource` paragraph
above for the full mechanism. It sits behind several combat-adjacent
helpers this session named individually
(`ApplyDamageToMapMonster`, `GetMonsterAtViewportRow`,
`ScrollCorridorBackgroundFromEMS`, `AnimateEffectFrame` — one
animation frame, same shape as `AnimateProjectileStep` but a different
layer flag and wait length, for some other in-viewport effect,
and `DimDungeonViewport` (was `sub_2BC56`) — darkens the 224×136
dungeon-viewport region of the offscreen buffer by 2 palette-index
steps, plausibly one frame of a hit-flash or transition dim
sequence). Also called (twice) from it:
`DrawAnimationFrameAndAdvance` (was `sub_2D3FE`) — a small, generic
"draw this animation frame, return the next (wrapping) frame index"
cycler, drawing picture `ax` at x=`bx` and advancing/wrapping the frame
counter within `[word_332EC, word_332EC+word_332EE)`. Also called once
from it: `ResolveAttackAndLatchFirstHit` (was `sub_2D195`) — calls
`ResolveAttack` then latches a value (`word_332E8`) into `g_stagedAttackDamage`
the first time through (only if it was still 0); exact field identities
not confirmed.

**Ranged attacks and area-effect abilities against a corridor monster**:
`ResolveAttackOrAbilityAction` (called from `HandleRangedOrCombatAction`,
the combat-round driver, itself called from `start`) handles two modes,
selected by `g_uiScratchFlags3` bit `0x100` (set by the caller — e.g.
`start`'s loc_10A65 branch, gated on *not* being in formal combat):

- **Set (ranged/thrown weapon)**: finds an equipped item in a party
  member's inventory slot, loads its catalog record for damage-type
  flags, rolls the hit via `ResolveAttack`, applies it via
  `ApplyResolvedDamageWithResistance`.
- **Clear (spell/ability)**: calls `ResolveAbilityEffect` — an 85%
  success roll, then dispatches on `g_currentActionId` (the ability id) to
  set a flat damage amount and, for several ids, a status-effect flag
  plus duration (the same `ApplyStatusEffect`/`TickStatusEffects`
  convention). Two specific ids are **area-effect spells**: when not
  in formal combat, `ResolveAttackOrAbilityAction` picks one of 4
  depth-row triples (by which band the current viewport row falls in)
  and calls `ApplyDamageAlongCorridorLine` for each — which in turn
  calls `GetMonsterAtViewportRow` (looking up the dungeon-viewport
  scratch buffer — the *same* `0x6D60` buffer `RenderDungeonViewRow`
  reads — for a monster at each depth row) and applies damage to
  whatever it finds. This matches the "IN A STRAIGHT LINE"/"IN A 3X3
  AREA" targeting text dumped from `ShowClueBookSpellDetail`'s message
  table, and directly ties the dungeon-rendering scratch buffer into
  the combat/targeting system.

`ApplyResolvedDamageWithResistance` is the shared damage-application
step used by both the ranged-weapon branch and
`ApplyDamageAlongCorridorLine`: reduces damage via a resistance
bit-scan (the attack's type flags vs. the target's `[+0x98]`
resistance flags, halving per match) before subtracting from HP.

**Resolved and reimplemented (`thrown.c`)**: the "abilities" `ResolveAbilityEffect` dispatches on are **thrown
consumables** -- GOLD POTION (60 damage), SILVER POTION (35 + poison, 6 x 5 timers), BLUE POTION (holy water: 50,
85 in Chapter 3, undead only: monster kind 13) and the FLAMING OIL FLASK (40 down the corridor, nothing in formal
combat). Item ids 0x19-0x1B / 0x240 in Chapter 2, 0x3D-0x3F / 0x3C in Chapter 3; two further branches (25 + 0x400,
15 + 0x4000) test id globals that are 0xFFFF in both games and are dead. 14 % of throws fizzle (roll above 85,
0..100 inclusive).

**The full ranged-weapon shot sequence**, `HandleRangedOrCombatAction`
(called from `start`, 2 sites — one sets `g_uiScratchFlags3` bit `0x100`
first): a 3-way combat-action dispatcher. If already in formal combat
(`g_uiScratchFlags4` bit `0x1000`), branches elsewhere (not traced). If the
caller set bit `0x100` (a ranged-attack request): scans the 4 party
inventory slots for a character with an eligible ranged weapon (item
`0x13A`, status-gated), bails if none; else draws a 4-icon weapon-select
UI (`DrawWeaponSelectIcon` per slot, with a highlighted variant for the
currently selected weapon; the 4 slot states, `word_328D8`/`word_328DA`/
`word_328DC`/`word_328DE`, at x-positions `0`/`0x36`/`0x69`/`0x9D`, are
also read by `ResetWeaponSlotDisplayCache` — was `sub_1DC73`, run every
Nth call — which clears a 105-row region of a dedicated EMS page
(`0x55FE`) at the first empty slot's x-offset, plausibly resetting a
per-slot display/animation cache) and **animates a projectile traveling
down
the corridor one depth
row at a time** — `AnimateProjectileStep` (draws the projectile sprite,
restores the background via `RestoreCorridorBackgroundFromEMS`
— **correction**: earlier described as "plays a sound", but it's an
EMS-backed graphics blit, not audio — then waits) then
`ClassifyObstacleAtViewportRow` (classifies
what's at that row: clear / wall / door / a `[+6]` bit `0x800` feature
/ a monster, reusing `GetMonsterAtViewportRow`'s `0x6D60` scratch-buffer
lookup) at successive rows (`0x31`→`0x2E`→`0x2B`→`0x28`→`0x24`→`0x19`,
i.e. the shot travels from far to near) until something stops it. A
wall/door shows a "deflected" message (`ShowCombatMessageOrWait`,
which shows the message unless speech/sound is currently busy, in
which case it just waits); a monster triggers
`ResolveAttackOrAbilityAction` and a hit/miss follow-up. If bit `0x100`
was clear (not in combat), it instead opens the **spell/ability-cast
sequence**: `HighlightSelectedAbilityIcon` then the *identical*
row-by-row `AnimateProjectileStep`/`ClassifyObstacleAtViewportRow`
scan the ranged-weapon branch uses — both paths converge into the same
code. On a hit that doesn't kill the target but the shooter has more
attempts left (`g_attacksRemaining`, decremented via `sub_1DCC6`), the loop
continues to the next depth row automatically — a multi-shot
continuation for characters with more than one attack. All paths
converge on a common epilogue: if the loot-staging counter (`0x51B6`)
has accumulated enough, `ShowLootAndAwardExperience` fires, then
`ProcessLevelMonsters` ticks and the screen/minimap redraw.

**The in-combat melee branch** (formal combat, `g_uiScratchFlags4` bit
`0x1000` set) is much simpler: `HighlightSelectedAbilityIcon` marks
the selected ability in the UI, one `AnimateProjectileStep`, then
`ResolveAttackOrAbilityAction` directly against `g_activeCombatMonster` (the
active combat monster) — no row-by-row search needed since the target
is already known. Miss shows `_val37` via `ShowCombatMessageOrWait`.

**The area-effect spell finish** (the 2 area-effect ability ids from
`ResolveAbilityEffect`, on a successful hit): shows a message, then
plays a 10-frame "explosion" animation (`DrawViewportSprite` at a new
z-layer `0xA`, with a `0x55AA` checkerboard blit-mask dither for a
flash effect), then — notably — scans the **entire** `g_levelMonsters`
pool (all 80 slots, not just the 3 rows the line-attack touched) for
any monster with HP `<= 0` and grants rewards / removes it via
`GrantMonsterRewards`/`RemoveMonsterFromMap` for each. So an
area-effect spell's kills are swept up level-wide after the animation,
not per-row during the attack itself.

### A riddle/password item mechanic

`UseRiddleAnswerItem` (was `sub_1A5F6`, called once from `UseItem`)
is a distinct item mechanic not seen elsewhere: it looks up an
expected-answer id from a table indexed by the item's tier value,
shows the item's description (if any) via `DrawIndentedTextColumn`,
then opens a 34-character text-entry field (`EditTextField`) for the
player to type an answer — "PRESS ESCAPE TO EXIT" shown as a standing
hint. The typed text is compared byte-for-byte against the expected
answer string; a match shows "THAT SOUNDS GOOD TO ME." and sets a
`g_uiScratchFlags2` unlock flag, a mismatch shows "THAT IS INCORRECT." —
either way looping back to prompt again unless the player cancels.
Reads as a riddle, puzzle-lock, or "speak the password" item; which
specific quest item(s) use this path is not identified.

### Combat: monster slots and turn order

Up to **3 simultaneous active monsters**, `g_monsterSlots` (base
`0x51C0`, 3 × `0x9C`/156-byte records, `[+0]==0` = empty slot;
zeroed wholesale by `InitializeDungeonLevel` when entering/loading a
level, alongside clearing `g_activeCombatMonster`, the active-combat-monster
global).
Confirmed fields: `+0xC` type/behavior flags (tested against `0x3010`
in `BuildCombatTurnOrder`); `+0x12` the monster's current target (a
party-member record pointer, assigned randomly among living party
members each round unless a flag is already set); `+0x56`
speed/initiative value used for turn ordering. `DrawMonsterInfoPanel`
reads name strings and up to 3 tiers of detail icons per monster,
gated by the party's average `+0x58` stat (see the party-record section
above) against its own `[+0xC]` 2-bit quality flags.

Every `RunDungeonGameLoop` iteration, `BuildCombatTurnOrder` rebuilds
`g_combatTurnOrder` (`0x539E`, 14 × 8-byte scratch entries, room for
4 party + 3 monster combatants): `+0` record pointer, `+2` party-slot
address (0 for monsters), `+4` the speed/initiative sort key (used for
a descending insertion sort — turn order fastest-first), `+6` flags
(`0x8000` = this entry is a monster; `0x4000` = **defeated, confirmed
below**; `0x2000` = unconfirmed). `SelectActiveMonster` then
picks the first non-defeated monster from that order into
`g_activeCombatMonster`. **`ProcessCombatRound` confirms the "defeated" flag**:
called every `RunDungeonGameLoop` iteration, it checks every occupied
`g_monsterSlots` entry's HP (`+0x10` <= 0) and, on death, sets its
`g_combatTurnOrder` entry's `0x4000` flag, clears `g_activeCombatMonster` if it
was the active target, and calls `GrantMonsterRewards` — closing the
loop from `HandleDungeonInput`'s HP subtraction through to loot. If no
monster died that pass, it instead advances the turn to the next
living combatant in `g_combatTurnOrder`. After a death pass and
`SelectActiveMonster` has picked the new active monster,
`ProcessCombatRound` calls `CompactMonsterSlots` (was `sub_2333B`):
shifts the remaining live `g_monsterSlots` records into a contiguous
front-loaded arrangement (picking source/destination among the 3
fixed slot addresses based on occupancy), then rewrites any
`g_combatTurnOrder` entry still pointing at the old, now-vacated
address.

`g_activeCombatMonster`, the "currently active monster" global read throughout
the combat-adjacent code already documented this session
(`UseAbilityOnTarget`, `ExamineTarget`, `CastSpell`'s target checks,
etc. — not yet cross-referenced against this specific variable, a
good next step). Mouse-clicking a monster panel's icon also sets
`g_activeCombatMonster` directly (a target-selection shortcut alongside
`SelectActiveMonster`'s automatic pick).

**A monster's turn is driven by `ProcessMonsterAttackTurn`** (was
`sub_16881`): `RunDungeonGameLoop` tests `g_combatTurnOrder`'s
current entry for the `0x8000` "monster" flag and calls it for a
monster's turn (calling `HandleDungeonInput` instead for a party
member's turn). It ticks the monster, briefly shows its info panel
on first reveal, then branches on a new find — `[+0x92]` bit
`0x1000`, an **area-effect/breath-weapon attack flag**: if set, it
attacks all 4 party slots (skipping incapacitated members) in one
turn via `ResolveAttackerActionOutcome`, playing its hit sound only
once; if clear (the common case), it attacks only its single
assigned target (`[+0x12]`). On a successful single-target hit, it
also calls `TickEquippedItemDurability(0x146)` on the defender —
confirming an ordinary monster attack, not just the corrosion
special attack, can wear/break the defender's equipped item. Two
more monster-record fields found in the process: `+0x52` (a save-DC
stat fed into `word_32DC0` for `ApplySavingThrowEffect`) and a pair
of distinct sound ids, `+0x5C` (attack-hit) vs `+0x5E` (idle/
grumble, played when no valid target is available).

**Attack-roll formula, found via that click handler**: `ResolveAttack`
(`ax`=target defense, `bx`=attacker accuracy, `cx`=weapon damage power)
— hit if `(accuracy-defense) >= RandomInRange(55)`, damage =
`weaponPower*(accuracy-defense)/100` (minimum 1), else a flat miss.
`UpdateMonsterWoundTier` then classifies a hit into an escalating
visual wound-severity flag on the monster record (`+0xE`: `0x8000`
light, `0x4000` moderate, `0x2000` severe, by percentage of `+0x50` —
plausibly max HP/toughness) plus an unconditional display flag
(`+0xC` `|= 0xA`) — it does not subtract HP itself, but its caller
does immediately afterward: **`[monster+0x10] -= g_stagedAttackDamage`** (the
damage just dealt). `DrawMonsterAndUpdateAttackState` (called for
every rendered monster, whether a corridor encounter or an active
`g_monsterSlots` combatant) reads exactly this wound state to pick a
hit-flash/recovery animation frame (`+0xC` bits 2/4), draws a weapon/
attack-effect sprite, and at the end checks the same `+0xC` `0x3010`
bits documented above for `BuildCombatTurnOrder`/`TickMonsterTimer`'s
countdown — resetting it if set, else calling
`AdvanceMonsterAnimationFrame` (also reused by
`ShowClueBookMonsterDetail` to animate its preview sprite the same
way): advances the monster's idle/walk animation frame within a small
cycle relative to a base frame, mode selected by `[+0x92]` flags. This confirms `+0x10` doubles as the monster's
*current HP* in the active-combat context — the same field
`TickMonsterTimer` uses as a presence/lifespan countdown in the
level-wide `g_levelMonsters` pool context, a polymorphic field reused
across the two roles (matching this codebase's established pattern for
fields like `errorCode`/`g_currentActionId`). `+0x50` is therefore plausibly
max HP. What actually happens once `+0x10` reaches 0 (death handling)
isn't traced yet.

`g_monsterSlots` is fed by a much larger **per-level monster spawn/
wander pool**, `g_levelMonsters` (base `0xF26`, 80 × `0x9C`-byte
records — same stride and `[+0xC]` flag conventions as
`g_monsterSlots`, strongly suggesting the identical record layout).
New monsters enter this pool via `SpawnMonsterInFacingDirection`,
called from `TryTriggerMonsterEncounterAtCell` — which turns out to be
part of the **first-person dungeon corridor viewport renderer**:
`RenderDungeonViewport` (called from two unnamed sites) resets a
per-frame row-depth counter (`g_viewportRowDepth`) and calls
`RenderDungeonViewRow` **seven** times with decreasing cell counts
(`0x11`, `0x11`, `5`, `3`, `3`, `3`, `3`) — the classic "draw each
depth row of the visible corridor, near to far" shape. **Correction**:
before each call it copies one of 7 consecutive globals
(`word_328E6`-`word_328F2`) into `g_shadeShiftDelta` — first described as "a
different row-data pointer," which is wrong; `g_shadeShiftDelta` is the
shade-shift delta `DrawPicture`/`ShiftPaletteShadeClamped` apply per
pixel (see the lighting-gradient note below), so this is a shared
7-entry per-depth-row *shade-delta* gradient, not pointers of any
kind. `RenderDungeonViewRow` draws each cell's picture (a
12-byte-stride lookup table at `0xE551`) and calls
`TryTriggerMonsterEncounterAtCell` once per cell, incrementing/
decrementing `g_viewportRowDepth` as it goes. `TryTriggerMonsterEncounterAtCell`
only fires for `g_viewportRowDepth >= 0x11` — since only the first two (and
therefore *farthest*) rows use `cx=0x11`, **monsters can only spawn in
the farthest visible cells, not right next to the party** — plus a
flag bit on the cell record, then **correction**: skips spawning if
this monster type already exists somewhere in `g_levelMonsters`
(`FindMonsterTypeInLevelPool` — first described as "a probability
roll," which was wrong; it's a duplicate/unique-monster prevention
check), before calling `SpawnMonsterInFacingDirection`, which:

Both `SpawnMonsterInFacingDirection` and `FindMonsterTypeInLevelPool`
call `TryActivateMonsterByDistance`: if a monster isn't already marked
active/aware (`[+0xC]` bit 0), it checks the render-depth counter
(`g_viewportRowDepth`) against a per-monster detection-range threshold
selected by `[+0x94]` flags, setting the aware bit once the party is
close enough — a distance-based monster "notices you" mechanic.

`RenderDungeonViewRow` draws each cell's base wall texture via
`DrawDungeonCellWallTexture` (the `0xE551` lookup table by cell id, at
z-layer `word_32918=0` — a different layer from the `3`/`4` values
used earlier in `RenderDungeonViewRow` for items/monsters standing in
the cell), plus a fixed overlay picture when the cell's `[+6]` flags
have bit `0x2000` set (a door/torch/decoration marker, not confirmed).
`RenderDungeonViewRow` also calls `TryDrawDungeonCellSideFeature` per
cell, which (if the cell's `[+2]` field is nonzero) calls
`DrawDungeonCellSideFeature`: a facing-direction-indexed door/
side-feature sprite (table at `0xE175`, same facing-tier pattern as
`ShowCompassDirection`/`SpawnMonsterInFacingDirection`), with a
conditional overlay for what's plausibly an open-door/lit-torch
variant.

Before `RenderDungeonViewport` runs, both `RedrawDungeonScreen` and
`RefreshDungeonScreen` first call `DrawDungeonFloorAndCeiling`: draws
the sky/ceiling and floor backdrop pictures for the current cell, then
calls `ExtendDungeonFloorTexture` six times (same row pattern) to
extend the floor texture across cells sharing the same floor type — a
simpler "seamless floor" pass, distinct from `RenderDungeonViewRow`'s
full per-cell wall/object rendering. Between that and
`RenderDungeonViewport`, both callers also run `ExtendDungeonCeilingPass`
(the ceiling counterpart, via `ExtendDungeonCeilingTexture`) — so the
dungeon-screen render sequence is: 1) `DrawDungeonFloorAndCeiling`
(backdrop images + floor-extension), 2) `ExtendDungeonCeilingPass`
(ceiling-extension), 3) `RenderDungeonViewport` (full wall/door/
monster/encounter rendering). All three passes share one underlying
primitive, `DrawViewportSprite` (was `sub_29B0F`, 632 lines, internals
not traced): every viewport-rendering function this session calls it
with the same (picture id `g_pictureId`, scale class `g_pictureCategory`,
z-layer/depth `word_32918`, transparency `_font_bgTransparent`)
convention — the perspective/depth-aware counterpart to the simpler
general-purpose `DrawPicture`. **First foothold into its internals**:
two RLE run-record blit primitives it calls are now named —
`DrawRleMaskedShadedRun` (was `sub_2A589`, a contiguous run copy with
optional `0xFF`-transparency) and `DrawRleScaledSpriteColumn` (was
`sub_2A5F7`, destination steps by a full `320`-byte screen row per
pixel while the source steps by a caller-supplied stride — the
classic shape for a perspective-scaled sprite column). Both shade
every pixel via `ShiftPaletteShadeClamped`. `DrawRleMaskedShadedRun`
also conditionally chains two more per-pixel effects, now also named:
`RemapOrMaskColorByHueTable` (was `sub_2A4B0`, though it turned out to
be called from `DrawPicture` directly too — splits a color into
hue-group/shade nibbles and looks up the hue-group in a 16-entry
table to remap it or force full transparency, plausibly a
status-effect tint) and `InvokePixelEffectCallback` (was `sub_2A217`,
a tiny tail-jump to a caller-configured function-pointer hook). The
run-record producer(s) that build the 6-byte run tables these
blitters consume remain untraced. Two more family members:
`DrawShadedPixelRun` (was `sub_2A51B`, called from `DrawViewportSprite`
directly, no RLE table — the simplest member, a straight shaded run
with optional masking) and `CopyShadedViewportRows` (was `sub_2A0FC`,
called from unnamed `sub_29FF6`) — copies a 224-pixel-wide row
(matching `DimDungeonViewport`'s viewport width) from `si` to `di`,
shading every pixel, advancing both by one screen row (`0x140`) per
iteration. `sub_29FF6` (262 bytes, references `_videoSegment`) is a
solid next-round candidate now that its main inner loop is understood.

`RenderDungeonViewport`'s 7th and final call is
`RenderDungeonVanishingPoint`, structurally different from the other
six: draws the far-wall/vanishing-point cells at the end of the
visible corridor (a different `0xE551` table field, z-layer 6), then
runs the same per-cell side-feature and encounter checks as
`RenderDungeonViewRow` for the final cell, and (when `g_uiScratchFlags4` bit
`0x1000` is set, plausibly "in combat") calls
`RenderActiveMonsterSprites` to draw the 3 `g_monsterSlots` combat
monsters into the viewport.

`RenderDungeonViewport` itself is called by two `start`-reachable
screen-redraw functions: `RedrawDungeonScreen` (a fuller variant with
extra setup calls) and `RefreshDungeonScreen` (a lighter variant that
also conditionally redraws the minimap) — the exact trigger
distinguishing when each is used isn't traced.

finds an empty slot, loads the monster's catalog record from
`WORLD.DAT` (same block math as `LoadClueBookMonsterEntry`), computes
a spawn position offset from the party's current facing direction
(the same `g_partyFacing` tier bits `ShowCompassDirection` reads) plus
current position, sets a countdown timer and full HP
(`[+0x10]=[+0x50]`).

`ProcessLevelMonsters` also calls `ClassifyObstacleAtWorldPosition`,
the `WORLD.DAT`-backed counterpart to `ClassifyObstacleAtViewportRow`:
same type-range obstacle classification, but reading a map cell
directly from `WORLD.DAT` rather than the live viewport scratch
buffer — used to check whether a monster's target cell (anywhere on
the level) is blocked before it moves there.

Separately, `ProcessLevelMonsters` also computes a one-cell step toward
the player's position (comparing the monster's `[+2]`/`[+4]` against
`g_partyWorldX`/`g_partyWorldY`) and calls `IsMonsterStepBlocked` (was
`sub_2B384`) to validate it before moving: outright blocked on cell
flag bits `0xC00`; a "special" cell (flag bits `0x6000`) passable only
if a monster trait flag (`[+0x94]` bit `0x10`) is set (plausibly a
wall/door-bypass trait — flying or incorporeal monsters?); a few more
branches gated on other `[+0x94]` bits (`8`/`0x14`/`0x1A`) and value
ranges; otherwise falling through to the same
`ClassifyFloorType`/`IsCellTypeImpassable` pair used for plain terrain
checks. The exact meaning of the `[+0x94]` trait bits isn't confirmed —
open lead for whichever monster types turn out to fly/phase through
walls.

The **player's own** movement uses the same `[_val32,_val31]`
"special cell type" range check: `HandleMovementInput` calls
`HandleSpecialCellEntry` (was `sub_116F3`) when the destination cell's
type falls in that range. If `g_lastKeyChar`=='H' (not one of the
manual's documented hotkeys — possibly unsurveyed, or an internal
sentinel rather than a literal keypress), it pulls two entries out of
the `0xE551` tile-type table (the destination cell's type, plus a fixed
index `0x6EB8`) into a scratch struct, then calls
`RefreshDungeonScreen`; otherwise it just plays a different sound.
Both paths fall through to the normal movement-apply code. Reads as
handling a trap-door/stairs-like special cell, but neither the cell
type's identity nor the 'H' condition are confirmed.
`HandleMovementInput` also calls `DrawMovementFeedbackIcon` (was
`sub_116CF`) in a few places, including its "destination out of the
dungeon grid bounds" branch — a small icon draw whose exact narrative
isn't confirmed either.

`ProcessLevelMonsters` ticks every occupied slot via `TickMonsterTimer`
each `RunDungeonGameLoop` iteration: a movement/attack-readiness
countdown (`[+0x10] -= [+0x1C]`, `errorCode`=1 on reaching 0, or a
two-phase variant with an intermediate `errorCode`=2 for monsters
flagged `0x3010`), plus a separate, slower countdown (`[+0x1E]`) that
on reaching 0 resets the monster wholesale — clears several `[+0xC]`
flag bits, zeroes `[+0x1A]`/`[+0x1C]`/`[+0x1E]`, restores `[+8]` from a
template value at `[+0x4C]` — plausibly a death/respawn cycle, not
confirmed. **Correction**: on `errorCode`==1, `ProcessLevelMonsters` does *not*
promote the monster into combat (last round's guess) — it calls
`GrantMonsterRewards` (adds 4 fixed BCD values into the monster's own
tally fields and into the global BCD counter `0x51B6` — see below) then
`RemoveMonsterFromMap` (clears the monster's "present here" flag on its
map cell and zeroes the entire record). So `[+0x10]`'s countdown is
better read as a remaining-presence/lifespan timer than a movement-
readiness one — reaching zero ends the monster's presence with a
reward grant, not a combat trigger. The actual "monster notices the
party and a fight starts" path, if distinct from this, isn't found
yet.

### Global material counters and BCD arithmetic

`DrawAlchemyStatusPanel` is the alchemy screen's character/resource
panel: name, the "MAGIC:" MP bar (drawn via `FormatAndDrawAlchemyFraction`,
was `sub_1E2E5` — a near-duplicate of `FormatAndDrawFraction`), and
readouts labeled "MAGIC ORE: " (`0x94B7`) and "NUORE: " (`0x94BB`) —
pairing with `CastSpell`'s `0x1C` ability below, which converts between
those same two counters.
It's drawn repeatedly by `RunAlchemyScreen` (reached directly from
`start`), the alchemy screen's own driver loop, which also draws one
of two fixed icon states (`ShowAlchemyIconActive`/`ShowAlchemyIconIdle`,
picture 6 vs 5 at the same position — exact narrative not confirmed) —
polls input,
hit-tests clickable regions, reuses the party-member panel-select
routine, and shows a confirm prompt (plausibly for an ore conversion)
before exiting back to the dungeon via `ApplyMapTriggerEffect`. Its
many internal helper calls aren't individually traced yet, but one
cluster now is: `BuildAlchemySpellList` (was `sub_1E1A7`) builds the
filtered list of known spells (an eligibility check, `TestRecordFlag_CA`
— **correction**: was described as untraced here; it's since been
named as the `+0xCA` per-record flag-bank test accessor) unless the
character is incapacitated, then calls
`CheckSpellCastability` (was `sub_1E285`) on each to check whether the
character can currently afford it — enough MP (`+0x54`), MAGIC ORE
(`0x94B7`), and NUORE (`0x94BB`), loading each spell's cost data via
`LoadClueBookSpellEntry` — setting a "castable" icon state for the ones
that qualify. Pagination is 13 spells/page. `DrawAlchemySpellList`
(was `sub_1E3AF`) is the visual counterpart, drawing each page's rows
(name colored by castability, cost values via `DrawSpellCostValue`,
was `sub_1E340`) and highlighting the current selection. Casting a
spell pays for it via `DeductAlchemySpellCosts` (was `sub_1E61B`) —
subtracting MP and the same two BCD ore counters. On screen entry,
`RestoreOrSelectAlchemyCaster` (was `sub_1E473`) re-validates a cached
caster id against `g_partySlotAssignment` (gated on the same `+0x94`
marker `ApplySecondaryClassTierFlags` uses) and sets `g_selectedPartySlotPtr`
accordingly, falling back to `SelectDefaultAlchemyCaster` (was
`sub_1E447` — the first occupied, `+0x94`-eligible slot) otherwise.
`RunAlchemyScreen` also calls `ShowCompassDirection`, a
"NORTH"/"SOUTH"/"EAST"/"WEST" HUD readout gated on an unidentified
"compass active" mode (`g_uiScratchFlags4` bit `0x1000` clear, `word_36C7F`
bit `0x400` set), drawn at the same screen position as the
material/gold HUD. `DrawMinimap` has its own small graphical
counterpart, `DrawMinimapCompassIcon` (was `sub_2169C`): draws a fixed
icon glyph (the same one used for the minimap's 7×9 grid cells) at a
fixed position, with a 0–3 remap value selected by the same
`g_partyFacing` facing-tier bits.

Three **global** (not per-party-member) counters at `0x94B3`
(`g_partyGold`), `0x94B7`, `0x94BB` — confirmed **exactly
consecutive**, 4-byte packed-BCD stride, by
`ShowResourceDepletedOverlay`'s scan of all three in one loop. All
three, fully labeled ("GOLD COINS:"/"MAGIC ORE: "/"NUORE: "), are
drawn together by `DrawResourceCounterPanel` (was `sub_2714A`) in the
normal status-panel area — the detailed counterpart to the
gold-only, icon-based `ShowMaterialCounterHud` used in shop screens.
Individual identities: `0x94BB`/`0x94B7` are used by `CastSpell`'s
`0x1C` alchemy ability (converts 10 units of one into the other —
`NUORE`/`MAGIC ORE`); `0x94B3` is the party's **gold** — **correction**:
first framed as a generic "material counter" (below), but its HUD
label (`ShowMaterialCounterHud`, msg `0x7FC4`) turned out to be a
literal `"$"`, and its two consumer functions were renamed to match:
`TrySellItemForGold` (was `TryConvertItemToMaterial` — a Space-bar
action, main input loop `g_uiScratchFlags2` bit `0x10`, while carrying an
item: rebuffed with "I HAVE NO NEED FOR THAT TYPE OF ITEM." if the
item's type doesn't match what the standing location accepts,
otherwise sells it and credits `g_partyGold`) and
`TryEnhanceItemForGold` (was unnamed — the sibling Space-bar action:
gated by a level/stat eligibility check, `CompareBCD4`/`SubBCD4`
against `g_partyGold` and a per-tier cost table at `0xCB2`, showing
"YOU DON'T HAVE ENOUGH GOLD!" on failure; on success, spends the gold
and advances the held item to the next entry in the item catalog —
the item "enhancement" itself). `ApplyEffectCost`'s trap/status-effect
cost dispatch also spends from this same 3-counter family, so a trap
stealing party gold is plausible. A third Space-bar sibling,
`TryRepairItemForGold` (`g_uiScratchFlags2` bit 4), pays gold (cost table
`0x5082`) to repair the held item, rejecting with "I CAN NOT REPAIR
THAT" if ineligible — distinct from the skill-based `RepairItemCommand`
minigame below, which can critically fail and destroy the item. Its
eligibility check is `IsItemEligibleForRepair` (a location/item
flag-pair match); `TryEnhanceItemForGold`'s is `IsItemEligibleForEnhance`
(a location-selected item field checked against a range table at
`0xBCE`). Both have other, untraced callers beyond this Space-bar
cluster. A related classifier: `ClassifyItemServiceTier` (was
`sub_1AE9D`, called 6 times from two other functions) loads an
item and, based on its `[+0xC]`/`[+2]` flags, returns one of 3 tier
codes (or a 4th "wrong item type" code) — plausibly gating which
service (repair/enhance-style) the item qualifies for, but not
confirmed. Called from `GetClassifiedItemStatField` (was `sub_1AE23`,
called from `ResolveAttackerActionOutcome`), which selects one of two
`word_2E548` sub-fields (`+4`/`+8`) based on the item's category flag,
or returns 0 if classification fails. One of `ClassifyItemServiceTier`'s
two other callers, `TickEquippedItemDurability` (was `sub_1ACD7`,
called from `HandleDungeonInput` and `ProcessMonsterAttackTurn`),
turned out to be the game's
full **equipped-item durability and random-breakage system**: given
an equipment-slot offset (`0x13A` weapon / `0x142` second slot /
`0x146` array), it increments a per-slot wear counter (`+0xBE`/
`+0xC0`/`+0xC2`), and once it crosses a slot-specific threshold,
re-classifies the item and rolls a percentage breakage chance from
`word_2E548`'s fields (`+0xA`/`+0x6`, paired with replacement-item
ids at `+0x8`/`+0x4`) — on a break, it calls
`ApplyItemEffectIconSlot` (was `sub_1AE4C`): populates an icon-bar slot
(the same `0xC50 + slot*0x14` layout `TickPartyAilmentIconBar`/
`ApplySavingThrowEffect` use) for effect id `0` — a new id not seen
elsewhere — tied to the current item and party record, then calls
`ApplyEffectAndDrawIconBar`. Its other caller (`sub_1AC80` ->
`ApplyTriggerEffectIconSlot`) is the same pattern reached from
`ApplyMapTriggerEffect` instead of direct item use — a map trap
triggering the same icon-bar-slot effect machinery. A second,
combat-driven path to equipment damage: `ResolveAttackerActionOutcome`
(was `sub_16BF6`, called twice from `ProcessMonsterAttackTurn`, was
`sub_16881`, since named) resolves one attacker-vs-defender action via one of 3 paths — the
normal `ResolveAttack` damage roll, a `FailsSavingThrow`-gated
status-effect application, or (a weaker-DC save) an "equipment
corrosion" effect that targets the *defender's* equipped item via
`GetClassifiedItemStatField` instead of dealing HP damage — a
monster special attack that damages gear directly, distinct from
`TickEquippedItemDurability`'s ordinary wear-and-tear.

**`TickEquippedItemDurability`/`ApplyItemEffectIconSlot` reimplemented,
2026-09-29** (instruction-identical in Chapter 3): with
`partyHandleIconBarItemExpiry` already done, this composed cleanly —
`ApplyItemEffectIconSlot` is exactly effect id 0's own definition
(embedded in `effect.c`'s tables: `modeFlags` = `EffectModeItemReplace`
for both games) feeding `partyHandleIconBarItemExpiry` directly, and
the "`word_2E548`'s fields" this section's own older note left vague
turned out to already be named: `item.h`'s `ItemTargetBreakChanceA`/
`ItemTargetBreakItemA` (and their `B` siblings for the non-weapon
category) are exactly the roll chance and replacement id, selected by
the *same* category-A/C flag test `itemCorrosionReplacement` already
uses. Confirmed one more original quirk while composing it: after a
break, which of the 3 wear counters gets reset to 0 is decided by the
*replacement* item's own equip-category flags, not the original slot
that broke — traced by resolving what the original's own
`g_currentItemRecord` actually is at that point (a fixed alias for
`LoadItemCatalogRecord`'s `0xB50` scratch buffer, last overwritten by
the replacement item's own load inside `partyHandleIconBarItemExpiry`
itself — not a separately-tracked pointer). Reimplemented as
`partyTickEquippedItemDurability` in `src23/party.c`/`.h`, deliberately
omitting the original's own redundant second `ClassifyItemServiceTier`
call (an artifact of its scratch-buffer-based item lookup possibly
going stale between calls — `item.h`'s own lookups don't share that
risk, so the second call would be a pure no-op repeat). Tests in
`tests/test_party.c` cover the empty-slot/non-classifying no-op, the
exactly-at-threshold boundary, and both roll outcomes including the
wear-counter-reset-follows-the-replacement quirk. All 18 suites pass
(`party.c` gained a new dependency on `effect.c`/`random.c` for
`effectGetDef`/`randomInRange`; every test file linking `party.c`
directly was updated to link `effect.c` too).

The shared "YOU DON'T HAVE ENOUGH GOLD!" rejection is
`ShowInsufficientGoldMessage`. The whole sell-item screen is entered
via `RunSellItemScreen` (from `UseItem`, when the used item's `[+0xE]`
flags have bit `0x4000` set). A sibling branch, gated on the item's
name literally matching `"BUY "` or its `[+0xE]` flags having bit
`0xC000` set, calls `ConfirmAndValidatePartyTarget` — a confirm
prompt to pick a party member, re-prompting with a warning if the
pick is incapacitated (caching the valid choice): sets `g_uiScratchFlags2` bit `0x10` and runs
`RunPartyInventoryScreen` (was `sub_1869D`) so Space triggers
`TrySellItemForGold`, then rebuilds/redraws the minimap on exit. Its
two siblings, `RunEnhanceItemScreen` (`UseItem+0x1C1`, sets bit 8) and
`RunRepairItemScreen` (`UseItem+0x1D0`, sets bit 4), are otherwise
identical — completing the shop cluster's three `UseItem`-reachable
entry points (sell/enhance/repair), each just setting a different
`g_uiScratchFlags2` action bit before running the same main input loop.
All three, plus `start` and `HandleDungeonInput` generally, call
`RefreshPartyPortraits`: its core role is refreshing the 4 party
portrait slots, but when a shop action bit is active
(`g_uiScratchFlags2 & 0x1C`) it also draws a context hint — "SPACEBAR TO
ENHANCE ITEM" / "SPACEBAR TO REPAIR ITEM" / default "SPACEBAR TO SELL
ITEM OR ESC TO UNDO". A simpler sibling, `RestoreAllPortraitsFromEMS`
(was `sub_18F6C`, called from `RunPartyInventoryScreen`), does the
same 4-portrait restore plus dirty-bit clearing without the shop-hint
text. It opens with `RestorePortraitPanelFromEMS`,
which — only when none of the portrait-dirty bits are already set —
blits a cached background region from EMS-paged memory straight back
into the video buffer instead of a full redraw. Its sibling
`ClearPortraitPanelAreas` (was `sub_1B47A`, also called from
`RunShopScreen`) instead *blanks* (fills with a fixed pattern) the same
portrait-sized EMS page (`0x55D8`) in both the cache and the live video
buffer — resetting it so a later restore doesn't show stale portrait
data. A few more `0x55D8`-page siblings were named alongside it:
`RestoreFullScreenFromEMS` (was `sub_223D4`, a full-screen restore,
called from `HandleMovementInput`) and `RestoreLargePanelFromEMS` (was
`sub_191FC`, a large-but-not-full-screen area restore, called from
`RunPartyInventoryScreen`, was `sub_1869D`, since named — see the
`ConsumeItemChargeResource`/`ApplyEncodedItemEffect` paragraph above
for how the two connect). Separately, `RestoreAndRedrawFixedStatusIcon`
(was `sub_22402`, 6 call sites incl. `start`) restores a small EMS-
cached area then redraws a fixed picture via `DrawFixedStatusIcon` (was
`sub_225F1`) — a picture from the same directory category (`0x60`)
`DrawPartyMemberPortrait` uses, drawn at a fixed screen position; which
specific HUD icon this is isn't confirmed. `HandlePortraitClick`
is the mouse-click counterpart to the keyboard `1`-`4` selector
(`HandlePartyStatusPanelInput`): hit-tests the 4 portrait zones and
sets the matching highlight bit when clicked — expanding a portrait
this way is what enters `RunPartyInventoryScreen`, the party/
inventory management screen (called once from `start` right after
this bit gets set). Both draw via `ShowPartyPortraitForSlot`
→ `DrawPartyMemberPortrait`: the character's icon (`+0x14`), a status
bar, and a condition icon selected by `+0x15C`/`+0x10`, plus
equipped-item icons via `DrawEquippedItemIcons` (reads the
`+0x13A`/`+0x13E`/`+0x142` equipment slot arrays, using an "active"
icon variant when an item's own `[+0xC]` bit `0x400` is set) — a
150-line function whose remaining icon-selection logic isn't
individually traced, plus two small overlay icon drawers,
`DrawPortraitOverlayIconA`/`DrawPortraitOverlayIconB` (each drawing a
"+1" highlighted icon variant at a fixed offset — exact narrative not
confirmed). **The equipment slot layout extends further**:
`RecomputeEquipmentStatBonuses` (was `sub_1B30C`, called from
`HandleItemDropOnPartyPortrait`/`sub_1AA9B`/`ConsumeItemChargeResource`) confirms `+0x13A`
(main weapon) and `+0x142` as individually-treated slots (matching
`DrawEquippedItemIcons` above), plus two more slot arrays beyond
`+0x13E`: a 3-entry array at `+0x146` (stride 4) and a 5-entry array
at `+0x152` (stride 2) — it sums each equipped item's catalog stat
bonus (via `LoadItemCatalogRecord`) across all of these into two
5-word "derived equipment bonus" blocks (`+0x48`-`+0x50`/
`+0x88`-`+0x90`, reset from base values `+0x32`-`+0x3A`/`+0x72`-`+0x7A`
first) — the concrete mechanism behind equipped gear's stat
contribution. **Fully reimplemented 2026-09-24** as
`partyRecomputeEquipmentStatBonuses` — see the "UseTrainingItem"
section below for the complete per-slot formula (which skill feeds
which rating, the main-weapon/off-hand split, the `PartyFieldUiFlags`
bit 0x20 side effect) and the corrected understanding that `+0x48`-`+0x50`
are `PartyStatEquipRating1`-`5`. A separate function, `DrawPartyMemberStatusPanel`
(called from `RunPartyInventoryScreen`), draws a fuller
combat-style status panel per party slot: portrait, unconscious/dead
overlay, three `DrawStatBar` gauges (HP `+0x52`/`+0x92`, MP
`+0x54`/`+0x94`, and a third — **now confirmed as carried weight
`+0x118` vs. max capacity `+0x56`**, via `DrawThreeStatBars`, the
character-sheet sibling of this panel — see the inventory-slot section
above for the full cross-confirmation), an ability-readiness icon
(`+0xB4`, the "learned abilities" bitmask), and level-up/training text.
It also calls `DrawAfflictionIconRow` (was `sub_22615`) — a visual,
icon-based counterpart to `DrawAfflictionsList`, directly re-testing
the same `+0x1C` bit groups (DISEASED/POISONED/SICK; STONED/FROZEN/
PARALYZED; CURSED/HEXED/JINXED) to pick one of 4 icon variants per
group, plus a 4th icon shown when the 9 protection values (`+0x20`..
`+0x30`) sum to nonzero (has some active protection bonus) —
independently confirming both bit-group and protection-value mappings
from a completely different function. `RedrawAllPartyStatusPanels`
(was `sub_2ADD0`, called from `ApplyMultiStatEffect`, `RestCharacter`,
and others) is the batch helper: calls `DrawPartyMemberStatusPanel`
for every occupied roster slot. `RedrawAllPartyStatusPanelsAlt` (was
`sub_222F8`, called from `HandleMovementInput`/`InitGame`) is a
different-segment, non-identical counterpart that redraws all 4 slots
unconditionally rather than stopping at the first empty one.
`DrawThreeStatBars` (was `sub_25F10`) is the character-sheet version of
this same 3-bar display, labeled exactly "HEALTH:"/"MAGIC:"/"WEIGHT:"
— and its "DEAD" override (shown instead of the HEALTH fraction)
**confirms `+0x1C` bit `0x40` as the character's dead/incapacitated
flag**. Its number formatting goes through `FormatAndDrawFraction` (was
`sub_25E5E`, "`<current>/<max>`") and a shared header,
`DrawCharacterNameHeader` (was `sub_25ED1`, also used by
`ShowLevelUpMessage`). A third, simpler party display,
`DrawPartyStatusIconRow` (was `sub_26C9E`, called from
`HandleDungeonInput`) draws a compact 4-icon row during dungeon
exploration via `DrawPartyStatusIcon` (was `sub_26CFB`) per
`g_partySlotAssignment` slot: the character's icon (`+0x12`, the same
field `DrawPartyRosterEntry` uses), an overlay icon when incapacitated
(`+0x1C` bits `0x1C40`, the same bits `CheckPartyWipeAndReinitLevel`
checks) or a new not-yet-documented flag (`+0x15E` bit `0x8000`,
plausibly a second "needs attention" condition), and a selection-
highlight overlay for the currently-selected slot. The other half of
that `+0x15E` finding: `MarkIneligiblePartyMembers` (was `sub_2D7A7`,
called from `InteractWithContainer`) is what *sets* the bit — for each
party slot, unless an eligibility check (`sub_27A66`, not traced) plus
a status-flag/level test passes, it sets `+0x15E` bit `0x8000` and
forces a `DrawPartyMemberStatusPanel` redraw. Reads as "flag party
members who don't qualify to use/interact with whatever's in this
container" (a class- or level-restricted item?), but the specific
restriction isn't confirmed. `InteractWithContainer` also calls
`ConfirmContainerInteraction` (was `sub_2D809`): shows a yes/no
confirm prompt (message id `0x12`), storing the result and the current
slot selection for the caller to act on afterward, and
`ClearIneligibleFlagForAllMembers` (was `sub_2D7EA`) — the exact
inverse of `MarkIneligiblePartyMembers`, unconditionally clearing
`+0x15E` bit `0x8000` for all 4 slots. `RunAlchemyScreen` uses a
byte-for-byte duplicate of `ConfirmContainerInteraction`,
`ConfirmAlchemyInteraction` (was `sub_1E4FA`) — the same
`DrawShadowedText`/`DrawShadowedTextAlt`-style overlay-segment
duplication found earlier this session. A third instance of the same
pattern: `PollForEscapeKeyOnly`/`PollForEscapeKeyOnlyAlt` (was
`sub_11900`/`sub_15249`) — polls for a keypress but discards anything
but ESC, called from `ShowIntroPicture` and
`PlayCharacterCreationIntroAnimation` (was `sub_15429`) respectively.
All three are manipulated via the packed-BCD
bignum library (`ConvertWordToBCD4`, `CompareBCD4`/
`IsBCDCounterAtLeast`, `AddBCD4`/`AddToBCDCounter`, `SubBCD4`/
`SubtractFromBCDCounter`) — 4 bytes (8 decimal digits) per counter,
most-significant-digit-first, `DAA`/`DAS`-adjusted arithmetic. This
same library is called from many unrelated places throughout the
executable, so it's very likely also the engine behind gold/currency,
not just these three material counters — not confirmed.

**Loot/XP staging pipeline** (fills in how monster kills actually
reach these counters): when a `g_levelMonsters` slot's presence timer
ends, `GrantMonsterRewards` stages that specific monster's own loot
fields (`+0x7E`/`+0x82`/`+0x86`/`+0x8A`) into 4 **global staging
counters** — `0x51BA += [+0x7E]`, `0x5396 += [+0x82]`,
`0x539A += [+0x86]`, `0x51B6 += [+0x8A]` — before `RemoveMonsterFromMap`
wipes the monster. Once `0x51B6` (the 4th staging counter) crosses a
threshold, `ShowLootAndAwardExperience` fires: shows a "found" panel,
drains `0x51BA`/`0x539A`/`0x5396` into the permanent counters
(`0x94B3`/`0x94B7`/`0x94BB` respectively — note the `0x539A`↔`0x5396`
pairing crosses over rather than matching numeric order), then drains
`0x51B6` itself into every valid party member's own `+0x18` field —
plausibly an **experience-points** counter, a new party-record field
find. **Confirmed**: `CheckForLevelUp` reads `+0x18` as packed-BCD XP,
comparing it against an XP-threshold table (`0x9277`, 65 × 4-byte
entries, one per level) indexed by the character's current level
(`+0x16`) — walking forward while XP still clears the next threshold.
A resulting higher level is staged into `+0x1E` (not applied
immediately); `ShowLevelUpMessage` then displays both the current and
pending-new level, confirming `+0x1E`'s "pending level-up" role too
(previously only known to gate a portrait-redraw call).
`CheckAndAnnounceLevelUp` (was `sub_1B7DD`, called from `UseItem` and
`UseTrainingItem`) is the shared wrapper: resolves the active party
slot (`g_selectedPartySlotPtr` → a `g_partySlotAssignment` entry → character id →
`SelectPartyRecordById`), calls `CheckForLevelUp`, and shows
`ShowLevelUpMessage` if `+0x1E` came back nonzero. A direct consumer
of `+0x18`: `UseExperienceBoostItem` (was `sub_1B5FD`, `UseItem`'s
item `[+0xE]` bit `0x400` path) is a one-time-use "tome of
experience" item that adds a fixed packed-BCD amount straight into
every eligible living party member's `+0x18`, gated by its own
one-time-use global flag so it can't be reused. **The narrative
behind these is now confirmed**: the clue book's
`ShowConsumableItemTypeLegend` page (was `sub_13463`, reached via
`WaitForKeypress`/`ShowClueBook`) lists the game's 6 consumable item
categories — POTIONS, SCROLLS, WANDS, VIALS, PARCHMENTS, RODS — each
explicitly described as permanently adding to an attribute or a
skill, exactly matching `UseAttributeBoostItem`/`UseExperienceBoostItem`'s
behavior.

### Global quest/world-state flags

A boolean bitfield array, `g_globalFlags` (base `0x94D1`), accessed
only through 4 small helpers rather than direct bit-twiddling:
`GetGlobalFlagBitAndWord` (index → word+mask, MSB-first within each
16-bit word), `SetGlobalFlag`, `ClearGlobalFlag`, `TestGlobalFlag`.
`TestGlobalFlag` is called directly from `start` at several points,
confirming this is a core, general-purpose mechanism — not something
local to combat. `GrantMonsterRewards` sets or clears one flag per
monster death via its own record's `+0x14`/`+0x16` fields (negative
index = clear, positive = set), so specific monster kills can flip
arbitrary quest/world-state flags (e.g. a boss-defeated flag). No
individual flag indices are identified yet. A separate, structurally
similar but distinct family manipulates *per-object* flag banks at
fixed offsets from a caller-supplied record rather than this global
array. **Follow-up**: traced two of the three accessors.
`GetRecordFlagBitAndWord_10C`/`SetRecordFlag_10C` (were `sub_27A6E`/
`sub_27A3E`) operate on a bank at the caller record's `+0x10C`; the one
traced real caller passes `si=g_currentPartyRecord` (the current party member),
inside `UseItemType_800`'s branch gated on having
enough of material `0x94B3` — plausibly per-character one-time-event
flags (quest steps, items read, NPCs met), not confirmed.
`GetRecordFlagBitAndWord_CA` (was `sub_27AC1`) is the same mechanism at
a *different* offset, `+0xCA`, on an unconfirmed record type. **Follow-
up**: named its own Set/Test accessors too — `SetRecordFlag_CA` (was
`sub_27A4E`, called from `UseTrainingItem`/`sub_25456`) and
`TestRecordFlag_CA` (was `sub_27A66`, called from
`BuildAlchemySpellList`/`MarkIneligiblePartyMembers`) — completing
both flag-bank families symmetrically. Also found and named the
missing Test accessor for the `+0x10C` bank,
`TestRecordFlag_10C` (was `sub_27A56`) — used by an item-target status
display (`CheckPartyMemberItemFlag`) to check whether the targeted
party member has already triggered the current item's personal flag
(same index `SetRecordFlag_10C` uses to mark it), strengthening the
"per-character one-time-event flag" reading. The `+0xCA` bank's
Set/Clear/Test accessors and the `+0x10C` bank's Clear accessor are
still unfound.

**Items can flip up to 6 global flags each**: `ApplyItemEffectFlags`
(was `sub_1BB48`, a shared step called from `UseItem`'s fallback and
several of its type handlers) walks 6 signed flag-index fields in the
current item-use record (`SelectItemUseRecord`'s `es:[si+0x2E]`
onward) — positive sets, negative (negated) clears, zero skips that
slot. The same function also directly manipulates two pairs of 16-bit
flag words, `word_328F6`/`word_328F8` and `word_2E40C`/`word_2E40E`,
with save/restore and set/clear-mask semantics driven by more fields
on the item record — plausibly current player/party status-effect or
equipment-bonus flags, not confirmed.

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

**Found the code that uses this string cluster**: `ShowLockStatus`
(was `sub_17795`) examines a targeted lock and shows `NOT LOCKED`/
`LOCKED`/`MAGICALLY LOCKED`/`LOCKED AND TRAPPED`, or `REQUIRES SPECIAL
KEY: <tier> KEY` — the exact 7-tier hierarchy above, selected by flag
bits on `g_lockStatusFlags`. How much detail is revealed is gated on the
current party member's `+0x6C` field against ASCII-looking thresholds
(`0x37`/`'7'`, `0x41`/`'A'`, `0x50`/`'P'`) — plausibly a lockpicking or
perception skill value (**`ApplySavingThrowEffect`, was `sub_28BD2`,
also passes this same field to `FailsSavingThrow` as a save's
resistance bonus** — fits a general perception/awareness stat better
than lockpicking specifically), not confirmed against `ShowCharacterSkills`'
15-entry skill array yet. **A neighboring field, `+0x6E`**, plays a
similar role for NPC conversations: `ClassifyConversationSkillTier`
(called before every `RunConversation` topic display) compares it
against tiered thresholds to gate how much an NPC reveals — plausibly
a charisma/persuasion-like stat. **A third field in the same cluster,
`+0x6A`**, gates `RepairItemCommand`'s repair-success roll — plausibly
a repair/crafting skill; its message strings confirm the minigame
outright: `YOUR ATTEMPT TO REPAIR THE ITEM HAS FAILED! THE ITEM WAS
DESTROYED.` (critical fail) / `...HAS FAILED.` (soft fail, item
survives) / `THE ITEM IS REPAIRED.` (success). All three (`+0x6A`,
`+0x6C`, `+0x6E`) sit outside the confirmed `+0xCA`–`+0xE9` skill
array, so they're either a separate small cluster of derived/practical
skills or something else entirely — not confirmed. A further,
unidentified skill check gates `ShowLocalAreaMap`/`ToggleMapViewMode`:
`ShowMapSkillTooLowMessage` shows "YOUR SKILL IS NOT HIGH ENOUGH!" —
plausibly a cartography/mapping skill, not yet traced to a specific
field.

**`+0x6C`/`+0x6E` also both feed trap-effect selection**:
`SelectTrapEffectVariant` (was `sub_16DAA`, called twice from
`ProcessMonsterAttackTurn`) picks one of these two fields as the trap effect id to
resolve via `PrepareTrapEffectSlots` — normally `+0x6C` (the
perception/save-resistance field), but a 25% chance
(`RandomInRange(100) < 0x19`) of substituting `+0x6E` (the
charisma/persuasion field) instead, gated on a flag bit (`[+0xC]`
`0x400`) and `+0x6E` being nonzero. Another data point that these are
general character-stat fields reused across multiple systems
(locks, conversation, trap resolution), not single-purpose flags.

### The "side trap"/ambush pipeline (decoded 2026-09-23: it's `g_levelMonsters`, not a separate table)

A second, independent trap/surprise-attack presentation system,
separate from `SelectTrapEffectVariant` above though both ultimately
feed `PrepareTrapEffectSlots`. **Correction to an earlier version of
this section**, which described `ProcessSideTrapsOnMovement` as
walking "an 80-entry wall/cell table" — that 80-entry, stride-`0x9C`
array at `si=0xF26` is **the exact same base address and stride as
`g_levelMonsters`** (`monster.h`'s `MonsterPoolSize`/`MonsterRecordSize`),
confirmed by cross-checking against every other `0xF26`/`0x9C`/`0x50`
walk in the codebase (`HandleMovementInput`'s monster-list scan,
`RefreshDungeonMapWindow`'s scroll relink). This isn't a separate
"wall/cell" concept — it's the live monster pool itself, and the flag
this pipeline checks (`[+0xE]` bit `0x1000`) was long described here as
settable two different ways — **resolved 2026-09-30: there is only
one way.** An exhaustive whole-binary search (both games) for every
site that writes `0x1000` into a pool record's `+0xE` field
(`MonsterFieldWound`/`monster.h`) found exactly one write site in each
game, and it's `ProcessLevelMonsters`' own ambush-arming write (already
fully decoded and reimplemented as `monsterApproachParty`, see "Monster
approach and ambush check" below) — a `RandomInRange(100)` roll against
a threshold selected by a *second*, independent bit range of `+0x94`
(`MonsterFieldAwareness` in `monster.h`) — `0x200`-`0x1000`, distinct
from the already-documented `0x20`-`0x100` "how far it notices the
party" range. **There is no separate wall/door trap creation
mechanism.** A "wall/door trap" pool entry is simply an ordinary
monster record in the ambush-pending state (`MonsterWoundAmbushPending`,
`0x1000`) — the "wall/door" framing describes how it's *experienced*,
not how it's *created*: whether the party ends up seeing a monster
sprite (ordinary combat engagement) or a wall/door-embedded trap
(the mechanism below) depends only on which direction the party
happens to be facing relative to where the ambush was armed, not on
any distinct record type. `monster.h`'s own `MonsterWound` doc comment
had already suspected this connection without confirming it as
definitively as this exhaustive search now does.

`ProcessSideTrapsOnMovement` (was `sub_2278C`, called directly from
`start`, likely once per movement step) fast-exits unless
`g_uiScratchFlags3` bit `0x10` is set, otherwise calls
`TriggerSideTrapForRandomPartyMember` (was `sub_22989`) for every pool
slot with that flag bit. That function picks a random active party
member (`PickRandomActivePartyMember`) and rolls
`RollTrapAvoidanceMagnitude` (was `sub_227F5`) — a save-vs-trap
avoidance check using the party record's own `PartyStatEquipRating5`
(`+0x50`, confirmed by offset arithmetic: `PartyFieldStats + 10*2`;
the same physical-defense stat `combatResolveAttack` already uses)
against the pool record's own `MonsterFieldRangedAccuracy`/
`MonsterFieldRangedDamage` fields (`monster.h`, `+0x64`/`+0x66`) as
threshold/magnitude-cap — already-named ordinary monster fields, not a
separate trap-specific pair. `MonsterFieldApproachGate` (`+0x60`) is
also read here (its own meaning still not confirmed); `+0x62`/`+0x70`/
`+0x78` remain unnamed (the last stored as a pointer-like reference).
**Confirms "trap" pool entries carry a full monster catalog block
too**, with `RangedAccuracy`/`RangedDamage` reused as trap avoidance
threshold/magnitude data instead of an actual ranged attack — a
genuinely deep unification of the trap and monster systems, now fully
confirmed rather than merely suggested. The higher the stat relative to
the threshold, the less likely and smaller the resulting effect.
**Reimplemented 2026-09-30** as `combatRollTrapAvoidanceMagnitude`
(`src23/combat.c`/`.h`) — and reading Chapter 3's copy directly (not
assumed instruction-identical just because the surrounding code is)
found a real, easy-to-miss difference: **Chapter 2 rolls
`RandomInRange(100)`; Chapter 3 rolls `RandomInRange(55)` instead**,
while the final `magnitudeCap * margin / 100` formula stays unchanged
in both — so Chapter 3 traps trigger noticeably more often for the
same margin (any margin ≥ 55 always triggers in Chapter 3, vs. needing
a margin of 100 to always trigger in Chapter 2). The roll happens
*unconditionally* once a slot is flagged, regardless of the party's
current facing — reproduced exactly for RNG-draw-count parity, not
just outcome parity.

The trap only actually *presents* if the party's current facing (the
same `SaveFacing` bit convention as `ShowCompassDirection`) matches one
of 4 direction bits also on `+0xE` (`monster.h`'s
`MonsterWoundPartyMustFace*`, the same bits `monsterApproachParty`
already arms) — i.e. it has to be a wall/door (or ambushing monster)
the party is currently facing, using the record's own `+2`/`+4` world
position (`MonsterFieldWorldX`/`Y`) exactly like an ordinary monster's.
**Reimplemented as `combatResolveSideTrap`** (`src23/combat.c`/`.h`),
combining the roll above with this facing gate as a single decision
function — confirmed instruction-identical to Chapter 2 (aside from
the roll-bound difference already noted). Up to 4 such results are
staged into a scratch table (`0xBC28`), then
`PresentTriggeredSideTrapEffects` (was `sub_2281F`) resolves and
presents them: plays the trap's sound cue once `WaitForSoundDriverIdle`
confirms the driver is free, draws weapon-style icons and does a full
dungeon-screen refresh, picks the highest-severity result to drive a
scaled `AnimateProjectileStep` animation, and finally transfers the
results into the confirmed icon-bar slot table (`0xC50`) via
`ApplyEffectAndDrawIconBar`. `+0x50` being used here as an avoidance
stat is a second, independent data point (alongside
`ComputeAlchemyRefinementYield`'s use of the neighboring `+0x70`) that
the still-open "`+0x4C`/`+0x4E`/`+0x50` trio" are general character
stats reused across systems. **Not reimplemented**: the 4-slot scratch
staging table and `PresentTriggeredSideTrapEffects` itself — purely
UI/drawing/sound sequencing, deferred to the eventual SDL2 layer like
this project's other presentation-only code. Tests in
`tests/test_combat.c` cover the roll's negative-margin early-out (and
that it consumes no RNG in that case), both games' differing roll
bounds via the RNG-peek technique, a concrete same-seed
Chapter-2-vs-Chapter-3 divergence, and all 4 facing-match cases
including the implicit "West" else-branch.

**Key items reference locks by their own catalog type value**:
`UseItem`'s `UseKeyItem` branch passes a key item's own type-flags
field directly as `LoadLockState`'s lock id — a key's catalog "type"
*is* the numbered door it opens, no separate item-to-lock lookup
table needed.

**The actual unlock-a-door command**: `UnlockDoorCommand` (a
`HandleGameCommand` handler, `g_currentActionId` `0x21`-`0x2E`/`0x2F`) uses
`ProbeFacingTile` to find the lock ahead, loads its state via
`LoadLockState`, shows `NOT LOCKED` directly if it's already open
(same bit test `ShowLockStatus` performs), and otherwise compares the
door's required-key flags against the player's currently held key to
resolve the attempt.

### In-game clock/calendar

**Everything below is wall-clock-driven, not turn-based**: `AdvanceGameClock`
and friends are all called from a real `INT 1Ch` timer interrupt
service routine (18.2 Hz hardware tick, ends in `iret`; paired with
the already-named `RestoreInt1cVector`). The ISR multiplexes 5
independent periodic sub-tasks, each with its own `word_3295A` gate
bit and its own countdown reload value: `TickRedrawTimer` (periodic
"mark screen dirty"), `AdvanceGameClock` (the minute tick, see below),
`AdvanceDayNightPaletteFade` (confirms its 113-step fade is driven by
*repeated* ISR calls, not one burst), `AnimatePaletteCycle` (a
palette-cycling animation effect — torch flicker/water shimmer style,
not fully decoded), and `UpdateAmbientMusic` — the 5th sub-task
(`word_32958`, ~1-second period), which switches between day and night
background music tracks based on `g_gameClockMinutes` (the clock) falling
inside or outside `[0x1A4, 0x474]` (7:00 AM–7:00 PM), via the
already-named `PlayMusicTrack`. `g_forcedMusicTrack` is the "forced track"
override this checks (0 = let the ambient day/night system choose):
`RunTitleScreen` sets it to `1` (title music) on entry and clears it
to `0` right at its `E` ("Enter"/leave-the-title-screen) exit point,
handing music control to the ambient system for the rest of gameplay
— and briefly forces `0` (silence) during character creation, restoring
`1` afterward. The
raw ISR entry itself is embedded in bytes IDA hasn't cleanly separated
from a preceding data declaration, so it's documented here rather than
renamed (renaming risks corrupting the disassembly boundary).

`ShowGameClockCommand` (a `HandleGameCommand` handler, `g_currentActionId==7`)
confirms the game tracks a genuine in-game date and time, not just a
coarse day/night or "time of day" value: it fills two fixed template
strings — `12:12 AM` and `12/12/1212` — with the current hour/minute/
AM-PM and month/day/year, via `ComputeGameClockTime`.

**Full mechanism traced**: `g_gameClockMinutes` is the master "minutes since
midnight" counter (0–1439), advanced by `AdvanceGameClock` — the
per-minute clock tick. `ComputeGameClockTime` converts it to a 12-hour
display (`word_32934`="AM"/"PM", `word_32948`=hour 1–12,
`word_3295C`=minute). Past 1440, `AdvanceGameClock` rolls the calendar:
day (`g_gameDay`) wraps at 31 into month (`g_gameMonth`), which wraps
at 13 into year (`g_gameYear`) — a **30-day-month, 12-month-year**
in-game calendar (new-game start: day 4, month 11, year `0x222`=546).
On the day rollover, `ResetDailyAbilityCharges` also zeroes every
party member's 4 special-ability charge fields (`+0xB6`-`+0xBC`,
see `RevealMapRegion`/`UseAbilityScroll`) — special abilities recharge
once per in-game day.

**The "R rest" command**, `RestPartyAndAdvanceClock` (an action-toolbar
entry from `start`, also reached from `ApplyEncodedItemEffect` —
plausibly one of its effect-type branches is a "rest/recover" item
effect): after an
eligibility check (`IsRestingAllowedHere` — rejects on a global flag,
forbidden map/level id, or a special-cell match via
`IsPositionInTriggerList`, confirmed by the "YOU CAN NOT REST HERE"
message), advances `g_gameClockMinutes` directly — a flat `+0x1E0`
(8 hours) for a full/uninterrupted rest, or up to 8 hourly `+0x3C`
ticks (calling `ProcessLevelMonsters` each hour and stopping early if
combat starts) otherwise — then inlines the exact same day-rollover
math `AdvanceGameClock` uses (wraps at `0x5A0`/1440 minutes, calendar
counters wrapping the same way) and calls `ResetDailyAbilityCharges`
on rollover, before resuming play via `RunDungeonGameLoop`. Each hourly
tick also calls `ApplyRestEffectsToCharacter` (was `sub_1E943`) per
party member: skips the incapacitated; if DISEASED or CURSED (among
other `+0x1C` bits), drains HP or MP instead of regenerating it
(DISEASED HP loss reaching 0 sets the DEAD flag) — otherwise applies
normal percentage-based HP/MP regeneration. Uses
`RestoreDialogAreaFromEMS` (shared with `RunGameDialog`) to restore
the status area from an EMS cache before drawing, and
`ClearMessageBoxArea` (shared with `HandleShopCatalogSlotClick` and
`UseAbilityCommand`) to clear the message-box background.
`UseAbilityCommand` also calls `ConsumeAbilityChargeAndRefresh` (was
`sub_17A65`): shows `ShowResourceDepletedOverlay`, then — unless a
flag (`g_lockStatusFlags` bit 1) says otherwise — plays a sound, increments a
counter at `[g_currentToolbarIconPtr+2]` (plausibly the ability's charge/uses
count, alongside the already-known `word_32DC0`/`word_32DC2`
effect-id/threshold parameters feeding `ApplySavingThrowEffect`), and
refreshes the dungeon screen.

**Fully decoded and reimplemented 2026-09-30** (as
`src23/gameclock.c`/`.h` and two new `party.c` functions), reading the
rest command's own logic in full rather than the summary above — which
turned out to have skipped over a real, previously-undiscovered
regen-rate mechanic and two genuine shared bugs.

`IsRestingAllowedHere` (yendor2.asm:26030, yendor3.asm:24610): **not
instruction-identical between games**, despite both being small. Chapter
2's own copy carries a whole extra branch testing a "forbidden map id"
global (`word_34748`) with elaborate range logic (specific ids
`0x45`/`0x46`/`0x93`/`0x94`, plus ranges `[0x3D,0x3E]` and `>0x46`) —
but `word_34748` has **no confirmed write site anywhere in Chapter 2's
disassembly** (a single XREF total, the read here), so it always reads
0 in practice, making the whole branch structurally unreachable.
**Chapter 3 confirms this**: its own copy of the function doesn't have
this branch at all — just the global flag test and the trigger-list
call, nothing else. Reimplemented as `gameClockRestAllowed` without the
dead branch, matching Chapter 3's simpler (and, with real data,
behaviorally identical) shape for both games.

`RestPartyAndAdvanceClock`'s own day-rollover math (inlined, not a call
to `AdvanceGameClock`) contains **two real, previously-undocumented
bugs, both confirmed byte-for-byte identical in Chapter 2 and Chapter
3** — genuinely shared, not one game's mistake the other fixes:
1. The rollover check fires at `minutes >= 0x59F` (1439), one minute
   short of the actual day length (`0x5A0` = 1440). Subtracting 1440
   from a value that's merely `>= 1439` underflows to `65535` whenever
   the pre-subtraction total lands exactly on 1439 — reachable with
   entirely ordinary inputs (e.g. resting for exactly one hour starting
   from minute 1379, a perfectly normal clock reading). The clock stays
   corrupted until the next *natural* per-minute `AdvanceGameClock`
   rollover eventually fixes it (~24 in-game hours later, since that
   function's own rollover only fires at exactly `0x5A1`).
2. When a month rollover (day wraps past 30) also triggers a year
   rollover (month wraps past 12), `month` is left at **13** instead of
   being reset to 1 — unlike `AdvanceGameClock`'s own per-minute tick,
   which does reset it correctly in the same situation. A rare but
   real date-display glitch (`ShowGameClockCommand` would show month
   13) until the next natural rollover corrects it.

Reimplemented as `gameClockAdvance` (`src23/gameclock.c`/`.h`),
reproducing both bugs faithfully rather than correcting them — this
project's standing practice for confirmed original bugs. Tests in
`test_gameclock.c` cover both edge cases explicitly (not just the
ordinary rollover path), plus `gameClockRestAllowed`'s simplified gate.

`ApplyRestEffectsToCharacter`'s regen-rate mechanic goes deeper than
"normal percentage-based HP/MP regeneration" — that phrase was true but
incomplete. The percentage itself (`word_328C2`) is computed once per
`RestPartyAndAdvanceClock` call, *before* the hourly loop (full read,
2026-10-01, `yendor2.asm:25832`-`25870`): count active (non-incapacitated)
party members (`activeCount`), then try to consume one
"camping supply"-range item per active member — up to `activeCount`
times, call `IsItemRangeAvailable(0x36, 0x40)` and, on a hit,
`ConsumeItemChargeResource`, breaking out the first time nothing more
is found; `regenPercent = (100 / activeCount) * consumedCount`. The
item range itself is **confirmed, not guessed**: real `WORLD.DAT` data
in both games shows ids `0x36`-`0x40` are MEAT, BREAD, FOOD, CHEESE,
ALE, and five more plain FOOD entries — literal camping provisions. So
resting really is a *resource-consumed* mechanic, not a free action:
fully supplying every active member with food yields a full 100% regen
tick, partially supplying them yields a proportionally smaller one, and
having none at all yields 0% (no regen, though the status-effect-gated
degen paths below still apply regardless).

**Reimplemented**: `itemRangeAvailable`'s own half (`partyFindItemInRange`/
`itemRangeAvailable`, `src23/party.c`/`.h`, 2026-10-01, see "Quest-item
and party-inventory range checks" above) and `ConsumeItemChargeResource`'s
own default-mode party-inventory half (`partyConsumeItemCharge`,
same file, see the "shared scratch buffer at `0xAFA8`" section above for
the full writeup) — confirmed to be the only mode `RestPartyAndAdvanceClock`
(and every caller except `RepairItemCommand`) ever reaches.

**The full derivation composed, 2026-10-01**: `partyDeriveRestRegenPercent`
(`src23/party.c`/`.h`) ties `itemRangeAvailable`/`partyConsumeItemCharge`
together exactly as `RestPartyAndAdvanceClock` does — count active
members (stopping dead at the first unoccupied party slot, same quirk
as the fallback-inventory scan), then up to that many consume attempts,
`regenPercent = (100 / activeCount) * consumedCount` using the
*original's own* truncation order (divide first, then multiply) rather
than the more natural `(100 * consumedCount) / activeCount` — with 3
active members all fed, that's `(100/3)*3 = 33*3 = 99`, not 100,
reproduced exactly. A match in the 6-entry global table is consumed via
`itemSlotConsumeGlobalCharge` (reimplemented the same day, see the
"shared scratch buffer at `0xAFA8`" section above), so food sitting in
the resource panel counts exactly the same as food in a party member's
own inventory. `partyApplyRestEffects` (`party.c`/`.h`) still takes
`regenPercent` as an explicit parameter rather than calling this
internally, matching this project's established "decide, don't apply"
split elsewhere. Tests in `test_party.c` cover the truncation quirk,
partial feeding, no food at all, an incapacitated member not counting,
the stop-dead quirk, and the global table actually being consumed; all
23 suites pass.

**The 6-entry global-table consumption case reimplemented too,
2026-10-01**: `itemSlotConsumeGlobalCharge` (`src23/party.c`/`.h`) --
ConsumeItemChargeResource's own global-table branch, the exact same
decrement-or-discard logic as `partyConsumeItemCharge` but with no
owning inventory group to deduct weight from (the resource panel isn't
weighed, so discarding the slot is the only state change). Shares a new
`static itemSlotSpendCharge` helper with `partyConsumeItemCharge` rather
than duplicating the decrement/discard logic. `partyDeriveRestRegenPercent`
now consumes a global-table match instead of treating it as a
conservative miss -- the regen-rate derivation no longer under-counts
food sitting in the resource panel. Tests in `test_party.c` cover the
global-table consume function directly (single-use discard, multi-use
decrement) and the composed derivation actually consuming from it; all
23 suites pass.

**Still not reimplemented**: `IsItemRangeAvailable`'s own
container-recursion half (`FindItemInsideContainer`{,`Level2`,`Level3`},
a genuinely separate CURGAME-backed subsystem this project has no
reader for) and `ConsumeItemChargeResource`'s 3 special modes.

The status-effect-gated part of `ApplyRestEffectsToCharacter` — fully
decoded — is genuinely richer than a simple "diseased/cursed drain
instead of regen": the gate tests 6 bits at once
(`Sick`/`Poisoned`/`Diseased`/`Hexed`/`Jinxed`/`Cursed`), but only 4 of
them have their own explicit handling inside it. Sick and Jinxed are
silently *cured* (cleared, no cost at all) every time this runs while
they're set. Diseased drains a flat 36 HP (clamped at 0, setting
`PartyStatusDead` there exactly like `partyDeductHp` already does) —
and if that kills the character, the Cursed check below is skipped
entirely (an early return in the original). Cursed drains a flat 48 MP
(floored at 0, no death). **Poisoned and Hexed have no explicit case
of their own at all** — a character with only one of those two set
still enters this branch (since they're part of the 6-bit gate) but
matches none of the 4 specific sub-checks, so nothing happens: no
regen, no drain, no cure. The practical effect is that Poisoned/Hexed
alone simply *block* normal regeneration for that tick without adding
any explicit penalty of their own here (their own periodic damage, if
any, presumably comes from the separate ailment-tick system,
`TickStatusEffects`, not from this function). Reimplemented as
`partyApplyRestEffects`, reusing the already-existing
`partyDeductHp`/`partyDeductMp` directly rather than re-deriving their
clamp/death-flag logic. Tests in `test_party.c` cover every one of
these cases individually, including the Diseased-kills-so-Cursed-
never-runs sequencing and the Poisoned/Hexed "blocks but doesn't
drain" quirk.

`ResetDailyAbilityCharges`'s own per-record step (zeroing the 4
`PartyFieldAbilityCharge` entries) is trivially reimplemented as
`partyResetDailyAbilityCharges`; `gameClockAdvance`'s own return value
(did a day roll over) tells the caller when to call it for every party
member, matching the original's own call site exactly.

**Still not reimplemented**: the regen-percentage derivation (needs the
item-range-availability subsystem, above); `IsPositionInTriggerList`
itself (a genuinely separate, general-purpose map-trigger table also
feeding `ApplyMapTriggerEffect`, bigger in scope than just this one
caller — not extracted yet); and the hourly loop's own orchestration
(calling `ProcessLevelMonsters` across the full monster pool once per
simulated hour, stopping early if combat starts) — pure orchestration
over an already-decided per-record function
(`monsterApproachParty`/`combatResolveSideTrap`), deferred the same way
combat's own top-level loop is.

### The "word_332D8-33306 encoded effect descriptor cluster" mystery, resolved for good (2026-09-30)

A multi-session-old open question, closed. This project had long known
that `ApplyEncodedItemEffect`'s corridor/ranged-attack branches (and
the map-monster attack family they reach — `ApplyAttackToTarget`,
`TryResolveAttackAgainstTarget`) read a cluster of roughly 14 globals
(`word_332D8` through `word_33306`) that no traced code ever appeared
to *write* — an earlier session's dedicated investigation confirmed via
live IDA cross-reference that none of them are written by a plain `mov`
anywhere in `yendor2.asm`, traced `LoadClueBookSpellEntry` as a
plausible lead, and concluded the mystery was "genuinely unresolved,
not just unstarted."

**The earlier investigation's own conclusion held the answer without
quite reaching it**: `LoadClueBookSpellEntry` really is the write
site — the prior pass just compared its copy destination (`es:0x5A5A`)
against the cluster's own addresses (`0x332D8` etc.) as if they were
directly comparable numbers, when `es:0x5A5A` is a segment:offset pair
that needs real-mode address arithmetic (`segment*16 + offset`) to
resolve to a linear address before it can be compared at all. Doing
that arithmetic: `es` here is loaded from `word_2E4AA`, which is set
*once*, at the very top of `start` (`yendor2.asm:51`), to
`seg seg129` — the game's own main data segment, the exact same
segment every `word_332Dx` symbol lives in. `seg129`'s selector is
`0x2D86`, so `es:0x5A5A` resolves to linear `0x2D86*16 + 0x5A5A =
0x332BA` — and `word_332D0`, the cluster's own first field, sits at
linear `0x332D0`, exactly `0x16` (22) bytes into that same 80-byte
buffer. Checking every field this project has ever referenced from
this cluster (`word_332D0` through `word_33306`) confirms all of them
fall cleanly within `[0x332BA, 0x332BA+80)` — the *entire* cluster is
simply a set of named offsets into the same 80-byte record
`LoadClueBookSpellEntry` copies, addressed two different ways by two
different pieces of code (`es:0x5A5A`-relative during the copy itself,
`seg129`-absolute afterward) that nothing in the disassembly visually
connects. There is no separate, unwritten scratch region at all — the
mystery was a segment-arithmetic illusion, not a missing write.

**The record itself, resolved down to real file bytes**: `LoadClueBookSpellEntry`
(`yendor2.asm:23355`, confirmed instruction-identical in Chapter 3) maps
in EMS page `0x5610` and copies one 80-byte record, by 1-based index in
`ax`, into that buffer — already correctly described elsewhere in this
document as "the spell-data equivalent of `LoadClueBookMonsterEntry`'s
`WORLD.DAT` read," but never connected to the descriptor cluster until
now. The EMS page's own backing data turns out to be a genuine,
already-partially-cataloged `WORLD.DAT` block: `WorldDat_setBlock4`
(`yendor2.asm:43128`, Chapter 3 instruction-identical at
`yendor3.asm:43517`) reads its file offset from a small fixed dword
table (`DS:0xCE6B` Chapter 2, `DS:0xB203` Chapter 3) and its record
size (`0x50` = 80 bytes) as a literal constant. Reading that dword
directly via IDA and then the real bytes at that `WORLD.DAT` offset in
**both** games' real files confirms it beyond any doubt — the first 6
records, byte-identical in both games:

```
[1] HEAL
[2] MAGIC ATTACK
[3] SLING SHOT
[4] COLD SLASH
[5] MINOR WOUNDS
[6] MINER'S LIGHT I
```

This is exactly the game's own spell/ability list — Chapter 2 at
`WORLD.DAT` offset `0x1AA5DD`, Chapter 3 at `0x41B5BF` (both extracted
via `ida_scripts/dump_worlddat_block4_offset.py`, one script per game).
Each 80-byte record: a null/space-terminated name string in the first
22 bytes (confirmed independently — `DrawAlchemySpellList` literally
calls `writeString(bx=0x5A5A)` right after loading a record, printing
straight out of this same buffer), followed by the numeric cost/effect
fields this project's own UI-tracing had *already* identified the
meaning of without realizing they were reading this exact table:
`CheckSpellCastability` reads MP cost (`+0x54` on the caster's own
record, compared against `word_332D2` at buffer offset `0x18`),
`NUORE`/`MAGIC ORE` cost (`word_332D4`/`word_332D6`, offsets `0x1A`/`0x1C`,
each checked via `IsBCDCounterAtLeast`), and `ShowClueBookSpellDetail`
draws "CLASS:"/"LEVEL:"/"MP:"/"NUORE:"/"ORE:"/"AFFECTS:"/"WHEN:"/"EFFECT:"
sections from further fields in the same record (see this document's
"`ShowClueBook`'s full F-key dispatch" section above for the full
label list, written up before this connection was made).

**What this unblocks**: `ApplyAttackToTarget`'s own "di's record type
here is unknown" mystery is resolved for its *inputs* the same way its
*output side* (`combatApplyTargetResistances`) already was — the
attack's own damage/status parameters (`word_332E8` magnitude,
`word_33300`/`word_33302`/`word_33306` flag words) are simply fields of
whichever spell/ability record was most recently loaded by index, the
same "already-resolved value, not rolled" pattern this project has
found repeatedly elsewhere (`combatApplyEncodedItemEffectSingle`'s own
inflicted-status/magnitude parameters, for instance). The remaining
work at the time was "find the write site" only — the full field
layout is now decoded too, see the next section.

### The spell/ability catalog: full field layout decoded, the attack-resolution family reimplemented (2026-10-01)

Picked back up where the previous section left off, now that the
record's identity (whichever 80-byte entry `LoadClueBookSpellEntry`
loaded by index) was settled. Traced every reader of the record —
`ShowClueBookSpellDetail`'s own UI (`CLASS:`/`LEVEL:`/`MP:`/`NUORE:`/
`ORE:`/`AFFECTS:`/`WHEN:`/`EFFECT:`), `ApplyEncodedItemEffect`'s
icon-bar branches, and the `ApplyAttackToTarget`/
`TryResolveAttackAgainstTarget`/`ApplyTargetResistancesToAttack` family
— back to a record-relative offset, then cross-checked every field
against real byte statistics from both games' actual `WORLD.DAT`
tables (every bit named below is set by at least one real record in
both games, except one explicitly noted as unreachable). New module
`src23/spellrecord.c`/`.h` has the full enum and both games' confirmed
layout (`0x1AA5DD`/125 records Chapter 2, `0x41B5BF`/107 records
Chapter 3 — the record count for each game is itself a confirmed named
engine constant, `word_3330C` in Chapter 2's `InitGlobals`/`ds:0x5DF8`
in Chapter 3's, the exact same value `party.h`'s
`partyKnownAbilityIdMax` already used without the connection to this
table being made).

**Three real, previously-unexplained quirks resolved as a side effect
of this trace**:

- `MonsterFieldUnknown4E` (monster.h, "1-13 in real data, not
  identified") turns out to be matched directly against this record's
  own `SpellFieldTargetTypeId` by the attack-resolution family — a
  type-restricted attack is a complete no-op against any monster whose
  own value here doesn't match. What the values themselves group
  (element? creature family?) still isn't identified, but the
  mechanism is.
- `MonsterFieldImmunities` (monster.h) is catalog data everywhere
  except one write: `ApplyAttackToTarget` ORs a landed attack's
  surviving status bits into this same field to mark the target as
  *currently afflicted*, reusing the identical bit positions as
  "permanently immune" — safe because the field is never re-read as
  immunity data after spawn. This is the "unless already afflicted"
  gate an earlier session's comment on a different function had
  already named without making the connection.
- `MonsterStateSpecialAttackDisabled`/`MonsterStateBusy` (monster.h,
  both "exact trigger not confirmed") share their bit positions with
  `MonsterImmuneCursing`/`MonsterImmuneHexing` — successfully cursing
  or hexing a monster via a player attack sets these state bits as a
  side effect of the shared bit position, not a separate mechanism.
  Also named a new bit, `MonsterStateHitFlashPending` (`0x2`, set
  alongside `MonsterStateAware` on every landed attack, cleared the
  next time `DrawMonsterAndUpdateAttackState` renders a one-shot hit
  flash sprite frame).

**Reimplemented**: `combatResolveSpellAttack`/`combatApplySpellAttack`
(`src23/combat.c`/`.h`) compose the whole family — the type-restriction
gate, the already-resolved/rolled magnitude split
(`g_uiScratchFlags4` bit `0x80`, still an untraced caller-context input,
but its own effect is now fully clear), `combatResolveAttack`'s roll
using the target's own `MonsterFieldAbsorption` as defense and the
caster's `PartyStatCasting` as accuracy, filtering through the
already-existing `combatApplyTargetResistances`, and the commit tail
(state bits, health, the tick-timer arm, the afflicted-immunities mark,
the half-target-damage override, the clear-aware quirk). One confirmed
genuinely unreachable branch, reproduced for fidelity anyway:
`SpellResistClearAware` (word_33306 bit `0x20`, clears the target's own
`MonsterStateAware` after the attack) is never set by any real record
in either game. `TickMonsterTimer`'s own gate bits (`MonsterFieldState`
mask `0xFC10`) still aren't armed by this same write, though — what
actually triggers the tick-timer mechanism this feeds remains a
separate, unfound write (monster.h's own long-open note).

**What this resolves in `ApplyEncodedItemEffect`'s own dispatch
(roadmap.md candidate 8)**: `word_33300`/`word_33302`/`word_33306`
(this record's `SpellFieldFlagsA`/`FlagsB`/`ResistFlags`) are confirmed
to be genuine record fields, not per-call caller state as every earlier
round assumed — the ~19-branch dispatch is simply data-driven from
whichever spell/ability id an item encodes, cross-checked against real
data: every record with `SpellFlagsBAttackPath` (`0x2000`) set is a
plain damage/status attack spell in both games (`MAGIC ATTACK`,
`COLD SLASH`, `FEET OF LEAD`, `INSECT REPELLENT`, `ELECTRIC BURST`,
...), confirming it really does route into the attack family above
rather than the icon-bar path. The full dispatch order and the
single/whole-party gate are now resolved too — see the next section.

Tests in `test_spellrecord.c` (22nd suite, new) cover the catalog parse,
bounds, field access, and real-data spot checks (`HEAL`/`MAGIC ATTACK`/
`INSECT REPELLENT`'s exact costs, flags and type restriction, both
games' record counts/offsets, and a sweep confirming every record name
is printable, every cost is plausible, `SpellResistClearAware` is
never set, and at least one record uses the attack path). Tests for
the composed attack family are in `test_combat.c` (type-restriction
block/match, already-resolved vs. rolled magnitude, a missed roll
skipping resistances entirely, the commit tail's every branch including
the Cursing/Hexing shared-bit quirk and the half-target-damage
override). All 22 suites pass.

### `ApplyEncodedItemEffect`'s full dispatch chain read, two open caller-context questions resolved (2026-10-01, same day)

Read `ApplyEncodedItemEffect`'s own dispatch top-to-bottom
(`yendor2.asm:51106`-`51217`) rather than spot-checking individual
branches as previous rounds did. It is a flat, sequential
if-clear-fall-through chain, not a jump table — each `word_33302` bit
is tested in this exact order, and the first one found set wins (no
combined conditions): `0x8000` (single-target, `loc_2C1CF`), `0x4000`
(whole-party, `loc_2C231`), `0x80` (`loc_2C287` — a sound cue plus
`word_36C93`/`95`/`97`/`99`, a field family close enough to `travel.c`'s
own still-undecoded `word_36CB1` group to be worth checking together in
a future pass, not confirmed to be the same mechanism), `0x10`
(`loc_2C2F9`), `0x4` (`loc_2C344` — the teleport-then-engage branch,
corrected below), `0x20` (`loc_2C5D9` — a position-bookmark
save/restore mechanic, new this round, also below), `0x8`
(`loc_2C674` — a third `interactResolveIfOutcome` user, now
reimplemented as `interactTriggerFacingCurgameEvent`, see below), `0x2`
(`loc_2C703` — a thin `RestPartyAndAdvanceClock` wrapper, already
scoped in an earlier round as "rest here"; this round just confirmed
the exact address), `0x1` (`loc_2C71D` — `interactKnock`'s own
trigger, already reimplemented), `0x40` (`loc_2C7BD` — its sibling,
also already reimplemented), `0x2000` (`loc_2C87F` — confirmed by
direct read this round to call `ApplyAttackToTarget` against
`g_activeCombatMonster`, now reimplemented, see below), `0x1000`
(`loc_2C8CC` — the same attack looped over all 3 `g_monsterSlots`
entries, also reimplemented), `0x100` (`loc_2C92A` — **corrected, see
below**: not pure UI after all, a multi-row piercing-projectile attack
that arms `TickMonsterTimer`'s own gate on a hit, not reimplemented),
`0x200` (`loc_2CEE7` — the real "straight-line multi-target attack,"
per-target primitive reimplemented as `combatApplyDamageToMapMonster`,
see below); falling through all of `word_33302` drops to 4 more
`word_33306` bits — `0x8` (`loc_2CCEE` — a projectile-animation variant
of the exact same attack, see below), `0x2` (`loc_2CE62` — a
screen-shake animation that falls straight into bit `0x200`'s own scan
tail), `0x4` (`loc_2D04D` — pure UI/rendering), `0x1` (`loc_2D137` — a
fade animation that also falls into bit `0x200`'s scan tail) — and
finally a true no-op (`loc_2C1C9`, just clears `g_lastKeyChar`). 19
branches plus the no-op, matching the "~19" estimate exactly, and every
one of them now accounted for: only `0x80`/`0x10`/`0x100` of
`word_33302` and `word_33306`'s own `0x4` are genuinely unimplemented
gameplay logic (`0x10` is the held-item cursor, blocked on its own
not-yet-built prerequisite system, confirmed by this round's own direct
read; `0x80` is the world-ailments system's own timer-arming branch —
see the dedicated section below, which resolves the "day/night music
vs. world-state timers" discrepancy this same document flagged a few
hours ago in favor of "world-state timers," now fully confirmed; `0x100`
is the piercing-projectile mechanic just corrected above, see its own
section below); everything else is either reimplemented or
confirmed to be UI/rendering layered on an
already-reimplemented primitive. See below for the full picture on
`0x200`/`word_33306`'s `0x8`/`0x2`/`0x1`.
up the remaining branches.

**Self-correction, same round**: this list's own first draft (written
earlier today) attributed the already-documented "teleport-then-engage"
mechanic (`yendor2.asm:51586`, the `g_wipeEffectX/Y` relocation that
copies a waiting monster into `g_monsterSlots` slot 1) to bit `0x20`.
Rereading both branches end to end to implement the attack family below
found that's wrong: `yendor2.asm:51586` is reached from bit `0x4`'s own
fall-through tail (`loc_2C344`'s direction-counting line-walk, past
`loc_2C4B4`/`loc_2C4D1`), not from bit `0x20` (`loc_2C5D9`) at all.
Bit `0x20`'s own branch does something this project hadn't documented
before: it reads/writes a small bookmark at `g_currentPartyRecord +
word_332FA` — `g_uiScratchFlags1` bit `0x80` set saves the party's
current world position, facing, and 4 more fields (`word_36CAF`/
`word_36CB1`/`word_36CB3`/`word_36C79 & 7`, all UI/rendering-mode
state) into that bookmark; clear restores from it (failing cleanly,
same "can't do that" message as other branches, if nothing was ever
saved). A "mark location" / "recall to it" pair of spell behaviors
sharing one branch, gated by a caller-context flag — resolved the same
round, see below. Corrected rather than left standing, per this
project's own established practice of fixing a wrong claim the moment
it's caught rather than letting it stand until a future round stumbles
on it.

**The caller-context flag traced, and the mechanic named, same round**:
`g_uiScratchFlags1` bit `0x80`'s only setter is `RunAlchemyScreen`
itself (`yendor2.asm:24997`-`25020`), reached *before*
`ApplyEncodedItemEffect` even runs: for a spell with this bit set, the
alchemy screen shows its own confirm prompt (message id `0x22`, not yet
extracted) whose response picks Mark (`ax == 5`) or Return (`ax == 7`)
— a cancel (`ax == 0`) aborts the cast before any cost is deducted.
Real data makes this completely unambiguous: exactly one record in
each game's catalog sets this bit, and it's named, literally, **"MARK
OR RETURN"** — not a hypothesis, the actual in-game spell name. Its own
bookmark-offset field (`word_332FA`, `spellrecord.h`'s new
`SpellFieldBookmarkOffset`, record offset `0x40`) is `0xF0` in both
games — landing exactly in the one genuinely unused gap in `party.h`'s
own field map (`PartyFieldFlagBankCA`'s 32 bytes end at `0xEA`,
`PartyFieldFlagBank10C` starts at `0x10C`), confirming it's real
reserved space being reused, not a collision with anything already
named. Reimplemented as `combatSaveLocationBookmark`/
`combatRestoreLocationBookmark` (`src23/combat.c`/`.h`); the
`ShowConfirmPrompt` call that picks which one to invoke is left to the
eventual UI layer, same as every other confirm-prompt gate this project
has deferred. Tests in `test_combat.c` cover the full 7-field
round-trip and the "never saved" failure case; all 22 suites pass.

**The attack-resolution family finally gets a real caller, same
round**: reading `loc_2C87F` (bit `0x2000`) directly confirmed it calls
`ApplyAttackToTarget` against `g_activeCombatMonster` — exactly the
caller `combat.h`'s own `combatResolveSpellAttack`/`combatApplySpellAttack`
doc comments had been written two rounds ago to anticipate but didn't
yet have. `loc_2C8CC` (bit `0x1000`) is the identical attack applied to
every occupied, still-alive `g_monsterSlots` entry (`MonsterFieldType
!= 0`, `MonsterFieldHealth > 0`) — an area-attack variant. Both
branches share one more real mechanic on a landed hit: a sound event
(keyed by `SpellFieldInflictedStatus`, reused here as a sound-event id,
not a status bitmask — the same "same bytes, different role per
dispatch branch" pattern this project keeps finding in this record) and
a write of `SpellFieldInflictedMagnitude` into the target's own
`MonsterFieldLastAttackMarker` — `monster.h`'s own long-standing
`+0x18`, "referenced by nothing traced so far," finally has a confirmed
writer (still no confirmed reader). Reimplemented as
`combatMarkSpellAttackHit`/`combatApplySpellAttackToActiveSlots`
(`src23/combat.c`/`.h`); the single-slot case needs no new function at
all, just `combatResolveSpellAttack`/`combatApplySpellAttack` against
`g_activeCombatMonster` directly, then `combatMarkSpellAttackHit` on a
hit. **One precondition not modeled**: bit `0x2000`'s own branch only
reaches this attack when the spell record's `SpellFieldResistFlags`
doesn't have bit `0x40` or `0x80` set — either one diverts to a
completely different, untraced branch (`loc_2CF51`) instead; a future
composing dispatcher needs to check this before calling. Tests in
`test_combat.c` cover the marker write, the per-slot skip conditions
(empty, already-dead), and every-slot-hit; all 22 suites pass.

**A third `interactResolveIfOutcome` user found, same day**: `loc_2C674`
(bit `0x8`) turned out to be the exact same probe-then-classify-then-mark
shape as `interactKnock` (bit `0x1`) and bit `0x40`'s own unwrapped
usage — `worldObjectProbeFacingTile`, `interactClassify`, then
`interactBitmapSet` on a match — just with its own qualifying set:
`{InteractOutcomeCurgameFlag10, InteractOutcomeCurgameFlag8}`, the two
*highest*-priority curgame flags (`TryInteractAtPosition`'s own
`0x10`/`0x8`/`0x40`/`0x20` test order, `interact.h`'s top-of-file note)
and, unlike `interactKnock`/bit `0x40`, no lock outcome in its set at
all — this branch can never resolve a door, only a curgame record.
Given its own name, `interactTriggerFacingCurgameEvent`, since its set
is at least internally consistent (both qualifying outcomes are "a
curgame flag fired"), unlike bit `0x40`'s own mixed lock/curgame set
which still has no confident unifying narrative. Tests in
`test_interact.c` cover both qualifying flags, both non-qualifying
flags (bit `0x40`'s and `interactKnock`'s own sets), the
already-triggered no-op, and a lock outcome never qualifying; all 22
suites pass. **Bit `0x2` confirmed, not newly reimplemented**: `loc_2C703`
is exactly the already-scoped "rest here" thin wrapper around
`RestPartyAndAdvanceClock` this project documented in an earlier round
(`gameclock.c`'s own section) — this round just pins down its exact
dispatch address for the map above, no new behavior.

**One branch investigated and deliberately left for its own pass,
same day**: `loc_2CF51`, the diversion bit `0x2000` takes when
`SpellFieldResistFlags` has bit `0x40` or `0x80` set. It's a real,
distinct mechanic — not a dead end — built around the already-named
`ResolveAttackAndLatchFirstHit` (a sibling of `ResolveAttack` that
falls back to the spell record's own `SpellFieldAttackMagnitude` as a
guaranteed minimum hit when a roll comes up a miss) against
`g_activeCombatMonster`, with two distinct tails depending on which of
`ResolveAttackAndLatchFirstHit`'s own two outcomes fired: one *adds*
the rolled damage back to the target's health (a graze/backlash-style
outcome, not yet understood), the other subtracts it normally and
writes the hit marker exactly like the two branches just reimplemented
above. Both tails then feed into a shared icon-bar population step
that reads two more spell-record fields this project hadn't connected
to a consumer before (`word_332E4`/`word_332E6`, record offsets
`0x2A`/`0x2C`, fed to `PrepareTrapEffectSlots` as effect ids) and, in
one sub-path, treats `SpellFieldPositionResetFlags` (`word_332E2`) as a
plain magnitude value rather than the flags word
`ResetOrCopyTargetPositionFields` elsewhere treats it as — yet another
instance of this record's fields being reused differently per dispatch
branch, but one this project doesn't yet have a confident enough read
on to implement without guessing. Recorded here rather than rushed:
`word_332E4`/`word_332E6`'s own role as effect-id sources is confirmed
field-offset information worth keeping, but the branch as a whole is a
better-scoped candidate for its own dedicated pass than something to
finish in the same sitting as three cleaner branches.

**Spell casting's gate and payment (2026-10-03)**: `CheckSpellCastability` and
`DeductAlchemySpellCosts` as `spellCanCast`/`spellDeductCosts`
(`src23/spellcast.c`, new, 25th suite). Context: in combat a spell with FlagsA
`0x400` is refused, out of combat a spell with FlagsB `0x2000`/`0x1000` is
refused -- which finally explains FlagsA `0x400`, set on 42 (Chapter 2) / 41
(Chapter 3) real records: it's "exploration-only" (every projectile, light,
jump, rest, unlock and mark spell), previously filed under "an EFFECT: text
variant". Costs: NUORE and MAGIC ORE must each be at least the cost (zero not
checked), and MP >= cost (signed). Payment subtracts MP *unclamped* and the two
ore counters clamped.

**The player's melee swing and `UpdateMonsterWoundTier` (2026-10-03)**:
`HandleDungeonInput`'s attack button wears the second equipment slot (`0x142`);
a broken weapon cancels the swing; otherwise `ResolveAttack(monster
absorption, EquipRating3 as accuracy, EquipRating4 as power)` -- those two
ratings are the melee-weapon-slot ones, which is the first confirmed consumer
of them -- and a hit sets the wound tier then subtracts the damage unclamped.
`UpdateMonsterWoundTier` is: Light always; Moderate (replacing Light) when the
hit exceeds `(10*max+50)/100`; Severe (replacing Moderate) above `(30*max+50)/100`;
state `|= 0xA`. The tier is per hit -- earlier bits are only cleared when a
higher tier is reached. `combatPlayerMeleeAttack`/`monsterApplyWoundTier`.

**`ProcessMonsterAttackTurn` composed, and the "stale" saving-throw threshold
identified (2026-10-03)**: the monster's whole combat turn is now
`combatProcessMonsterTurn` (tick timer, then either the area-attack loop over
the party or the single target, each through the already-reimplemented
select-variant / resolve-action / apply-effect / corrosion / wear-tick
pieces). Details that only the composition revealed: the area path rolls
every member's attack first and applies the effect slots afterwards (so the
RNG draw order is all-resolves-then-all-applies); a Damage outcome presets the
slot's HP amount so there's no magnitude roll; the staged "damage" is 1 for
status/corrosion outcomes, so it doubles as an "anything landed" flag; the
equipment wear tick runs only for the single-target, non-special case; and it
sets `word_32DC0` -- `RollEffectResistance`'s saving-throw threshold -- to the
monster's own `MonsterFieldSaveDifficulty`, which is exactly the "stale" value
the LIFE FORCE branch later reads (the last monster's DC). The Chapter 3 copy
differs only by one extra UI/driver hook call (`sub_286D8`, a far call gated
on a flag) after a landed attack. Not modeled: sounds/redraws, the key-press
abort, and the party-wipe check `ApplyEffectAndDrawIconBar` runs.

**The whole `ApplyEncodedItemEffect` dispatch is now a function, and it
shows which branches real data uses (2026-10-03)**: `spellSelectBranch`
(`src23/spellrecord.c`) reproduces the flat if-chain (FlagsB bits in the
order 0x8000, 0x4000, 0x80, 0x10, 0x4, 0x20, 0x8, 0x2, 0x1, 0x40, 0x2000,
0x1000, 0x100, 0x200, then ResistFlags 0x8, 0x2, 0x4, 0x1). Run over the real
catalogs it gives, for Chapter 2 (Chapter 3 is within one record of it):
16 single-target, 3 whole-party, 3 light-timer, 2 held-item (CREATE FOOD,
FORGE), 2 teleport (JUMP OVER/THROUGH), 1 bookmark, 1 each of the
curgame-event/rest/knock/lock branches, 19 plain attacks, 4 LIFE FORCE, 13
attack-all-slots, 24 projectiles, **0 for FlagsB 0x200** (bit 0x200 is dead
in real data; the screen-wide scan is reached by the ResistFlags branches:
7 beams `0x8`, 2 tremors `0x2`, 4 rains `0x4`, 1 turbulence `0x1`), and 20
empty records (ids 106-125 in Chapter 2). So every branch with real users is
now implemented or has its decision/mechanics decoded.

**Held item (bit `0x10`, CREATE FOOD / FORGE)**: only if the cursor is empty
(`g_heldItemType == 0`; otherwise an error line); item id = record word
`0x2E` if nonzero (55 = BREAD for CREATE FOOD, 586 for FORGE) else
`RandomInRange(word 0x34 - word 0x32) + word 0x32` (unused by real data);
extra word = `0x30`. `spellCreatedItemRollBound`/`spellCreatedItem`.

**Teleport (bit `0x4`, JUMP OVER / JUMP THROUGH) and the "10 unidentified
bytes"**: words `0x36/0x38/0x3A/0x3C/0x3E` are the jump distances
forward/backward/left/right/through (first nonzero wins), settling the
long-open `0x36-0x3F` range -- with the usual caveat that ~20 projectile and
area records have the same bytes nonzero as animation parameters (one more
per-branch union). Forward/back/left/right walk cell by cell (each cell's
wall type must classify as Normal, except intermediate cells of type 0/1,
and its floor type must be passable); Through hops once and checks only the
destination. `spellResolveJump` (`src23/spelljump.c`, new, 24th suite)
decides the landing cell through a cell-lookup callback; the party move,
`RevealCellsAroundPlayer` and the pull of a monster standing on the
destination into a combat slot aren't modeled.

**Screen-wide attack**: the `0x200` scan, and the tremor/rain/turbulence
branches after their animations, all `jmp loc_2CEED`: optionally the 3
combat slots, then the monster at viewport depth 0x32, 0x30, 0x2F..0x00, each
through `ApplyDamageToMapMonster`. `combatApplyScreenWideAttack`. The beam
(`0x8`, `loc_2CCEE`) is a single-monster hit with the same attack, marker
only on a landed hit and a kill check only after one -- exactly
`combatApplyDamageToMapMonster` again.

**Bit `0x100` is the projectile-spell family; its per-monster hit is
reimplemented (2026-10-03)**: listing the real records with word_33302 bit
`0x100` gives SLING SHOT, FIERY ARROW, POISON ARROW, LIGHTNING BOLT, the
BALL/CLOUD/BLOCK-OF spells and so on (24 in Chapter 2, 23 in Chapter 3) --
every "fires down the corridor" spell. The branch (`loc_2C92A`) is mostly
animation: it steps `g_viewportRowDepth` down 0x31/0x2E/0x2B/0x28/0x24/0x19,
calls `ClassifyObstacleAtViewportRow` at each, and on a monster
(errorCode 4) runs the hit. Bit `0x800` ("piercing", the 0x900 records:
HEX MONSTER, LINKED LIGHTNING, FINGER OF FLAME, SHARD OF ICE, POWER SURGE,
BEAM OF DEATH) continues to the next row after any hit, even a kill (the
`g_uiScratchFlags1` 0x40 flag only skips a redundant animation step); bit
`0x400` (SHRAPNEL, the BALLs and CLOUDs) replaces the single hit with
`ApplyAttackAlongCorridorLine` over three rows (not traced; it reads
record words `0x2A`/`0x2C`/`0x28` as picture id, frame count and sound).
The hit is `ApplyAttackToTarget` plus a marker write and, when
`SpellFieldAttackFlags` has `0x10` (only BLOCK OF ICE/FIRE/ELECTRICITY/
POWER), a damage-over-time arming: monster `+0x1A` (`MonsterFieldTickTarget`)
gets record word `0x30` if the monster's `MonsterFieldAnimSet` is 0xA else
`0x32` (the `0x30-0x33` bytes that were "referenced by nothing traced" --
by their values, 131/211 for ICE, 130/210 FIRE, 132/212 ELECTRICITY,
133/213 POWER, overlay-sprite picture ids), `+0x1C`/`+0x1E` get the tick
amount/countdown, state bit `0x10` is set (inside `TickMonsterTimer`'s
0xFC10 gate), and with `SpellFlagsAPersistAffliction` a fixed `0x10` is
OR'd into `MonsterFieldImmunities`. The kill check runs after any landed
hit or any piercing projectile, and is skipped only for a plain miss from
a non-piercing one. Reimplemented as `combatApplyProjectileHit`
(`src23/combat.c`/`.h`); four tests.

**The splash variant (bit `0x400`) is reimplemented too**: it runs
`ApplyAttackAlongCorridorLine` three times over row triples (the start rows
come from a small table keyed by the current depth, e.g. 0x24 -> 0x18/0x23/
0x27), each calling `ApplyAttackToTarget` per occupied row and then
`ReapplyDamageWithCompoundedResistance`, which is a genuine *second* damage
application on the same target: health -= damage again (floored at 0,
signed), with the damage halved once per set bit of `ResistFlags & 0xFE00 &
MonsterFieldResistances` (compounding; the first pass halves at most once),
state bits 0x3 re-set and the status flags re-OR'd (a no-op). A sweep over
all 80 `g_levelMonsters` entries then rewards and removes everything at
health <= 0. Whether the double hit is design or artifact is unknowable;
reproduced as `combatApplySplashHit` and `combatReapDeadMapMonsters`, five
tests. What remains of the projectile branch is only the row-walking/
animation shell around these per-monster steps.

**`loc_2CF51` resolved later (2026-10-03): it's the LIFE FORCE spells**:
dumping the real records that take it (word_33302 bit `0x2000` *and*
`SpellFieldResistFlags & 0xC0`) gives exactly LIFE FORCE I-IV in both
games, which made the "graze/backlash" tails legible. `0x40` (I-III)
means the caster pays; `0x80` (IV) means the whole party does.
`ResolveAttackAndLatchFirstHit` is "roll; if the roll is 0, use
`SpellFieldAttackMagnitude` as the damage anyway and set errorCode". A
landed roll subtracts the damage from the monster (and writes the
`+0x18` marker and the hit-flash bit, but not Aware); the fallback *adds*
it, healing the monster. The two tails then pick a trap-effect id from
record words `0x2A` (hit; 24 in both games, "costs HP, may inflict Sick")
and `0x2C` (fallback; 32, "costs HP") -- the same offsets bit `0x80`
reads as light-timer slot and duration, so this record region is a
per-branch union just like `0x22`-`0x28`. `0x28` is the HP amount here
(82 in all four records; RESURRECT's `0xFFBF` in the same field is a
status-clear mask for `ApplyIconBarStatDelta`).

**A quirk reproduced rather than fixed**: the branch stages the icon slot's
`+0xE` word -- which `ApplyEffectCost` ORs into the recipient's status
flags -- with `g_stagedAttackDamage`. So the damage value's own bits
become status bits (damage 70 = `0x46` sets Dead|0x4|0x2). Whether
that's an original bug or intent isn't knowable from the code; the
disassembly is unambiguous (`mov [di+0Eh], ax` straight after loading the
staged damage), and `combatApplyEffect` already ORs its status argument,
so the reimplementation just does what the original does. The
whole-party path zeroes both words for a Dead recipient instead, which
makes that slot fall through to the full RollEffectMagnitude/
RollEffectResistance pair against a stale saving-throw threshold
(`word_32DC0`, set by whichever lock/trap last wrote it) -- effect 24
inflicts Sick, so an RNG draw happens even for the dead member. Hence the
`savingThrowThreshold` parameter. Reimplemented as
`combatApplyLifeForceSpell` (`src23/combat.c`/`.h`), with the shared
per-slot roll-or-use-preset logic factored out of
`combatApplyTrapEffectToRecipient` into `combatApplyEffectSlot`; four
new tests in `test_combat.c` (hit, fallback-heals-the-monster, party
variant with the stop-dead scan and the dead-member RNG draw, unknown
effect id).

**`ApplyDamageToMapMonster` finally traced, closing a gap left open
since an earlier round, same day**: this function (`yendor2.asm:52979`,
instruction-identical in Chapter 3) was already named and had a
one-line summary comment from an earlier session ("applies damage...
to a dungeon-corridor monster... via sub_2D498/sub_2D4B6, not traced"),
but a direct read shows those two unnamed callees are simply
`ApplyAttackToTarget` itself — this function is exactly
`combatResolveSpellAttack`/`combatApplySpellAttack`/
`combatMarkSpellAttackHit`, the same sequence this project already
built for combat-slot targets, plus one real step those don't reach:
if the target's health is <= 0 afterward, `GrantMonsterRewards`/
`RemoveMonsterFromMap` (`monsterGrantRewards`/`monsterPoolRemove`, both
already reimplemented) finish the kill. Reimplemented as
`combatApplyDamageToMapMonster` (`src23/combat.c`/`.h`).

**A second, older misattribution corrected along the way**: tracing
this function's own caller (`ApplyEncodedItemEffect`'s word_33302 bit
`0x200`, `loc_2CEE7`) confirmed it — not bit `0x4` — is the
"straight-line multi-target attack" mechanic an *earlier* round's
comment (on `GetMonsterAtViewportRow`, predating this session) had
flagged matching `ShowClueBookSpellDetail`'s "IN A STRAIGHT LINE"
targeting text. Bit `0x4` is the teleport-then-engage branch (see this
file's own correction above); bit `0x200` is the real corridor-attack
entry point, confirmed by direct read: it either loops the 3
`g_monsterSlots` entries (when `g_uiScratchFlags4` bit `0x1000` is set)
or scans up to 3-plus-30 consecutive viewport depth rows via
`GetMonsterAtViewportRow`, applying `ApplyDamageToMapMonster` to
whatever's found at each. Neither loop is composed here -- the per-target
primitive is the bounded, reusable piece reimplemented this round; the
surrounding scan shape is left for whoever picks up bit `0x200`'s own
dispatch wiring. Tests in `test_combat.c` cover the survive/die/miss
cases, including the reward-staging and record-zeroing on death; all 22
suites pass.

**The remaining three `word_33306` bits checked too, same day — all
reduce to the same primitive**: `0x2` (`loc_2CE62`) and `0x1`
(`loc_2D137`) both turn out to be pure animation wrappers (a
screen-shake scroll effect and a fade-to-black dim effect,
respectively) that, once the animation finishes, `jmp` straight into
`loc_2CEED` — the *exact same* `g_monsterSlots`/`GetMonsterAtViewportRow`
scan tail bit `0x200` itself starts at. These two bits don't reach a
different mechanic at all, just a different screen effect before the
identical attack. `0x8` (`loc_2CCEE`) is its own thing — a projectile
sprite flown across up to 5 viewport rows via `ClassifyObstacleAtViewportRow`,
stopping at the first wall or monster it hits (`errorCode == 3` or
`4`) — but its own hit-resolution tail (`loc_2CDD1`) is, field for
field, identical to `ApplyDamageToMapMonster`: `ApplyAttackToTarget`,
then the hit marker write, then the same health-check-gated
`GrantMonsterRewards`/`RemoveMonsterFromMap`. So all four of these
branches — `word_33302`'s `0x200` and `word_33306`'s `0x8`/`0x2`/`0x1`
— share the one already-reimplemented `combatApplyDamageToMapMonster`
primitive this round built; what's left for every one of them is
orchestration and animation (screen-shake, fade, a flying sprite, a
scan loop), not gameplay logic. `ApplyEncodedItemEffect` itself ends
right after bit `0x1`'s own branch (`yendor2.asm:52748`), confirming
the dispatch chain this file documents is now completely accounted
for, branch by branch, even where a given branch's full UI
orchestration isn't reimplemented.

### The "world ailments" system: the light-source burn-down mechanic reimplemented, the rest scoped (2026-10-01, same day)

Followed `word_33302` bit `0x80` (`loc_2C287`) the rest of the way,
since it was the one dispatch branch left with a genuine semantic
question mark rather than a clean "blocked on a UI prerequisite"
answer. It fully resolves the discrepancy flagged a few paragraphs
up — this branch, and the wider mechanism it belongs to, really is
the "world-state timers" an older round's comment called it, not
day/night music selection as this round's own first guess had it.

**The branch itself**: `bx = spellGetU16(record, 0x2A)`
(`spellrecord.h`'s own still-unnamed `word_332E4` offset) selects 1 of
6 slots (`word_36C93`/`95`/`97`/`99`/`9B`/`9D`, set to the record's own
`SpellFieldAttackMagnitude`-adjacent field), each paired with a
`word_36C79` bit (`0x8100`/`0x8080`/`0x8040`/`0x8020`/`0x8010`/`0x8008`
— note every one of these also carries `word_36C79`'s own `0x8000`
"something is pending" bit, OR'd in via the same immediate rather than
as a separate step) and, critically, sets `word_3295A` bit `0x800` —
the exact flag `MaybeForceTickWorldAilments` (`yendor2.asm:27680`,
called from `ApplyMapTriggerEffect`'s already-documented ailment-tick
branch and from `RestPartyAndAdvanceClock`) tests before calling
`TickWorldAilments` at all. This is the arming write this project
didn't have before — `ApplyMapTriggerEffect`'s own ailment-tick branch
and the "R rest" command's own `MaybeForceTickWorldAilments` call were
both already reimplemented as *readers* of this gate without this
project knowing what, besides the natural 5-minute clock tick, could
set it.

**`TickWorldAilments`** (`yendor2.asm:27799`, called every 5 game-minutes
from `AdvanceGameClock` as well as forced via the gate above) turns out
to manage at least three genuinely separate counter families, confirmed
by direct read but not yet reconciled into one coherent narrative:

1. A 6-entry table at `DS:0x9519` (4 bytes/entry: a code byte matching
   `TickAilmentDuration`'s own `9`/`0xF`/`0xC` test, then a 2-byte
   duration), ticked by `TickAilmentDuration` and, on expiry,
   decrementing one of 3 global per-type counters (`0x9425`/`0x9429`/
   `0x942B`) and clearing a `word_36C79` bit (`0x2000`/`0x800`/`0x400`
   via masks `0xDFFF`/`0xF7FF`/`0xFBFF`).
2. The *same* `TickAilmentDuration` function also sweeps every party
   member's 8 main inventory slots (`PartyFieldInventory`-adjacent,
   `[+0x11A]`) with the same 3 codes -- ailments apparently occupy the
   same slot storage as real inventory items, not a separate per-member
   field. Not cross-checked against `item.h`'s own inventory-slot
   layout yet.
3. `TickWorldAilmentTimers` (`yendor2.asm:27925`), gated on
   `word_36C79` bit `0x8000`, ticks 6 *more* counters at `DS:0x9433`,
   each paired with a `word_36C79` bit in the `0x8`-`0x100` range --
   note these bit positions overlap numerically with the `0x8008`-`0x8100`
   constants bit `0x80`'s own branch (above) writes, but the counter
   *array* (`0x9433`) is a different address from that branch's own
   targets (`word_36C93`-`9D`). Whether `0x9433`'s 6 entries and
   `word_36C93`-`9D`'s 6 globals are the same data accessed two ways, or
   two genuinely parallel 6-slot systems, isn't resolved.

`TickWorldAilments` finishes by summing a *third* family of 12 fields
(`word_36C83` through `word_36C9D`) and clearing `word_3295A` bit
`0x800` (disarming its own gate) only when the sum is 0 -- confirming
all 12 really do belong to one "is anything world-ailment-related still
active" accounting, even though they're written by at least 3
unrelated-looking code paths:

- `word_36C85`/`89`/`8B`: 3 timed light-source duration counters -- **the
  `8`/`9`, `0xE`/`0xF`, `0xB`/`0xC` ability-id pairs are confirmed, not
  guessed**: they're literal item ids, checked directly against real
  `WORLD.DAT` item records in both games: `8`=CANDLE, `9`=LIT CANDLE,
  `0xB`=LIGHT SOURCE, `0xC`=LIT LIGHT, `0xE`=TORCH, `0xF`=LIT TORCH (a
  third id per family also exists in the catalog -- `10`/`0xD`/`0x10`,
  USED CANDLE/LIGHT/TORCH -- but neither function here writes it; see
  below). `ApplyStatusEffect` (`yendor2.asm:29214`, a `HandleGameCommand`
  top-level handler -- lighting one of the 3) arms the matching counter
  and `word_36C79` bit (`0x2000`/`0x800`/`0x400`); `TickStatusEffects`
  (`yendor2.asm:29153`, called once per `TickWorldAilments` 5-minute
  sweep) decrements the matching counter by exactly 1 per call (no
  elapsed-time parameter -- duration is counted in 5-minute ticks, not
  raw minutes) and, at 0, clears the flag (the light goes out).
  Reimplemented as `lightSourceApply`/`lightSourceTick` (new
  `src23/lightsource.c`/`.h`); tests in `test_lightsource.c` (23rd
  suite) cover every light source, the "relighting extends rather than
  resets" behavior, expiry, and that ticking one never touches the
  others. All 23 suites pass.
- `word_36C83`/`87`/`8D`: unconditionally zeroed by both of the
  functions just above whenever their own selector doesn't match --
  no write setting them to anything *else* found yet, so either dead
  weight kept for the sum's sake or written by a caller not yet traced.
- `word_36C93`/`95`/`97`/`99`/`9B`/`9D`: bit `0x80`'s own 6 slots,
  above.

**What's still deliberately not reimplemented**: the item-slot-level
transition (an actual CANDLE/TORCH/LIGHT SOURCE item in the 6-entry
table or a party member's inventory advancing from unlit to lit to
used, `TickAilmentDuration`'s own job, items 1 and 2 above) is a
*separate* mechanic from the 3 standalone counters just reimplemented
-- confirmed by direct read that neither `ApplyStatusEffect` nor
`TickStatusEffects` touches an item slot at all, only these 3 scalars
and `word_36C79`.

**`IsItemRangeAvailable` reimplemented too, same day** -- see "Quest-item
and party-inventory range checks" above for the full writeup
(`partyFindItemInRange`/`itemRangeAvailable`, `src23/party.c`/`.h`,
tests in `test_party.c`). This is the exact "item-availability"
dependency `RestPartyAndAdvanceClock`'s own regen-rate derivation
waited on -- that derivation is now fully composed too
(`partyDeriveRestRegenPercent`, `src23/party.c`/`.h`, see the "R rest"
section above for the complete writeup).

**The item-slot tick/transition logic reimplemented too, 2026-10-01**
(`TickAilmentDuration`, item 1/2 above): confirmed genuinely separate
from `lightSourceApply`/`Tick`'s own standalone duration counters (both
happen to clear the same `word_36C79` bits, but neither function
touches the other's state) -- reimplemented as `lightSourceTickItemSlot`
(`src23/lightsource.c`/`.h`, extending `LightSourceState` with a new
`instanceCount[3]` field for the 3 global per-type counters,
`word_9425`/`9429`/`942B`). Operates on the same generic 4-byte item
slot shape `TickWorldAilments` sweeps both the global table and party
inventory with: on expiry, the slot's own id advances by 1 (9->10,
0xC->0xD, 0xF->0x10 -- confirmed against real `WORLD.DAT` data: USED
CANDLE/LIGHT/TORCH) rather than being zeroed outright, `instanceCount`
for that light source decrements, and only once that count itself
reaches 0 does the shared `litFlags` bit clear -- so multiple lit
instances of the same light source can coexist, with the UI "currently
lit" indicator staying on until the last one burns out. Tests in
`test_lightsource.c` cover an unrelated item being a no-op, a normal
decrement, expiry advancing the item id and decrementing the instance
count, multiple coexisting instances keeping the flag lit until the
last expires, and the exact-boundary elapsed-time case; all 23 suites
pass.

**The "unreconciled timer array" and the "three counter families" were
one segment-offset alias (2026-10-02)**: IDA's `word_36C93`..`9D`,
`word_36C85`/`89`/`8B` and `word_36C79` are the very same words as
`DS:0x9433`.., `DS:0x9425`/`9429`/`942B` and `DS:0x9419` (`0x36C93 -
0x2D860 = 0x9433`, etc.) -- the same linear-vs-offset trap that hid the
spell-record cluster. So `TickWorldAilmentTimers`' 6-timer array **is**
bit `0x80`'s 6 slots, and `TickAilmentDuration`'s "global per-type
counters" **are** `word_36C85`/`89`/`8B` -- one field, not two. They're
a *count of lit light sources* per kind (incremented by
`ApplyStatusEffect`, decremented by `TickStatusEffects` and by item
expiry), not a duration; `LightSourceState`'s separate `duration` and
`instanceCount` fields were merged into one `litCount` (a correction to
this session's own earlier model). Real data pins bit `0x80`'s meaning
too: the only records in either game that set it are MINER'S LIGHT I,
MINER'S LIGHT II and INFINITE ILLUMINATION -- it arms one of 6 light
timers (`SpellFieldTimerSlot`/`TimerDuration`, record offsets `0x2A`/
`0x2C`; slot/duration 3/15, 5/21, 6/48 in Chapter 2, 4/15 for LIGHT I in
Chapter 3). Reimplemented as `lightSourceArmSpellTimer`/
`lightSourceTickTimers` (`lightsource.c`); the whole "world ailments"
system is a *lighting* system. Tests in `test_lightsource.c`.

What's left in this whole system: container recursion. Recording the full
confirmed address/field map here is the honest contribution for the
parts not yet done; implementing a C module on top of an unconfirmed
data model risks baking in a wrong structure for what's left.

### A self-correction, and `TickMonsterTimer`'s gate-bit setter found (2026-10-01, same day)

Picked bit `0x100` (`loc_2C92A`) back up while looking for a quick,
bounded piece to close out, since an earlier pass this same day had
classified it as "pure UI/rendering, a weapon-select icon redraw" after
reading only its first ~45 lines. Reading the rest of it found that
conclusion was wrong — those first lines are setup before a real
mechanic, not the whole branch. **Corrected rather than left standing**,
per this project's own practice.

The real branch: a multi-row piercing projectile (`AnimateProjectileStep`/
`ClassifyObstacleAtViewportRow` across up to 5 viewport rows, same shape
as `word_33306` bit `0x8`'s own single-shot projectile), except on a
hit it checks `word_33302` bit `0x800` — set, it keeps scanning past
the hit monster for more targets (a piercing arrow); clear, it stops
after the first. Both cases call `ApplyAttackToTarget` per target hit,
same as every other branch in this family. **The genuinely new piece**:
on a landed hit, this branch also arms a mechanism no previously-traced
caller ever set — `monster.h`'s own `TickMonsterTimer` gate (state bits
within mask `0xFC10`). It sets state bit `0x10` (inside that mask),
writes `MonsterFieldTickAmount`/`TickCountdown` from the attacking
spell record's own fields (the same two fields the ordinary attack
family already reuses), and writes `MonsterFieldTickTarget` from one of
two spell-record fields selected by the target's own
`MonsterFieldAnimSet` value — plus, gated on a separate flag, marks a
*fixed* bit `0x10` into `MonsterFieldImmunities` (not the attack's own
filtered status flags the way `combatApplySpellAttack` does it). This
answers, for the first time, "what sets `TickMonsterTimer`'s own gate
bits" — a specific, fairly narrow player-triggered attack variant
(apparently a burning/piercing arrow effect), not a general monster-AI
mechanism as this project had speculated might be the case. See
`monster.h`'s own updated doc comment on `monsterTickTimer`.

**Not reimplemented**: the piercing-projectile mechanic itself (the
scan/pierce logic, the `MonsterFieldAnimSet`-based field selection, and
the fixed-bit-`0x10` immunity mark) is genuinely its own thing, distinct
enough from the already-reimplemented attack family that forcing it
into `combatApplySpellAttack`'s existing shape would misrepresent it —
a good candidate for a future, narrowly-scoped pass once more of its
remaining unknowns (what `MonsterFieldAnimSet`'s value actually
selects between, what the fixed immunity-bit mark represents
narratively) are worth chasing.

**Two real caller-context questions this project had flagged as
"untraced" are resolved by this same read**:

- **`g_uiScratchFlags4` bit `0x80`** (`combatResolveSpellAttack`'s own
  `alreadyResolved` parameter): its two setters are
  `ApplyEncodedItemEffect`'s only two callers, and they disagree on
  purpose. `RunAlchemyScreen` explicitly *clears* it
  (`yendor2.asm:25055`) right before the call, for a player-cast
  alchemy spell. `InteractWithContainer` explicitly *sets* it
  (`yendor2.asm:53515`, restored right after), for a container/trap
  effect. A clean, complete answer: player-cast spells always roll;
  container/trap effects always use the record's own preset magnitude.
  Neither caller is reimplemented (both are UI-heavy top-level command
  handlers), but the input itself is no longer a mystery.
- **The single-target/whole-party gate** (`SpellFieldFlagsA` bits
  `0x800`/`0x1000`, `spellrecord.h`'s `SpellFlagsAPositionReset`/
  `IconBarPresetAmount`): confirmed by direct read
  (`yendor2.asm:51241`-`51305`) rather than inferred. In the
  single-target branch, if *neither* bit is set, the branch does
  nothing at all — no icon-bar call, no effect of any kind. In the
  whole-party branch there's no such all-or-nothing gate: the icon-bar
  call always happens once after the loop; these same two bits there
  only control whether each individual recipient's icon-slot position
  fields get freshly populated or left as whatever a prior call left
  behind (a "genuine loose end" this project already knew about from
  the other side, now explained from this side too). `combat.h`'s own
  doc comments for `combatApplyEncodedItemEffectSingle`/`Party` spell
  out exactly when a future composing dispatcher should call each one.

No code changes beyond the documentation/naming corrections this
finding implied (`spellrecord.h`'s `SpellFlagsAIconBarGate1`/`Gate2`
renamed to `SpellFlagsAPositionReset`/`IconBarPresetAmount` with the
precise mechanics above); the dispatch itself and its remaining
untraced branches are still not reimplemented. See `roadmap.md`
candidate 8 for the updated status.

### `ApplyMapTriggerEffect`: dispatch structure and real table data resolved, C reimplementation still pending (2026-09-30)

Read `ApplyMapTriggerEffect` (yendor2.asm:17458, yendor3.asm:19629;
called once per movement step from `HandleMovementInput`, and — new
finding today — also called by Chapter 3's own `TravelToDestination`
on arrival, see below) in full while chasing `IsPositionInTriggerList`'s
real record shape for `gameClockRestAllowed`. This closes several
loose threads at once, but opens a genuinely bigger one.

**`IsPositionInTriggerList`'s real record shape, resolved**: its own
disassembly (`yendor2.asm:17681`) only ever reads/compares `[di]`/
`[di+2]` (the x-or-y coordinate and the match-selector flag), and its
`di` register is left pointing at the *matched table entry* on success
(it's never restored before `retn`) — an implicit output the function
doesn't document as such, but its only real caller,
`ApplyMapTriggerEffect`, immediately reads further fields off that same
`di` (`+4` through `+0x12` depending on the game and branch). So the
table's true per-entry record is wider than the 4 bytes
`IsPositionInTriggerList` itself touches — Chapter 2's own 8-byte
stride (`DS:0xD1C9`) carries 2 more data words (`+4`/`+6`) beyond the
coordinate/flags pair; Chapter 3's 20-byte stride (`DS:0xB71F`) carries
considerably more.

**`ApplyMapTriggerEffect`'s Chapter 2 dispatch** (on the matched
entry's own `[+2]` flags field, tested high-bit-first): `0x4000` =
teleport (`[+4]`/`[+6]` → `g_partyWorldX`/`Y` directly, full redraw,
no unlock gate at all — a *simpler*, unconditional teleport, distinct
from the `WorldObjectFlagUnknown2000`/`TravelToDestination` fast-travel
system); `0x2000` = an "ailment tick" effect
(`MaybeForceTickWorldAilments` + redraw — ties into the same
not-yet-scoped world-ailments system candidate 8's bit `0x80` needs);
`0x1000`/`0x800`/`0x400` = a **fixed** effect id (`0xF`/`0x10`/`0x11`
respectively) applied via the icon-bar pipeline to every eligible
(non-incapacitated) party member, sourcing the icon slot's own
`+0x10`/`+0x12` fields from the *same* `[+4]`/`[+6]` words the teleport
branch uses for X/Y (a real field reuse, not a coincidence — confirmed
by reading both branches directly); `0x100`/`0x200` (tested together,
`"0x300-pair"`) = a **data-driven** effect id (fixed `0x2B` if `0x200`
is set, else `1`) applied via `ApplyTriggerEffectIconSlot` instead of
the ordinary icon-bar path — and `ApplyTriggerEffectIconSlot` turns out
to already be documented elsewhere in this file as the exact same
mechanism behind `ApplyItemEffectIconSlot`/`partyHandleIconBarItemExpiry`
(the equipped-item wear/corrosion write-back, effect id `0`'s own
`EffectModeItemReplace` dispatch) — so this branch is a **map-triggered
equipment-corrosion event**, gated per party member by testing
`[recordBase + matchedEntry's own +4 field]` nonzero (the trigger
record's `+4` field doubles as a configurable party-record byte offset
to check eligibility — presumably "is a corrodible item equipped in
this slot", though the exact field identity isn't pinned down); no
bits set at all falls through to a **fully data-driven** default,
reading the effect id directly out of the matched entry's own `[+4]`
field rather than a fixed constant.

Every branch that doesn't fall into the ailment-tick or teleport cases
funnels into the same already-reimplemented icon-bar pipeline
(`PrepareTrapEffectSlots` is already known to be `effectGetDef`'s
original name — see `effect.h` — and the icon-bar population itself
matches `ApplyEffectAndDrawIconBar`'s existing 3-way dispatch this
project completed weeks ago), so *most* of the pieces this function
would need are already sitting in `src23/effect.c`/`party.c`/`combat.c`.

**A genuine, substantial Chapter 3 divergence, now confirmed against
real data, not just a wider record**: Chapter 3's teleport branch
(`0x4000`) is dramatically richer than Chapter 2's simple direct
assignment — it populates `ds:0xCF75`/`0xCF77`/`0xCF73`/`0xCF2F`/
`0xCF31`/`0xCF33`/`0xCF3F`, the **exact same globals**
`TravelToDestination` populates from its own destination-table records
(see "Party teleport/fast-travel destinations" above), and even calls
`TickTravelResourceAilments` — the same helper `TravelToDestination`
calls. This is presumably why Chapter 3's `TravelToDestination` calls
`ApplyMapTriggerEffect` at its own tail end (already noted as a
Chapter-3-only addition earlier in this document, previously
unexplained) — arriving at a fast-travel destination in Chapter 3 also
re-checks the *new* cell for its own map trigger (you might teleport
onto a trap or an ailment zone).

**Both tables extracted** (`ida_scripts/dump_trigger_list_table.py`,
one script per game, mirroring the destination-table scripts' own
style). Real data settles the record-shape question above: the two
systems are **not** byte-for-byte the same shape (Chapter 3's trigger
record is 20 bytes vs. the destination record's 18; fields land at
different relative offsets — worldX/Y/facing sit at `+4`/`+6`/`+8`
here vs. `+0`/`+2`/`+4` there), but every field the two share is
confirmed, by direct value cross-reference, to feed the *identical*
global variables: e.g. entry `[5]`'s `+8` value `16384` (`0x4000`)
matches `SaveFacingSouth` exactly, and its `+0xA` value `-32768`
(`0x8000` unsigned) is copied straight to `ds:0xCEF9` — the exact
global `TravelToDestination`'s own `+0x10` field feeds. So this is a
genuine semantic convergence (same underlying "teleport record"
concept, same consuming globals) with a genuinely different physical
layout — not a coincidence, but not literally one shared table either.
Chapter 3's real table has only 19 entries, overwhelmingly teleports
(`flags=0x4000`, 16 of 19) with a fixed arrival sound (`+0xE`
consistently `43`/`0x2B` across every teleport row) — plus 2 entries
with `flags=0x0000` (the same "fully data-driven default" case Chapter
2 has) and none at all exercising the fixed-effect-id or
ailment-tick branches with real data.

**Chapter 2's real table is tiny — 9 entries** — and, cross-referenced
against the dispatch above, exercises exactly 3 of its 6 possible
branches: entries with `flags=0x0000` (6 of 9, the fully data-driven
default, `+4` read directly as an effect id), `flags=0x4000` (2 of 9,
teleport), and one `flags=0x0200` entry (the equipment-corrosion
branch). **The corrosion branch's own per-member eligibility field is
now resolved, not just plausible**: that one entry's own `+4` value is
`322` (`0x142`) — and `0x142` is **already a named, confirmed
equipment-slot offset** in this project's own `party.h`
(`PartyFieldWearMain`/`Second`/`Third`'s own item-slot addresses,
`0x13A`/`0x142`/`0x146`, already used by
`partyTickEquippedItemDurability`/`partyHandleIconBarItemExpiry`) — so
the trigger record's `+4` field is exactly what it looks like: a raw
party-record byte offset naming which equipment slot this particular
map trigger corrodes, here the "Second" slot. Tracing which effect id
gets selected (`0x2B`/43 if flag bit `0x200` is set, else `1`) against
the already-embedded `g_effectsYendor2` table confirms the exact
mechanism: id 43's `modeFlags` is `0x0400`
(`EffectModeItemReplace`, effect.h) and id 1's is `0x0200`
(`EffectModeItemDestroy`) — the map-trigger corrosion branch is
choosing between destroying or replacing whatever's equipped in a
*data-driven, per-trigger-record* slot, using the *exact same*
`modeFlags` dispatch `ApplyEffectAndDrawIconBar`'s existing 3-way
switch (already fully reimplemented) already handles. In other words:
`ApplyTriggerEffectIconSlot` needs no new mechanism at all here — it's
`partyHandleIconBarItemExpiry` called with `slotOffset` sourced from
the trigger record's own `+4` field instead of a fixed constant, and
`equippedItemId`/`replacementItemId` resolved exactly the way
`combatApplyCorrosion` already does (`itemClassifyServiceTier`/
`itemCorrosionReplacement` against whatever's at that slot).

**Chapter 3's dispatch fully traced too, same round**: instruction-
identical to Chapter 2 for the ailment-tick, fixed-id, and corrosion
branches (same offsets, same `[bp+4]`-as-party-offset eligibility gate
for corrosion). **One genuine, confirmed Chapter 2 vs. Chapter 3
difference found in the default (fully data-driven) branch**: Chapter
3 adds an extra per-member exclusion Chapter 2 doesn't have at all —
when the matched trigger record's own flags bit `0x1` is set, a party
member is skipped if they have item `0x275` equipped in equipment code
`0x13`'s slot (`party.h`'s `+0x158`, one of the "id-only" short
equipment slots) — plausibly a protective item immune to this specific
trigger, narrative not confirmed. **Correction**: an earlier pass this
round undercounted Chapter 3's real default-branch entries as only 2
(missing that flag value `0x8001` — bit `0x8000`, the axis selector,
plus bit `0x1` — is *also* a default-branch entry, since neither bit is
one of the dispatch-selecting ones). Re-checking the full real table
finds **6** real Chapter 3 default-branch entries, and **4 of them**
(coordinates 332/333/336/337, all `flags=0x8001`) genuinely exercise
this exclusion — it is not dormant, it's a real, active mechanic in
Chapter 3's own data today.

**The fixed-effect-id branches (`0xF`/`0x10`/`0x11`) are gold/ore-theft
traps, not HP/MP damage** — resolved by checking their own entries in
the already-embedded effect table (`effect.c`'s `g_effectsYendor2`):
id `0xF`'s `costFlags` is `EffectCostGold`, `0x10`'s is `EffectCostOre1`,
`0x11`'s is `EffectCostOre2`. This matters because the icon slot's own
`+0x10`/`+0x12` fields (already documented in "The staged combat
event's consumer" section above) mean different things depending on
the occupying effect's cost type: for an HP/MP-cost effect they're
"plain `u16` magnitude, `+0x12` unused"; for a gold/ore-cost effect
they're *together* a 4-byte packed BCD amount (`+0x10` the high digit
pair, `+0x12` the low pair) — exactly matching how these 3 branches
populate *both* words from `[+4]`/`[+6]` (the same physical bytes the
teleport branch uses for X/Y) rather than just one. So these branches
are literally "step on this row and lose gold/ore," reusing the exact
same Bcd4-as-two-words convention `ResolveAttackerActionOutcome`'s own
gold-theft branch already established. The default branch, by
contrast, only ever populates `+0x10` (leaving `+0x12` untouched) — a
real structural difference confirming it's restricted to plain
HP/MP-cost effects, never gold/ore ones, by construction (a data-driven
effect id could in principle name a gold/ore effect here too, but the
record format only supplies one word for it, so such an entry would
silently steal `Bcd4{0,0,0,rawLowDigits}}`-style garbage — not observed
in either game's real, tiny dataset, so not a practical concern).
`+0xE` (inflicted status) is never written by any branch here — the
same already-documented, already-accepted "reads whatever a prior
unrelated call left in the shared scratch slot" quirk
`ResolveAttackerActionOutcome`'s own branch 2 has, not a new mystery.

**Reimplemented 2026-09-30** (same day, continued): `mapTriggerFind`/
`mapTriggerDecide`/`mapTriggerApplyEffect`/`mapTriggerApplyCorrosion`
in `src23/maptrigger.c`/`.h` (a new module), both games' full tables
embedded as literal data. `mapTriggerFind` is a pure lookup (linear
scan, first-match-wins, matching the original exactly); `mapTriggerDecide`
is the pure decision function (which of `MapTriggerNone`/
`ApplyEffect`/`ApplyCorrosion` applies, plus the resolved effect
id/slot offset/corrosion mode flags/Chapter-3 exclusion requirement) —
teleport and the ailment-tick branch stay `MapTriggerNone`, deliberately
not decided further (pure UI orchestration and a not-yet-built
prerequisite system, respectively, matching this project's established
scope boundaries elsewhere). `mapTriggerApplyEffect` builds the correct
`Bcd4` from `rawA`/`rawB` for the gold/ore-theft branches (each raw
word's own two bytes become one BCD digit pair, high word first,
matching `ResolveAttackerActionOutcome`'s own established convention)
and passes a plain magnitude straight through otherwise, applying the
Chapter 3 item-exclusion check first when required.
`mapTriggerApplyCorrosion` reproduces
`combatResolveAttackerAction`'s own equipment-corrosion eligibility
chain exactly (empty slot / unknown item / no valid replacement all
no-op) before calling the already-existing
`partyHandleIconBarItemExpiry`.

Tests in a new `test_maptrigger.c` (21st suite) cover the lookup
against real entries from both games' extracted tables, every dispatch
branch (including Chapter 3's real, exercised item-exclusion case, not
a synthetic one), the Bcd4 construction for gold theft (necessarily
synthetic — no real trigger entry in either game exercises this
branch), and the corrosion apply path's full no-op chain plus a
successful replace. All 21 suites pass.

**Still not reimplemented**: the teleport and ailment-tick branches
themselves (pure orchestration/prerequisite-system gaps, as above);
Chapter 3's own richer teleport record fields remain fully undecoded
here too, same as `TravelDestination`'s own `rawA`-`rawD` convention.

It also fires a dawn event at exactly 6:00 AM and a dusk event at
6:00 PM (`g_gameClockMinutes`==`0x168`/`0x438`, via `AdvanceDayNightPaletteFade`
— a genuine ambient-lighting system: a gradual 113-step palette fade
through a snapshot table, written into VGA palette entries `0xE0`-
`0xFF` (the last 32 slots, plausibly a dedicated sky/ambient-light
ramp) via `SetPaletteRange`, walked forward from dawn and backward
from dusk). `AdvanceDayNightPaletteFade` and `RedrawDungeonScreen` (the
core first-person render) both call `ComputeAmbientLightingTable`
(was `sub_2784A`) to compute the actual lighting-gradient snapshot
that fade walks through: a day/night cycle lookup against `g_gameClockMinutes`
against a 32-byte-entry table (`0x7228`), with environmental-override
flags (`word_36C79`) and facing/region refinements, then a weather-
darkening pass subtracting deltas under rain/storm/fog-style
conditions — the system tying together the clock, weather, and
dungeon lighting into one 7-word working buffer (`0x5086`). This
also independently confirms `g_gameClockMinutes` wraps at its `0xFFFF`
table terminator back to `0`, consistent with the "minutes since
midnight, 0–1439" model above. A separate 5-minute periodic timer
(`word_32954`,
gated on `word_3295A` bit `0x800`, calling `TickWorldAilments`) — a
status-ailment duration sweep, not an item timer: it ticks a shared
"ailment slot" format (`[+0]`=ailment code `9`/`0xF`/`0xC`, matching
`TickStatusEffects`; `[+2]`=remaining duration) across a 6-entry world
table (`0x9519`, also read by `IsItemRangeAvailable` — see the
"Quest-item and party-inventory range checks" note below — and by
`TryCureAilmentFromIconClick`, the click handler for this same 6-slot
active-ailment icon bar: hit-tests region table `0x636C`, loads the
clicked ailment's code as an item-catalog record — ailment codes and
item ids appear to share a numbering space — then, unless `IsItemTypeAcceptedByLocation` says otherwise, stages it
into the "carrying" state, plausibly to apply a held cure item to it)
and every
party member's main inventory (`+0x11A`), decrementing one of 3 global
per-ailment counters (`0x9425`/`0x9429`/`0x942B`) to zero before
clearing the corresponding `word_36C79` flag — and disables itself
once nothing is left ticking.

**The 6-entry table at `0x9519` is likely the tail of a larger 9-slot
array starting at `0x950D`**: `HandleStatusIconBarClick` (was
`sub_270FE`, called from `start` and `HandleDungeonInput`) hit-tests
the *same* region table `0x636C` (with a different mouse-position
variable pair) and computes `(hit-1)*4 + 0x950D` for hits 4-9 — which
lands on the exact same addresses (`0x9519`, `0x951D`, ... `0x952D`)
as the ailment table. Hits 1-3 (i.e. slots at `0x950D`/`0x9511`/
`0x9515`, before the ailment table starts) instead dispatch to the
still-untraced `sub_271DC` — plausibly a different kind of icon
(equipment slots?) sharing the same 9-icon bar, but not confirmed.

Resting advances the clock
by a fixed 8 hours (`g_gameClockMinutes += 0x1E0`, matching the classic
"resting takes 8 hours" convention); a separate `+0x3C` (1-hour) advance
exists elsewhere too, context not traced.

A **separate** per-member status icon bar exists alongside
`TickWorldAilments`' 6-slot world table: `TickPartyAilmentIconBar` (was
`sub_1A085`, called from `RunDungeonGameLoop` and
`ApplyMapTriggerEffect`) periodically (a counter reaching 40, or a
much slower ~32768-call wraparound path gated on a 4th `word_36C79`
flag, bit `2`) recomputes each of the 4 roster members' ailment
severity via `PrepareTrapEffectSlots(ax=2` or `0xE)` plus a helper that
checks `+0x1C` status bits — **now identified via `DrawAfflictionsList`,
and all three helpers now individually named**:
`TickDiseasePoisonSickAilmentSlot` (was `sub_1A14D`, the "normal"
path, id `2`) sums `0x2000`/`0x4000`/`0x8000` = DISEASED/POISONED/SICK
as a weighted severity (`0xC`/`6`/`3`); `TickCurseHexJinxAilmentSlot`
(was `sub_1A195`, id `0xE`) sums `0x80`/`0x100`/`0x200` =
CURSED/HEXED/JINXED (`0x10`/`8`/`4`), gated on having MP
(`+0x54 != 0`); or, on the slow path, `TickPerceptionGatedAilmentSlot`
(was `sub_1A233`) tiers off the derived stat `+0x58` instead —
confirmed to no-op for dead characters and produce a *decreasing*
severity as `+0x58` rises across 6 thresholds, the opposite direction
from a "bigger stat, bigger effect" reading, consistent with `+0x58`
being a perception stat that *reduces* susceptibility. All three share
an identical tail: if the summed severity is nonzero, populate the
per-member icon-bar slot (`[di+8]`/`[di+0xA]`=the staged effect id/
magnitude, `[di+0xC]`=the character pointer) and set `g_uiScratchFlags4` bit
`0x100`; `TickPartyAilmentIconBar` itself then calls
`ApplyEffectAndDrawIconBar`. This closes out
`TickPartyAilmentIconBar`'s dispatch structure end to end — only the
`word_36C79` bit-`2` slow-path trigger condition remains unconfirmed.

**Reimplemented as `ailment.c`** (`ailmentTickIconBar`). Corrections found on the way: the Chapter 2 "slow
path" counter is reset to 1 after every sweep, so while the party is in a travel-ailment place (word_36C79
bit 2) the Survival-tiered pass simply runs on every call; its trigger is a travel destination with flag
`0x4000`. The tiers are Survival <= 55 -> 12, <= 75 -> 9, <= 80 -> 6, <= 100 -> 3, else none. **Chapter 3
restructures it**: no Survival pass; the disease/curse counter must exceed 40; and a new **cold** pass
(word_36C79 bit 1, its own counter) stages effect `0x2E` on every member who is not incapacitated and not
wearing item `0x10B` (DWARVEN FUR, slot `+0x154`), except one member picked with `RandomInRange(3)+1` by loop
position, who gets effect `0x2F` with magnitude level + 4.

`ApplyEffectAndDrawIconBar` also calls `ApplyIconBarStatDelta` (was
`sub_182CE`): applies a capped or floored stat delta to a
data-selected party field via the icon-bar slot, clears a `+0x1C`
status-bit range, then finishes with `UpdatePartyAverageStatTiers` and
— notably — `CheckForLevelUp`, suggesting at least one use is a
gradual/staged XP-granting effect (the specific field isn't hardcoded
here, so not confirmed).

**Reimplemented 2026-09-29** as `partyApplyIconBarStatDelta` in
`src23/party.c`/`.h`, taking the icon slot's own field-offset pair
(`currentFieldOffset`/`maxFieldOffset`) and the effect's `modeFlags`
directly, since no caller populating those offsets has been traced yet
— same "mechanism confirmed, specific field/caller not" situation as
this paragraph already flagged, now reproduced faithfully rather than
guessed at. Reading both games side by side while confirming
instruction-identity turned up **two genuine Chapter 2 bugs, both
fixed in Chapter 3**: (1) a zero `maxFieldOffset` (meant as "uncapped")
gets read as a real field offset in Chapter 2, treating the party
record's own first 2 bytes (`PartyFieldName`'s start) as the cap;
Chapter 3 adds a guard. (2) `RefreshCarryCapacityAndAttributeBonuses`/
`CheckForLevelUp` read `g_currentPartyRecord` internally rather than
taking a record parameter, and Chapter 2's own
`ApplyIconBarStatDelta` never updates that global before calling them
— for a whole-party effect they'd silently operate on whatever record
was left over from an unrelated earlier call; Chapter 3 saves/sets/
restores `g_currentPartyRecord` around the same 3 calls. Both fixes
adopted uniformly for both games (this reimplementation always takes
the record as an explicit parameter, sidestepping bug (2) entirely,
and treats `maxFieldOffset == 0` as uncapped per bug (1)'s fix) —
matching this project's established practice of reproducing a
*corrected* behavior rather than replicating an incidental original
bug. See `engine-diffs.md`. Tests in `tests/test_party.c` cover both
mode bits, the uncapped case, the neither-bit-set no-op, the
status-mask clear, and that the tail's carry-capacity refresh and
level-up check both actually run.

`ApplyEffectAndDrawIconBar` itself calls `HandleIconBarItemExpiry` (was
`sub_1819B`) — a significant find: when an icon-bar item's timed effect
expires, it strips the item's stat bonuses via `RemoveMultiStatEffect`,
then either **replaces** the inventory slot with a new item (applying
*that* item's effect in turn) or **destroys** it outright (clearing the
slot and subtracting its weight from the confirmed `+0x118` counter) —
gated on a flag on the icon-bar entry. This is the mechanism behind
consumable magic items that transform or are used up (a wand running
out, ice melting, etc.), though the specific items involved aren't
identified yet.

**Fully reimplemented, 2026-09-29 — turned out to need much less new
groundwork than expected**: the "`g_itemStatEffectTable`" this section
worried was still unextracted was, it turns out, already fully decoded
by an earlier round of this project under a different name —
`src23/item.h`'s `itemEffectEntry`/`itemEffectPairs`/`itemEffectField`/
`itemEffectAmount` (an item's own `ItemFieldEffectOffset` field, `+2`,
already documented as "byte offset into the effect table") are exactly
this table, just connected to `item.c`'s own module before anyone
traced `RemoveMultiStatEffect`/`ApplyMultiStatEffectForItem` far enough
to recognize it. Reading those two functions directly resolved the
last open question — that a pair's "type id" is used as a **raw byte
offset straight into the party record**, not an index into any name
table (this project's own `isPartyEffectField` test helper in
`tests/test_item.c`, already checking real `WORLD.DAT` data against
exactly the `PartyFieldProtections`/`PartyFieldStats`/`PartyFieldStatsMax`
ranges this implies, had already validated the hypothesis without
anyone connecting it to these two functions specifically).

Two real asymmetries confirmed and reproduced exactly: fields `< 0x32`
(protections) apply/remove unconditionally, while fields `>= 0x32`
(stats/stat maxes) additionally skip the pair entirely when the target
field currently reads 0 (the same "don't grow an untrained stat from
zero" rule `partyApplyTraining` already uses) — and, going the other
direction, removing a `< 0x32` field is **not** floor-clamped at 0 (an
original quirk: the u16 field can wrap on underflow) while `>= 0x32`
fields are. Reimplemented as `partyApplyMultiStatEffect`/
`partyRemoveMultiStatEffect`/`partyHandleIconBarItemExpiry` in
`src23/party.c`/`.h`, all instruction-identical in Chapter 3. One more
original quirk reproduced rather than reinterpreted: the replace
branch parks the *expiring* item's own id in the equipment slot's
"extra" field (`itemSlotSet`'s second argument) while the *replacement*
item's id becomes the slot's primary id — backwards from what the
field names alone might suggest. This closes out
`ApplyEffectAndDrawIconBar`'s full 3-way dispatch entirely — all three
variants (plain damage/status, stat delta, item expiry) are now
reimplemented. Tests in `tests/test_party.c` cover both the destroy and
replace branches, the weight subtraction, and the stat-bonus swap.

`ApplyEffectAndDrawIconBar` and `RunDungeonGameLoop` both also call
`CheckPartyWipeAndReinitLevel` (was `sub_25AAC`) — a **total party
incapacitation** check: it scans all 4 `g_partySlotAssignment` members,
and if it finds even one whose `+0x1C` has *none* of bits `6`/`10`/`11`/
`12` set (bit 6 now confirmed as the "DEAD" flag via `DrawThreeStatBars`;
bits 10/11 being the confirmed `TickStatusEffects`/`ApplyStatusEffect`
timed-ailment flags), it returns immediately —
that member is still capable of acting. Only when *every* slot is
either empty or flagged with one of those bits does it fall through to
`ShowPartyWipeScreen` (was `sub_2ADE8` — stops music, plays a sound
effect via the sound dispatch, draws a full-screen picture, redraws the
fixed status icon), then `RunGameDialog`, then (unless `g_lastKeyChar`==
`0xFF`) `InitializeDungeonLevel` — reading very much like a "whole
party is down → show a screen → reset the level" handler. Bit `6`'s
specific ailment isn't confirmed (it's not one of the 3 timed-ailment
bits), nor is the exact meaning of `g_lastKeyChar`==`0xFF` skipping the
reset.

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
`WorldDat_setBlock6`. Error strings suggest it holds at least: text data,
NPC data, conversation data (per `"Problem retreiving text/NPC/conversation
data."`), and maps. **The first bytes are now identified, not a guess**:
they're the start of the world map's tile grid (see "World map" below) —
the repeating `00 00 00 00 01 00 00 00` pattern first read here as "a table
of `(flag/type, count-or-offset)` pairs" is the map's own bordering "void"
band, wall-type column values alternating 0/1. **The "conversation data"
the error string alludes to is also now decoded** — see "In-world readable
text" below; despite the string, it turned out to be found books/notes/
plaques, not NPC dialogue. "NPC data" itself is still unidentified.

**One resource confirmed**: offset `0x8270A`, 768 bytes, is the game's
master 256-color VGA palette (see `PICTURES.VGA`'s "Palette" section
below) — loaded via `LoadMasterPalette` (`0x27CB0`), one of the ~27
`FileEntry`-block-setup stub functions at `0x27B42`-`0x2801A`
(`ida_scripts/document_resource_stubs.py`, `ida_scripts/extract_resource_stubs.py`
has every stub's own offset/size if more need identifying the same way).

**Item data catalog**: `UseItem`'s `LoadItemData` (was `sub_1C890`)
looks up a per-item data block via a fixed catalog record at `0xBCE`
(same shared lookup helper, `sub_27B42`, as the resource-stub family
above — cross-confirming it's a generic "look up catalog entry N in
`WORLD.DAT`" primitive), allocates a buffer sized to fit, and reads the
item's raw data in. Item contents/size/count not yet examined —
`sub_27B42`'s own signature (id in, offset+size out) would be the
fastest way to enumerate the whole catalog if that's wanted later.

**Update 2026-10-03: this "item data catalog" is the NPC dialogue/service
catalog** -- see "NPC dialogue catalog" below.

### NPC dialogue catalog (decoded 2026-10-03; `src23/dialog.c`)

`UseItem` (`yendor2.asm:13169`), `start`'s handler for a world object with the
matching flag (it passes the object's `value`, `[si+4]`, as the id), is not an
item-use routine: it is the **talk-to-an-NPC / use-a-service** engine, and
`LoadItemData` (`:22216`) loads that NPC's data through three stubs,
`PrepareItemDataBlockRead28/3A/22`, whose file offsets sit in DS
(`DS:0xCE43/0xCE47/0xCE4F` in Chapter 2, `0xB1DB/0xB1DF/0xB1E7` in Chapter 3;
`ida_scripts/dump_itemdata_offsets.py` in both games; the `...4B`/`...53`
siblings are interior split points, not separate tables). The data is every
character's dialogue and the services they offer; the text itself gave it
away ("AS YOU APPROACH, YOU CAN SEE THAT THE GOVERNOR IS VERY CONCERNED...",
"WELCOME TO OUR ESTABLISHMENT. THIS IS THE FINEST TAVERN IN ALL OF YENDOR").

| Table | Ch2 offset | Ch2 entries | Ch3 offset | Ch3 entries |
|---|---|---|---|---|
| NPC headers, 40 bytes | `0x1677A1` | 105 (+blank slot 0, +1760 bytes slack) | `0x3D8EB9` | 140 (+slot 0) |
| topics, 58 B (Ch2) / 60 B (Ch3) | `0x168F11` | 929 (index 0 blank; 72 more slack) | `0x3DA4C1` | 1073 (index 0 blank; 37 slack) |
| text lines, 34 bytes | `0x1771DB` | 3218 | `0x3EA03D` | 4090 |

The table sizes fall out of the headers: each NPC's first topic/line equals
the previous NPC's first + count, and the totals match what the data holds
(the last real line ends at the byte where printable text stops). The three
"counts" `LoadItemData` itself reads from the header are `+4` (topics), `+6`
(lines), with `+8`/`+0xA` the first indices. A text line is 33 characters
space-padded plus a NUL; `%` is a line break, `~` the font's apostrophe.

**NPC header (40 bytes)**: `+0` portrait picture id (category 0x70), `+2` a
speaker mode (0/2/4/6 -- nonzero makes `LoadItemData` ask which party member is
speaking, and keys `ComputeCostMessageIndentMode`'s stat thresholds), `+4/+6`
topic/line counts, `+8/+0xA` first topic/line, `+0xC/+0xE/+0x10` three global
flag ids choosing the opening topic (2nd greeting if set, else 3rd, else 4th,
else the 1st), then service parameters read by the topic handlers: `+0x12` the
one-time flag a tome-giver sets, `+0x14/+0x16` a party-record field offset and
an amount (attribute tomes: e.g. +10 Strength; also `IsItemEligibleForEnhance`'s
level range and `UseTrainingItem`'s level cap), `+0x18` a price multiplier
(`ShowHealingCostPrompt`), `+0x1A` the per-character flag-bank index (the
`+0x10C` bank: "this character already used this service"), `+0x1C/+0x1E`
cost-message parameters, `+0x20` an item range the party must carry
(`IsItemRangeAvailable`, feeding the topic-type word's bit 0). Chapter 3 adds a
word at `+0x22` (usually equal to `+0x12`).

**Topic**: a 13-character keyword + NUL ("HELLO", "BLACKWING", "NUORE",
"PURCHASE FOOD", "BYE"), then `+0xE` flags (below), `+0x10` an argument (an exit
code for flag 1, a lock id for key checks, a type word for the handlers...),
`+0x12` the *byte* offset of the topic's first line from the NPC's first line
(always a multiple of 34), `+0x14` the line count, then the **menu machinery**:
`+0x16`/`+0x18` the topic's own bits in two 16-bit "available topics" masks A
and B, `+0x1A`/`+0x1C` bits it unlocks, `+0x1E`/`+0x20` bits it clears (usually
its own: it is used up; -1 clears everything), then six signed global-flag
ids at `+0x22` that the topic *requires* to be listed (positive: set,
negative: must be clear) and six at `+0x2E` it applies when visited (positive
sets, negative clears; `ApplyItemEffectFlags`).
**Chapter 3 topics are 60 bytes: an extra word at `+0x10` pushes everything
after it up by 2** (`ShowItemUsagePreview` reads `+0x12` where Chapter 2 reads
`+0x10`); `dialogTopicU16` hides the difference. A topic with no text keeps a
bit mask in its offset/count words.

**How a conversation works** (`dialogTopicListed`/`dialogVisitTopic`, checked
on both games' real NPCs): `LoadItemData` zeroes masks A/B and opens on the
NPC's greeting topic; visiting any topic clears its clear-masks, ORs in its
unlock-masks, and applies its result flags. A topic is *listed* while it has an
own bit, each nonzero own mask overlaps the matching available mask, and its
six require-flags hold (`CheckItemEligibilityAndCopyName`, `:20527`). So
the Chapter 2 governor's HELLO unlocks BLACKWING, PORT HOPE and BYE; BLACKWING
then unlocks NUORE and BATS; the tavern's GO TO MENU shows YES / NO and NO
restores the main menu. Two flags implement sub-menus: `0x20` saves the masks
(minus the topic's own bits) and `0x10` restores them. `0x2` makes a topic's
result flags conditional on the handler succeeding (the original's
`g_uiScratchFlags2` bit `0x40`, set by a correct riddle answer or completed
purchase). Chapter 3's NPC 1 (after HELLO: NAME, ZAMORA, TASKS, BYE) behaves the
same way.

**Topic flags** (`UseItem`'s tests of `es:[si+0Eh]`): `0x1` end the
conversation (wait for a key, then leave with the argument as exit code if <=
2), `0x40` the repair screen, `0x80` buy ore (for a topic named "BUY ...") or a
riddle answer prompt, `0x100` the enhance screen, `0x200` an attribute tome,
`0x400` an experience tome, `0x800` a preview, `0x1000` the preview twin of
`0x8000`, `0x4000` the sell screen, `0x8000` `UseKeyItem`, which is really
`LoadLockState(arg)` then `RunShopScreen` -- the shop / item-grant screen
(topics named BUY ARMOR, PICK UP KEY, OPEN CHEST, REWARD). The old "key item"
names for these handlers were guesses. `UseItem` additionally dispatches on a
second word (the topic's argument, loaded into `word_2E410`): `0x8000`
healing, `0x4000` training (`UseTrainingItem`, already reimplemented as
`partyApplyTraining`), `0x3000` an ability scroll, `0x400`/`0x800` the paid
service handlers. All of those handlers are UI-heavy and not reimplemented;
this round is the data layer plus the opening-topic choice and text assembly.

**Services decoded (2026-10-03, `src23/dialogservice.c`)**: the topic's argument
is a "type word" whose high bits name the service and whose low bits carry
the answer (2 = YES / accept, 4 = NO / decline). Healers (e.g. Chapter 2 NPC 7:
HEAL arg 0x8000, CURE 0x4000, RESURRECT 0x2000, RESTORATION 0x1000, YES 2, NO 4)
list only what the chosen party member needs, because
`ClassifyPartyMemberCondition` rewrites the top bits of topic mask A with
exactly those bits (0x2000 dead, 0x4000 any condition, 0x8000 hurt, 0x1000
both, 0x200 nothing wrong -- keeping the low ten bits, which includes a stale
0x200). The price is base x the NPC's price multiplier (`+0x18`) x the member's
level, summed in BCD: 20 for HP, 100 for a revive, per-condition prices for a
cure (Sick 5, Poisoned 10, Diseased 20, Paralyzed 40, Frozen 50, Stoned 60,
Jinxed 20, Hexed 30, Cursed 40), the sum of what applies for the full service.
Paying clears/sets exactly as the type says. Tomes (attribute and experience)
are one-time gifts to every living member, capped at 999 (9999 for HP/MP).
`dialogApplyAttributeTome`, `dialogApplyExperienceTome`, `dialogClassifyCondition`,
`dialogAfflictionCost`, `dialogHealingCost`, `dialogApplyHealing`; the Chapter 3
versions of the tome handlers differ only in data addresses. `UseKeyItem`
turns out to be `LoadLockState` + `RunShopScreen`: the shop / item-grant
screen.

**Trainers and challenges (2026-10-03)**: the NPCs' greeting topic carries a type
word that picks the handler: `0x8000` healer (5 in Chapter 2, 1 in Chapter 3),
`0x4000` level trainer (5 / 4), `0x800` "challenge" (20 / 14). A trainer
(`UseTrainingItem`) quotes `100 x multiplier x level` and refuses once
`level + 1` would pass its cap (`DialogNpcParamB`); accepting is
`partyApplyTraining`. A **challenge** NPC (the data text is "WELCOME TO THE
CHALLENGE OF PROJECTILE ACCURACY. FOR A SMALL FEE YOU CAN TRY TO HIT THE CENTER
OF THE TARGET. IF YOU ARE SUCCESSFUL, I WILL REWARD YOU GREATLY") is a pure
stat check, no dice: the header gives a party-record offset into the
*maximum*-stat array (`0x88` the projectile-accuracy rating, `0x82`
intelligence, `0x92` hit points), a threshold, a 4-byte BCD entry fee at
header `+0x1C` (the original keeps it at `DS:0x512A`; this was the `word_3298A`
aliasing again), a reward multiplier (`+0x18`) and a per-character "already
won" flag index. The fee is taken first, win or lose; at or above the threshold
the fee comes back `multiplier` times over and the flag is set (so the member
can't win again); below it the fee is lost and they may retry. Real data: the
projectile challenge costs 1000 gold for 3x at 70, the intelligence one 1200 for
4x at 70, the HP one 2000 for 10x at 400. `dialogTrainingQuote`,
`dialogAttemptChallenge`, `dialogMarkServiceAvailability` (the once-per-
character availability bits).

**The lock catalog's "uninterpreted +4..+25" is the contents of chests, shops and
reward piles (2026-10-03)**: the topics that open the shop screen
(`UseKeyItem` -> `LoadLockState(topic arg)` -> `RunShopScreen`) point at 26-byte
lock-catalog records, and following `BuildShopCategoryTabList` /
`HandleShopCatalogSlotClick` / `PayGoldAndAcquireItem` through them gave: `+0`
flags, `+2` a packed trap value (threshold x 100 + effect id, applied once on
first opening when flag `0x80` is set -- not a price, despite the older note),
`+4..+0x13` **eight item-catalog ids** (0 = empty), `+0x14` gold, `+0x16` MAGIC
ORE, `+0x18` NUORE. Items 1/2/3 are the pile items (GOLD COINS / MAGIC ORE /
NUORE, item flags 0x80/0x40/0x20) and take their amounts from those words.
Taking a slot sets a bit in the persistent taken-mask (`byte_32DCC`), so
emptied containers stay empty. Real data: the Chapter 2 armour shop (lock 319)
stocks items 313, 363, 251, 223, 276, 181; the governor's key grant (333) is item
41 plus 10000 gold; lock 3 holds all three piles (7250 gold, 200 magic ore, 400
nuore). `LockRecord` now exposes `items[]`, `gold`, `magicOre`, `nuore`.

**Shop arithmetic (`src23/shop.c`, 2026-10-03)**: `ComputeBarterPricingPreview`
sets a percentage from the haggling member's Bartering stat in seven tiers
(<=54: 55%, <=64: 45%, <=79: 35%, <=100: 25%, <=124: 15%, <=149: 8%, <=999: 2%,
anything else 55%); buying costs base x (100+p)%, selling back pays
base x (100-p)%. `PayGoldAndAcquireItem`: affordable -> subtract; paying the
exact balance flags the "resource depleted" overlay. Chapter 3's service
handlers are the same code with every topic field 2 bytes higher (the new
word) and one difference: the handler "finished" argument bit is `0x8` where
Chapter 2 tests `0x1` (the topic data uses the FINISHED topic's mask bits and
flags instead, so it doesn't matter for the data layer).

**Riddles and ore (2026-10-03)**: a topic with flag `0x80` is either a purchase
(its name starts with "BUY ") or a riddle. A riddle's argument is its 1-based id
into a table of answers **in the executable**, not WORLD.DAT (near-pointer table
at `DS:0xBF48` / `0xA8C6`, 6 / 11 entries; `ida_scripts/dump_riddle_answers.py`):
Chapter 2 PENTAGON, LINGUISTIC, WHITE POTION, THAINE, SHIRLEY, GAIN; Chapter 3
PEACEFUL, 120, ARCHIBALD, OVIAS, WIN, 30, 500, 3925, 46080, 400000, 70. The compare
is exact and case-sensitive; success is what lets a conditional topic's result
flags apply. Every riddle topic in both games' data carries an id inside that
range, which confirms the table sizes. Buying ore ("ORE COSTS 10 GOLD PER UNIT",
argument 2 = MAGIC ORE, 3 = NUORE): quantity up to gold/10 (last digit dropped),
ore += quantity, gold -= 10 x quantity. `dialogRiddleAnswer`,
`dialogCheckRiddleAnswer`, `dialogBuyOre`.

**Sell / enhance / repair (2026-10-03)**: a SELL topic's argument is a class mask
the NPC will buy (shares a bit with the item's class word); selling pays
`shopSellPrice`. An ENHANCE smith takes armour whose target word 3 (what
`item.h` calls the break chance -- in real data it is the item's "+N") lies in
`[ParamA, ParamB]`, or weapons by target word 4, and swaps the item for the next
catalog id (the "+1" version) for that item's base price x the smith's percent.
Verified on real data: the "+5 up to +7" smith (Chapter 2 NPC 57, 80%) accepts
exactly STEEL/SILVER/GOLD SHIELD +5 and +6, gloves and boots +5/+6, 58 items in
all; the apprentice smith (NPC 9, 0-2, 50%) the plain through +2 ones. Repair
needs a target slot-flag bit and costs the *original* item's price x percent.
`dialogSellAccepts`, `dialogEnhanceEligible/Cost`, `dialogRepairEligible/Cost`.

**The "ability scrolls" are the fast-travel mounts (2026-10-03)**: `UseAbilityScroll`
teaches/sells the four special abilities that `PartyFieldAbilities` (`+0xB4`)
records -- PEGASUS `0x8000`, GIANT EAGLE `0x4000`, FLYING RUG `0x2000`, MAGIC
DRAGON `0x1000` -- from a 4-entry, 26-byte table in the executable (identical
in both games; `ida_scripts/dump_ability_scroll_table.py`) priced 5000 / 15000 /
25000 / 35000 gold. The mount's mask rides in the topic's *text-offset* word of
a zero-line topic (the topics with bit-mask "offsets" and `0` lines noted
earlier). Buying: refused if already known, then if gold is short; pays, sets
the bit, zeroes that mount's charge counter (`+0xB6`, high bit first). Selling
back clears the bit and refunds the **full** price. Mount sellers have a plain
greeting and *switch into* the handler through a BUY/SELL topic whose flag `0x800`
and argument (`0x1000` buy, `0x2000` sell) load the "type word" (3 such NPCs in each
game). The type word is only ever loaded while it names no handler yet -- a
healer/trainer/challenge/mount handler intercepts later topics before their own
`0x800` flag is looked at -- which is why Chapter 3's healer topics can all carry
`0x800`. **`UseItemType_400` is dead code in both games' data**: no topic ever
loads type `0x400`. `dialogTypeAfterTopic`, `dialogServiceForType`.
`dialogTransport`, `dialogLearnTransport`, `dialogSellTransport`.

**The skill-based repair attempt (2026-10-03, `src23/repair.c`)**: `RepairItemCommand`
rolls `RandomInRange(100)` against a (low, high) pair from a 16-row x 5-tier table
in the executable (`DS:0x6B7E` / `0x6EAC`, identical in both games;
`ida_scripts/dump_repair_table.py`). Row = the damaged item's difficulty (armour "+N",
weapon target word 4); tier = the repairer's `PartyStatRepair` (<50, <65, <80, <95,
else). Roll < low: critical failure, the item is destroyed; low..high: success; above
high: soft failure. The kit's charge is spent on the first two. Row 4 tier 0 is
(100, 0) -- an unskilled repairer destroys it on any roll below 100 -- while row 0
tier 4 is (0, 100), a sure thing; across rows the destroy threshold never rises with
skill (checked). The comment in the old name table had the success/soft-failure
labels reversed.

**The Search command (2026-10-03, `combatSearchTrap`)**: `HandleSearchCommand` checks the
faced lock/trigger record: already opened -> nothing more; lock flag `0x40` clear -> just
shows the lock status; flag `0x40` set -> a detection roll `FailsSavingThrow(level,
DC = packed trap / 100, bonus = Thievery)`. Failing it plays a buzz and spends an item
charge; passing marks the object and runs the trap through the existing
`ApplySavingThrowEffect` composition -- a second, independent roll (so finding a trap
doesn't always disarm it). That settles flag `0x40` as "hidden trap present".

**Spell scrolls: use or study (2026-10-03, `spellCanLearn`)**: `InteractWithContainer` -- a
"container" target whose target word names a spell (`LoadClueBookSpellEntry`) -- is the
spell-scroll handler: the spell's context rules are the cast gate (exploration-only spells
refused in combat, attack spells outside it); a spell with FlagsA `0x8000` first asks who it
targets; the cast runs `ApplyEncodedItemEffect` with the "already resolved" flag set and spends
a charge; the alternative answer *studies* it, via `MarkIneligiblePartyMembers`: a member can
learn it only if they don't already know it (the `+0xCA` ability bank), their low-six secondary-
class status bits share a bit with the spell's class-eligibility word, and their level is at
least the spell's word `0x16` -- which this settles as the **required level** (the old
"group/tier" guess; max 38 in both games, climbing with record id). 74 of Chapter 2's 105 spells
have an empty class mask: they aren't learnable from scrolls at all.
`spellCanLearn`, `spellLearn`, `spellUsableInContext`.

**`CastSpell` is the potions handler (2026-10-03, `src23/consumable.c`)**: `HandleGameCommand`
sends the restorative item ids here (Chapter 2: 0x12-0x14, 0x17, 0x18, 0x1D; Chapter 3: 0x34-0x39) --
HP +25% / +50% of max / full, MP +50% / full, cure Diseased/Poisoned/Sick -- each refusing (flash a
warning, spend nothing) when it would do nothing. Chapter 2's id 0x1C is the alchemist's ore
transmutation: Chemistry >= 65 and >= 10 units, then up to **100** units of the source (not 10, as an
earlier note said) become floor(units / d) of the other ore, d = 10/5/4/2 at Chemistry
<80/>=80/>=95/>=110. Chapter 3 has no transmutation (its ore economy is replaced by the artifact
quest). `partyRestorativeForItem`, `partyUseRestorative`, `partyTransmuteOre`.

**Chapter 2's quest relics (2026-10-03, `src23/relics.c`)**: `DispatchItemAbilityCommand`
(item ids `0x242`-`0x2C8`, reached from `HandleGameCommand`'s fall-through) is the relic cluster. The
four charge-gated relics need global flag `0xB1` (the "recharge" flag; otherwise "PATIENCE IS A
VIRTUE."): `0x246` +5000 NUORE, `0x247` +5000 MAGIC ORE, `0x248` mass heal and **overheal** (clears every
status above the low six -- including Dead -- and sets HP and MP to *twice* their maxima, 16-bit),
`0x249` instant kill of the engaged monster (combat only). `0x258` is a potion that works only on map
cell (104, 110), where it sets global flag `0x48`; `0x2C8` completes a quest when items
`0x254`-`0x257` are all carried (it consumes them and hands over `0x258`) -- that last one needs the
inventory range check (`IsItemRangeAvailable`) and isn't modeled. Chapter 3 has none of these.

### Item catalog (decoded 2026-09-19)

A contiguous `WORLD.DAT` region, loaded by `loadWorldDat1` and read back by
`LoadItemCatalogRecord`. It is five back-to-back tables (each starts where the
previous ends; both games' chains verified against their real files):

| Table | Ch2 offset | Ch2 count x size | Ch3 offset | Ch3 count x size |
|---|---|---|---|---|
| item records | `0x71138` | 759 x 58 (699 + 60, two reads) | `0x83EE8` | 631 x 58 |
| effects | `0x7BD2E` | 175 x 16 | `0x8CDDE` | 148 x 16 |
| wearable targets | `0x7C81E` | 275 x 12 | `0x8D71E` | 221 x 12 |
| consumable targets | `0x7D502` | 250 x 8 | `0x8E17A` | 151 x 8 |
| weapon targets | `0x7DCD2` | 190 x 12 | `0x8E632` | 210 x 12 |
| end | `0x7E5BA` | | `0x8F00A` | |

**Ids** are 1-based; an inventory slot's id is `page * 256 + number` (the
community item guide's "second byte is the page"; `0x21E` = page 2, number
`0x1E` = SLING). The slot's second word is the uses/charges count (low byte;
the guide says 0 and 1 both mean one use). Chapter 2's real items are ids
1-744 (`SCROLL OF RESURRECTION` is last); the 15 records after it are slack in
the game's block size and hold unrelated data (record 745 even contains a
leftover build path, `BK1_CH2\GAME\`). All 631 Chapter 3 records are items.
**Ids differ between the games** (Ch3 id 2 is FOOD, id 6 CLOTHES +2). Names
match the guide for 321 of 759 exactly; the rest differ only in the guide's
paraphrasing (`Wood shield` / `+1`), the game's own misspelling `SAPHIRE`
(fixed to `SAPPHIRE` in Ch3), and `NOT USED` placeholders.

**Item record** (58 bytes): `+0x00` u16 byte offset into a target table (see
below); `+0x02` u16 byte offset into the effect table, 0 = none; `+0x04` packed
BCD4 gold price; `+0x08` icon picture id (category `0x80`, +1 if flag `0x1000`);
`+0x0A` u16 weight; `+0x0C` flags; `+0x0E` fit flags; `+0x10` class bits
(`0x8000` gear, `0x4000` potions, `0x1000` food, `0x800` gems, `0x2000` bags;
not fully mapped); `+0x13`, `+0x20`, `+0x2D` three 13-byte text lines (12
characters + NUL) that `BuildItemDisplayName` joins as
`trim(l1) + " " + l2`, trimmed, `+ " " + l3`, trimmed (an empty middle line
therefore collapses to one space).

**Flags `+0x0C`** say which inventory slot codes accept the item
(`IsItemEligibleForCommand`): `0x8000` code `0xA`, `0x2000` code `0xB` (also
containers), `0x4000` code `0xC`, `0x800` code `0xD`, `0x400` codes `0xE`/`0xF`
(rings), `0x200` codes `0x10`-`0x14` (which one is chosen by the wearable
target's flag word: `0x8000`/`0x4000`/`0x2000`/`0x1000`/`0x800` = `0x10`..
`0x14`), `0x100` consumable, `0x1000` alternate icon. **Fit flags `+0x0E`**:
`0x8000` backpack, `0x4000` box, `0x2000` bag (`0xE000` fits all).

**Low flag bits** (found 2026-10-03, `inventory.c`): `0x1` = cannot be dropped (all keys, maps and quest items: 47
items in Chapter 2, 22 in Chapter 3), `0x2` a container that accepts everything (MAGIC CONTAINER), `0x4`/`0x8`/`0x10`
a BAG / BOX / BACKPACK, which accept items whose fit flags have `0x2000` / `0x4000` / `0x8000` (a BAG, fit `0xC000`,
fits a BOX or BACKPACK but not another BAG; a BOX fits only a BACKPACK). `IsItemDroppable` is misnamed: it returns
non-zero when dropping is BLOCKED -- the item itself, or any item up to three containers deep inside it, has bit 1.
`IsItemEligibleForCommand`'s per-slot rules (two-handed weapons need the shield slot empty, shields need no
two-handed weapon, an occupied slot always swaps) are `inventoryEligibleForSlot`.

**Target tables**, chosen by the flags in this order (`LoadItemCatalogRecord`):
`0xE00` bits -> wearable table (12 B); else `0xC000` -> weapon table (12 B);
else `0x100` -> consumable table (8 B). Wearable/weapon words: `[0]`
absorption ("ABSORPTION-"), `[1]` flags (for weapons the skill type: `0x8000`
projectile, `0x4000` slashing ...), `[2]`/`[3]` and `[4]`/`[5]` two
(replacement item id, break-chance %) pairs used by
`TickEquippedItemDurability`. The consumable words are not decoded (bread is
`{0,0,10,0}`, potions `{2,1,...}`). Records with none of those flags but a
nonzero `+0x00` (e.g. bags: 24) use it for something else.

**Effect entries** are 4 pairs of `(party-record field offset, amount)`, read up
to the first zero offset. The offsets are exactly the party record's
protection (`0x20`-`0x30`), current-stat (`0x3C`-`0x70`) and maximum-stat
(`0x7C`-`0xB0`) fields: HALFLING HELMET OF INTELLIGENCE is `(0x82,+24)`
(INTELLIGENCE max), `(0x42,+24)` (current), `(0x30,+30)`, `(0x2E,+15)` (jinxing
and hexing protection); STRENGTH POTION is `(0x7C,+2)`, `(0x3C,+2)`. Amounts are
unsigned (max 500 in Ch2, 100 in Ch3). `ApplyMultiStatEffectForItem` skips a
stat whose current value is 0 and caps at 999. Reimplemented in
`src23/item.c`.

### Trap and status effect definitions (decoded 2026-09-22)

`g_trapEffectDefs`: a static, in-EXE table (not in `WORLD.DAT`) of 12-byte
records — `DS:0x905B` in Chapter 2 (45 entries), `DS:0x96DA` in Chapter 3 (49)
— indexed by effect id via `PrepareTrapEffectSlots` (`id*0xC + base`). An
effect is the unit every trap, monster attack, ailment tick, healing item and
expiring item ultimately applies to a party member — it's what the game means
by an "effect": a cost (HP/MP/gold/ore) plus, after a saving throw, one or
more status conditions inflicted. Both games' tables embedded verbatim in
`src23/effect.c` (extracted with `ida_scripts/dump_trap_effects.py`).

Record layout, decoded from `ApplyEffectAndDrawIconBar`/`ApplyEffectCost`/
`RollEffectMagnitude`/`RollEffectResistance`/`ApplyIconBarStatDelta`: `+0`
sound id; `+2` icon picture id (category `0x70`, or `0x80` for the two
item-expiry effects, ids 0 and 22 — `InitGlobals` overwrites their sound word
at startup with `_val20`/`8`); `+4`/`+6` magnitude min/max; `+8` cost flags —
low bits select what's spent (`0x10` HP, `0x8` MP, `0x20` HP+MP, `0x1` gold,
`0x4`/`0x2` the two ore counters, checked in that order, first match wins),
**high bits `0xFF80` double as the inflicted-status bitmask — exactly the
party record's own `+0x1C` status bits** (`0x80` cursed through `0x8000`
sick); `+0xA` mode flags: `0x1000` roll a saving throw (protection sum vs.
`FailsSavingThrow`, only when high cost bits are set), `0x2000` magnitude =
min×level (no random roll), `0x4000` magnitude = min (fixed), `0x80`/`0x100`
a stat-delta effect (floored at 0 / capped at max) instead of a status effect,
`0x200`/`0x400` an item-expiry effect (destroy / replace).

**Magnitude and the RNG's range are inclusive of the bound**: `RollEffectMagnitude`
computes `(RandomInRange(max-min) + min) * level` (or just `min×level`/`min`
per the mode bits above), and `RandomInRange(n)` (`yendor2.asm:41476`) returns
a value in **`0..n` inclusive**, not the more usual `0..n-1` — confirmed by
sampling every result over 200k draws per bound (`src23/tests/test_random.c`).
This means an effect defined `min=1,max=10` (`RandomInRange(9)+1`) reaches 10,
not 9. **This corrects an earlier note**: `RollCharacterAttributes`'s stat
roll, `RandomInRange(15)+45`, actually ranges 45–60, not 45–59 as previously
written (see the attribute section above). Reimplemented in `src23/random.c`
(faithful LCG port, same seeding and masking) and `src23/effect.c`.

**Cross-checked against both monster catalogs** (`+0x6C`/`+0x6E`, see below):
every monster's primary-attack effect id resolves to a real, HP-costing
effect in its own game's table, and every nonzero special-attack id resolves
to a real effect — a full round-trip check with no exceptions in either game.

### Monster catalog and records (decoded 2026-09-19)

**Live record** (156 bytes; the 80-entry `g_levelMonsters` pool saved in
`CURGAME` section 7, and the 3 active combat slots): **50 bytes of runtime
state followed by a verbatim 106-byte copy of the monster's catalog block**
(`SpawnMonsterInFacingDirection` reads the block straight to record `+0x32`;
`0x32 + 0x6A = 0x9C`, the record's end). The real saves contain no live
monsters, so the runtime prefix is from the code only: `+0x00` type id (0 =
empty slot), `+0x02`/`+0x04` world x/y, `+0x06` byte offset of its cell in the
dungeon grid (`(y-originRow)*0x270 + (x-originCol)*8`), `+0x08` animation
frame (sprite base + `RandomInRange(5)`, i.e. 0-5 inclusive), `+0x0A` anim set (`0xA` if flag `0x1` of
`+0x92`, else `0xD`), `+0x0C` state bits (bit 0 = aware), `+0x0E` wound tier
(`0x8000`/`0x4000`/`0x2000`), `+0x10` current HP (set to `+0x50` at spawn),
`+0x12` target pointer (a runtime address, meaningless on disk), `+0x14`/
`+0x16` global flag indices set (>0) or cleared (<0) when it dies.

**Catalog block fields** (record offsets), all confirmed against the clue-book
monster sheet (`ShowClueBookMonsterDetail`) and real data: `+0x32`/`+0x3F` two
13-byte name lines; `+0x4C` sprite base picture id; `+0x4E` unidentified
(1-13); `+0x50` HEALTH; `+0x52` save difficulty; `+0x54` ACCURACY; `+0x56`
DEXTERITY (also the initiative); `+0x58` ABSORPTION; `+0x5A` DAMAGE; `+0x5C`
hit sound, `+0x5E` idle sound; `+0x64` RANGED ACC.; `+0x66` RANGED DAM.;
`+0x6C` its ordinary attack (an effect id, always HP-cost — see "Trap and
status effect definitions" above), `+0x6E` special attack (an effect id, 0 =
none, used 25% of the time per `SelectTrapEffectVariant`); `+0x72`..`+0x76` colour
remap (used when flag `0x4` is set); loot as packed BCD4: **`+0x7E` GOLD,
`+0x82` NUORE, `+0x86` MAGIC ORE, `+0x8A` EXPERIENCE** (an ALLIGATOR gives 495
gold, 10 NUORE, 5 ore, 1340 XP; the RED DRAGON 1,000,000 gold); `+0x92` flags
(`0x1` alternate sprite layout, `0x4` remap palette, `0x1000` area attack,
`0xE00` special-attack modifiers); `+0x94` awareness range (`0x20` never
wakes by distance, `0x40`/`0x80`/`0x100` pick 0x2C/0x29/0x26 viewport rows);
**`+0x96` immunities** (`0x8000` poison, `0x4000` disease, `0x2000` paralysis,
`0x1000` freezing, `0x800` hexing, `0x400` cursing, `0x8` fire, `0x4` cold,
`0x2` electric, `0x1` power, `0x10` magic-damage "resistant"); **`+0x98`
resistances** (`0x3A00` magic damage, `0xC000` physical damage).

**Catalog in `WORLD.DAT`**: an array of 106-byte blocks (block 0 is empty)
followed directly by a lookup of `u16` entries mapping a **type id** (what a
map cell holds) to a block index, addressed as `base + index * size` by the
generic read descriptor `{dest, length, index, base}` (`WorldDat_setBlock5` /
`_6`). Chapter 2: blocks `0x1A78A9` (62 x 106 = 6572), lookup `0x1A9255`
(2500 entries, real ones up to type 2143). Chapter 3: blocks `0x417075` (73 x
106), lookup `0x418EAF` (real entries end at type 1862 — the highest id in its
flag table; what follows are the engine's `InitGlobals` constants, the same
kind of data that trails Chapter 2's item table). Chapter 3 block 62 is a
`NOT USED` filler. **In `SW.EXE`/`Yendor3-full.exe`** a sorted table of 6-byte
`(type id, flagA, flagB)` entries (Ch2 `DS:0xE4E9`, 17 entries; Ch3 `DS:0xCE51`,
23) lists the boss/quest monsters whose death sets a global flag; every type in
it is a real monster in its game's lookup. Reimplemented in `src23/monster.c`.

### World map (decoded 2026-09-22)

**There is no separate per-level map data — the whole game is one
continuous tile grid at the very start of `WORLD.DAT`** (byte offset 0).
`RefreshDungeonMapWindow` (below) reads map rows through the generic
`PrepareWorldDatRead` stub, whose 32-bit base offset (table `DS:0xCDEF`,
`ida_scripts/dump_map_layout.py`) is a **static 0** — not a per-level
value set elsewhere, and there's no "current level" selector anywhere in
the read path. `g_partyWorldX`/`g_partyWorldY` address this one grid
directly, so towns, wilderness, and dungeon interiors are all baked into
a single coordinate space (matching how the "region passwords" — see
"Open questions" below — read more like teleport coordinates into one
world than separate files to load).

**Row size 3200 bytes, confirmed two ways**: `PrepareWorldDatRead` sets
the read size to `4*_blockSize3`, and `InitGlobals` sets `_blockSize3 =
0x320` (800) in *both* games (yendor2.asm and yendor3.asm each have their
own `mov _blockSize3-equivalent, 320h`) — record size `4*0x320 = 0xC80` =
**3200 bytes**. **800 columns** follows directly (3200 / 4 bytes/column).
**Row count**: 144 (Chapter 2) / 168 (Chapter 3) — found from where the
row pattern gives way to the item catalog (a different, already-decoded
region), and cross-checked two independent ways: it lands on a whole
number of rows with no partial row at the boundary, and it matches
`CURGAME`'s own fog-of-war bitmap row count *exactly* — `100 bytes/row =
800 columns / 8 explored-bits-per-byte`, the same `144`/`168` already
documented for that savegame section. Total map region: `0` to `0x70800`
(Ch2) / `0` to `0x84D00` (Ch3), i.e. **800×144** / **800×168** tiles.
Both games' full grids were checked against their real `WORLD.DAT` files:
every tileA/tileB value stays within the two legend tables' real range
(below), and the party's actual saved position (`CURGAME`'s X=166/Y=36)
decodes to a sane in-range cell.

**Column format**: 4 bytes, two `u16` tile-type indices — identical to
the first 4 bytes of the in-memory 8-byte dungeon-grid cell copied from
here (see "In-memory dungeon map grid" below): **tileA** (`+0`) indexes
the **wall type** table (`0xE551`, below); **tileB** (`+2`) indexes the
**floor/overlay type** table (`0xE175`, below). The first several rows of
both games are a distinctive placeholder/border pattern — tileA
alternating `0`/`1` column by column, tileB always `0` — a bordering
"void" band around the real playable area, not a decode error (this is
literally `WORLD.DAT`'s first 8 bytes, `00 00 00 00 01 00 00 00`
repeating, previously read as an unrelated unknown table). The last few
rows are a similarly low-variety, bounded pattern using different small
wall-type values, not yet characterized as precisely.

**Wall type table** (`0xE551`, 12-byte stride, `ida_scripts/dump_tile_type_tables.py`):
58 real entries (indices `0`-`57`, matching the highest tileA value seen
in real Chapter 2 map data exactly), then zero-filled reserved slots.
Confirmed field **`+0xA`: `g_pictureDir` picture offset**
(`DrawWallTypeLegendRow`, the map editor's wall-type legend strip). The
other five words per entry (`+0`, `+2`, `+4`, `+6`, `+8`) aren't
individually traced — some look like they might encode facing-variant
sub-ids (entries 26-37 read like paired/rotated variants of 16-25), but
that's a read of the data, not a confirmed finding. **Floor/overlay type
table** (`0xE175`, 10-byte stride): 65 real entries (`0`-`64`; tileB does
go up to `67` in real data, but `65`-`67` are legitimately zero-filled
reserved slots, not missing data — confirmed by dumping past them).
Confirmed field **`+8`: `g_pictureDir` picture offset**
(`DrawFloorTypeLegendRow`). Both tables reimplemented (picture-offset
column only, the only confirmed field) in `src23/worldmap.c`.

**Chapter 3's equivalent tables are a genuinely different, undecoded
mechanism** — not just a different address. `RunMapEditorScreen`'s
Chapter 3 legend-row drawers (`DrawWallTypeLegendRow`/
`DrawFloorTypeLegendRow`) call `sub_1BC98`/`sub_1BCDB` instead of doing a
flat `id*stride+base` lookup inline: those functions divide the tile-type
id by 100, use the quotient to select a **page** (a small table at
`DS:0xC8E7` for the floor side; the wall side's own page-table base
wasn't pinned down — its code only shows `+2`, suggesting it's relative
to something not yet identified, plausibly an EMS-paged resource
segment), then index within that 100-entry page. This looks like the
same "paged as country grows" pattern used elsewhere for larger Chapter 3
resources, consistent with its bigger map, but the paging mechanism
itself isn't traced. `src23/worldmap.c`'s `worldMapWallPictureOffset`/
`worldMapFloorPictureOffset` return `false` for `GameYendor3` until this
is done — a real, open gap, not silently wrong data.

### In-memory dungeon map grid

**The loader is now found**: `RefreshDungeonMapWindow` (was
`sub_209D2`, called from 16 sites in `start`, always right before
`RedrawDungeonScreen`+`BuildMinimapTileData`+`DrawMinimap` — i.e.
after any position-changing action) (re)builds this grid from
`WORLD.DAT`'s world map (**"World map" above** — this function is what
that section's decode is based on) around the party's current position:
reads 78 rows,
unpacking a packed-bit "explored" flag per cell alongside the two
tile-type indices below, then a second pass calls
`TryInteractAtPosition` per cell to bake item/trigger/trap markers
directly into the grid data. It also walks the 80-slot
`g_levelMonsters` array, placing monster markers into cells that
scroll into the window and despawning (via the confirmed
`ClearCellMonsterSpawnedFlag`) monsters that scroll outside it.

Found via `GetMapCellPtr` (`0x16F64`), the address computation the
fog-of-war reveal system (`RevealCellsAroundPlayer`
`ida_scripts/name_map_reveal.py`) uses: a 2D grid, **8 bytes per cell**,
rows **78 cells wide** (row stride `0x270` = `78*8`), segment
`g_dungeonMapGridSegment`, with the grid's own origin held in `g_dungeonMapGridOriginRow`
(row/y)/`g_dungeonMapGridOriginCol` (column/x) — i.e. addressing is relative to
whatever sub-region of the full map is currently loaded, not the map's
absolute origin. Confirmed fields: **`+0`/`+2`: two tile-type indices**
(used by `BuildMinimapTileData` — `ida_scripts/name_minimap.py` — as
lookups into two small tables, 12 bytes/entry at `0xE551` and 10
bytes/entry at `0xE175`, giving the two picture ids `DrawMinimap`
draws per cell — **now fully decoded, see "World map" above**: `+0` is
the wall type, `+2` the floor/overlay type, copied verbatim from the
on-disk world map); **`+6`, a flags word, bit `0x8000` = "already explored"** —
the automap's "cells become known as you walk near them" mechanic
(matches the manual's "M uses the party map"). The reveal action itself
(`PersistExploredCell`) writes the explored bit into `CURGAME` — the
automap survives save/load because it's part of the savegame, not just
in-memory state.

**The window, its build process, and a corrected field count (decoded
2026-09-23).** `RefreshDungeonMapWindow` (`yendor2.asm:29286`,
`yendor3.asm:28425`, both traced fully) is what (re)builds this grid,
called after every position-changing action right before
`RedrawDungeonScreen`/`BuildMinimapTileData`/`DrawMinimap`. It's a
**78x78 window**, not an unbounded grid — confirmed by both the outer
(row) and inner (column) loop counts (`cx = 0x4E = 78`). Its origin is
recomputed every call, centered on the party but clamped to stay
within 15 cells of the playable bounding box (`movement.h`'s
`MovementBounds` — same constants, confirmed identical values in both
games' `InitGlobals`) rather than the party's raw position:
```
origin = clamp(partyPos - 39, boundsMin - 15, boundsMax - 15)   // independently per axis
```
39 is half the 78-cell window; the `-15` slack means the window can
show at most 15 cells of the map's void border past the playable
edge, never more. Reimplemented as `dungeonGridComputeOrigin` in
`src23/dungeongrid.c`.

The build itself, per cell: copies the wall/floor type words verbatim
from the world map (the `+0`/`+2` fields above) via a row-buffered
read, then **a third word previously miscounted as part of the flags
field — `+4`, zeroed here** (`xor ax,ax` / `stosw` right after the two
tile-type words) **— making the layout `+0`/`+2`/`+4`/`+6`, still 8
bytes total, not `+0`/`+2`/`+6` with 2 bytes of padding.** `+4`'s write
side is now traced (decoded 2026-09-23, see "Monster pool: scroll
relink and despawn" below): the `+4`/`+6` bit `0x400` pair is a "monster
here" marker, written by exactly two producers that represent the same
underlying fact at different life-cycle stages — the scroll-relink pass
for an *already-spawned* monster (`+4` = its type id), and
`TryInteractAtPosition`'s own per-cell scan (still not reimplemented)
for a *not-yet-spawned* one, `errorCode=5` from a `worldobjects.c`
`0x800` marker (`+4` = the marker's `value`, which is itself the type
id to spawn — see `worldobjects.h`). **Correction to an earlier version
of this note**: `+4` is not written by the `0x4000` curgame-record
branch at all — that branch's cell-visible outcomes only ever overwrite
`+0` (wall type) or `+2` (floor type), confirmed by rereading
`RefreshDungeonMapWindow`'s `errorCode`-to-branch dispatch precisely
after an earlier pass mismapped `errorCode` 5 and 7 to the wrong
branches. The explored bit (`+6` bit `0x8000`) is filled from
`CURGAME`'s `SaveSectionExploredMap` bitmap in the same per-cell pass,
packed **MSB-first within each byte** (byte `col/8`, bit `7 - col%8`)
— derived directly from the shift-and-test sequence that extracts it,
not guessed.

Explicitly **not** covered by this base-window build: `TryInteractAtPosition`'s
per-cell marker baking (which is what actually sets the door/lock flag,
`+6` bit `0x6000`, the not-yet-spawned monster-marker overlay just
mentioned, and separately the `0x4000`-branch's wall/floor-type
overwrites — this base pass never sets bits below `0x8000` or touches
`+0`/`+2` after the initial copy). `g_levelMonsters` placement/despawn
as monsters scroll into or out of the window **is** now covered, in a
separate module — see below. Reimplemented (base window only) in
`src23/dungeongrid.c`/`.h`.

`ProbeFacingTile` computes `g_facingTileCellPtr` — the grid cell
directly ahead of the party — by offsetting the party's own cell
address by one row (`0x270`, matching the confirmed row stride) or one
column (`8`, the confirmed per-cell size) depending on facing
direction; it's the "what's directly in front of the party" primitive
the unlock-door handler and other facing-tile interaction code use.

`DrawMinimap` (`0x21588`) draws a 7×9 grid of 8×8-pixel tiles at a
fixed on-screen position (base tile + optional overlay per cell, from
`BuildMinimapTileData`'s buffer) — this pair is what `start`'s main
loop calls after every movement/state change to refresh the small
tile-grid minimap widget. **Correction**: this paragraph previously
claimed "no separate '3D corridor' renderer has turned up" and that
the minimap was the game's primary way of showing the dungeon layout
— wrong, superseded by this session's later, much more thorough
first-person rendering trace (`RenderDungeonViewport`,
`RedrawDungeonScreen`, `DrawDungeonCellWallTexture`,
`ExtendDungeonFloorTexture`/`ExtendDungeonCeilingTexture`, etc. — see
the dedicated sections elsewhere in this file). The minimap is a
secondary automap widget alongside the full first-person corridor
render, not a replacement for it.

**The `0xE551`/`0xE175` tile-type tables have a confirmed layout and a
second consumer.** Entry stride: 12 bytes (`0xE551`) / 10 bytes
(`0xE175`). Confirmed fields on `0xE551`: `+0xA` = `g_pictureDir` byte
offset (used by `DrawMinimap`/`DrawCellIconPair`/`DrawWallTypeLegendRow`
to pick the drawn picture); `+0` and `+2` hold two further per-type
values read only by the still-unnamed `sub_20D2F`/`sub_20CEC`/
`sub_20E12` cluster (see below) — not picture offsets, their exact
meaning is unconfirmed. `0xE175`'s confirmed field is `+8` (same role
as `0xE551`'s `+0xA`, for the overlay picture).

Checked whether `g_shadeShiftDelta` (a parameter `BuildMinimapTileData` sets
per-cell before drawing) is a color/remap value, since `DrawMinimap`
itself keeps its picture index fixed at `g_pictureDir` entry 9
throughout its loop rather than varying it per cell — read the
candidate consumer `sub_2A53C` directly and ruled this out, it never
touches `g_shadeShiftDelta` at all. **Resolved**: the actual reader is
`ShiftPaletteShadeClamped` (was `sub_2A653`, called 9x from
`DrawPicture` and sibling picture-draw code, sharing `DrawPicture`'s
stack frame where `g_shadeShiftDelta` is copied to `[bp+var_21]`). It treats
the drawn color as belonging to a 16-entry VGA palette "hue block"
(16 hues × 16 shades) and shifts it by the `g_shadeShiftDelta` delta, clamped
to stay within the same hue block — so `g_shadeShiftDelta` is a **shade-shift
amount** (a distance/light dimming delta), not a remap/color-table
index as such. The two tile-lookup tables' *own* per-cell values still
aren't fully traced, but their role is now understood: they're
plausibly per-cell lighting/distance deltas feeding this shade-shift,
not picture ids.

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
persisted bulk edit. The single-cell counterpart,
`PaintCursorCellAndPersist` (was `sub_20652`), does the same for just
the cursor's current cell (the floor field) — paint, persist, redraw
via `DrawCellIconPair`; `PaintCursorOverlayCellAndPersist` (was
`sub_2070C`) is its overlay/wall-field sibling, at a second cursor
position. `BrowseWallTilePalette`/`BrowseFloorTilePalette` (`B`/`F`) jump the
legend strips to a per-level tile palette read from `WORLD.DAT` via
`LoadWorldDatTilePalette` (FileEntry `bx=0x9043`, record selected by
`_blockSize3*word_329FE`), which calls `PrepareWorldDatRead` — a
generic sibling of `WorldDat_setBlock1`-`6` that also feeds
`DrawClueBookMapGrid`; the palette record layout itself not fully
traced yet. This is a
debug/level-editor screen left reachable in the shipped binary, not a
passive legend. A sibling cluster, then called
`sub_20C8E`/`sub_20CEC`/`sub_20D2F`/`sub_20E12`/`sub_29FF6` — since
named `ExtendDungeonCeilingPass`/`ExtendDungeonCeilingTexture`/
`DrawDungeonFloorAndCeiling`/`ExtendDungeonFloorTexture` and an
untraced tiling helper — was speculated here to draw "two small
'current cell class' preview boxes... to highlight the matching
legend icon". **Correction, from this session's later dungeon-
rendering trace**: that reading doesn't hold up. These functions
draw the actual floor/ceiling pictures read directly from the cell
table (`word_2E498`/`word_2E4A0`, not fixed `g_pictureDir` entries
4/5) via `DrawPicture`, and are called from `RedrawDungeonScreen`/
`RefreshDungeonScreen` (`sub_20C1E`/`sub_20C46`, since named) as part
of the ordinary first-person corridor render — floor/ceiling texture
continuity across matching cells, not a map-editor legend-highlight
box. `IsPairedValueMatch` is still correctly identified as the
fuzzy-equality helper driving it. The driving inputs
(`word_328E6`..`word_328F2`, seven consecutive per-row source-cell
pointers into the level's map data) are now understood
*semantically* — `BuildDungeonViewportCells` (since named) computes
the facing-dependent stride that ultimately produces them, and every
render pass this session traced reads them the same way — but their
literal write site is still not found in the disassembly (no
`mov word_328E6, ax` anywhere), so they're almost certainly filled by
an indirect/computed pointer write. Left as an open question, though
now a much narrower one. `RunMapEditorScreen` also uses
`ClearVideoMemoryRegion` (a partial VGA-segment clear, 2560 bytes at
`0xA000:0000`) before some of its redraws, and
`RedrawMapEditorGrid` to redraw the full visible 40×24 cell grid
(`PersistExploredCell` + `LoadWorldDatTilePalette` + `DrawCellIconPair`
per cell) — the same area `FillVisibleAreaWithSelectedTile` floods.

**Open question — how the two tile-type lookup tables actually work**:
dumped both (`ida_scripts/dump_tile_tables.py`) and the picture-id-like
values they yield (`0x16`-`0x50` range) are far outside `g_pictureDir`'s
10 valid entries, while `DrawMinimap` keeps `g_pictureCategory` (the actual
`g_pictureDir` byte offset `DrawPicture` reads) fixed at `0x90` — entry
9, the small 8×8 icon — for the whole 7×9 loop. So every cell likely
draws the *same* base glyph, and the varying table value instead feeds
`g_shadeShiftDelta`, a parameter `DrawPicture` passes (as `[bp+var_21]`) to
`sub_2A53C` first thing. Checked whether that's a per-cell color/remap
parameter by reading `sub_2A53C` — **ruled out**: it never reads
`[bp+var_21]` at all (it's a local stack-buffer init/copy routine keyed
off different globals, `g_uiScratchFlags2`/`word_2E48E`/`word_2E490`). So
`g_shadeShiftDelta`'s actual role in `DrawPicture` — and by extension what the
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
16-byte entries, indexed as `g_pictureDir + category*0x10` (see the correction below):

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

**Corrected 2026-10-04: the ten entries are picture *categories*, not ten
pictures.** Every picture of a category has the entry's width x height; they lie
back to back in the file, and picture `id` of category `c` is at
`base(c) + id * width * height` (`DrawPicture` indexes the directory with
`g_pictureCategory` = `c * 0x10` and `LoadPictureIntoEms` reads with
`g_pictureId` = `id`; the first 8 bytes of each entry -- the EMS slot-table
segment, slot count, size and LRU cursor -- are filled in at run time by
`InitGraphics`/`Struc1_Allocate`, from the per-category `dx` sizes it passes).
The earlier "10 entries" scan only found these ten descriptors. The categories
tile the file exactly (every division is an integer, both games):

| cat | size | Ch2 count | Ch3 count | content |
|-----|------|-----------|-----------|---------|
| 0 | 318x198 | 15 | 23 | full-screen scenes (SmithWare logo, title, ...) |
| 1 | 210x105 | 101 | 156 | panels and view props: dialog/stat/inventory backgrounds, book, scroll, doors, barrels, beds, fireplaces, mountains |
| 2 | 140x155 | 215 | 270 | monster sprites, 10 frames each (base..base+5 idle, +6..+8 attack, +9 hit flash); Chapter 3 also object sprites (trees, furniture) |
| 3 | 190x110 | 162 | 238 | the large monsters (`MonsterFlagAltSprite`), 10 frames each |
| 4 | 224x74 | 18 | 28 | lower half of the first-person view: sky, floor textures |
| 5 | 224x62 | 12 | 14 | upper half: sky with horizon, ceilings |
| 6 | 56x136 | 55 | 70 | paper-doll bodies (inventory screen), chapter title card, slot grid |
| 7 | 32x32 | 270 | 180 | effect/shop icons, portrait faces, clothing layers |
| 8 | 16x16 | 510 | 340 | mouse cursor, UI buttons, equipment/item icons |
| 9 | 8x8 | 576 | 576 | minimap tile glyphs |

Chapter 2 bases: 0, 944460, 3171510, 7837010, 11222810, 11521178, 11687834,
12106714, 12383194, 12513754 (file size 12,550,618); Chapter 3: 0, 1448172,
4887972, 10746972, 15721172, 16185300, 16379732, 16912852, 17097172, 17184212
(17,221,076). The directory is at `DS:0x782E` (Chapter 3 `DS:0x7B5C`);
`ida_scripts/dump_picture_dir.py` dumps it, `src23/pictures.c` carries the
tables, and `src23/tools/pic_sheet.py` renders any run of pictures to a PNG with
the master palette (used to identify the contents above). Colour `0xFF` is the
transparent key. The tile legends' picture offsets (`worldmap.h`) and the many
`DrawPicture` call sites (ids such as 0x22-0x2F for a 14-frame animation) are
ids within one of these categories.

Loading path: `DrawPicture` (`0x29878`) adds `g_pictureCategory` to the
directory base, calls `LoadPictureIntoEms` (`0x2A68D`) to ensure the picture's
bytes are mapped into the category's small LRU cache of LIM EMS 4.0 pages
(evicting the oldest entry on a miss and reading fresh bytes from
`PICTURES.VGA`, the fixed `FileEntry` at `bx=0x9011`, opened once in
`InitGame`), then blits `width` x `height` pixels to the video buffer at
`(x, y)`; blit mode by `_font_bgTransparent`: 0 plain row copy, 1 skip `0xFF`
(optionally remapping hues), 2 skip `0xFF` and shift the colour by the shade
delta (clamped), 3 the same without a key colour, 4/5 copy a sub-rectangle
(x/y offset and size in `word_32980..88`) skipping `0xFF` / opaque.
`sub_23874` indexes the same directory.

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
or out smoothly. The full boot sequence is now mapped: `InitGame`
calls `PlayTitleScreenSequence` (title picture + theme music, timed
or skippable) then `ShowIntroPicture`; `start` separately calls
`PlayStudioCreditsIntro` (an elaborate multi-scene animated credits
cinematic — several picture reveals, multiple `DrawShadowedTextAlt`
credit panels, sound cues, and two small effect helpers,
`PlayCreditsWipeAnimation` and `PlayCreditsFrameAnimation`) plus its
own separate call to `ShowIntroPicture` for the splash-screen logo
(entry 0, the "SmithWare" logo below).

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
second encoding layer on the base palette; not fully traced. **The
same transform recurs in `PlayCharacterCreationIntroAnimation`** (was
`sub_15429`, character creation's opening animation): its own opening
step runs the identical `al=[si]; al-=0x3F; [di]=al` loop, `0x442A` ->
`0x4D5C`, 768 bytes — almost certainly building the same kind of
palette fade buffer for its own animated sequence (corrected from an
initial "decodes a graphics block" guess when that function was
named). `0x442A` is a heavily-referenced address elsewhere in the
binary, plausibly a shared palette/DAC staging buffer, not traced
further. Also related: `WaitFrameTicksOrEscape` (was `sub_119E0`,
called from `ShowIntroPicture`) is a frame-paced wait-with-abort
primitive — busy-waits for `g_uiScratchFlags1` bit `0x400` ("tick ready",
plausibly raised by an untraced timer/vsync interrupt handler), checks
ESC via `PollForEscapeKeyOnly`, clears the bit, repeats for `cx`
ticks.

Not yet decoded: the real VGA palette (so images render in true color,
not grayscale), and the directory's `+0x4` field's meaning (varies per
entry, didn't fit an obvious role from the entries examined so far).

### Command-line switches

`ParseCommandLineSwitches` (was `sub_16F84`, called directly from
`start` at program entry) scans the PSP command-tail for `/`-prefixed
switches: `/P` sets `g_uiScratchFlags1` bit `0x8000`; `/NOM` sets
`g_uiScratchFlags3` bit `2` (plausibly no-music); `/NOS` sets `g_uiScratchFlags3`
bit `1` (plausibly no-sound) — the latter two tie into the
sound-driver detection below.

### Sound Blaster auto-detection

`InitSoundSystem` calls `ParseSoundBlasterEnvironmentVariable` (was
`sub_28619`), which uses `FindEnvironmentVariable` (was `sub_2D578` —
the classic DOS technique: PSP via `INT 21h AH=0x51`, environment
segment from `PSP+0x2C`, linear-scan the env block for a match) twice
against two env-var name constants, then parses the returned value as
`'A'<3 digits>` (base address, into `word_32916`) and `'I'<1 digit>`
(IRQ, into `word_32914`) — exactly the classic `BLASTER=A220 I5 D1 T3`
format. A nearby string cluster (`BLASTER=`, `SOUND=`, `EMMXXXX0`,
`FMDRV` — the latter matching `SBFMDRV.COM` above) supports this
reading, though the exact addresses of the two name constants didn't
line up byte-for-byte with those strings on manual inspection; the
identification rests on the unambiguous parsing logic rather than a
confirmed string match. `WaitForSoundDriverIdle` (was `sub_2827E`)
is a small companion gate used elsewhere: returns immediately unless
`g_driverStateFlags` bit `0x8` is set, in which case it busy-waits for
`word_2E494` to reach 0. **The top-level sound-event dispatcher,
`TriggerSoundEvent` (was `sub_28412`), is now named too** — resolves a
lead flagged early this session. Takes a command in `ax`: if the
driver isn't active (`g_driverStateFlags` bit `3` clear), only
`ax==3` does anything, calling `PlayPcSpeakerBeep` (was `sub_16DEA`, a
classic PIT-channel-2 + PPI-port-`0x61` PC speaker beep) as a
fallback; every other command is silently ignored. If the driver is
active, it reads driver data (`FileEntry` `bx=0x9043`) and forwards to
the loaded driver's own routine via `g_soundDriverFarPtr(bx=6)` —
likely "load+play a sound effect" in that external driver's own
protocol, which remains unconfirmed since it lives outside this
binary. Its two main consumers are the byte-for-byte-
identical overlay-segment duplicate pair `TryPlaySoundCue`/
`TryPlaySoundCueAlt` (was `sub_16234`/`sub_11EAE`, another instance of
this session's recurring duplication pattern): drop a sound cue if the
driver was busy or the id is the `0xFFFF` sentinel, else dispatch via
their own per-segment tick-wait primitive (`WaitForTickFlagAndClear`/
`WaitForTickFlagAndClearAlt`, since named). A third, structurally
different caller, `TriggerSoundEventAfterDriverWait` (was `sub_2D498`,
called from `ApplyEncodedItemEffect`, was `sub_2C0FE`, since named),
behaves the
*opposite* way: dispatches the sound only when the driver *was* busy
(discarding it unplayed if already idle). It still carries a genuine
IDA "sp-analysis failed" flag on its `ax==0` path (a jump back into
its own `pop` instruction) — left as observed, most likely unreachable
dead code, rather than reinterpreted.

### In-world readable text (decoded 2026-09-23)

**"RunConversation" isn't NPC dialogue — it's found books, notes, and
plaques.** The name was an early, unverified guess. Tracing its real
caller settles it: `RunConversation` is reached from `HandleGameCommand`'s
item-use dispatch, gated on `word_2E548` (the item-classification record
`GetClassifiedItemStatField` populates) — the exact same record
`RepairItemCommand`/`InteractWithContainer` read right next to it in the
same function. Using/reading an in-world item — a book, sign, plaque, or
note — is what triggers it, not talking to a character.

**Four independent id-indexed text pools**, one per `word_2E548[+2]` flag
bit (`0x4000`/`0x2000`/`0x1000`/`0x800`), each read via its own
`LookupConversationTextBlockOffset_*` stub (`yendor2.asm:42441` on).
Unlike every other `WORLD.DAT` catalog documented so far, **the index
tables themselves are baked into the executable, not `WORLD.DAT`** — two
small static tables per category, `(offset-lo, offset-hi)` dwords and
`length` words, both indexed by a 1-based id. Only the actual *text* lives
in `WORLD.DAT`. All four categories' real entries turned out to tile one
small contiguous span exactly, no gaps: **Chapter 2: 14,332 bytes at
`0x15181C`; Chapter 3: 11,986 bytes at `0x3C2030`** — confirmed by every
entry's `offset` and `length` adding up perfectly to the next entry's
`offset`, end to end, in both games.

**Record format**: fixed-width lines, category-specific width, NUL after
the real content, no cross-line word-wrap (pre-wrapped at authoring time,
same convention already noted for `DrawWordToken`). Line count = byte
length / line width, with a **zero remainder on every single real entry
in both games** — the strongest possible structural confirmation of each
category's width. Leading/trailing spaces are meaningful (used to center
short lines) and not trimmed by the reimplementation.

| Category (was) | Line width | Ch2 entries | Ch3 entries |
|---|---|---|---|
| Journal (`_4000`) | 16 bytes (15 chars) | 6 | 8 |
| Note (`_1000`) | 23 bytes (22 chars) | 1, empty | 8 |
| Document (`_2000`) | 22 bytes (21 chars) | 26 | 26 (only 6 real; 7-26 are real, present, zero-length entries) |
| Unused (`_800`) | 23 bytes (22 chars) | 1, empty | 1, empty |

Category names describe what was actually found in the text, not a
confirmed official taxonomy — **Journal** mixes dated `MM/DD/YYY`
history/lore entries and undated verses (Ch2 id 6 is a short poem, no
date); **Note** holds short in-world messages — one Chapter 3 entry
(id 3) is *literally a spoken password*, `THE PASSWORD IS` `` `RUSE~ ``,
signed `-QUEEN OBVERSIA` — worth revisiting against the "region
password"/"town password" open question below; **Document** holds longer
found documents, letters and short stories (Chapter 2 id 1 is a puzzle
with a numbered diagram and answer sequence `1-2-3-4-5-1-4-2-5-3-1`;
Chapter 3 id 1 is a short story titled `%THE BLACK CAT%`, `BY JASPER`);
**Unused** has no real content in either game examined — wired up in the
engine (the driver code and lookup stub both exist) but with an empty
index table, like a few other "engine supports more than this chapter's
data uses" cases already documented (Chapter 2's 15 slack item-catalog
records, Chapter 3's empty monster block 62). Note: the game's font
renders `` ` `` and `~` as opening/closing curly-quote glyphs, and `~` also
substitutes for an apostrophe (`I~VE` = "I'VE") — a DOS custom-font
limitation, not a transcription error.

**Not yet traced**: which field of an item's own catalog record supplies
the `(category, id)` pair that reaches `RunConversation` — i.e. which
items are books/notes/plaques and what they each say isn't mapped, only
the text-storage mechanism itself. Reimplemented in `src23/document.c`
(`ida_scripts/dump_document_tables.py` in both `yendor2/` and `yendor3/`
extracts the underlying tables).

### Movement and cell passability (decoded 2026-09-23)

`HandleMovementInput` (`yendor2.asm:2004`, `yendor3.asm:5638`) is the
shared handler for all 6 movement/turn actions — forward, backward,
turn left, turn right, strafe left, strafe right — reached both from
the arrow keys (BIOS extended scan codes `'H'`/`'P'`/`'K'`/`'M'`
remapped by the input layer) and from an on-screen 6-button
directional pad (a mouse hit-test dispatcher right before
`HandleMovementInput` maps its 6 click zones to the same key codes).
Its key dispatch is a 5-way explicit compare (`'H'`/`'P'`/`'K'`/`'M'`/`'s'`)
with everything else falling through to a 6th, implicit branch — the
caller only ever sets `g_lastKeyChar` to one of the 6 real action keys,
so the unlabeled 6th branch is a real action too, not a dead default.
Traced instruction-for-instruction identical between both games (same
key set, same facing-bit remap, same relative position deltas) —
reimplemented once in `src23/movement.c`/`.h`.

**Facing and turning**: `g_partyFacing`/`word_328D2` is one of the four
`SaveFacing` bits (`file-formats.md`'s CURGAME header field). `'K'`
(Left arrow) rotates counterclockwise (N→W→S→E→N) by remapping the bit
directly, no numeric direction index; `'M'` (Right arrow) rotates
clockwise (N→E→S→W→N), the exact mirror. Turning changes only facing,
never position.

**Moving/strafing**: the other 4 actions add a facing-dependent
`(Δcol, Δrow)` to the party's world position (`bx`/`cx`, later added to
`g_partyWorldX`/`g_partyWorldY`; the matching grid-pointer delta `dx`,
in ±8/±0x270 units, is the in-memory dungeon grid's per-cell/per-row
stride — see "In-memory dungeon map grid" above):

| Action | Key | North | South | East | West |
|---|---|---|---|---|---|
| Forward | `'H'` | row −1 | row +1 | col +1 | col −1 |
| Backward | `'P'` | row +1 | row −1 | col −1 | col +1 |
| Strafe left | `'s'` | col −1 | col +1 | row −1 | row +1 |
| Strafe right | (6th/implicit) | col +1 | col −1 | row +1 | row −1 |

**Playable bounding box**: the destination `(col, row)` is rejected
before any tile lookup if outside a fixed rectangle — **Chapter 2**:
col `0x28`-`0x2F7`, row `0x18`-`0x77`; **Chapter 3**: same columns, row
`0x18`-`0x8F` (taller, matching Chapter 3's 168-row map vs. Chapter
2's 144-row map, see "World map" above). This is a strict subset of
the full map grid — the bordering "void" band (wall type alternating
0/1) sits outside it.

**Per-cell decision**, checked in this exact order once the
destination is in bounds:
1. **Door/lock** — if the destination's in-memory grid cell (`+6` flags
   word) has bit `0x6000` set, the move is always rejected: the code
   calls `SelectPartyRecordById`, `TryInteractAtPosition`, then
   `ShowLockStatus` (not reimplemented here) and falls straight into
   the "blocked, play a sound" path — it never reaches the
   position-commit code, regardless of any other flag.
2. **Special wall type** — if the wall type (`+0` word, `worldMapTileA`)
   is in the game's special range, the move always commits, and
   `HandleSpecialCellEntry` (not reimplemented here) is called first.
   Chapter 2: `6`-`11` (`_val32`/`_val31`, from
   `InitGlobals`); Chapter 3: `200`-`299` (`ds:5450h`/`5452h`) — see
   below, this is exactly Chapter 3's second "blocked" band from its
   own `ClassifyFloorType`.
3. **Force-move override** — `g_uiScratchFlags1` bit `0x8000` (set by
   callers not yet traced) skips the remaining checks and commits
   immediately.
4. **`ClassifyFloorType`** (`yendor2.asm:1810`, `yendor3.asm:5474`), on
   the wall type — a 3-way result, not a simple low/high split:
   - **Chapter 2**: void `{0,1}` (silent block, matches the map's own
     border band); blocked `[2,15]` (bump sound; note this fully
     contains the special range `6`-`11`, which is intercepted at step 2
     before this runs); normal `[16,57]`; blocked again `58+`
     unbounded — anything past the real 58-entry wall-type table also
     blocks, it isn't treated as open floor.
   - **Chapter 3**: void `{0,1}`; blocked `[2,99]`; normal
     `[100,199]`; blocked `[200,299]` (exactly the special range from
     step 2); normal `300+` unbounded.
   A void result blocks silently; a blocked result plays a bump sound
   (`TriggerSoundEvent` with `_val33` = 6 in Chapter 2) and blocks.
5. **`IsCellTypeImpassable`** (`yendor2.asm:1849`, `yendor3.asm:5505`),
   on the floor/overlay type (`+2` word, `worldMapTileB`) — only
   consulted once step 4 returns "normal". **Chapter 2**'s impassable
   set is three disjoint pieces, not one range: `[21,35]`, the single
   value `37` (`36` and `38` are passable gaps), and `[39,42]`.
   **Chapter 3**'s is one contiguous range, `[200,399]`. An impassable
   result always plays the bump sound and blocks (never silent) — the
   original reuses the same "was it errorCode 2?" check from step 4 to
   decide silent-vs-sound, and `IsCellTypeImpassable` never produces
   that value, so this path is always the sound variant.
6. Otherwise the move commits: world position and the grid pointer
   advance, `RevealCellsAroundPlayer` runs (automap reveal), then (for
   move/strafe only, not turns) monster processing, side-trap
   processing, and map-trigger effects run.

**Turning re-checks the current cell.** Since `'K'`/`'M'` never touch
`bx`/`cx`/`dx`, the shared tail code above still runs with a zero
delta — i.e. turning in place re-evaluates the *party's own* cell
through the exact same door/special/passability logic. In practice
this can't reject the turn (the party is already standing there), but
it does mean a door cell re-shows lock status and a special cell
re-triggers `HandleSpecialCellEntry` on every turn, not just on entry
— not yet confirmed against real gameplay, just what the branch
structure implies.

Reimplemented in `src23/movement.c`/`.h`: `movementApply` (pure
direction/turn math), `movementBounds`/`movementInBounds`,
`movementClassifyFloorType`, `movementIsFloorTypeImpassable`,
`movementIsSpecialWallType`, and `movementClassifyCell` (the full
per-cell decision, steps 1-5 above, as one function returning an
outcome enum). `HandleSpecialCellEntry` and `ShowLockStatus` themselves
are out of scope for this module — the caller is expected to invoke
them based on the returned outcome.

### World object index: a previously-undocumented `WORLD.DAT` block (decoded 2026-09-23)

A sparse per-cell index of doors/locks, searchable/found-item triggers,
and scripted monster-spawn markers — the actual source of the door/lock
flag `movement.c`'s `movementClassifyCell` takes as a plain `isDoor`
bool, and what `TryInteractAtPosition` (`yendor2.asm:30813`,
instruction-identical in Chapter 3) consults while baking the
in-memory dungeon grid's per-cell flags (see "In-memory dungeon map
grid" above). Not referenced anywhere else in this document before now
— found this session by following `TryInteractAtPosition` into
`FindObjectAtPosition` (`yendor2.asm:30970`).

**Location, found via the resource-block-setup stub family**
(`extract_resource_stubs.py` in both games' `ida_scripts/`, matching
the pattern already used to confirm the master-palette offset):
`PreloadWorldDataTable` (`yendor2.asm:3661`, `yendor3.asm:27171`)
allocates a fixed `0x677`-paragraph buffer and reads `0x6768` (26,472)
bytes from `WORLD.DAT` in one shot via `PrepareWorldDataTableBlockRead`
— **Chapter 2: offset `0x1A1141`; Chapter 3: offset `0x41090D`**, same
size both games (identical `0x677`/`0x6768` constants in both games'
`InitGlobals`). Identified by cross-referencing the dumped stub table
against the seven already-known `CURGAME` section offsets (all matched
exactly, confirming the extraction method), then position-matching the
remaining `WORLD.DAT`-block stubs between the two games' otherwise
identical stub orderings.

**On-disk/in-memory shape** (traced from `FindObjectAtPosition` and
confirmed structurally against both real `WORLD.DAT` files — every one
of 720 columns' lists sorted ascending by row with zero malformed
terminators in either game): a **720-entry array of 16-bit byte
offsets**, one per playable world-X column (`0x28` through `0x2F7`,
indexed by `worldX - 0x28`), each pointing — relative to the block's
own start, not the file — to that column's list of **6-byte records**,
terminated by row `0xFFFF`:
- `+0` row (`u16`) — the list is sorted ascending by this field, and
  the original scan relies on that, stopping as soon as a list entry's
  row exceeds the query.
- `+2` flags (`u16`) — see below.
- `+4` value (`u16`) — meaning depends on which flag bit is set.

`FindObjectAtPosition`'s own bounds check before the lookup is exactly
`movement.h`'s `MovementBounds` (same four global constants) — reused
directly by `src23/worldobjects.c` rather than duplicated.

**Flag bits**, tested by `TryInteractAtPosition` in this priority
order (real data never showed more than one of these bits set on the
same record in either game):
| Flag | Ch2 records | Ch3 records | Behavior |
|---|---|---|---|
| `0x8000` | 443 | 355 | `LoadLockState(value)` — a lock/door id; always blocks movement. |
| `0x4000` | 231 | 71 | `LoadCurgameRecord(value)`, an index into `CURGAME`'s event-state section; further outcome depends on `g_lockStatusFlags` bits not reimplemented here — plausibly searchable containers/found-item/document triggers. |
| `0x1000` | 105 | 139 | Always reports a fixed `errorCode=4`; `value` isn't read for this branch at all. Semantic meaning of `errorCode=4` not confirmed. |
| `0x800` | 2141 | 1862 | `TestCellMonsterSpawnedFlag(value)` — `value` is a monster *type id* directly (confirmed 2026-09-23 by cross-referencing `RefreshDungeonMapWindow`'s despawn path, which clears the same bitmap using a live monster's own type id — see "Monster pool" below), not an abstract spawn-point index. The majority flag in both games' real data. |
| `0x2000` | 187 | 139 | **Not tested by `TryInteractAtPosition` at all** — every flag check above it falls through to "nothing here" for a `0x2000`-only record. `ProbeFacingTile` (`yendor2.asm:30915`) reaches the same underlying record via the same `FindObjectAtPosition`, but its own callers weren't traced far enough this session to confirm whether any of them read this bit. |
| `0x400` | 1 | 6 | Rare outlier in both games, not chased further. |

(Counts above are the *reachable* totals — i.e. only records whose row
also falls within the playable bounding box, which is what
`worldObjectFind`, mirroring `FindObjectAtPosition`'s own bounds check,
can ever return. Chapter 2's raw on-disk data has 3 additional dead
records with rows outside that range, e.g. two around row 998 —
provably unreachable by the real game too, since `FindObjectAtPosition`
rejects any out-of-bounds query before the scan even starts.)

**Not yet traced**: the real `WORLD.DAT` offset/format of whatever
`LoadCurgameRecord` reads using a `0x4000` record's `value`, and the
`0x400` bit's consumer, if any. `LoadLockState`'s own format (the
`0x8000` door/lock case) is now fully traced — see "Lock/door
definition catalog" below. The `0x2000` bit's own consumer is now
found — see "Party teleport/fast-travel destinations" below.
Reimplemented (lookup only, not the deeper state resolution the
`0x4000` branch feeds into) in `src23/worldobjects.c`/`.h`:
`worldObjectTableParse`/`worldObjectTableParseWorldDat` and
`worldObjectFind`, tests in `tests/test_worldobjects.c` including
exact reachable-record and per-flag counts checked against both real
`WORLD.DAT` files.

### Party teleport/fast-travel destinations, including a literal typed PASSWORD system (mechanism fully resolved 2026-09-30, data extracted for both games, not yet reimplemented)

`WorldObjectFlagUnknown2000` (`0x2000` — real, common in both games'
data, previously untested by any traced caller) turned out to be a
teleport/fast-travel waypoint marker — the long-open "region/town
password" mystery this project has carried since an early session (see
`roadmap.md`'s "Open questions"). `start`'s own per-cell dispatcher,
reached the same way `interactKnock`'s own probe is (`ProbeFacingTile`,
`yendor2.asm:30915`), tests this bit directly against `[si+2]` (the
`WorldObjectRecord`'s own flags field) and, on a match, calls
`TravelToDestination(ax = [si+4]`, the record's own `value` field`).
The "region/town password" name turns out to be completely literal:
unlocking a locked destination can mean **typing an actual English
word or phrase** at a prompt, character-compared against a fixed
string embedded in the data — not a metaphor for a flag check.

**`TravelToDestination`** (`yendor2.asm:18170`, `yendor3.asm:10081`)
looks up a destination table by 1-based id:

| | Chapter 2 | Chapter 3 |
|---|---|---|
| table address | `DS:0xD40B` | `DS:0xBA95` |
| stride | 16 bytes | 18 bytes |
| entry count | **exactly 187** | **exactly 139** |
| `+0`/`+2` | world X/Y (i16) | world X/Y (i16) |
| `+4` | facing (one of the 4 `SaveFacing` bits) | facing (same) |
| `+6` | arrival sound id (0 = silent) | arrival sound id |
| `+8` | unused — never read by `TravelToDestination` | → `ds:0xCF2F` (read after the map redraw; meaning not traced) |
| `+0xA` | day-track music id → `word_36CB1` | → `ds:0xCF31` (same position as Ch2's `+8`/`+0xA` pair, shifted) |
| `+0xC` | night-track music id → `word_36CB3` | plain "travel mode" value (1-7 observed) → `ds:0xCF33`, copied verbatim, not bit-tested |
| `+0xE` | flags: `0x2000` = needs-unlock gate; `0x8000`/`0x4000`/`0x1000` mutually-exclusive travel-mode bits; `0x800` = hardcoded landing-spot override | flags: `0xC000` (`0x8000\|0x4000` together) = needs-unlock gate, reused again *inside* `IsDestinationUnlocked` itself (see below) |
| `+0x10` | (doesn't exist, 16-byte stride) | bit `0x1` conditionally zeroes `ds:0xCF3F`; whole word also copied to `ds:0xCEF9` |

Both games' entry counts are **structurally exact, not estimated**:
Chapter 2's table base (`0xD40B`) plus `187 * 16` lands exactly on
`IsDestinationUnlocked`'s own gate-table base (`0xDFBB`); Chapter 3's
(`0xBA95` plus `139 * 18`) lands exactly on its own gate table
(`0xC45B`) — the destination table runs right up against the next
table with zero gap or terminator, so the entry counts are provable by
address arithmetic, not just "where the data looks like it degrades."
Both match `WorldObjectFlagUnknown2000`'s own reachable-record counts
(187 Chapter 2, 139 Chapter 3) exactly — one destination-table entry
per `0x2000`-flagged world marker, confirmed 1:1 both games.

The `word_36CB1`/`word_36CB3` identification (day/night music track
id) comes from an existing IDC comment on a separate function
(`yendor2.asm:0x28320`, the periodic day/night ambient-music switcher):
it picks `word_36CB1` for daytime or `word_36CB3` for nighttime based
on whether `g_gameClockMinutes` falls in `[0x1A4, 0x474]` (7:00 AM –
7:00 PM) — the *exact same* two boundary constants
`TravelToDestination`'s own hardcoded landing-spot override compares
against, which is presumably not a coincidence (both are "is it
daytime" checks, just for different purposes). Chapter 2's own
landing-spot override is a genuine one-off special case: arriving at
world coordinates `(0x62, 0x53)` during that same night window
redirects to a different hardcoded cell (`0x17A, 0xE3`) — likely a
flooded/blocked-at-night location. **Chapter 3 has no equivalent
override anywhere in `TravelToDestination`**, and Chapter 3's version
additionally calls `ApplyMapTriggerEffect` at the very end, which
Chapter 2's version never calls at all — two more confirmed, real
per-game differences, not just the record-shape/stride change. None of
`ds:0xCF2F`/`0xCF31`/`0xCF33`/`0xCF3F`/`0xCEF9` (Chapter 3) or
`word_36C79`/`word_36CBF` (Chapter 2's travel-mode/ailment-timer
globals, already known from the icon-bar/ailment work but not
connected to this table before now) have been traced far enough to
reimplement the music/mode side effects — left open, see below.

**`IsDestinationUnlocked`** (`yendor2.asm:18260`, `yendor3.asm:10142`)
is the unlock gate, a small, cleanly `0xFFFF`-terminated table (22-byte
stride both games: Chapter 2 exactly 20 entries at `DS:0xDFBB`,
Chapter 3 exactly 35 at `DS:0xC45B`) of:

```
+0  destinationId (u16, matched against the id TravelToDestination looked up)
+2  flagWordAddress (u16, a direct DS-relative address, not an offset)
+4  bitMask (u16)
+6  promptId (u16, passed to ShowConfirmPrompt when the password path is taken)
+8  hasMsg (u16) -- nonzero selects a silent generic-rejection path with a canned message
+0xA..+0x15 (12 bytes) -- password text (only meaningful when hasMsg == 0)
```

Every single flag address in both games' gate tables (`0x94D1`-family
in Chapter 2, `0xCFB1`-family in Chapter 3) lands inside the
already-reimplemented `g_globalFlags` region (`globalflags.c`) — so
unlocking a destination is the *same* global quest/world-state flag
system this project already ported, just addressed by a raw word
offset + bitmask pair instead of `globalFlagTest`'s own 1-based
bit-index convention. No new flag mechanism needed, only a
`(wordOffset, mask)` → `globalFlagTest`-equivalent translation.

The real logic, traced fully in both games:
1. Scan the gate table for a matching `destinationId`; no match (or
   list exhausted) → **unlocked** (returns 1) — a destination with no
   gate-table entry at all needs no unlocking.
2. Match found: test `*flagWordAddress & bitMask`. Set → **already
   unlocked** (returns 1, no prompt at all — this is the persistent
   "solved once, stays solved" case, backed by the exact same
   `g_globalFlags` bits other quest-flag systems already set).
3. Not set, and `hasMsg != 0` → **denied**, shows a fixed rejection
   string via `writeString` (Chapter 2: message id `0x7BCE`; Chapter 3
   additionally sets several UI/text-position globals first) and
   returns 0. No password prompt for these rows at all — 4 of Chapter
   2's 20 rows and 1 of Chapter 3's 35 are this kind.
4. Not set, `hasMsg == 0`: **the password path.** Calls
   `ShowConfirmPrompt(ax = promptId)`; if the player confirms, compares
   up to 12 typed characters against the embedded password text
   byte-for-byte (a space in the stored text ends the comparison early
   — the wildcard/terminator, not a literal space to type), and on a
   full match sets the same `*flagWordAddress |= bitMask` the "already
   unlocked" check reads, returning 2 (a distinct "just unlocked, not
   already-unlocked" code) — any mismatch returns 0 (denied, no
   message). **Chapter 3 adds a third sub-case here that Chapter 2
   doesn't have**: past the gate-table's own `hasMsg`, it re-reads the
   *destination record's own* `+0xE` flags a second time (recovering
   the original destination-table pointer off the stack, since `si`
   itself was repurposed to scan the gate table) — bit `0x4000` means
   silent-deny-with-a-3-line-message (`DrawStringColumn`, `bx=0x9606`)
   instead of a prompt at all, bit `0x8000` means the password prompt
   described above. Chapter 2 always goes straight to the password
   prompt for this case, no such secondary bit test.

**Real extracted passwords** (from
`ida_scripts/dump_travel_destination_table.py`'s output, both games —
confirms the mechanism is exactly what the name always implied):
Chapter 2 — `ALEXANDER`, `DOMAIN`, `OPPOSITION`, `HORSEMAN`, `ANATOLAY`
(shared by 2 different destination ids), `IMPRISONED`, `WHITE`,
`TREES`, `NORTH`, `EAST`, `SOUTH`, `WEST`, `MYSELF`, `SAFARI`, and one
genuinely unreachable row (destination id 125) whose stored "password"
is 8 literal null bytes — can never match typed text, a dead entry in
the same family of "provably unreachable but present in the data"
quirks this project keeps finding elsewhere. Chapter 3 —
`NOBLEMAN`, `GAUNTLET`, `COMPASSION`, `RUSE`, `GEMSTONE`, `CALANTHA`,
`ALLIANCE`, `TIMBER`, `SOLITAIRE`, `DELIA`, `DRAGONSKIN`, and several
rows with an empty password field (`hasMsg == 0` but no text — these
are presumably the ones Chapter 3's extra `+0xE`-bit dispatch above
routes to the silent 3-line-message path instead of ever reaching the
character-compare loop, though the exact bit is per-destination-record
and wasn't cross-checked entry-by-entry against the destination
table).

**Reimplemented 2026-09-30** (same day, continued): the lookup/gate
logic (steps 1-4 above) as `travelDestinationLookup`/`travelCheckUnlock`/
`travelResolvePassword` in `src23/travel.c`/`.h`, both games' full
destination and gate tables embedded as literal C data (extracted by
the two `dump_travel_destination_table.py` scripts). `travelCheckUnlock`
is a pure decision function (no UI) returning one of
`TravelUnlockAlreadyUnlocked`/`DeniedWithMessage`/`DeniedFixedMessage`
(Chapter 3 only)/`DeniedSilent`/`NeedsPassword`; `travelResolvePassword`
is the separate "apply" half, called once the caller's UI layer has
shown the prompt and collected typed text, mutating the caller's
`g_globalFlags`-equivalent buffer on a match via the already-existing
`globalflags.c` API — the same "decide, don't apply" split this
project uses throughout `combat.c`. Each gate row's `(flagPtrRaw, mask)`
pair was precomputed into a 1-based `globalFlagTest` index at
table-authoring time (Chapter 2's `g_globalFlags` base is `DS:0x94D1`,
already documented; Chapter 3's, confirmed by reading
`GetGlobalFlagBitAndWord` directly, is `DS:0xCFAF` — not previously
written down anywhere in this project). Cross-checking every Chapter 3
gate row against its own destination record's `+0xE` flags (a
mechanical, exhaustive check, not a sample) confirmed the "empty
password" rows are **exactly** the ones whose destination has bit
`0x4000` set (21/21) and every real password row's destination has bit
`0x8000` set instead (13/13) — so `TravelUnlockDeniedSilent` (neither
bit set) is provably unreachable with real data, reproduced anyway for
fidelity and covered by a synthetic test. `travelResolvePassword` also
preserves a real, easy-to-miss original quirk: the stored password
comparison stops at the stored text's own space terminator without
ever checking whether the *typed* input has extra trailing characters
past that point, so e.g. typing `NORTHEAST` still satisfies a `NORTH`
password — reproduced exactly rather than tightened into an exact-match
comparison. Tests in `tests/test_travel.c` (19th suite) cover
destination-table boundaries, all unlock outcomes in both games
(including the real `RUSE` destination end to end: needs password →
wrong/short guesses rejected → correct guess sets the flag →
subsequent check reports already-unlocked), and the prefix-match quirk.
**Deliberately still not covered**: the music-track/travel-mode side
effects (`word_36CB1`/`word_36CB3`/`word_36C79`/`word_36CBF` in
Chapter 2, `ds:0xCF2F`/`0xCF31`/`0xCF33`/`0xCF3F`/`0xCEF9` in Chapter
3) — none of these are traced to a confirmed consumer, so
`TravelDestination` carries them as raw, documented-but-undecoded
fields (`rawA`/`rawB`/`rawC`/`rawD`) rather than composing behavior
against unconfirmed inputs. `TravelToDestination` itself (the
orchestration that would call these two functions, apply position/
facing, trigger sound, and redraw) is UI-driving composition, deferred
to the eventual SDL2 layer like the rest of this project's top-level
input handlers.

### `TryInteractAtPosition`: the per-cell interaction dispatcher (decoded 2026-09-24)

The function that actually consumes a `worldobjects.c` record —
`TryInteractAtPosition` (`yendor2.asm:30813`, instruction-identical in
Chapter 3), called once per movement step (and from `UnlockDoorCommand`,
the map-editor debug overlay, and click-to-travel). Looks up the cell
via `FindObjectAtPosition`, branches on the record's flags in exactly
the priority order `worldobjects.h`'s `WorldObjectFlag` already
documents (`0x8000` door > `0x4000` curgame record > `0x1000` fixed
response > `0x800` monster spawn > nothing), and sets a global
`errorCode` the `start` main loop reads to decide whether to autosave
`CURGAME` — `ShowLockStatus`/`HandleSpecialCellEntry` (neither
reimplemented, pure UI/rendering) read it too, to choose what to show.

**`errorCode` values** (kept identical in `src23/interact.h`'s
`InteractOutcome` so the two can be cross-referenced directly):
- `0` — nothing here, or the door/curgame-record/monster is already
  resolved (unlocked, triggered, or spawned).
- `1`/`2` — curgame-record fallback: none of flags `0x10`/`0x8`/`0x40`
  are set; `2` if flag `0x20` is also set, `1` otherwise. (Bit meanings
  unconfirmed — see the "LoadCurgameRecord" note below.)
- `3` — door, `LockFlagMagical` set ("magically locked").
- `4` — a `WorldObjectFlagFixedResponse` (`0x1000`) record; `value`
  isn't even read for this branch. Meaning still not confirmed.
- `5` — a `WorldObjectFlagMonsterSpawn` (`0x800`) record whose monster
  hasn't spawned yet (`TestCellMonsterSpawnedFlag` clear) — the
  "unspawned monster marker" `dungeongrid.c`'s own writeup already
  anticipated but didn't reimplement.
- `6`/`7`/`10` — curgame-record, flags `0x10`/`0x8`/`0x40` respectively
  (tested in that priority order, ahead of the `1`/`2` fallback).
- `8` — door, `LockFlagUnknown40` (`0x40`) set — meaning unconfirmed,
  but now known to be what selects this outcome.
- `9` — door, not magical, `LockFlagUnknown40` clear, and the lock
  record's own `price` field (`lockcatalog.h`) is nonzero.

**A previously-undocumented finding made while tracing this**: both the
door branch and the curgame-record branch test "is this already
resolved?" via the exact same in-memory scratch pair
(`g_lockUnlockedAccumulator`/`g_lockUnlockedMask`), and `LoadLockState`/
`LoadCurgameRecord` (`yendor2.asm:12598`/`12660`) both populate it from
the **same CURGAME section** — section 4 in the byte-layout table above,
previously documented only as generic "byte-addressed state" without
knowing it was bit-packed or shared between two id spaces. Confirmed
end-to-end by reading `UnlockDoorCommand` (`yendor2.asm:45970`), which
writes the bit back with `FileEntry_Write` to that exact same section
after a successful unlock:
- Locks: bit index `lockId - 1` (0-based, matching `lockcatalog.h`'s
  1-based ids), MSB-first within its byte — the same packing already
  confirmed for the explored-map and monster-spawn bitmaps.
- Curgame records: bit index `curgameId - 1 + curgameIdOffset`, same
  packing, offset so the two id spaces don't collide.
  `curgameIdOffset` is `_val10` (`yendor2.asm:56832`) — confirmed
  arithmetically to be exactly Chapter 2's lock count, **608**.
  **Chapter 3's equivalent global (`word_2ECF8`, `yendor3.asm:58618`)
  is read but never written anywhere in the disassembly, always 0** —
  the same always-zero-global quirk already found for
  `LoadCurgameRecord`'s *other*, unrelated EMS-record multiplier
  (`word_3320E`/`_val9`, see "LoadCurgameRecord" below) — meaning
  Chapter 3's curgame-record ids collide with its own lowest lock ids'
  unlock bits in this bitmap too. Two independently-discovered
  always-zero offset globals in the same function is suggestive but
  still not conclusive proof of a genuine bug; not chased further.

Reimplemented in `src23/interact.c`/`.h`: `interactBitmapTest`/`Set`
(the shared bitmap, addressed via `savegame.h`'s `SaveSectionEventState`
directly rather than a caller-supplied buffer, since — unlike
`globalflags.c`'s `g_globalFlags` — this section's exact offset and
size were already confirmed), `interactSelectBranch`,
`interactClassifyLock`/`Curgame`/`MonsterSpawn`, and the composed
`interactClassify`. Tests in `tests/test_interact.c`. Two of the
original's side effects aren't modeled (both belong to the rendering
layer, not the data model): `g_uiScratchFlags3` bit `0x80` being
cleared on entry and set again on the monster-spawn branch.

### Lock/door definition catalog (decoded 2026-09-23)

A flat, ordinary catalog — one 26-byte record per lock id (the same
1-based id a `worldobjects.c` door record's `value` field carries) —
despite the original loading it through EMS paging rather than a
single flat-block read like every other catalog in this project.
Found by tracing `LoadLockState`'s (`yendor2.asm:12600`) EMS mapping
array (`bx=0x55EA`, shared with `loadWorldDat2`/`loadWorldDat3`,
`yendor2.asm:3553`/`3573`) back to those two loaders' own resource
stubs — `PrepareWorldDat2BlockRead`'s fixed table (`si=0xCE17`)
resolves to **`WORLD.DAT` offset `0x7E5BA`** for Chapter 2 (confirmed:
this offset was already present in this session's earlier
`extract_resource_stubs.py` dump, just not yet matched to a consumer),
and `PrepareWorldDat3BlockRead`'s offset (`0x822AA`) is exactly
`0x7E5BA + 0x3CF0` — `loadWorldDat2`'s own read size — confirming
`loadWorldDat2` then `loadWorldDat3` read one contiguous region in two
back-to-back chunks. **Chapter 3: offset `0x8F00A`**, found the same
way. No EMS behavior needs reimplementing: the whole catalog (608
records for Chapter 2, 1008 for Chapter 3, matching
`SaveSectionLockAndShopState`'s `recordCount` in `savegame.h`) is read
directly as a flat array, exactly like every other catalog — confirmed
against both real `WORLD.DAT` files.

**Record format** (26 bytes), cross-referenced against `LoadLockState`'s
own copy (13 words = 26 bytes into scratch starting at
`g_lockStatusFlags`) and `ShowLockStatus`'s (`yendor2.asm:12719`, pure
UI display, not reimplemented) message dispatch:
- `+0` flags (`u16`) — bit `0x20` = "magically locked"; bits
  `0x200`-`0x8000` mark which of the 7 door-key items
  (`BRASS`/`BRONZE`/`COPPER`/`IRON`/`STEEL`/`SILVER`/`GOLD KEY`,
  already cross-confirmed against the Hex Hacking Item Guide's item
  table — see "Item-slot encoding" above) the lock requires. **The
  exact bit-to-key mapping needed address-level verification, not
  just reading the dispatch order** — a script
  (`yendor2/ida_scripts/check_lock_key_strings.py`) resolved each
  message string's real address against the `mov bx, <offset>`
  immediates `ShowLockStatus` uses per bit, giving `0x8000`=BRASS
  (tested first) down to `0x200`=GOLD (tested last) — the reverse of
  what the declaration order alone would suggest. **More than one of
  these 7 bits can be set on the same record** — a genuine bitmask,
  not a one-hot selector: 0/608 Chapter 2 records have more than one
  set, but 123/1008 Chapter 3 records do. `ShowLockStatus` (and this
  module's `lockRequiredKeyType`) resolves a multi-bit record by
  testing Brass first, Gold last. Bits `0x1`/`0x2`/`0x80` are real and
  common in both games' data but their exact meaning isn't confirmed —
  `ShowLockStatus` branches on them, but only to select among a few
  very similar messages, not a different outcome.
- `+2` price (`u16`) — a plain binary value (not packed BCD), shown
  split by 100 into two denominations (`LoadLockState`:
  `value / 100`, `value % 100`); which currency isn't confirmed.
- `+4`..`+25` (22 bytes) — not yet traced by any function read so far.

**`LoadCurgameRecord` (`worldobjects.c`'s `0x4000` flag) reads from
this exact same EMS-backed region, at a per-game-inconsistent base
offset — the base-offset side of this is still open, but what the 2
copied words *mean* once loaded is now fully resolved (2026-09-26, via
a live IDA cross-reference check — see
`yendor2/ida_scripts/check_curgame_record_buffer.py`).** It copies 2
words (4 bytes, not 26) from `si = (id-1)*4 + 26*_val9` (Chapter 2) —
`_val9 = 600`, i.e. starting 15,600 bytes into the region, which is
`26 * 600`, *before* the 608-record boundary this section uses. In
Chapter 3 the equivalent
multiplier (`word_3320E`) is a global that's **read but never written
anywhere in the whole disassembly — always 0** (confirmed via
`yendor3/ida_scripts/check_lock_catalog_size.py`), meaning Chapter 3's
`LoadCurgameRecord` table would start at offset 0, overlapping lock id
1's own record entirely. Whether this is a real quirk, dead/unused
code, or a sign the id spaces don't actually collide in practice
(e.g. `LoadCurgameRecord` might never be called with an id that lands
in the overlap) isn't determined. Separately, Chapter 2's real lock
catalog has an 88-record run of all-zero bytes (ids 513-600) right
before that same `_val9=600` boundary — plausibly reserved/unused
slots (matches the "engine supports more than this chapter's data
uses" pattern already seen elsewhere, e.g. Chapter 2's 15 slack
item-catalog records), but not confirmed. None of this changes
`lockcatalog.c`'s own correctness (it reads the real bytes faithfully
either way) — recorded here so a future session picking up
`LoadCurgameRecord` doesn't have to rediscover it. **This is a
different offset/multiplier than the shared "already
unlocked/triggered" bitmap's own `curgameIdOffset`** (`_val10` = 608,
see `TryInteractAtPosition`'s writeup above) — two separate
mechanisms, both keyed off a lock-count-ish per-game constant, both
apparently disabled (always-zero) in Chapter 3.

**What the 2 copied words mean, resolved 2026-09-26**: they land in
the *exact same two globals* `LoadLockState` populates for an
ordinary lock — `g_lockStatusFlags` (the first word) and a packed
second word (`word_32DD0`) — confirmed by checking `yendor2.idb`
directly (both addresses already carried those names; a plain-text
`.asm` grep for the raw hex offset had missed this, since IDA renders
a named symbol at a read site, not the write site's own `mov di,
<hex>` immediate). A CURGAME "trigger" record and a lock record are
therefore the *same physical shape* read through two different
loaders into the same scratch pair — not a separate format needing
its own decode, just a different source for data every consumer
already expects in lock-shaped form.

The consumer that makes this legible: `UseAbilityCommand`
(`yendor2.asm:12821`) and `HandleSearchCommand` both call whichever
loader matches the target (`LoadLockState` for a lock, `LoadCurgameRecord`
otherwise) and then feed the result straight to `ApplySavingThrowEffect`
(`yendor2.asm:44646`, instruction-identical in Chapter 3) — a
search/lockpicking-triggered magical trap. The packed second word is
`threshold*100 + effectId`: `word_32DD0 / 100` is a saving-throw DC,
`word_32DD0 % 100` is an `effect.h` effect id — `< 50` targets the
character attempting the lock/search alone, `>= 50` (subtract 50 for
the real id) targets every occupied, non-incapacitated party member
instead. A value of exactly 0 means no trap is configured at all.

The original's own two-tier roll structure: one *trigger* roll first
(`FailsSavingThrow`, the attempting character's own `PartyFieldLevel`
against the threshold, with their own field `+0x6C` — `PartyStatThievery`,
confirmed by its enum position — as the resistance bonus: their own
lockpicking/search skill helping them avoid setting the trap off at
all). A failed trigger roll (the "fails" naming is the usual inverted
one — failing means the trap *does* activate) then runs the ordinary
effect pipeline (`RollEffectMagnitude`/`RollEffectResistance`/
`ApplyEffectCost`, i.e. `effectRollMagnitude`/`effectResolveInflictedStatus`/
`combatApplyEffect`) once per recipient — the single target, or each
of up to 4 party members for the whole-party case — each with their
*own* independent resistance roll via `ApplyEffectAndDrawIconBar`'s
usual per-slot processing (the same `threshold` reused for every
recipient, only the resistance *bonus* differing since it's each
recipient's own protections).

This closes out the `interact.h`/`lockcatalog.h` "still-undecoded EMS
record format" framing for `LoadCurgameRecord` specifically — the base
EMS offset per id is still open (a smaller, separate question, see
above), but the record's own on-load *meaning* no longer is.
Reimplemented the packed-value decode as `partyDecodeSavingThrowEffect`
in `src23/party.c`/`.h`. Tests in `tests/test_party.c` cover the zero
case, both target-scope cases, and the exact 49/50 id boundary between
them.

**The roll-and-apply composition itself — done, same day
(2026-09-26)**: confirmed `ApplySavingThrowEffect`/`RollEffectResistance`/
`RollEffectMagnitude` are all instruction-identical in Chapter 3
(matching relative offsets and opcodes throughout, `yendor3.asm:45061`/
`6688`/`6775`, only the DS-relative addresses differ, as usual).
`ApplySavingThrowEffect`'s own trigger roll, plus the
per-recipient application `ApplyEffectAndDrawIconBar` performs for
either target scope, reimplemented as `combatApplySavingThrowTrap` in
`src23/combat.c`/`.h` (kept there rather than `party.c`/`effect.c`
since `party.h` can't include `combat.h` — the same circular-dependency
constraint noted for `effectResolveInflictedStatus`/
`partyDecodeSavingThrowEffect` themselves). Reads `RollEffectResistance`
(`yendor2.asm:14127`) and `RollEffectMagnitude` (`yendor2.asm:14214`)
directly to confirm the per-recipient party record is each recipient's
*own* (`[si+0xC]`, the icon slot's own stored pointer) for both the
magnitude roll's level and the resistance roll's defenderStat/bonus —
not the triggering character's. Two quirks confirmed and reproduced
exactly rather than "fixed": (1) `RollEffectResistance` only rolls a
second saving throw at all when the effect both inflicts something
(`effectInflictedStatus(def) != 0`) and gates on one
(`EffectModeRollResistance` set) — otherwise it's skipped entirely, not
just discounted, so this composition's own RNG-draw count matches the
original exactly rather than merely its outcome; (2) the whole-party
loop (`ApplySavingThrowEffect`'s own `di`-indexed scan over
`SaveHeaderPartySlots`) stops dead at the first unoccupied slot instead
of skipping past it to check the rest — real party layouts are always
front-packed in practice, so this is presumed never observed, but the
disassembly does it and this reproduces it. Also confirmed (and
reproduced by omission): the original never populates a gold/ore
effect's material amount for this call path at all — `RollEffectMagnitude`'s
own low-bit early-out (`test word ptr [di+8], 7`) leaves it at the icon
slot's cleared 0 — so `combatApplySavingThrowTrap` always passes a
zeroed `Bcd4` rather than inventing a nonzero source; a search/lock trap
configured with a gold-cost effect id would, in the original, always
"steal" exactly 0. Tests in `tests/test_combat.c` cover the zero-value
and avoided-trigger cases, a single-target application with an exact
peeked-RNG magnitude check, the whole-party case (incapacitated skip +
stop-dead-at-empty-slot together), and an out-of-range effect id.
`UseAbilityCommand`/`HandleSearchCommand` themselves — the UI-heavy
top-level commands that load a record and feed it here — remain
unreimplemented (both still need the SDL2 UI layer for their
prompt/confirm/message-box scaffolding), but the entire decision-and-
apply logic beneath them is now done.

Reimplemented in `src23/lockcatalog.c`/`.h`:
`lockCatalogParse`/`lockCatalogParseWorldDat`, `lockCatalogRecord`,
`lockRequiredKeyType`/`lockKeyTypeName`, tests in
`tests/test_lockcatalog.c` including exact per-key-type and
multi-bit-record counts checked against both real `WORLD.DAT` files.

### Monster pool: scroll relink and despawn (decoded 2026-09-23)

The rest of `RefreshDungeonMapWindow` (`yendor2.asm:29496` on,
instruction-identical in Chapter 3) after the base grid build (see
"In-memory dungeon map grid" above): a pass over all 80
`g_levelMonsters` slots that keeps already-live monsters in sync with
the freshly rebuilt window, called every time the party moves. This is
**not** how a monster first comes into existence — that's
`SpawnMonsterInFacingDirection`, not traced this pass — only how an
*already-spawned* monster is tracked as the window scrolls around it.

For each non-empty slot (`+0` type id `!= 0`):
- **In window** — `worldX`/`worldY` (`+2`/`+4`) both fall within
  `[gridOrigin, gridOrigin + 78]`, **inclusive on the high end**: a
  genuine 79-wide tracked range, one cell wider than the 78-cell grid
  itself, not an off-by-one to paper over. The slot's `+6` cell-offset
  field is recomputed relative to the new origin (same `row*0x270 +
  col*8` formula as `GetMapCellPtr`), and the corresponding
  `DungeonGridCell` gets `+4` set to the monster's type id and `+6`
  bit `0x400` set — a "monster here" overlay marker. **The one edge
  case where the relative offset is exactly 78** (worldPos exactly at
  `gridOrigin + 78`) has no cell in the 78-wide grid array to write
  the overlay into in this reimplementation — the monster still
  survives the refresh (matching the original's own in-window
  decision), it just doesn't get an overlay baked in that one case.
- **Out of window** — the monster's entire 156-byte record is zeroed
  (despawned) and its spawn flag is cleared via
  `ClearCellMonsterSpawnedFlag(typeId)`.

**The "already spawned" bitmap (`Test`/`Set`/`ClearCellMonsterSpawnedFlag`,
CURGAME's `SaveSectionMonsterSpawnFlags`) is indexed by monster *type
id*, not a per-location spawn-point index** — confirmed by
cross-referencing both call sites: `TryInteractAtPosition`'s `0x800`
(monster-spawn marker) branch tests it with a `worldobjects.c` record's
own `value` field, and this despawn path clears it with the live
record's own type id (`+0`) — the same number space, meaning **a
`worldobjects.c` `0x800` marker's `value` field is the type id it
spawns**, not an abstract spawn-point index as the earlier writeup
speculated. Packed MSB-first within each byte (byte `typeId/8`, bit
`7 - typeId%8`), the same convention already confirmed for the
explored-map and lock "already unlocked" bitmaps.

Reimplemented in `src23/monsterpool.c`/`.h`: `monsterSpawnFlagTest`/
`Set`/`Clear` and `monsterPoolRefreshWindow`, tests in
`tests/test_monsterpool.c` covering the bitmap's bit-packing, the
in-window relink path (including the 78-offset edge case), the
out-of-window despawn path, and spawn-flag clearing on despawn.

### Monster spawning: SpawnMonsterInFacingDirection (decoded 2026-09-23)

How a monster first comes into existence (`yendor2.asm:33008`,
instruction-identical in Chapter 3, including every data table it
reads — checked byte-for-byte against both real executables). Its
catalog-copy, positioning and animation-start steps turned out to
match `monster.h`'s already-written `monsterRecordSpawn`/
`monsterRecordPlace`/`monsterRecordStartAnimation` closely enough that
reimplementing this function is mostly composition of those three,
plus one genuinely new piece:

- **A facing-dependent spawn-position offset table**, one 51-entry
  table per facing (`North` at `0x7090`, `South` `0x70F6`, `East`
  `0x715C`, `West` `0x71C2`), each entry a signed `(dx, dy)` byte pair
  added to the party's world position. Indexed by a "viewport index"
  (`g_viewportRowDepth`, 0-50) that identifies one specific rendered
  cell of the first-person dungeon viewport — the table's shape is a
  perspective viewing cone, not a literal row/column grid: entries are
  grouped by `dy`, the farthest/widest group is 17 entries wide
  (`dx` -8..8), narrowing at each successive `dy` down to a single
  3-cell-wide strip (`dx` -1..1) right next to the party. Confirmed by
  direct extraction (`yendor2/ida_scripts/dump_spawn_offset_tables.py`,
  ported to `yendor3/`), not inferred.
- Find an empty pool slot (linear scan, same as
  `monsterPoolRefreshWindow`'s own scan pattern).
- `SetCellMonsterSpawnedFlag(typeId)` — marks the type as spawned,
  same bitmap `monsterpool.c` already reimplements.
- A small in-EXE ascending table (id-sorted, terminated by id `0`)
  sets the two on-death flags — the exact table `monster.h`'s
  `monsterDeathFlags` already reimplements.

**`TryTriggerMonsterEncounterAtCell`** (`yendor2.asm:30285`) is the
caller: fires once per cell during first-person viewport rendering
(`RenderDungeonViewRow`), gated on `g_viewportRowDepth >= 0x11` (17 —
excludes only the single farthest/widest row from ever spawning) and
the cell's own `+6` bit `0x400` (the "monster here" marker — see "In-memory
dungeon map grid" above), reads `+4` as the type id, skips if
`FindMonsterTypeInLevelPool` says that type already exists somewhere
on the level (**not a probability roll**, correcting an earlier,
casual first read of this function), and calls
`SpawnMonsterInFacingDirection`. **Not reimplemented**:
`TryTriggerMonsterEncounterAtCell` itself (it's rendering-loop glue,
out of scope until the rendering layer exists) and
`TryActivateMonsterByDistance` (awareness-on-spawn, not traced).

Reimplemented in `src23/monsterpool.c`/`.h`: `monsterSpawnOffsetTable`
and `monsterPoolSpawn` (the full orchestration — find a slot, spawn,
place, animate, mark spawned), tests in `tests/test_monsterpool.c`
covering the offset table against the real extracted data and every
failure path (full pool, unknown type id, out-of-range viewport index).

### Monster death: rewards and removal (decoded 2026-09-23)

Two small, clean functions `ProcessLevelMonsters` (monster AI/turn
processing — not traced this pass, see the "side trap"/ambush section
above) calls when a monster's presence ends:

- **`GrantMonsterRewards`** (`yendor2.asm:33151`) stages a dying
  monster's own loot fields (`monster.h`'s `MonsterLoot`: gold, nuore,
  magic ore, experience) into 4 **global staging counters** — not the
  permanent material totals directly; those are only updated later, by
  `ShowLootAndAwardExperience`. Also applies the monster's two on-death
  global-flag deltas (`MonsterFieldFlagOnDeath`/`FlagOnDeath2` — a
  signed 1-based flag index: positive sets, negative clears, zero does
  nothing) via the global-flags mechanism below.
- **`RemoveMonsterFromMap`** (`yendor2.asm:33784`) clears the "monster
  here" overlay (`+4`/`+6` bit `0x400`, see "In-memory dungeon map
  grid" above) from the grid cell the monster's own `+6` field points
  at, then zeroes the whole 156-byte record.

Reimplemented in `src23/monsterpool.c`/`.h`: `monsterGrantRewards`
(staging-counter accumulation + flag deltas) and `monsterPoolRemove`
(recomputes the grid cell from the record's own world position rather
than trusting its raw `+6` offset, so it's safe to call even if the
monster has scrolled outside the grid's current window). Tests in
`tests/test_monsterpool.c`.

### `ShowLootAndAwardExperience`'s staging drain and character leveling (decoded 2026-09-24)

The function `GrantMonsterRewards`'s own doc comment already pointed
to: `ShowLootAndAwardExperience` (`yendor2.asm:33827`, instruction-
identical in Chapter 3) is mostly a "treasure found" UI panel, but two
of its steps are real state mutation, not rendering, and are now
reimplemented:

- **Draining the 4 staging counters into permanent totals.** Tracing
  both `GrantMonsterRewards`' stage-in and this function's drain-out
  against the same four global scratch addresses pins down exactly
  which staging field maps to which permanent counter — genuinely not
  obvious from field names alone, since `monster.h`'s `MonsterLootOre`
  drains into `savegame.h`'s `SaveHeaderOreCounter1` while
  `MonsterLootNuore` drains into `SaveHeaderOreCounter2` (i.e. the
  *second* counter is nuore, not the first, despite the declaration
  order in both structs suggesting otherwise): gold → `SaveHeaderGold`,
  ore → `SaveHeaderOreCounter1`, nuore → `SaveHeaderOreCounter2`. The
  original gates the ore/nuore drains on `IsBCDCounterAtLeast` (really
  just "is this staging counter nonzero," gating the UI line, not the
  add itself) — reimplemented as an unconditional `bcd4Add`, which is
  behaviorally identical since adding a zero BCD4 value is a no-op.
- **Awarding experience and checking for a level-up**, once per
  occupied `SaveHeaderPartySlots` entry: incapacitated
  (`PartyStatusIncapacitated`) members don't receive the staged
  experience, but `CheckForLevelUp` (below) still runs for every
  occupied slot regardless — it has its own internal incapacitated
  guard, so this is a no-op for them rather than a special case to
  replicate at the call site.

Reimplemented in `src23/monsterpool.c`/`.h`: `monsterRewardsAward`.
Tests in `tests/test_monsterpool.c`. The panel/sound/portrait-redraw
parts of `ShowLootAndAwardExperience` remain out of scope (rendering).

**`CheckForLevelUp`** (`yendor2.asm:20036`, `yendor3.asm:12025`,
instruction-identical) is pure computation, no UI: walks a per-game
**89-entry packed-BCD XP-threshold table** from a character's current
`PartyFieldLevel`, extracted directly from each EXE via a new IDA
script (`ida_scripts/dump_xp_threshold_table.py` in both games) rather
than guessed — index 0 is the XP needed to advance from level 1 to
level 2, ..., index 88 is level 89 to level 90, matching
`PartyFieldLevel`'s own "capped at 90 by training items" ceiling: the
XP curve alone cannot reach past level 90. If the computed level
exceeds the current one, it's stored into a new field,
`PartyFieldPendingLevel` (`+0x1E`) — **computed but not applied.
Resolved 2026-09-24: nothing ever applies it, because it's purely a UI
eligibility hint** (`ShowLevelUpMessage`'s second display line,
`DrawPartyMemberStatusPanel`'s training-icon glyph), not a staged
value consumed elsewhere. Character leveling in this game is a wholly
separate, **paid mechanic**: `UseTrainingItem` (`yendor2.asm:21510`,
present and BinDiff-matched in both games). XP accumulation past what
the real curve can reach (levels 40-90, per the "effectively
unreachable" sentinel below) is therefore *only* ever reachable via
training items, not automatic play — this explains `PartyFieldLevel`'s
pre-existing "capped at 90 by training items" doc note precisely.

### `UseTrainingItem`: the leveling mechanic itself (decoded 2026-09-24)

Its core state-mutating branch (item-flag bit `0x2`, `yendor2.asm:21550`
on) spends a fixed gold cost (`SaveHeaderGold` vs. a threshold at a
fixed scratch address, `0x512A`, shared by several other `Use*Item`
handlers as "this item's price" — not yet traced back to its own
source, since this project hasn't built the upstream item-use pipeline
`UseItem`/`SelectItemUseRecord` belong to), then:
- `PartyFieldLevel += 1`, capped at 90.
- **Max HP grows by 30% of max Stamina**, and current HP is set to the
  new max (a full heal). Uniform across every class.
- **Max MP grows by a class-base-dependent formula** — confirmed
  against `RestCharacter`'s independent, matching `[+0xE]` reduction
  (an existing, older note in this file already flagged the same
  `cmp 9 / -0xA / cmp 9 / -0xA` pattern there) — using
  `partyClassBase` (already existing in `party.c`) to read the branch
  structure directly rather than guess:
  | Base (1-9) | Class (tier 0) | MP growth (before the final 30% scale) |
  |---|---|---|
  | 1-3 | FIGHTER/MERCHANT/ROGUE | none at all — not even a zero-delta call |
  | 4 | MONK | 100% max Wisdom |
  | 5 | ALCHEMIST | 75% Wisdom + 25% Intelligence |
  | 6 | PALADIN | 50% Wisdom |
  | 7 | MAGE (the unmatched-base default) | 100% max Intelligence |
  | 8 | DRUID | 75% Intelligence + 25% Wisdom |
  | 9 | MARKSMAN | 50% Intelligence |

  The blended figure is then scaled by 30% (same as HP) and added to
  max MP; current MP is set to the new max, same full-heal pattern as
  HP — but only for bases 4-9; bases 1-3 skip both the growth *and*
  the current-MP sync entirely.
- **Every one of the 6 core attributes and 13 skills grows by a flat
  `+2`** (max only), regardless of class.
- Every max-stat growth above silently no-ops for a stat that starts
  at exactly 0 (`AddToStatCapped`'s own "untrained/inapplicable slot"
  skip — e.g. a class with no magic never gets MP created from
  nothing), and is capped at 9999 for HP/MP or 999 for everything
  else.
- **Secondary-class promotion**: at two level thresholds
  (`_val25`=10, `_val26`=30 in Chapter 2 — confirmed real constants,
  `yendor2.asm:3489`), `PartyFieldClass += 10` (promotes tier 0→1 or
  1→2). **In Chapter 3, the equivalent globals (`word_331F8`/
  `word_331FA`) are read but never written anywhere in the
  disassembly — always 0** — so this comparison can never match a
  real level (always ≥ 1), and secondary-class promotion via training
  is **effectively disabled in Chapter 3**. This is the *third*
  independent always-zero-global quirk found in this project (see
  `interact.h`'s `curgameIdOffset` note and the "LoadCurgameRecord"
  note below for the other two, both in unrelated subsystems) — three
  separate instances make a systemic Chapter 3 change more plausible
  than three coincidences, but that still isn't confirmed either way.
  A genuine, sharp Ch2/Ch3 behavioral difference regardless — see
  `engine-diffs.md`.
- **Training also refills already-depleted attributes and skills, not
  just their max** — a real effect easy to miss from the growth
  formulas alone. `UseTrainingItem`'s tail calls two more functions in
  sequence, both now reimplemented too:
  - `SyncPartyRecordStagedStats` (`yendor2.asm:22633`; also called from
    `UseItemType_400`, a different, not-yet-decoded item handler): a
    blind bulk copy, current = max, for every `PartyStat` *except*
    `PartyStatHitPoints`/`MagicPoints` (already full-healed directly,
    above). Confirmed its exact address range by tracing the copy's own
    byte counts (32 then 28 bytes) against the record's field offsets,
    not assumed from the name alone.
  - `RefreshCarryCapacityAndAttributeBonuses` (`yendor2.asm:18962`):
    recomputes `PartyStatCarryCapacity` (current/max) as 10× the
    matching Strength value, and two "excess over 72" bonus pairs —
    new fields `PartyFieldStrengthBonus`/`DexterityBonus` (+ `Max`
    variants) at `+0x38`/`+0x3A`/`+0x78`/`+0x7A` — as 20% of however
    far Strength/Dexterity is past 72 (0 otherwise). This resolves this
    file's older "6 core attributes" note, which had flagged
    `+0x38`/`+0x3A`/`+0x78`/`+0x7A` as feeding
    `RecomputeEquipmentStatBonuses`'s baseline without knowing what
    computed them.
  - This function's own tail call, `RecomputeEquipmentStatBonuses`
    (`yendor2.asm:19907`, instruction-identical in Chapter 3) — also
    now reimplemented, same round, as `partyRecomputeEquipmentStatBonuses`.
    Resets `PartyStatEquipRating1-5` (current/max) from five baseline
    fields — the gap's remaining three slots are now named too,
    `PartyFieldEquipRatingBase1`/`2`/`3` (`+0x32`/`+0x36`/`+0x34` — note
    the swap: base*2* is the gap's *third* slot, base*3* its *second*,
    reproduced exactly) plus the two bonus fields above — then adds
    equipped items' own bonuses on top, resolved via `item.c`'s
    already-existing `itemCatalogRecord`/`itemTargetEntry`/`itemTargetWord`
    (this function turned out not to need any new item-catalog
    decoding at all — `LoadItemCatalogRecord`'s return value in this
    context is exactly `word_2E548`, the target-entry pointer
    `itemTargetEntry` already computes, confirmed by reading
    `LoadItemCatalogRecord` itself rather than assuming):
    - Main weapon (equipment code `0xA`): `EquipRating1` +=
      the character's own **Projectile skill** (current/max — a skill
      value, not an item property); `EquipRating2` += the weapon's own
      target-entry bonus (`ItemTargetAbsorption`, which for a weapon
      is really a damage/accuracy figure despite the field's
      wearable-oriented name).
    - Second slot (code `0xC`, likely off-hand/shield — both this and
      the main weapon are `ItemTargetKind::Weapon`, confirmed by
      `ItemFlagEquipCode0A`/`0C` both mapping there in `itemTargetKind`):
      `EquipRating3` += a melee skill selected by the item's own
      `ItemTargetSlotFlags` bit (`0x4000`→Slashing, `0x2000`→Bashing,
      `0x1000`→Polearm, none of the three → no skill bonus at all);
      `EquipRating4` += the item's own bonus. If `ItemTargetSlotFlags`
      bit `0x1` is also set, `PartyFieldUiFlags` bit `0x20` is set
      (else cleared unconditionally) — meaning still not confirmed, a
      field already used for other UI purposes per its own comment.
    - Every other equipped item (codes `0xD`-`0xF`, then `0x10`-`0x14`):
      `EquipRating5` += each one's own bonus, accumulated across all of
      them.
    An empty slot or an item with no target entry contributes nothing.
    Not called automatically by `partyApplyTraining` (it needs an
    `ItemCatalog` that function doesn't take) — a caller wanting full
    fidelity calls both.

### The ability/spell-unlock table: exactly 6 rows, confirmed architecturally (decoded 2026-09-24)

`UseTrainingItem`'s ability-unlock walk (`yendor2.asm:21764` on, only
reached on an *even* `PartyFieldLevel`) indexes a table at `DS:0xD22B`
by a row derived from the character's class id (via the same
`cmp 9 / -0xA / cmp 9 / -0xA`-style reduction already confirmed
elsewhere, but landing in a *different* 0-9 range than `partyClassBase`)
and a column from `(level/2)-1`. Dumping it directly (a new IDA script,
`dump_ability_unlock_table.py` in both games' `ida_scripts/`) at first
looked like it had up to 10 rows, but rows 6-9's contents didn't look
like ability ids at all — small values mixed with `0x8000`/`0x4000`/
`0x2000`/`0x1000`, exactly the bit values `TravelToDestination` tests
on its own `+0xE` flags field. Checking the arithmetic confirmed why:
`0xD22B + 6*0x50` (six rows of the assumed 80-byte stride) equals
`0xD40B` *exactly* — `TravelToDestination`'s own destination-table base
address (`yendor2.asm:18176`). Chapter 3 shows the identical pattern at
its own addresses (`0xB8B5 + 6*0x50 == 0xBA95`, `TravelToDestination`'s
Chapter 3 destination-table base). **The real table is exactly 6 rows**,
one per class base 4-9 (MONK/ALCHEMIST/PALADIN/MAGE/DRUID/MARKSMAN,
matching the `MP`-growth formula's own base range above) — rows 6-9
were never real, just the tail end of the destination table read
through the wrong lens.

This resolves the row-index question for class base 1-3
(FIGHTER/MERCHANT/ROGUE) too: at **any** tier (not just unpromoted),
their row-index arithmetic lands negative or in the `6-9` range this
session just proved is out of the real table's bounds — reading
`TravelToDestination`'s own data as if it were ability ids. This is a
genuine reachable state in normal play (an ordinary level-2+ FIGHTER,
at any tier, triggers it on every even level), so it's very likely a
real, if minor and probably never-noticed, original-engine bug — or at
minimum, "physical classes have nothing to learn here" implemented via
an unguarded out-of-bounds read rather than an explicit check, rather
than a deliberate design choice with a safe fallback. **Not
reproduced**: `partyAbilityUnlocksAtLevel` returns 0 ids for class
base 1-3 at any tier, instead of reading `TravelToDestination`'s data.

Each of the 6 rows is 20 columns (levels 2, 4, ..., 40 — the real
per-level curve stops mattering past 40, same story as the XP curve
stopping at 39) of 2 `u16` ability-flag-bank ids each (0 = none,
stopping the column's scan — the original never checks a second slot
once the first is 0). **A genuine per-game content difference**: the
ids themselves differ between Chapter 2 and Chapter 3 (e.g. MONK's
level-4 unlocks are ids 7 and 0xb in Chapter 2, but 6 and 7 in Chapter
3) — consistent with every other per-game id space in this project,
but the table *shape* (6 rows, 20 columns, 2 slots) is identical.

Reimplemented in `src23/party.c`/`.h`: `partyAbilityUnlockTable`,
`partyAbilityUnlocksAtLevel`, `partyApplyAbilityUnlocks` — the last one
composes the lookup with `flagBankSet` on `PartyFieldFlagBankCA` and is
now called automatically from `partyApplyTraining`, using the
character's *pre*-promotion class id (matching the original's exact
call order — the ability walk runs before the secondary-class
promotion check). Tests in `tests/test_party.c`, including the
Chapter 2 vs. Chapter 3 content difference and every class-base-1-3
"no valid row" case (tier 0, 1 and 2, to be thorough about the "any
tier" claim).

### Reading back which abilities a character knows (decoded 2026-09-25)

Found while chasing `ApplyEncodedItemEffect`'s two callers
(`RunAlchemyScreen`/`InteractWithContainer`) for context, looking for
where the effect ids it applies get selected in the first place:
`BuildAlchemySpellList` (`yendor2.asm:25154`, `yendor3.asm:23659`,
instruction-identical) is the alchemy/spell-casting screen's "which
spells can this character even attempt" filter, and its core test —
`TestRecordFlag_CA` — is confirmed to be the *exact same*
`PartyFieldFlagBankCA` bit test `flagBankTest` already reimplements
(`SetRecordFlag_CA`, its write-side counterpart, is the very function
`UseTrainingItem` calls that this project already ported as
`partyApplyAbilityUnlocks`'s `flagBankSet` call). In other words: this
is the read-back query for the exact same data `partyApplyAbilityUnlocks`
writes.

The scan itself: test flag indices `1..N` in ascending order (`N` is
an `InitGlobals` constant — `word_3330C` in Chapter 2, confirmed
`125`; the equivalent Chapter 3 global, `1..107`, confirmed by direct
comparison of both `InitGlobals` copies), collecting every set index
into a buffer. `BuildAlchemySpellList` goes on to call
`CheckSpellCastability` per entry (an "can this actually be cast right
now" affordability gate, not traced — belongs with the eventual UI
work) and compute pagination (13 entries/page) — neither reimplemented
here, both pure UI/list-management concerns layered on top of the
data-model query this section covers.

Reimplemented as `partyKnownAbilityIdMax`/`partyKnownAbilityIds` in
`src23/party.c`/`.h`. Tests in `tests/test_party.c`: the two games'
max-index constants, an empty record finding nothing, ascending-order
output, the Chapter 2 vs. Chapter 3 max-index boundary (an id valid in
one game but out of range in the other), the `outCapacity`-vs-true-count
distinction, and a direct round-trip against `partyApplyAbilityUnlocks`
(what one sets, the other finds).

**Deliberately not reimplemented**, all pure UI/rendering with a much
wider blast radius than training specifically:
- `ShowLevelUpMessage`/`DrawItemUseConfirmDialog`/status-panel redraws
  (pure display).
- `RunItemServiceRecipientLoop` (offers the same training item to
  other party members — a whole separate UI flow; the 13%-of-Charisma
  value computed early in `UseTrainingItem`, `word_2E38E`, is a
  parameter into *this* loop, not a character stat — resolves an
  older "feeds a separate growth calculation" note in this file to
  "feeds UI flow control, not a stat").

Reimplemented in `src23/party.c`/`.h`: `partyApplyTraining` (calls
`partyApplyAbilityUnlocks`, then `partySyncStagedStats`, then
`partyRefreshCarryCapacityAndAttributeBonuses`, at its tail, matching
the original's own call order exactly), `partyClassPromotionThresholds`,
`partySyncStagedStats`, `partyRefreshCarryCapacityAndAttributeBonuses`,
`partyRecomputeEquipmentStatBonuses` (not auto-called by
`partyApplyTraining`, see above), and the ability-unlock table
functions above. `cost` is caller-supplied (see above). Tests in
`tests/test_party.c`, including per-class-base MP-formula spot checks
(MAGE, DRUID, PALADIN), the untrained-slot skip, both stat/HP/MP caps,
promotion at both Chapter 2 thresholds plus their absence in Chapter 3,
the current-value sync, the excess-over-72 bonus threshold, the
equipment-bonus formula for the main weapon, off-hand and ring/misc
slots (a synthetic item catalog, not real `WORLD.DAT` data), and the
ability-unlock table.

Two things worth flagging for anyone extending this:
- **A real asymmetry in the threshold comparison**, reproduced exactly
  rather than smoothed over: the *first* step only requires experience
  `>=` the current level's threshold to advance once, but every
  further cascade step (advancing two or more levels from a single
  award) requires experience to be *strictly greater than* the next
  threshold. Landing exactly on a later threshold stops the cascade one
  level short of where a `>=` test would put it — confirmed by direct
  x86 flag reading (`jb` vs. `ja`), not inferred.
- **A genuine Chapter 2 vs. Chapter 3 content difference**: the curve
  itself differs at nearly every entry, and even the "effectively
  unreachable" sentinel repeated for levels 40-89 (once the real curve
  ends at level 39's jump) differs — `90,000,000` in Chapter 2 vs.
  `99,999,999` in Chapter 3. See `engine-diffs.md`.
- The original's first table access is **unguarded** — a level-90
  character (already at the max) would index one entry past the
  table's end, a latent original-engine bug rather than something ever
  observably exercised. `partyCheckForLevelUp` guards it instead
  (returns false without touching `PartyFieldPendingLevel`'s already-
  zeroed value), since reproducing an out-of-bounds read is undefined
  behavior in C and changes nothing observable for level-90 characters
  either way.

Reimplemented in `src23/party.c`/`.h`: `partyCheckForLevelUp`,
`partyXpThresholdTable`. Tests in `tests/test_party.c`, including a
test that exercises the `>=`-vs-`>` asymmetry directly.

### Global quest/world-state flags: bit-packing reimplemented (decoded 2026-09-23)

`file-formats.md`'s existing "Global quest/world-state flags" section
(above) already identified `GetGlobalFlagBitAndWord`/`SetGlobalFlag`/
`ClearGlobalFlag`/`TestGlobalFlag` and the base address (`g_globalFlags`,
`0x94D1`) from an earlier session; this pass traced the exact
bit-packing arithmetic (needed to correctly reimplement
`GrantMonsterRewards`'s flag-delta step) and confirmed it's **1-based
and MSB-first**: flag index 1 is bit `0x8000` of word 0, index 16 is
bit `0x0001` of word 0 (**not** word 1 — the original's division has a
zero-remainder special case that steps back one word, verified
directly by tracing `GetGlobalFlagBitAndWord`'s exact `div`/`shr`
sequence rather than assumed from the 0-based conventions confirmed
elsewhere this session), index 17 is bit `0x8000` of word 1, and so on.

**Still not confirmed**: how many total flags exist, or whether
`g_globalFlags` is itself backed by a `CURGAME` section (`0x94D1`
doesn't match any of `savegame.h`'s known section offsets) or is
purely in-memory/session state — genuinely unresolved, not just
unchecked. Reimplemented in `src23/globalflags.c`/`.h` as pure
bit-packing arithmetic over a caller-supplied buffer (so it stays
correct regardless of how that's eventually resolved) —
`globalFlagTest`/`Set`/`Clear` and `globalFlagApplySigned` (the
signed-index convention `GrantMonsterRewards`/`ApplyItemEffectFlags`
both use), tests in `tests/test_globalflags.c`.

### Monster approach and ambush check (decoded 2026-09-23)

`ProcessLevelMonsters`' movement AI (`yendor2.asm:33367` on) turned out
to not really be movement at all — a monster's world position is never
changed by this function. It only checks whether the party is
grid-aligned and reachable, and if so, arms an ambush from wherever
the monster already stands.

**Whether monsters ever reposition themselves at all — resolved
2026-09-24: no.** Swept every reachable loop over `g_levelMonsters`
in the disassembly (not just `ProcessLevelMonsters`) looking for any
write to a live record's `MonsterFieldWorldX`/`Y` (`+2`/`+4`) outside
its one-time placement at spawn (`monsterRecordPlace`,
`SpawnMonsterInFacingDirection`). Found none. Every other
`g_levelMonsters` consumer either spawns (once), reads/displays,
or removes a record wholesale — combat's turn-order setup
(`BuildCombatTurnOrder`/`ProcessCombatRound`, a separate 3-slot
`g_monsterSlots` combat-staging array copied from live records, now
fully reimplemented — see "Turn-based combat" below), scroll-out
despawn (`monsterPoolRefreshWindow`), combat/area-attack death cleanup
(`GrantMonsterRewards`+`RemoveMonsterFromMap`, both already
reimplemented), and one mechanic first misread in this same paragraph
a round ago — corrected 2026-09-25, still not reimplemented: an
`ApplyEncodedItemEffect` branch (`yendor2.asm:51586`) reached after
relocating the party to a new world position (`g_wipeEffectX/Y` ->
`g_partyWorldX/Y`, following a destination-search loop earlier in the
same branch this project hasn't fully traced — a teleportation-style
effect, not confirmed which item/spell triggers it). Once there, it
scans `g_levelMonsters` for a record whose `MonsterFieldCell` matches
the destination cell and, if found, copies its full 156-byte record
into `g_monsterSlots` slot 1 (`DS:0x525C`) — **not** a UI scratch
buffer as this paragraph previously (and wrongly) described it, but
one of the very same 3 turn-based-combat slots `combat.c` already
reimplements — then zeroes the original `g_levelMonsters` record and
clears the dungeon grid's "monster here" overlay at that cell (the
same record-move pattern `CompactMonsterSlots` uses elsewhere, see
"Turn-based combat" below). In short, this is a teleport effect that
also immediately engages any monster waiting at the destination in
turn-based combat — not a "banish" mechanic as this project's own
docs and memory previously called it; there is no removal-without-
combat case found here after all. Not reimplemented — the preceding
destination-search loop and the still-unnamed `word_328FC`/`word_328D2`
fields it uses need their own pass first. Aside from this one
case-still-being-chased-down mechanism: `g_levelMonsters` records are
created once, occasionally copied out (to a combat slot, not removed
from existence) or removed entirely, and never moved in place.

**Alignment**: a monster only ever engages if it shares the party's
exact world row *or* column — no diagonal engagement, no pathfinding
around corners. If neither matches, nothing happens.

**Direction**: once aligned, `MonsterFieldWound`'s `0x100`-`0x800` bits
(`monster.h`'s `MonsterWoundPartyMustFace*`) record which side of the
party the monster is on — confirmed by cross-referencing
`TriggerSideTrapForRandomPartyMember` (the "side trap"/ambush section
above), which tests the exact same 4 bits against the party's *current*
facing to decide whether an armed ambush/trap should actually resolve.

**Reachability**: a cell-by-cell scan from one step away up to
adjacent-to-the-party, via `ClassifyObstacleAtWorldPosition`
(`yendor2.asm:1878`, `yendor3.asm:5526`) — a **monster-specific**
passability check, genuinely different from `movement.h`'s
player-facing `ClassifyFloorType`/`IsCellTypeImpassable` (confirmed by
reading both independently, not assumed to match):
- **Wall-type band** (matches `movement.h`'s own "blocked" band for
  each game, confirmed the two coincide): Chapter 2 blocks `[2,15]`;
  Chapter 3 blocks `[2,99]`∪`[200,299]`. Outside that band, the wall
  type alone never blocks a monster (unlike player movement, which
  additionally blocks unbounded past-table values in Chapter 2 — a
  real difference between the two passability systems).
- **Floor-type check** (only consulted when the wall type doesn't
  already block) differs sharply between games in a way player
  movement's floor check doesn't: **Chapter 2** is a 6-band alternation
  — clear at `0`, blocked (`"feature"`) `[1,16]`, clear `[17,20]`,
  blocked `[21,62]`, clear `[63,67]`, blocked `68+`; **Chapter 3** is
  just "zero is clear, nonzero blocks". Both errorCode `1` (wall) and
  `2` (feature) stop a monster equally — the distinction exists in the
  original but every known caller treats them the same.
- Scanning is bounded to **5 steps**. **A confirmed Chapter 2 bug**:
  Chapter 2's code only explicitly sets this bound (`cx=5`) on the
  "far" side of the party — the "near" side reuses whatever's left in
  `ProcessLevelMonsters`' own unrelated 80-slot outer-loop counter,
  an uninitialized-register reuse that makes the real bound depend on
  which pool slot index is being processed. **Chapter 3 adds the
  missing explicit `cx=5` for both sides**, fixing it — confirmed by
  direct side-by-side comparison of the two games' disassembly, not
  inferred. This reimplementation always uses the corrected 5-step
  bound for both games.

**Ambush roll**: once a clear path to adjacency is found,
`RandomInRange(100)` is rolled against a threshold from
`MonsterFieldAwareness`'s second bit range (`monster.h`'s
`MonsterAmbushChance*`: `0x1000`→90, `0x800`→75, `0x400`→50,
`0x200`→25, none set→5); success sets `MonsterWoundAmbushPending`,
which is what `ProcessSideTrapsOnMovement` (the "side trap"/ambush
section above) actually checks. Confirmed identical thresholds and
bit assignments in both games by direct comparison.

Reimplemented in `src23/monsterpool.c`/`.h`: `monsterClassifyObstacle`
and `monsterApproachParty`, tests in `tests/test_monsterai.c` covering
both games' obstacle bands, all 4 approach directions, wall-blocked
paths, the step limit, and a successful ambush roll. **Not
reimplemented**: `TryActivateMonsterByDistance` and everything about
how a "trap" pool entry (as opposed to an ordinary spawned monster)
actually gets created — see the "side trap"/ambush section above for
what's still open there. `TickMonsterTimer`'s state machine is now
covered too — see below.

### Monster walking (`IsMonsterStepBlocked`, the tail of `ProcessLevelMonsters`; `monsterpool.c`; decoded 2026-10-04)

Correcting the notes above: the ambush check does not end a monster's turn. Unless it triggers, the monster (aware, timer idle,
whether or not the approach gate/busy flag let the ambush roll run) clears its direction bits (`+0xE &= 0xE0FF`) and **walks one cell
toward the party**: vertical first when it is one row above/below the party, else horizontal toward the party's column, else vertical;
a blocked step retries the other axis once. A free step onto the party's cell starts combat (the record is copied into a combat slot);
otherwise the record's position, `+6` cell offset and the grid's cell flag `0x400`/occupant word move. `IsMonsterStepBlocked`: cell flags
`0xC00` always block; `0x6000` (a door) needs awareness trait `0x10` and the monster then hops **two** cells; the special wall range
(Chapter 2 types 6-11, Chapter 3 200-299) needs `0x14` (also a two-cell hop); wall types 0-1 (water) need `0x1A`; Chapter 2 then blocks
floor types 0x27-0x2A, lets trait `0x10` through, and needs `0x08` for floor 0x25; Chapter 3 instead blocks everything with trait `0x02`;
finally the player's `ClassifyFloorType`/`IsCellTypeImpassable` pair decides.

### Turn-based combat: turn order (decoded 2026-09-24)

A genuinely separate subsystem from the dungeon-exploration monster AI
above, surfaced (but not chased down) while resolving "do monsters
ever reposition themselves" the round before. Combat operates over its
own small, fixed-size pool — up to `CombatMonsterSlotCount` (3) live
monster records, full 156-byte copies rather than pointers into
`g_levelMonsters` (confirmed by `DrawMonsterInfoPanels`' own reads of
these addresses as ordinary `MonsterRecordSize` records, not indices)
— distinct from the 80-slot dungeon pool. Turn-order *construction*
(`BuildCombatTurnOrder`) and per-round death/advancement handling
(`ProcessCombatRound`) are both reimplemented; attack resolution
itself (`ResolveAttack`/`ResolveAttackerActionOutcome`/
`ProcessMonsterAttackTurn`, plus the player-input attack path inside
`HandleDungeonInput`) is a separate, much larger piece not started.

**How the whole thing fits into the main loop**, read directly from
`RunDungeonGameLoop` (`yendor2.asm:10150`) to place these pieces in
context — this is also the dungeon-exploration loop, not a
combat-specific one:
```
g_activeCombatMonster = 0
rebuild_round:
    BuildCombatTurnOrder()        // resets the turn cursor to entry 0
    DrawMonsterInfoPanels(); DrawMouseCursor()
dispatch_turn:
    entry = turnOrder[turnCursor]
    if entry is a monster: ProcessMonsterAttackTurn()      // not reimplemented
    else: HandleDungeonInput()                             // not reimplemented; player's turn
    ProcessCombatRound()
    switch (outcome):
      NoMonstersLeft:  end combat, show loot/XP, return     // one full frame
      NewRound:        ProcessLevelMonsters(); RedrawDungeonScreen();
                        ProcessSideTrapsOnMovement(); goto rebuild_round
      Continue:        redraw panels, goto dispatch_turn    // same frame, next entry
```
Two things worth calling out since they explain behavior this project
had already independently observed in earlier rounds: **every normal
exploration input tick is a degenerate combat round** — with no
monster slots occupied, `ProcessCombatRound` finds no live monster at
all and returns `NoMonstersLeft` after exactly one turn-order entry
(`HandleDungeonInput`, i.e. the player's own action), which is why a
single "frame" of `RunDungeonGameLoop` corresponds to one player input
action. And **`ProcessLevelMonsters`/`ProcessSideTrapsOnMovement`
(the dungeon-exploration monster-AI pass documented above) only run
once a full round is exhausted** (`NewRound`), not every input tick —
consistent with, and now fully explaining, this project's earlier
finding that monster approach/ambush checks are driven by the
movement/input loop rather than by any independent timer.

**`BuildCombatTurnOrder`** (`yendor2.asm:10952`, instruction-identical
in Chapter 3 — checked directly, `yendor3.asm:1929-2018`): builds a
single ordering combining every occupied, non-incapacitated
(`PartyStatusIncapacitated`) party slot and every occupied
(`MonsterFieldType != 0`) monster slot, sorted by Dexterity descending.
The sort is a stable insertion sort — the original only swaps on
strictly-greater, so equal-Dexterity entries keep their original build
order (party slots 0-3 first, then monster slots 0-2). For every
occupied monster slot, it also assigns a random living party target:
`RandomInRange(3)` re-rolled until it lands on an occupied,
non-incapacitated party slot.

**A confirmed-dead branch, deliberately not reproduced, root cause now
fully traced**: before the random-target roll, the original ORs flag
`0x2000` onto the entry it's about to build if the monster's own
`MonsterFieldState` has any of bits `0x3010` set (the same bits
`monsterTickTimer`'s "decrement twice" gate reads — see below,
still-unconfirmed status-effect state). It then immediately advances
`di` past that entry (`add di,8`) and tests `[di+6] & 0x2000` on the
*next*, not-yet-built entry to decide whether to skip the roll — a
stale-register reuse: that memory was zeroed at function entry and
nothing has written to it yet, so the test always reads 0 and the
skip branch can never fire. The `0x2000` write itself is therefore
equally inert — nothing ever reads it back through this path either.
Not reproduced, since reproducing a write-then-read pair that
provably never has any effect would add code with zero observable
behavior.

**Modernization**: the original stores each monster's chosen target as
a raw pointer to the party record. This reimplementation stores a
1-based `SaveHeaderPartySlots` id instead (0 = no living target found)
— the same convention `saveGetPartySlot`/`saveGamePartyRecordById`
already use everywhere else in `src23`, avoiding a raw-pointer field
that wouldn't survive save/load. Also, the original's target-search
loop has no bound at all — reachable only because combat can never
actually be entered with a fully incapacitated party — but that
invariant lives far outside this function, so a defensive 64-attempt
cap was added rather than porting a genuine infinite-loop hazard.

**`SelectActiveMonster`** (`yendor2.asm:11340`, instruction-identical
in Chapter 3): the first turn-order entry that's a monster and not yet
flagged defeated this round — a simple linear scan, capped at 7
entries (4 party + 3 monster) matching `CombatTurnOrderCapacity`
exactly, confirming the original's 14-slot buffer is genuinely
over-allocated (it never populates or scans past index 6).
`BuildCombatTurnOrder` itself calls `SelectActiveMonster` at its own
tail, but only if `g_activeCombatMonster` is still unset —
`ProcessCombatRound` does the identical unset-check before its own
turn-advance branch. **Not reproduced as cached state**: this
reimplementation treats "the active monster" as a pure function of
the current turn order and defeated set (`combatSelectActiveMonster`,
callable on demand) rather than a mutable global re-established from
two call sites — recomputing it is cheap and always gives the same
answer the original's caching was preserving, so `combatBuildTurnOrder`
doesn't call it automatically; see `combat.h`'s own design note.

Reimplemented in `src23/combat.c`/`.h`: `combatBuildTurnOrder` and
`combatSelectActiveMonster`, tests in `tests/test_combat.c` covering
descending sort order, stable ties, incapacitated-party exclusion, the
no-living-party-member edge case (monster left untargeted rather than
looping forever), and active-monster selection/skipping defeated
monsters.

**`ProcessCombatRound`** (`yendor2.asm:11094`, instruction-identical in
Chapter 3): called once per game-loop tick, right after whichever
turn-order entry was current has acted (see the loop sketch above).
Two independent things happen, in this order:
1. **Death scan**: every occupied `g_monsterSlots` entry with
   `MonsterFieldHealth <= 0` (a signed compare) is processed — its
   turn-order entry gets flagged `0x4000` ("defeated"), the active
   monster pointer is cleared if it was pointing at this one,
   `GrantMonsterRewards` stages its loot (see `monsterGrantRewards`
   above), and the record is zeroed (`ClearMonsterSlotRecord`, a plain
   156-byte zero, distinct from but functionally identical to
   `RemoveMonsterFromMap`'s record-zeroing half — that one also clears
   a dungeon-grid "monster here" overlay this pool doesn't have).
2. **Turn advance**, but *only if at least one monster slot is still
   occupied and alive* — this condition (not "did anyone die this
   pass") is what the original's `errorCode == 2` check after the
   death scan actually tests, easy to misread as "if none died". If no
   monster is alive at all (everything either just died or was already
   empty), the function returns immediately signaling "combat over" —
   **this is also why a single `RunDungeonGameLoop` tick during
   ordinary exploration (no monsters present) always ends after
   exactly one turn-order entry**, see the loop sketch above. Otherwise
   it ensures an active monster is set (`SelectActiveMonster` if
   unset), then walks forward from the current turn-order position
   (never wrapping) for the next entry that isn't a defeated monster —
   found: turn advances within the same round; not found (walked off
   the end): the round is over, signaling the caller to rebuild the
   turn order via `BuildCombatTurnOrder` for a fresh one.

**`CompactMonsterSlots`, deliberately not reproduced**: between the
"ensure active monster" step and the turn-advance walk, the original
also calls `CompactMonsterSlots` (`yendor2.asm:34001`), which
physically shifts the remaining live `g_monsterSlots` records into a
front-loaded arrangement among the 3 fixed slot addresses, then
rewrites any `g_combatTurnOrder` entry that still points at a moved
record's *old* address (a linear scan matching by pointer value) and
re-resolves the active-monster pointer the same way
(`RelocateActiveMonsterPointer`, `yendor2.asm:34226` — a small, clever
trick worth recording: before compacting, it temporarily stashes the
active monster's *type id* — dereferencing the soon-to-be-stale
pointer once, before the move — back into the global, then re-resolves
a fresh pointer afterward by matching that type id against the 3
post-compaction slots). All of this exists purely to fix up raw
pointers after records move in memory. This reimplementation's
`CombatTurnOrderEntry` never stores a raw record pointer — monster
entries reference their slot by stable index (0-2) instead (see the
type above) — so there is no stale address to fix up in the first
place, and a defeated slot is simply left zeroed at its own index
rather than physically compacted forward. No difference in observable
combat behavior, only in how "which slots are occupied" is tracked
internally.

Reimplemented as `combatProcessRound` in `src23/combat.c`/`.h`; tests
in `tests/test_combat.c` covering: turn advance with no deaths, reward
granting + record zeroing + defeat-flagging on death while another
monster is still alive, the "no monsters left" outcome, the
"walked off the end, start a new round" outcome (no wraparound), and
that the forward walk correctly skips an already-defeated entry.

### Attack resolution: the primitives, plus the effect-application pipeline they feed (decoded 2026-09-25)

Picked up where turn-order/round processing left off: the actual
combat-damage math, and — a second pass the same day — the pipeline
that consumes it. `ResolveAttack`/`FailsSavingThrow` are small,
fully self-contained functions, reimplemented directly. The function
that composes them into a full attack (`ResolveAttackerActionOutcome`)
is documented in detail below but still deliberately not reimplemented
as a whole, since one of its 3 branches needs an entirely undecoded
item system — but its downstream consumer, previously an open
question, **is now found and traced**: `ApplyEffectAndDrawIconBar`
and the effect-table primitives underneath it
(`RollEffectMagnitude`/`RollEffectResistance`/`ApplyEffectCost`),
whose state-mutating halves are now reimplemented as
`effectRollMagnitude`/`effectResolveInflictedStatus` (`effect.c`) and
`combatApplyEffect` (`combat.c`).

**`ResolveAttack`** (`yendor2.asm:38488`, instruction-identical in
Chapter 3 — checked directly): `ResolveAttack(defense, accuracy,
power)`. Misses (damage 0) if `power == 0`, if `accuracy < defense`,
or if `RandomInRange(55)` exceeds `accuracy - defense`. Otherwise
hits: `damage = (power * (accuracy - defense) + 50) / 100`, clamped
to a minimum of 1 (the original computes this as a genuine 32-bit
`mul`/`div` pair — `DX:AX = power * diff`, then `(DX:AX + 50) / 100`
— not a 16-bit truncated multiply, confirmed by reading the exact
register lifetime across the `mul`/`add`/`div` sequence). In its one
confirmed call site (inside `ResolveAttackerActionOutcome`, see
below), `accuracy`/`power` are the attacking monster's
`MonsterFieldAccuracy`/`MonsterFieldDamage`, and `defense` is the
defending party member's own `PartyStatEquipRating5` — a genuinely
satisfying find, since `party.h`'s own doc comment already flagged
`EquipRating5` (the sum of every miscellaneous/armor-slot equipped
item's bonus) as having no confirmed use for the aggregate; "the
party's defense roll against a monster's physical attack" is exactly
that use. Also called from `HandleDungeonInput`'s own player-attack
path (`yendor2.asm:10654`) and 3 other sites (`yendor2.asm:24358`,
`:52768`, `:52784`) this project hasn't traced yet — presumably other
spell/item-driven damage rolls reusing the same primitive.

**`FailsSavingThrow`** (`yendor2.asm:41947`, instruction-identical in
Chapter 3): `FailsSavingThrow(defenderStat, threshold, bonus)`.
`chance = max(5, 5*(defenderStat - threshold) + bonus)`; rolls
`RandomInRange(100)` against it; returns true ("fails", the effect
applies) if the roll exceeds chance, false ("resisted") otherwise. In
`ResolveAttackerActionOutcome`'s two call sites, `defenderStat` is the
target's `PartyFieldLevel`, `threshold` is the attacking monster's
`MonsterFieldSaveDifficulty`, and `bonus` is the target's own
`PartyStatSurvival` — both plainly-named fields once the defender side
was confirmed to always be a party member (see below). `FailsSavingThrow`
has 3 more call sites this project hasn't traced (`yendor2.asm:14197`,
`:44666`, `:48381`), presumably other status-effect/item-effect
gates reusing the same primitive; kept here as a pure function of
numbers, not tied to a record type, matching the original's own
genericity.

**`ResolveAttackerActionOutcome`** (`yendor2.asm:11173`, called twice
from `sub_16881`, itself called from `ProcessMonsterAttackTurn` —
confirming this whole path is monster-attacks-party only, never the
reverse, which is what pins down `FailsSavingThrow`'s defender-side
field names above): selects one of 3 outcome branches through 3
nested tests, not 2 as first read last round — the second round's own
deeper trace (below) turned up a third:
1. **`g_uiScratchFlags4` bit `0x200`** — set by `SelectTrapEffectVariant`
   exactly when it randomly picked the monster's *special* effect
   (`MonsterFieldSpecialAttack`, 25% chance) over its ordinary one.
   Clear -> straight to branch 1. Set -> continue.
2. **The attacking monster's own `MonsterFieldFlags` bits `0xE00`** —
   set -> branch 3 ("equipment corrosion"). Clear -> continue.
3. **`MonsterFieldGoldTheftAmount` (the attacker's own packed-BCD
   field, see below) compared exactly against 0** — nonzero -> branch
   2 (status-effect application, in practice always a gold theft, see
   below). **Exactly zero -> falls back to branch 1 anyway**, despite
   the special effect having already been selected — a real, confirmed
   fallback path (`IsBCDCounterAtLeast`'s comparison here is an exact
   *equality* test against 0, read from its `jz`, not the `>=` its own
   doc comment describes for its more common caller convention — this
   call site uses the flags differently).

1. **Normal damage roll** (the common case, and the fallback for a
   "special attack selected but has no theft amount configured"
   monster): calls `ResolveAttack` with the defender's
   `PartyStatEquipRating5` vs. the attacker's
   `MonsterFieldAccuracy`/`MonsterFieldDamage` — note this always uses
   the monster's *ordinary* combat stats for the damage roll itself,
   regardless of whether the primary or special effect definition was
   selected for the icon/sound; on a hit, stages the result into a
   6-field "pending combat event" record.
2. **Status-effect application, confirmed to be gold theft in every
   real case found**: calls `FailsSavingThrow` (defender's
   `PartyFieldLevel`/`PartyStatSurvival` vs. the attacker's
   `MonsterFieldSaveDifficulty`); on a failed save, stages the
   attacker's own `MonsterFieldGoldTheftAmount` (a 4-byte packed-BCD
   field, its two halves copied into the event record's magnitude
   fields) instead of a damage number. **Confirmed against real
   `WORLD.DAT` data in both games**: every monster with a nonzero
   value there — Bridge Troll, Harrier, Worker Ant, Rogue, Opposition
   Leader, and Thief in Chapter 2; Thief, Elf Assassin, and Frost Dwarf
   Tower in Chapter 3 — either has no special attack of its own or
   (in every Chapter 3 case and Chapter 2's Thief) has
   `MonsterFieldSpecialAttack` set to effect id 15, `effect.h`'s
   "takes gold, rolls no magnitude" effect (its own `magnitudeMin`/
   `Max` are both 0 in the table, so the game genuinely can't roll a
   meaningful steal amount from the effect definition and must supply
   one directly from the monster record instead — precisely explaining
   why this field exists as a monster-record override rather than
   table data). Named `MonsterFieldGoldTheftAmount` in `monster.h`.
3. **"Equipment corrosion"** (gated on a narrower subset of the same
   flag bits, `0x800`/`0x400`): a weaker-DC `FailsSavingThrow` (bonus
   halved) gating a call to `GetClassifiedItemStatField`
   (`yendor2.asm:19410`), which needs `ClassifyItemServiceTier`
   (`yendor2.asm:19477`). On success, targets the defender's *equipped
   item* rather than HP. **Correction, 2026-09-25**: this was
   described here (and in `combat.h`) for one round as needing "a whole
   item-compatibility-tier classification system this project hasn't
   decoded at all" — overstated. An earlier session had already
   characterized both functions in real depth (see the "Global
   material counters and BCD arithmetic" section's "equipped-item
   durability" writeup, reached via `TickEquippedItemDurability`'s
   ordinary-wear path): `ClassifyItemServiceTier` returns one of 3
   tier codes from an item's own `[+0xC]`/`[+2]` flags (or a 4th
   "wrong item type" code), and `GetClassifiedItemStatField` uses that
   tier to pick one of `word_2E548`'s `+4`/`+8` sub-fields. What's
   genuinely still unconfirmed, narrower than the old framing: whether/
   how `word_2E548` — a scratch structure neither function populates
   itself — holds a value meaningful in *this* combat call path
   specifically, as opposed to its already-documented role in item
   durability/breakage.

**Why `ResolveAttackerActionOutcome` itself still isn't fully
composed**: branch 3's own use of `GetClassifiedItemStatField` in the
combat path isn't independently verified (see the correction just
above), and branches 1/2's downstream consumer, while now confirmed
(see below), still needs composing into the whole outcome function.

Reimplemented as `combatResolveAttack`/`combatFailsSavingThrow` in
`src23/combat.c`/`.h`; tests in `tests/test_combat.c` covering:
`combatResolveAttack`'s zero-power and outclassed-defense misses, an
exact roll-vs-diff-gated outcome check (peeking the same RNG state to
compute the expected outcome rather than looping for a lucky seed),
and the 32-bit damage formula; `combatFailsSavingThrow`'s
guaranteed-resist case (chance far exceeding the roll's maximum) and
an exact roll-vs-clamped-floor-chance check using the same peek
technique.

### The staged combat event's consumer, found: `ApplyEffectAndDrawIconBar` and the icon-bar effect pipeline (decoded 2026-09-25, same day)

Reading `ProcessMonsterAttackTurn` (`yendor2.asm:10768`) in full —
the actual caller of `ResolveAttackerActionOutcome`, not chased down
the previous round — answered the "who reads the staged event"
question directly. `word_32906`, the base address
`ResolveAttackerActionOutcome` writes its 6-field record into, is set
by `ProcessMonsterAttackTurn` itself to `0xC50 + slotIndex*0x14`: one
of 4 entries in `g_partyEffectIconSlots`, the same icon-bar mechanism
`PrepareTrapEffectSlots`/`ApplyItemEffectIconSlot` (both named in
earlier sessions) already reference but that this project had not
yet connected to combat. Right after `ResolveAttackerActionOutcome`
returns, if `g_stagedAttackDamage` is nonzero,
`ProcessMonsterAttackTurn` calls `ApplyEffectAndDrawIconBar`
(`yendor2.asm:13789`), which is what actually reads that slot.

**The full icon slot record (20 bytes), reverse-engineered from every
field this session traced reading or writing it**:
- `+0`/`+2`, `+4`/`+6`: two X/Y draw-position pairs (one per draw
  variant — pure UI, not modeled).
- `+0xA`: the **occupancy flag and effect-definition pointer** —
  `ApplyEffectAndDrawIconBar`'s own scan treats a slot as occupied
  exactly when this is nonzero, then dereferences it directly as
  `g_trapEffectDefs`'s per-effect record. Traced back to
  `SelectTrapEffectVariant` (`yendor2.asm:11373`, called right before
  `ResolveAttackerActionOutcome`): it resolves the attacking monster's
  chosen effect id (`MonsterFieldAttackEffect`, or 25% of the time
  `MonsterFieldSpecialAttack` when the monster has one and isn't
  flagged single-effect-only) via `PrepareTrapEffectSlots`, then
  stashes the definition pointer in `word_32940` — which
  `ResolveAttackerActionOutcome`'s branch 1 copies straight into
  `+0xA` unchanged, and branch 2 explicitly saves/restores around its
  own `FailsSavingThrow` call so it survives to be copied too.
- `+0xC`: the defender's party-record pointer (`word_32908`).
- `+0xE`: the **resolved inflicted-status bits** — `0` if none/
  resisted, or the effect's `EffectInflictMask` bits if a status
  applies. Branch 1 pre-supplies this directly from
  `g_stagedAttackStatusFlags` (bypassing a fresh roll, matching
  `RollEffectResistance`'s own "already resolved" skip case, see
  below); branch 2 doesn't touch it at all, leaving whatever a *prior*
  call happened to leave there — a genuine loose end, not confirmed to
  matter in practice.
- `+0x10`/`+0x12` together: the **magnitude/amount** — `RollEffectMagnitude`
  computes a plain `u16` into `+0x10` alone if it's still 0 (bypassed
  entirely for gold/ore-cost effects, whose own `costFlags` gate the
  roll off); branch 1 pre-supplies `combatResolveAttack`'s own damage
  roll into `+0x10` instead (bypassing the roll, same "already
  resolved" pattern, `+0x12` left untouched); branch 2 pre-supplies
  the attacking monster's own `MonsterFieldGoldTheftAmount` (a 4-byte
  packed BCD value) split across both words — `+0x10` its high digit
  pair (confirmed always 0 in every real record found; no monster
  steals >= 10000 in one hit), `+0x12` its low digit pair. This is why
  `ApplyEffectCost`'s material-spend branch treats `+0x10` as a
  pointer into a 4-byte BCD span rather than a plain word: for a
  gold/ore effect, `+0x10`/`+0x12` together *are* that Bcd4 amount,
  read directly rather than rolled — the same field slot doing double
  duty as "plain `u16` magnitude" or "half of a Bcd4 amount" depending
  on which kind of effect occupies the slot.

**`ApplyEffectAndDrawIconBar`'s own dispatch**, once a slot is
occupied, picks one of 3 sub-pipelines by the effect definition's
`modeFlags`: item expiry (`0x600` — destroys or replaces an equipped
item, via `HandleIconBarItemExpiry`), a capped/floored stat delta
(`0x180`, via `ApplyIconBarStatDelta`), or — the one this session
reimplements — the "normal" trap/attack-effect path:
`RollEffectMagnitude` -> `RollEffectResistance` -> `ApplyEffectCost`,
then draws the effect's icon. **Both other variants are deliberately
not reimplemented this round** — genuinely different mechanisms (item
transformation, direct stat manipulation) with their own call chains
(`RemoveMultiStatEffect`, `ApplyMultiStatEffectForItem`,
`RefreshCarryCapacityAndAttributeBonuses`, `CheckForLevelUp`) not yet
traced.

**`RollEffectMagnitude`** (`yendor2.asm:14214`): if the effect doesn't
already have a magnitude staged (`+0x10 == 0`) and isn't a
gold/ore-cost effect (`costFlags` bits 0-2), computes one — fixed
(`magnitudeMin`, no level scaling) if `EffectModeMagnitudeFixed`;
`magnitudeMin * level` (no random roll) if `EffectModeMagnitudeScaled`;
otherwise `(RandomInRange(magnitudeMax - magnitudeMin) + magnitudeMin)
* level`. `level` is the defender's own `PartyFieldLevel`. Already
fully captured by `effect.h`'s existing `effectMagnitude` (from an
earlier session) except for the RNG call itself and the "should I
even roll" gate, both now added as `effectRollMagnitude`.

**`RollEffectResistance`** (`yendor2.asm:14127`): if the effect
inflicts nothing (`effectInflictedStatus(def) == 0`), no status. If it
inflicts but doesn't roll a resistance (`EffectModeRollResistance`
unset), the status applies **unconditionally, with no saving throw at
all** — a real, easy-to-miss branch (the natural assumption is "no
roll mode set" means "never inflicted"; it's the opposite). Otherwise
rolls `FailsSavingThrow(defenderLevel, threshold, effectResistanceBonus(def,
defenderRecord))` — `threshold` here is `word_32DC0`, set by
`ProcessMonsterAttackTurn` from the attacker's own
`MonsterFieldSaveDifficulty` right before calling
`ApplyEffectAndDrawIconBar`, *not* read from the icon slot at all.
Captured as `effect.h`'s new `effectResolveInflictedStatus`, taking
the saving-throw's own boolean outcome as a parameter rather than
rolling it internally — keeps `effect.c` free of an RNG/combat.h
dependency; the roll itself is the caller's `combatFailsSavingThrow`.

**`ApplyEffectCost`** (`yendor2.asm:14000`): dispatches the effect's
cost (`effect.h`'s `effectSpend`) to HP/MP/HP+MP deduction
(`DeductHPClamped`/`DeductMPClamped`, now `partyDeductHp`/
`partyDeductMp` in `party.c` — clamped at 0, hitting 0 HP also sets
`PartyStatusDead`) or one of 3 material counters via
`SpendMaterialCounterClamped` (gold/ore — **the confirmed combat use**:
a monster whose special attack resolves as branch 2, effect id 15,
steals `MonsterFieldGoldTheftAmount` gold from `SaveHeaderGold`; see
above and `monster.h`). `SpendMaterialCounterClamped` itself
(`yendor2.asm:13939`) is a real, easy-to-miss quirk: its own gate is
strictly `counter > amount` (`ja`, not `jae`) — an exact match between
counter and amount still takes the "can't cover it" clamp path, even
though the arithmetic result would be identical either way; reproduced
exactly as `bcd4SubClamped` in `bcd4.c` rather than smoothed over,
since it changes which path the original's "resource depleted"
overlay fires on. Then ORs the resolved inflicted-status bits into the
defender's `PartyFieldStatusFlags` if nonzero, regardless of spend
type. **Two side effects deliberately not reproduced**:
`ClearPartySlotReferenceOnDamage` (a raw-pointer "who's targeting
whom" scratch table this project already avoids — targets are tracked
by `SaveHeaderPartySlots` id instead, see `combatBuildTurnOrder`) and
`UpdatePartyAverageStatTiers` (`yendor2.asm:19029` — averages 3 party
fields into UI-only display-tier globals: a minimap fog/torch level, a
4-tier weather overlay, and the monster-info-panel detail-reveal tier;
pure rendering bookkeeping, deferred to the eventual SDL2 layer along
with the rest of `ApplyEffectAndDrawIconBar`'s drawing and its own
"resource depleted" overlay).

Reimplemented as `combatApplyEffect` in `src23/combat.c`/`.h` (the
`ApplyEffectCost` dispatch + status write, taking an already-resolved
spend/amount/inflictedStatus rather than reading an icon slot — a
`SaveGame*` and a `const Bcd4 materialAmount` parameter feed the
gold/ore branches specifically) plus
`effectRollMagnitude`/`effectResolveInflictedStatus` in
`src23/effect.c`/`.h`, `partyDeductHp`/`partyDeductMp` in
`src23/party.c`/`.h`, and `bcd4SubClamped` in `src23/bcd4.c`/`.h`.
Tests: `test_effect.c` covers
`effectResolveInflictedStatus`'s 3 cases (no inflict / unconditional
inflict / rolled) and `effectRollMagnitude`'s fixed/scaled/plain cases
(the plain case using the same RNG-peek technique as combat's tests);
`test_party.c` covers HP/MP deduction including the exactly-to-0 edge
case and the death flag; `test_bcd4.c` covers `bcd4SubClamped`'s
normal/clamped/exact-match cases; `test_combat.c` covers
`combatApplyEffect`'s HP/MP/HP+MP cost dispatch, status-flag OR-in
alongside a pre-existing flag, the confirmed gold-theft path against a
real `SaveGame`, and the ore-cost clamp. All 18 suites pass.

### Equipment corrosion: `ClassifyItemServiceTier`/`GetClassifiedItemStatField` (decoded 2026-09-25)

Closes out the specific gap left by the previous round's correction
(see "Global material counters and BCD arithmetic" above for the
original writeup of both functions via `TickEquippedItemDurability`'s
ordinary-wear path): `word_2E548`, the scratch value
`GetClassifiedItemStatField` reads from, turns out to have an entirely
mundane origin — `LoadItemCatalogRecord`'s own return value, exactly
as `RecomputeEquipmentStatBonuses` had already confirmed for a
different caller. `ClassifyItemServiceTier` calls
`LoadItemCatalogRecord` itself as its very first step, so `word_2E548`
is simply "the item just classified" by the time
`GetClassifiedItemStatField` reads it back a few instructions later —
no external population/lifetime question at all for this specific call
path, once traced end to end.

**`ClassifyItemServiceTier`** (`yendor2.asm:19477`, `yendor3.asm:11461`,
instruction-identical): category A is `ItemFieldFlags &
(ItemFlagEquipCode0A|ItemFlagEquipCode0C)` — the *exact* same test
`itemTargetKind` uses for `ItemTargetWeapon`. Category B is
`ItemFieldFlags & ItemFlagEquipCode0D` *alone* — narrower than
`itemTargetKind`'s own `ItemTargetWearable` (which also accepts
`ItemFlagEquipShort`/`EquipRing`), so a wearable without
`ItemFlagEquipCode0D` fails this classification even though
`itemTargetKind` would still resolve it to a target entry. Within
whichever category matched, a second flag pair at the item's own raw
byte offset `0x02` (`0x100`/`0x200` for category A, `0x40`/`0x80` for
category B) picks tier 2 / tier 0 / tier 1 respectively (tier 1 if
neither sub-flag is set). **Not asserted**: whether byte `0x02` here
is the same field `item.h` already documents at that address as
`ItemFieldEffectOffset` ("byte offset into the effect table") — the
bits tested here don't obviously fit that meaning, so this is flagged
as a live tension rather than resolved one way or the other.

**`GetClassifiedItemStatField`** (`yendor2.asm:19410`,
`yendor3.asm:11394`, instruction-identical): if the item classifies,
re-tests the *same* category-A flags (not the tier) to pick
`ItemTargetBreakItemA` (category A) or `ItemTargetBreakItemB`
(category B) from the item's own target entry — the identical
"replaced by this item on breakage" field `TickEquippedItemDurability`'s
ordinary wear-and-tear path already uses. Returns 0 if classification
fails.

**Confirmed against real data**: SLING (item id `0x21E`, Chapter 2) is
category A (a weapon) and its own `ItemTargetBreakItemA` is `667` —
`itemCorrosionReplacement` returns exactly that value for the real
record. BREAD (a consumable, flags `0x100` only) and BAG (flags
`0x2004`, no target entry at all) both correctly fail to classify.

This resolves `ResolveAttackerActionOutcome`'s branch 3 ("equipment
corrosion") down to a fully understood, narrow remaining question: the
branch selects an equipment slot (`0x13A`/`0x142`/`0x146`) by the
*attacker's* own `MonsterFieldFlags` bits `0x800`/`0x400` (named
`MonsterFlagCorrodeWeaponSlot`/`CorrodeSecondSlot` in `monster.h` as of
the next section below — tightening `MonsterFlagSpecialMask`'s old
"modifiers shown next to its special attack" doc comment), reads the
*defender's* equipped item id at that slot, and stages
`itemCorrosionReplacement`'s result into the combat event record.

Reimplemented as `itemClassifyServiceTier`/`itemCorrosionReplacement`
in `src23/item.c`/`.h`. Tests in `tests/test_item.c`: all 3 tiers for
both categories via synthetic records, the category-A/category-B
boundary (a wearable outside category B correctly failing), and the
real-data SLING/BREAD/BAG cases above.

### `ResolveAttackerActionOutcome`, fully composed: `combatSelectTrapEffectVariant`/`combatResolveAttackerAction` (decoded 2026-09-26)

With every underlying primitive now solid (`combatResolveAttack`,
`combatFailsSavingThrow`, `effect.h`'s magnitude/resistance helpers,
and this session's own `itemClassifyServiceTier`/`itemCorrosionReplacement`),
this round composed the full 3-way outcome decision the earlier
rounds had been building toward — the actual disassembly reading was
already done; what remained was assembling it correctly.

**`SelectTrapEffectVariant`** (`yendor2.asm:11373`, `yendor3.asm:2358`,
instruction-identical): picks the attacking monster's ordinary effect
(`MonsterFieldAttackEffect`) or, 25% of the time, its special one
(`MonsterFieldSpecialAttack`) — always ordinary if a newly-named
`MonsterFieldState` bit, `MonsterStateSpecialAttackDisabled` (`0x400`),
is set, or if there's no special attack configured at all (id 0).
Reimplemented as `combatSelectTrapEffectVariant`, returning the chosen
effect id plus whether it was the special one (`isSpecial` — mirrors
the original's own `g_uiScratchFlags4` bit `0x200`, which
`ResolveAttackerActionOutcome`'s own outer dispatch reads next).

**`ResolveAttackerActionOutcome`'s outer dispatch**, reconstructed
precisely (a 3-nested-test structure first written up two rounds ago,
now implemented exactly as documented then): `isSpecial == false` (or
`true` but the attacker has none of `MonsterFlagSpecialMask`'s bits
*and* a zero `MonsterFieldGoldTheftAmount` — the defensive fallback
case, confirmed unreachable in every real monster found so far) goes
to the ordinary `combatResolveAttack` damage roll. `isSpecial == true`
with `MonsterFlagSpecialMask` clear but a nonzero
`MonsterFieldGoldTheftAmount` goes to the status-effect/gold-theft
branch. `isSpecial == true` with any `MonsterFlagSpecialMask` bit set
goes to equipment corrosion. Reimplemented as
`combatResolveAttackerAction`, returning a `CombatAttackerAction`
(a `CombatAttackOutcome` tag plus whichever of damage/gold-amount/
equip-slot-and-item-ids applies) rather than staging into an icon
slot — the caller applies a `CombatAttackDamage`/`StatusEffect`
outcome via `combatApplyEffect` directly.

**The corrosion write-back, resolved 2026-09-29 — a long-open question
finally closed**: `CombatAttackCorrosion` gives the caller
`equipSlotOffset`/`equippedItemId`/`corrosionReplacementId` but,
historically, didn't write the replacement back itself, leaving open
whether the original's own write-back path (`HandleIconBarItemExpiry`)
genuinely matches combat's own staging or just superficially reuses
the same byte offsets for a different meaning. Now that
`HandleIconBarItemExpiry` itself is fully reimplemented (see the
"icon-bar effect-application pipeline" note above), tracing
`ResolveAttackerActionOutcome`'s corrosion branch and
`GetClassifiedItemStatField` (`yendor2.asm:19410`, instruction-identical
in Chapter 3) directly confirmed they match *exactly*:
`GetClassifiedItemStatField(ax=equipped item id)` leaves `ax`
unchanged and returns `bx = itemCorrosionReplacement`'s own result
(0 on failure) — precisely the `(equippedItemId, corrosionReplacementId)`
pair `partyHandleIconBarItemExpiry` expects — and the icon slot's own
`+0x8`/`+0xA` fields hold the *attacker's own selected trap-effect*
id/definition (from `combatSelectTrapEffectVariant`'s own
`PrepareTrapEffectSlots` call), whose `modeFlags` is exactly what the
real `ApplyEffectAndDrawIconBar` dispatch tests to route a slot to
`HandleIconBarItemExpiry` in the first place — no reinterpretation
needed, no coincidence, a clean match end to end.

**How rare this mechanic actually is, confirmed against real data**:
scanning every monster block in both real `WORLD.DAT` files for a
legitimate `MonsterFlagSpecialMask` flag combination paired with an
*in-range* special-attack effect id found exactly **one** hit in
either game — Chapter 3's **CROCODILE** (catalog block 70), whose
special-attack effect (id 22) has `modeFlags` `EffectModeItemReplace`.
Chapter 2 has no such monster at all; its own sole flag-matching block
(index 60) turned out to be an unnamed placeholder with wildly
out-of-range effect ids (21060, against a 45-entry table) and garbage-
looking numeric fields, reachable only via unused/reserved type-id
lookup slots — not a real monster, the same "engine supports more
slots than a chapter's data uses" pattern already seen elsewhere in
this project. So equipment corrosion is a genuinely rare, single-
creature mechanic in practice — but now fully composable and correct
whenever it does trigger.

Reimplemented as `combatApplyCorrosion` (`src23/combat.c`/`.h`),
composing a `CombatAttackCorrosion` outcome with the attacker's own
`CombatEffectSelection` (for its effect definition's `modeFlags`) into
a `partyHandleIconBarItemExpiry` call. Tests in `tests/test_combat.c`
cover the write-back itself (slot replaced, corroded item parked as
the slot's extra field, matching `HandleIconBarItemExpiry`'s own
quirk), a non-corrosion outcome (no-op), and an out-of-range effect id
(no-op, doesn't read past the effect table).

Reimplemented in `src23/combat.c`/`.h`. Tests in `tests/test_combat.c`
cover: `combatSelectTrapEffectVariant`'s disabled-state and
no-special-attack cases plus an RNG-peek check of the 25% roll itself;
`combatResolveAttackerAction`'s plain-damage path (guaranteed hit and
guaranteed miss), the gold-theft path (peeked saving-throw outcome,
both fail and resist), the "special selected but nothing configured"
fallback to plain damage, the corrosion path against a synthetic item
catalog (peeked saving throw, both outcomes, plus the exact slot/item/
replacement-id values), all 3 equipment-slot selections, an empty
target slot, and an equipped item that fails classification. All 18
suites pass.

### `TickMonsterTimer`: a per-monster state machine, mechanism confirmed, trigger not (decoded 2026-09-23)

`TickMonsterTimer` (`yendor2.asm:33320`, `yendor3.asm:33098`,
instruction-identical) is called once per monster per game-loop pass,
from both `ProcessLevelMonsters` (the approach/ambush check above) and
`ProcessMonsterAttackTurn` (combat, not otherwise reimplemented). Its
own control flow and arithmetic are fully confirmed; what triggers it
in the first place is not.

**Gated on `MonsterFieldState` bits `0xFC10`** — for an ordinary
monster with none of those bits set (the common case), this is a
complete no-op. When gated in:
- Subtracts `MonsterFieldTickAmount` from `MonsterFieldHealth`. If the
  result is `<= 0`, the monster's tick has **expired**
  (`MonsterTickExpired`) — health is clamped to 0 and the function
  returns immediately. `ProcessLevelMonsters` treats this as "this
  monster's presence has ended": grants its rewards and removes it
  from the map (see "Monster death" above) — the *same* mechanism a
  combat death presumably also drives, though the combat-damage path
  itself isn't traced.
- If state bits `0x3010` are *also* set, a **second** subtraction of
  the same amount is applied; crossing zero on this second pass still
  counts as expired. Surviving both passes is reported as
  `MonsterTickOngoing` — `ProcessMonsterAttackTurn` skips the
  monster's attack this round either way (`MonsterTickOngoing` or
  `MonsterTickExpired`), only letting a fully-idle (`MonsterTickIdle`)
  monster act.
- Independently, `MonsterFieldTickCountdown` is decremented by 1 every
  time the health check doesn't expire the monster; reaching `<= 0`
  resets the whole mechanism — `MonsterFieldState` is masked down to
  bits `0x3ED` (clearing the gate bits themselves, among others),
  `MonsterFieldTickTarget`/`TickAmount`/`TickCountdown` are zeroed, and
  `MonsterFieldAnim` is reset to `MonsterFieldSpriteBase`.

**Genuinely unresolved**: neither traced caller *sets* any of the
`0xFC10`/`0x3010` state bits or the three `MonsterFieldTick*` fields —
both only read the result. So what this mechanism actually represents
(a status-effect duration? a scripted despawn countdown? something
else) isn't determined; some form of "temporary condition that also
drains health over time, then either kills the monster or wears off"
is the most that can honestly be said. `MonsterFieldTickAmount`
overlapping in role with ordinary combat damage to the very same
`MonsterFieldHealth` field is worth keeping in mind if a real combat
implementation later needs to reconcile the two.

Reimplemented faithfully (the confirmed mechanism, not a guessed
narrative) as `monsterTickTimer` in `src23/monster.c`/`.h`, and
composed with the already-existing pieces — the approach/ambush check,
reward granting, and map removal — as `monsterPoolProcessSlot` in
`src23/monsterpool.c`/`.h`, matching `ProcessLevelMonsters`' exact
per-slot flow (skip if not `MonsterStateAware`; tick; expired ⇒
reward+remove; ongoing ⇒ skip, no approach attempt; idle ⇒ approach
check, gated on `MonsterFieldApproachGate` and `MonsterStateBusy`).
Tests in `tests/test_monster.c` (the tick state machine in isolation)
and `tests/test_monsterai.c` (the composed per-slot flow). **A real
bug caught by the test run itself**: an early draft of the
`MonsterStateBusy` test case crashed — `0x800` (`MonsterStateBusy`)
turns out to also be one of `TickMonsterTimer`'s own `0xFC10` gate
bits, so a real tick ran unexpectedly with uninitialized health/tick
fields and reached the reward-granting step with no staging buffer
supplied. Fixed by giving that test case a harmless, non-expiring tick
setup — a useful reminder that these bit ranges genuinely overlap and
any future caller needs to account for it.

### `TryActivateMonsterByDistance`: the `MonsterStateAware` setter (decoded 2026-09-23)

`TryActivateMonsterByDistance` (`yendor2.asm:34123`, `yendor3.asm:33917`,
instruction-identical including every threshold) is the actual setter
for `MonsterStateAware` — every other piece decoded this session
(`ProcessLevelMonsters`' approach check, `TickMonsterTimer`'s gate)
only ever *reads* that bit as an input. Called from
`SpawnMonsterInFacingDirection` (right after placing a freshly-spawned
monster) and `FindMonsterTypeInLevelPool` (not reimplemented,
rendering-driven).

A no-op if already aware. Otherwise a shared baseline gate
(`viewportDepth > 0x21`/33) must pass first — below that, nothing ever
activates, regardless of `MonsterFieldAwareness`. Above it:
`MonsterAwarenessNever` blocks activation outright; otherwise the
highest-priority tier bit that's set (`Far`, then `Middle`, then
`Near`) requires a stricter threshold be exceeded too (`0x2C`/44,
`0x29`/41, `0x26`/38 respectively); with none of the three tier bits
set, the baseline alone suffices.

**A naming tension, checked against real data, still unresolved**:
read literally as "how close before it notices you," the threshold
ordering is backwards from the names — `Far` requires the *closest*
approach to activate, `Near` the *farthest*. Checked whether this
might instead describe preferred engagement range (ranged/ambush
types staying dormant vs. melee types waking early) against Chapter
2's real monster catalog: **not supported**. No real Chapter 2 monster
uses `MonsterAwarenessFar` at all, and the few that use `Near`/`Middle`
(CARNIVOROUS, FOREST GIANT, both real OGRE entries, SCAVENGER, SPIDER,
GRIZZLY BEAR) are all melee types — every ranged monster (CENTAUR
MAGE, DARK MAGE, EVIL WIZARD, HALFLING, WIZARD, ROGUE, THIEF,
NECROMANCER, SEA DRAGON, ...) uses the *default* tier instead (no
Far/Middle/Near/Never bit at all). So the naming remains genuinely
unresolved — kept the existing names (from an earlier session) rather
than guess at a rename. The same real-data check surfaced two more
unnamed `MonsterFieldAwareness` bits in common use (`0x4` on roughly
two-thirds of real monsters, `0x8` correlating with `MonsterFlagAreaAttack`
monsters) — not chased further this pass.

Reimplemented as `monsterTryActivateByDistance` in `src23/monster.c`/`.h`,
wired into `monsterPoolSpawn` at the same point
`SpawnMonsterInFacingDirection` calls it. Tests in `tests/test_monster.c`
covering the baseline gate, all three tiers' thresholds, `Never`, and
priority when multiple tier bits are combined.

## Not yet examined

- `SBFMDRV.COM` — third-party(?) Sound Blaster FM driver, likely not
  worth reverse-engineering in detail (not game logic).

## Minor curiosities

- **`ErrorTable`'s 4 trailing entries look vestigial.** This
  pre-existing jump table (indexed from `ErrorCheck`) has 16 real
  entries setting `ax` to `offset aXxx`, a genuine message-string
  pointer. Its last 4 (`ErrorExitCode281`/`ErrorExitCode285`/
  `ErrorExitCode289`/`ErrorExitCode289Alt`, was `sub_28A19`/
  `sub_28A1F`/`sub_28A25`/`sub_28A2B`) instead set `ax` to a tiny raw
  value (`0x281`/`0x285`/`0x289`/`0x289`) — far too small to address
  the real message-string block (~`0x28715`+), and the last two share
  the identical value. Plausibly reserved error-code slots that never
  got real message text, or an artifact of shareware content removal;
  not confirmed either way.

- **The shareware "REGISTER TODAY!" demo-boundary nag is dead code in
  this binary, but its data survives intact.** `EnforceDemoBoundary`
  (was `sub_1075E`) checks the party's position against a single
  hardcoded coordinate triple and, if matched, shows a 2-line
  "REGISTER TODAY!" message and blocks movement past that point,
  the classic shareware "edge of the demo area" gate, guarded by
  `g_uiScratchFlags4` bit `0x2`. That bit is unconditionally forced on at
  boot in `start` (`or g_uiScratchFlags4, 2`, right after
  `ParseCommandLineSwitches`) and is never cleared anywhere else in
  the binary, so the check can never actually trigger. A second dead
  branch exists in the game's shutdown sequence (`start`, right after
  `ReleaseEmsHandles`): the same bit gates two DOS `INT 21h AH=9`
  prints, `"Thank You for playing Yendorian Tales Book I Chapter 2"`
  and `"Please register your copy today."`, also unreachable. Reads
  as `g_uiScratchFlags4` bit `0x2` being a "registered version" flag that
  this particular `SW.EXE` build forces on unconditionally -- the
  shareware-era code and its message strings are still compiled in,
  just permanently disabled.

- **A family of functions is reachable only from unresolved raw
  addresses (`seg000:09AB`/`09C3`/`0AD2`/`0AEA`/`0AFA`) very early in
  the binary, outside any function IDA named.** `EnforceDemoBoundary`,
  `DrawDebugPositionOverlay`, `DebugSetFloorTileByNumber`,
  `DebugSetOverlayTileByNumber`, `DebugTeleportToCoordinates`, and
  `DebugToggleViewportCellHidden` are all called this way. Between
  them: typing a floor/overlay tile number directly into the current
  cell (bypassing the map editor's palette-picker UI), typing X/Y
  coordinates to teleport the party instantly, and toggling the
  "hidden" flag of an arbitrary dungeon-viewport scratch cell by
  index — consistent with this being a small developer-only hotkey
  table left wired into the shipped binary (in the same spirit as
  `RunMapEditorScreen` itself, already documented above as "a
  debug/level-editor screen left reachable in the shipped binary, not
  a passive legend"). The dispatch mechanism that actually reaches
  these raw addresses (keyboard scan-code table, low-memory jump
  table, or something else) has not been located.
