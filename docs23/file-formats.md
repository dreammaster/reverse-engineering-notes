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

**Flagged, not renamed**: `word_33302`/`word_33304`/`word_33306` (3
consecutive words) are exhaustively bit-tested — every bit from `1`
through `0x8000` is checked against at least one of them somewhere —
confirming they're a genuine encoded-effect bitmask, not scratch. But
the exact word-pairing differs by consumer: `ApplyEncodedItemEffect`
reads `word_33302`+`word_33306` as the effect-type switch, while
`ApplyTargetResistancesToAttack` reads `word_33304`+`word_33306`
(filtering by target immunity into `g_stagedAttackStatusFlags`, and
comparing `word_33306` against resistance-category bits to halve
`g_stagedAttackDamage`). `word_33306` being shared between both readers
suggests one underlying "encoded effect" descriptor consumed from two
angles (item/spell effect application vs. attack resistance
filtering), but the precise field boundaries aren't confirmed enough
to name individually yet — a good target for a future dedicated trace.

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
item set. **Also part of this cluster**: item `0x253` (immediately
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
`TickEquippedItemDurability`'s ordinary wear-and-tear. The shared "YOU DON'T HAVE ENOUGH GOLD!" rejection is
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
this pipeline checks (`[+0xE]` bit `0x1000`) turns out to be settable
two different ways:
- **A wall/door trap**, populated into a pool slot by some mechanism
  not yet traced (plausibly the same map-marker-driven spawn path as
  an ordinary monster, `worldobjects.c`/`monsterpool.c`'s
  `monsterPoolSpawn` — not confirmed).
- **A monster ambush** — **fully decoded and reimplemented, 2026-09-23**
  (see "Monster approach and ambush check" below): set by
  `ProcessLevelMonsters` when a monster is grid-aligned with the party
  (same world row or column) with a clear line of cells between them,
  via a `RandomInRange(100)` roll against a threshold selected by a
  *second*, independent bit range of `+0x94` (`MonsterFieldAwareness`
  in `monster.h`) — `0x200`-`0x1000`, distinct from the
  already-documented `0x20`-`0x100` "how far it notices the party"
  range.

`ProcessSideTrapsOnMovement` (was `sub_2278C`, called directly from
`start`, likely once per movement step) fast-exits unless
`g_uiScratchFlags3` bit `0x10` is set, otherwise calls
`TriggerSideTrapForRandomPartyMember` (was `sub_22989`) for every pool
slot with that flag bit. That function picks a random active party
member (`PickRandomActivePartyMember`) and rolls
`RollTrapAvoidanceMagnitude` (was `sub_227F5`) — a save-vs-trap
avoidance check using the still-mysterious party-record field `+0x50`
against threshold/magnitude-cap fields at the pool record's `+0x64`/
`+0x66` — offsets that fall **within the monster catalog block's own
byte range** (`monster.h`'s `MonsterBlockOffset` `0x32` through
`0x9B`), not among its currently-named fields. Same for `+0x60`/`+0x62`/
`+0x70`, also read here, and `+0x78` (stored as a pointer-like
reference). **This strongly suggests "trap" pool entries carry a full
monster catalog block too, with these particular fields reused as
trap avoidance threshold/magnitude/sound data instead of ordinary
monster stats** — a genuinely deep unification of the trap and monster
systems, but not confirmed or reimplemented this pass; would need
tracing `ProcessLevelMonsters` (monster AI/turn processing, not
otherwise touched yet) and however wall-trap pool entries actually get
created. The higher `+0x50` relative to the threshold, the less likely
and smaller the resulting effect. The trap only actually fires if the
party's current facing (the same `g_partyFacing` tier-bit convention
as `DrawDungeonCellSideFeature`/`ShowCompassDirection`) matches one of
4 direction bits also on `+0xE` — i.e. it has to be a wall/door (or
ambushing monster) the party is currently facing, using the record's
own `+2`/`+4` world position (`MonsterFieldWorldX`/`Y`) exactly like
an ordinary monster's. Up to 4 such results are staged into a scratch
table (`0xBC28`), then `PresentTriggeredSideTrapEffects` (was
`sub_2281F`) resolves and presents them: plays the trap's sound cue
once `WaitForSoundDriverIdle` confirms the driver is free, draws
weapon-style icons and does a full dungeon-screen refresh, picks the
highest-severity result to drive a scaled `AnimateProjectileStep`
animation, and finally transfers the results into the confirmed
icon-bar slot table (`0xC50`) via `ApplyEffectAndDrawIconBar`. `+0x50`
being used here as an avoidance stat is a second, independent data
point (alongside `ComputeAlchemyRefinementYield`'s use of the
neighboring `+0x70`) that the still-open "`+0x4C`/`+0x4E`/`+0x50`
trio" are general character stats reused across systems. **Not
reimplemented** — genuinely comparable in scope to the monster-pool
work already done, but blocked on tracing `ProcessLevelMonsters` and
the wall-trap creation path first; a good candidate for its own
dedicated session rather than an extension of `monsterpool.c`.

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

`ApplyEffectAndDrawIconBar` also calls `ApplyIconBarStatDelta` (was
`sub_182CE`): applies a capped or floored stat delta to a
data-selected party field via the icon-bar slot, clears a `+0x1C`
status-bit range, then finishes with `UpdatePartyAverageStatTiers` and
— notably — `CheckForLevelUp`, suggesting at least one use is a
gradual/staged XP-granting effect (the specific field isn't hardcoded
here, so not confirmed).

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
`g_pictureDir` table the same way (`g_pictureDir + g_pictureCategory`,
`g_pictureCategory` = `picture_id*0x10`) — it's one shared directory, not a
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
`0x2000`/`0x400` bits' consumers, if any. `LoadLockState`'s own format
(the `0x8000` door/lock case) is now fully traced — see "Lock/door
definition catalog" below. Reimplemented (lookup only, not the deeper
state resolution the `0x4000` branch feeds into) in
`src23/worldobjects.c`/`.h`: `worldObjectTableParse`/
`worldObjectTableParseWorldDat` and `worldObjectFind`, tests in
`tests/test_worldobjects.c` including exact reachable-record and
per-flag counts checked against both real `WORLD.DAT` files.

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
offset — investigated, not resolved.** It copies 2 words (4 bytes, not
26) from `si = (id-1)*4 + 26*_val9` (Chapter 2) — `_val9 = 600`, i.e.
starting 15,600 bytes into the region, which is `26 * 600`, *before*
the 608-record boundary this section uses. In Chapter 3 the equivalent
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
`g_monsterSlots` combat-staging array copied from live records, not
yet reimplemented — a real additional system this sweep surfaced but
didn't chase further), scroll-out despawn
(`monsterPoolRefreshWindow`), combat/area-attack death cleanup
(`GrantMonsterRewards`+`RemoveMonsterFromMap`, both already
reimplemented), and one previously-undocumented mechanic: an
item-effect handler (`ApplyEncodedItemEffect`, `yendor2.asm:51586`)
that finds whichever monster occupies the party's facing tile (by
matching `MonsterFieldCell`), copies its full 156-byte record into a
UI scratch buffer (`DS:0x525C` — plausibly for an "identify"/"examine"
display, not confirmed), zeroes the live record in place (a despawn,
distinct from combat death — no rewards granted), and clears the
dungeon grid's "monster here" overlay at that cell. Not reimplemented
this pass — needs the broader `ApplyEncodedItemEffect`/item-effect-code
dispatch framework this project hasn't built. In short: `g_levelMonsters`
records are created once, occasionally copied out or removed, and
never moved.

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
