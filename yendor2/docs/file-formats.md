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
by `word_328D4` — plausibly backed by this same `CURGAME` data once
loaded): lives in a **confirmed fixed array**, `g_partyRecords` (base
`0x95F3`, stride `0x1F4` = 500 bytes/record, up to 9 slots per
`SelectDefaultPartyRecord`'s scan bound) — not a linked list.
**Correction**: earlier documentation here claimed `word_328D4` was
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
existing panel-select routine (`sub_25B34`), then draws that member's
status-bar row: portrait icon, name, level (`+0x16`), packed-BCD XP
(`+0x18`) — called both from a `UseItemType_400` path and from the
F1-F4/click-portrait party-member selection handler. The main
mechanism setting `word_328D4` is now confirmed:
`SelectPartyRecordById` (a foundational, extremely widely-called
function) takes a 1-based record id and sets `word_328D4` =
`(id-1)*0x1F4 + 0x95F3` (or 0 for id 0), also caching the id itself in
`word_328D6` — this is the function behind `g_partyRecords`'
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
`DrawAfflictionsList` (was `sub_25D82`, called from `sub_25B34`), which
draws the header "AFFLICTIONS:" then tests every individual bit and
shows the matching name (or "NONE"):
`0x2000`=**DISEASED**, `0x4000`=**POISONED**, `0x8000`=**SICK**,
`0x400`=**STONED**, `0x800`=**FROZEN**, `0x1000`=**PARALYZED**,
`0x80`=**CURSED**, `0x100`=**HEXED**, `0x200`=**JINXED**; bit `0x40`
separately confirmed as **DEAD** via `DrawThreeStatBars` (see below).
This one function resolves several previously-separate findings at
once: `TickStatusEffects`/`ApplyStatusEffect`'s `0x400`/`0x800`/
`0x1000` group is STONED/FROZEN/PARALYZED (the *timed* ailments);
`CastSpell`'s `0x18` dispel bits (`0x2000`/`0x4000`/`0x8000`) are
DISEASED/POISONED/SICK (the *dispellable-only* group);
`CheckPartyWipeAndReinitLevel`'s `0x1C40` incapacitation mask is
DEAD+STONED+FROZEN+PARALYZED — literally "can't act"; and
`TickPartyAilmentIconBar`'s two effect-id groups are DISEASED/
POISONED/SICK (id `2`, all characters) vs. CURSED/HEXED/JINXED (id
`0xE`, MP-gated characters only); and `PickRandomActivePartyMember`
(was `sub_22A35`, called from `sub_22989`) uses the same `0x1C40` mask
to retry-pick a random party slot until it lands on one that's
occupied and not incapacitated — a classic "pick a valid random
target" utility.

**`+0x20`–`+0x30`: 9 contiguous 2-byte protection/resistance values**
(`+0x20`, `+0x22`, `+0x24`, `+0x26`, `+0x28`, `+0x2A`, `+0x2C`, `+0x2E`,
`+0x30`), found via `RollEffectResistance` — each one is conditionally
summed into a trap/status effect's resistance-check total, selected by
one of 9 matching high bits (`0x8000`..`0x80`) in the effect-definition
record's cost flags. **Now identified by name** via
`DrawCharacterProtectionsList` (was `sub_25FCD`, called from
`sub_25B34`), which draws "PROTECTIONS:" and pairs each value with its
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
not confirmed; `+0x66` → `word_36CA7`, gating a 4-tier area size in
`RevealMapRegion` (**correction**: previously guessed "plausibly
weather" — traced further and it's a `Locate`/`Scout`/`Magic-Mapping`-
style special ability that reads `WORLD.DAT`/`CURGAME` directly and
reveals a `word_36CA7`-sized box of the map around the player, not a
visual weather effect). `RevealMapRegion` also calls
`TryTravelToClickedMapCell`: click a cell within the revealed area
(gated on that cell's "explored" bit) to instantly travel/teleport the
party there — a scry-then-teleport interaction consistent with a
Locate/Scout/Magic-Mapping ability; `+0x58` → `word_36CA9`, gating progressively-
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
`0x18` dispels/cures — clears bits 13-15 of `+0x1C`, a *different* 3-bit group
than the one `TickStatusEffects`/`ApplyStatusEffect` manage (bits
10/11/13 — bit 13 appears in both groups).

**The 6 core attributes are now mapped** (found via `RollCharacterAttributes`,
character creation's stat roller — resolves the "not yet mapped" note
that stood since early in the session): 6 base/derived field pairs,
each rolled `RandomInRange(15)+45` (45–59) into the base field, copied
to the derived field 0x40 higher: `+0x3C`/`+0x7C` (also ×10 into a
weight-like derived stat at `+0x56`/`+0x96` — plausibly **Strength**,
carry capacity); `+0x3E`/`+0x7E` (no secondary use found yet);
`+0x42`/`+0x82` (one component of `UseTrainingItem`'s MP-growth blend —
plausibly **Intelligence**); `+0x44`/`+0x84` (the other MP-growth
component — plausibly **Wisdom**); `+0x46`/`+0x86` (feeds a separate
`UseTrainingItem` growth calculation); `+0x40`/`+0x80` (25%-scaled to
set both current and max HP, `+0x52`/`+0x92` — plausibly
**Stamina/Constitution**). Exact name-to-offset assignment for all 6
isn't independently confirmed — the roll order doesn't obviously match
the manual's STR/DEX/STA/INT/WIS/CHA listing — but the *pairing*
(base ↔ derived, and which pair feeds HP vs. MP) is solid.
**Correction**: `DrawThreeThresholdStats` — called right after
`RollCharacterAttributes` in `ShowCharacterSkills`, at `+0x4C`/`+0x8C`,
`+0x4E`/`+0x8E`, `+0x50`/`+0x90` — was first guessed to be 3 of these
6 attributes; it isn't, since those 6 are confirmed at the entirely
different `+0x3C`–`+0x86` range above. What `+0x4C`/`+0x4E`/`+0x50`
actually are (drawn alongside the attributes on the same screen, so
presumably related) is still open — though `DrawCharacterStatSheet`
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
role" slots (`word_36D03`/`word_36D05`/`word_36D07`/`word_36D09`/
`word_36D0B`) — consistent with practical party-role skills (a
designated navigator, mapper, barterer, etc., per the earlier
attribute/skill string survey) where the game highlights whichever
character currently holds that role. Individual field-to-skill-name
assignment isn't confirmed yet, but the "5 assignable roles" shape is
a solid new lead for pinning them down.

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
(highlighting the `Q`) when `word_328CA` bit `0x8000` is clear, or
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
(`sub_26415`) down into the new `GetInventorySlotPtr`. Each character
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
`sub_26415`'s open/close branches: clicking an unopened container item
assigns it to the first free marker (`+0x17C`/`+0x1A2`/`+0x1C8`,
priority order) and calls the new `LoadContainerContents`, which reads
that container's own saved inventory contents from **`CURGAME`**
(`FileEntry` `bx=0x8FFB`, `errorCode=0xB`) into the matching bag slot
area. Clicking an open container again calls `SaveAndCloseContainer`,
which writes the bag's contents back to `CURGAME` (same `FileEntry`)
if nonempty, then clears the marker — unloading it. So each bag's
contents persist independently in the savegame, swapped into the
character's inventory groups only while open.

**`IsContainerTypeCompatible`** (called from `sub_2621C`, a 230-line
main-input-loop handler not traced this round) checks whether the
currently-open alternate bag matches an allowed-type bitmask before
letting an item be placed into it, rejecting with
`FlashStatusWarning` otherwise. `sub_2621C` also uses
`LoadNextContainerInChain`, which walks a linked chain of container/
world-object records via `CURGAME` (each record's own `[+8]` field
points to the next id) — e.g. multiple containers found together —
loading each via `LoadContainerContents` in turn. Both
`LoadNextContainerInChain` and `sub_2621C` also call
`CommitContainerWrite` (was `sub_26C0E`) — a minimal write-commit
(`FileEntry_Write(errorCode=0xB)` + `ErrorCheck`) that, unlike the
similarly-shaped `SyncContainerContents`, does no descriptor setup of
its own and assumes the caller already configured the container-write
descriptor.

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
`word_36C81` (a global not otherwise documented) — plausibly a
container/inventory-slot placement rather than the ground-drop
`PlaceItemOnGround` handles, but not confirmed.

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
`sub_274B4`) for the 3 "alternate bag" inventory groups at
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
`word_328D4` the same way `SelectPartyRecordById` does) and call
`sub_23C18` (still an open lead — a ~270-line handler, not traced) to
open some detail/interaction screen. A second, differently-routed key
range reaches the same slot math but first tests a flag
(`+0x15C` bit `0x800`); when set, it clears the bit and removes the
slot's index from two small lookup tables (`0x95EB`, 5 slots at
`0x94A3`) before falling through — plausibly a recruit/dismiss
mechanic (adding/removing a character from the active adventuring
group), but not confirmed. Worth revisiting once `sub_23C18` is
traced. Separately, `ShowWorldMap`'s exit path (`D` key or an
equivalent mouse click, both leading straight to a `retf`) calls
`CompactPartyRosterSlots` (was `sub_2BF3C`) as a cleanup-on-exit step:
it cascades non-empty roster entries down to fill gaps, across not
just the 4 active slots but **3 more "reserve" globals**
(`word_36E4D`/`word_36E4F`/`word_36E51`, not otherwise documented) —
confirming the roster extends beyond the 4 active party members into
at least 3 reserve slots.

### Character creation wizard (`RunCharacterCreation`)

`RunCharacterCreation` (called from `InitGame` and from
`RunTitleScreen`'s `I` key, per its own pre-existing comment) is a
3-step wizard, each step ESC-cancelable: `ComposeCharacterPortrait`
(step 1), `sub_15429` (step 2, not yet traced), `sub_1559A` (step 3,
not yet traced), then always `FinalizeCharacterCreation` (was
`sub_15267`, runs regardless of which step was reached). Matches the
manual/string-survey's `CHARACTER CREATION`/`PICK A CLASS`/`MALE`/
`FEMALE`/`PICK A PORTRAIT` cluster. Step 3 (`sub_1559A`) calls
`DrawShadowedText` (was `sub_161D0`) — a drop-shadow text/list draw
(background-color pass, then a foreground-color pass shifted 1 pixel
up-left), with a byte-for-byte identical duplicate,
`DrawShadowedTextAlt` (was `sub_11E4A`), used elsewhere by another
still-untraced function (`sub_11A10`, called directly from `start`).
`FinalizeCharacterCreation` loads a
transition palette, reads file entry `#3`, frees a temp memory block if
one was allocated, clears the screen, and stops the
character-creation music before returning — the wizard's common
cleanup/exit path. The entry point *into* this wizard from the party
roster screen is `ShowCreateCharacterPrompt` (see above), which first
finds and wipes an empty `g_partyRecords` slot.

### The on-line clue book (F8)

`ShowClueBook` (the manual's "F8 On-line clue book") closes by calling
`RestoreClueBookBackgroundFromEMS` (was `sub_14DFC`) — a full-screen
restore from its own dedicated EMS page (`0x5616`, distinct from the
`0x55D8` page the portrait/dungeon-screen cluster uses) — bringing back
whatever was on screen before the book opened. It drives an
interactive, categorized clue-entry browser: `RunClueEntryMenu` (the
per-category menu loop) → `ShowClueCategoryEntries` (init+draw one
category, reading its entry count from a table at `0xF3F4` indexed by
`word_2E3F6`, the category selector) → `DrawClueEntryList` (the
scrollable entry list itself, positions from a table at `0x68D2`) →
`BuildClueEntryText` (composes one entry's display text, dispatching
on `word_2E3F6` to different lookups per category) → for at least
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
flag (`word_328CA` bit 1) is clear and that entry's own flag
(`[+2]` bit `0x8000`) marks it as requiring registration — a shareware
limitation.

`HandleClueCategorySelection` (was `sub_14D26`) is `RunClueEntryMenu`'s
category-switching input handler: keyboard (`ESC`/digit keys, plus
`K`/`P` hotkeys gated on the same `word_328CC` `0x40`/`0x20` bits
`DrawClueBookNavBar` uses for its "d) LIST"/"c) MAP" hints — not the
registration lock, a different flag) and mouse (region table `0x6876`,
categories 1-9) both funnel into a shared "apply new category" block
that walks a 7-bit category mask in `word_328CC` and plays a sound cue
on change.

`RunClueEntryMenu` also calls `DrawClueBookNavBar` (twice) to draw the
book's top bar: conditional "d) LIST" / "c) MAP" hotkey hints
(`word_328CC` bits `0x40`/`0x20`), then a row of 7 category-tab icons
(fixed base picture ids, each swapped to a highlighted +1 variant when
its bit in `word_328CC`, `0x8000` down to `0x200`, is set).

`ShowClueBook` also calls `ShowClueBookHelpScreen` (bound to TAB,
per its own title text), which lists the clue book's categories: F1
Maps (world/towns/mines), F2 Monster Statistics, F3 Spells, F4 Magic
Users (spells by class), F5 Inventory Items, F6 Complete Walk Through,
ESC Return to Game — very likely (order not yet matched bit-for-bit)
the identities of (at least 6 of) `DrawClueBookNavBar`'s 7 tabs.

**`ShowClueBook`'s full F-key dispatch**, traced directly from its own
`word_2E40A` (`PollKeyboardInput` result) switch: F1 (`word_2E3F6=1`,
Maps) → `RunClueEntryMenu` + `RunClueBookMapCategory` (loads the map
via `LoadClueBookMapEntry`, draws a row/col grid of per-cell location
labels via `DrawClueBookMapGrid`, and dispatches cell clicks to an
untraced `sub_14122`). F2
(`word_2E3F6=2`, Monster Statistics) → `RunClueEntryMenu` +
`RunClueBookMonsterCategory`. F3 (`word_2E3F6=3`, Spells) →
`RunClueEntryMenu` + `RunClueBookSpellCategory`, which (along with
`BuildClueEntryText`) reads each spell's data via `LoadClueBookSpellEntry`
(was `sub_1D198`) — an 80-byte record from its own dedicated EMS page
(`0x5610`), the spell-data equivalent of `LoadClueBookMonsterEntry`'s
`WORLD.DAT` read. F4
(`word_2E3F6=4` lists classes, then `word_2E3F6=[selected class]+4`,
Magic Users) → two chained `RunClueEntryMenu` calls (class picker,
then that class's spell list) + `RunClueBookSpellCategory` again —
`ShowClueBookSpellDetail` draws "CLASS:"/"LEVEL:" plus "MP:"/
"NUORE:"/"ORE:" cost fields (spells cost MP and the same two alchemy
ore counters used elsewhere) and "AFFECTS:"/"WHEN:"/"EFFECT:"
description sections with a 6-class eligibility marker row. F5
(`word_2E3F6=0xB`,
Inventory Items) → `RunClueEntryMenu` lists **8 item subtypes**
(`word_2E3EE[0]` 1–8), each with its own sub-loop and now fully
identified by title dump: **1 "ARMOR/RINGS"** →
`RunClueBookItemCategory` (**correction**: previously described below
as "the F5 category's own loop" — it's actually only item subtype 1's
loop within F5's subtype selector); **2** → just `WaitForKeypress`
(an empty/placeholder subtype, no title); **3 "JEWELS/ARTIFACTS/
UNIQUE ITEMS"**, **4 "MAGIC SCROLLS/QUARTZ"**, **5 "POTIONS"**, **6
"SUPPLIES/FOOD"** (`word_2E3F6=0xD/0xE/0xF/0x10`) → all four route
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
**8 "WEAPONS"** (`word_2E3F6=0x11`) → `RunClueEntryMenu` +
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
`RunClueBookItemCategory` hit-tests, toggled by `word_328FE` bits). Both
also call `ListCompatibleClueBookItems` (was `sub_1472A`, using the
same `0x6976` table/position): checks the current entry's usability
flags, and if eligible, scans up to 9 more catalog ids re-checking the
same eligibility test and drawing each match — a filtered
compatible-items list for the category view.

**Major reference find**: `ShowArmorDetailRow` also calls two
bonus-list drawers, each iterating up to 4 `(type id, amount)` pairs
at `word_2E54A` and printing `"+<amount> <name>"`:
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
AGREEABLE?". All 6 traced call sites `retf` immediately after the
call — none poll Y/N or deduct gold there, so the actual confirm+pay
step (if it exists) happens on some later, separate re-entry not yet
found. The total is a per-unit base cost (varies by call site,
sometimes built from `word_2E40C` condition bits) times an
item-catalog quantity field (`0xBCE+0x18`), added in a loop running
once per `[word_328D4+0x16]` — plausibly once per afflicted/eligible
party member.

A 4th shop-mode bit, `word_328C6` `0x200`, gates a mouse-click-driven
shop purchase path: `sub_17032` (a catalog-click handler reached from
the main input loop via a hit-test against region table `0x5AC0`, not
fully traced — its own click gate is `HitTestCatalogSlot`, region
table `0x63C8` plus an 8-entry exclusion check) calls
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
not `UseItem`): it calls the main input loop `sub_1869D` directly
(twice, enabling the Space-bar cluster) and `sub_17032` (enabling the
click-to-buy path), redraws `ShowMaterialCounterHud` repeatedly, and
writes state back via `FileEntry_Write` near an exit. Its many
internal helpers aren't individually traced yet.

A separate, likely shop/vendor "buy" `UseItem` handler (`sub_1BBED`,
reached from `UseItem+0x65`) spends `g_partyGold` via
`CompareBCD4`/`SubBCD4` against a price table at `0x512A`, gated by
inventory-capacity checks against the same `0xBCE` range table used
elsewhere, and redraws the gold readout afterward via
`RedrawPartyGoldDisplay`. Left unnamed — it has several branches
(a single-item purchase path and a quantity-loop path that repeatedly
adds the unit price to both `g_partyGold` and a second counter
`0xB30`) not yet disentangled with enough confidence to name safely.

A specific, fully-traced instance of a quantity-purchase flow:
`PromptBuyOreQuantity` (was `sub_1AF49`, called from `UseItem`) shows
"ORE COSTS 10 GOLD PER UNIT.", the current gold balance ("GOLD COINS:"),
and "ENTER QUANTITY TO BUY", reads the quantity via
`PromptForBCD4Quantity` (was `sub_19BE6` — parses a digit string typed
through `EditTextField` backward into a packed-BCD4 value), then
validates affordability via `CompareBCD4` against `g_partyGold`. This
is the purchase flow for using an Ore-type item from the inventory —
plausibly connected to the unnamed `sub_1BBED`'s quantity-loop path
above, though that link isn't confirmed.

### Ambient music by map region

`UpdateAmbientMusicForRegion` computes a coarse map-region index from
the party's position and, when it changes, reads that region's
`WORLD.DAT` record and plays its associated music track
(`PlayMusicTrack`) — background music changes as the party crosses
between zones.

### The map legend editor

`RunMapEditorScreen` (name pre-existing from an earlier session; not
otherwise documented) hosts a wall/floor legend editor:
`EditWallLegendTypeNumber` and `EditFloorLegendTypeNumber` are a
symmetric pair of numeric-entry fields (via `ReadTypedInteger`, was
`sub_1D146` — a generic typed-integer prompt built on `EditTextField`,
reused 7 times) storing a wall/floor type number into
`word_2E384`/`word_2E386`, then
redrawing the corresponding legend row (`DrawWallTypeLegendRow`/
`DrawFloorTypeLegendRow`). One error path in the floor field falls
through into the wall field, suggesting Tab-style navigation between
the two.

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
fixed 6-entry table (`0x9519`) for a direct match, then falls back to
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
`word_32974==0x2C8`) — a **quest-item-completion check**: 4 specific
item ids (`0x254`-`0x257`) are each checked for being *absent* from
every party member's inventory; if all 4 are gone, it plays a success
sound and runs an animated sequence re-checking those 4 plus a 5th
(`0x2C8`) in reverse order. Reads as "the party has used/given away all
N required quest items," a completion reward sequence — exact narrative
(which items, what they unlock) not identified.

**One of the 5 items identified**: item `0x258` — immediately adjacent
to the `0x254`-`0x257` completion range — is `UseLocationBoundPotion`
(`word_32974==0x258`, another item-icon-dispatch handler): a potion
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
`word_36CF5` tier bits used throughout) from the current position,
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
combat**: `ApplyDamageToMapMonster` (called from an unnamed dispatcher,
`sub_2C0FE`) applies damage to a `g_levelMonsters`-pool monster, sets
wound/display flags, redraws, then resolves death (`GrantMonsterRewards`
+ `RemoveMonsterFromMap` + `RedrawDungeonScreen`) or survival
(`RefreshDungeonScreen`) based on its HP — the corridor-encounter
counterpart to the turn-based `g_monsterSlots` combat flow documented
below. `HandleRangedOrCombatAction` also uses `SaveCorridorBackgroundToEMS`
(the mirror image of `RestoreCorridorBackgroundFromEMS` — video
buffer → EMS, caching the corridor before an animated overlay draws
over it) and `SaveActionIconPanelToEMS` (same idea for the action-icon
panel area, also used by `HighlightSelectedAbilityIcon`).

A large unnamed dispatcher, `sub_2C0FE`, sits behind several
combat-adjacent helpers this session named individually
(`ApplyDamageToMapMonster`, `GetMonsterAtViewportRow`,
`ScrollCorridorBackgroundFromEMS`, and `AnimateEffectFrame` — one
animation frame, same shape as `AnimateProjectileStep` but a different
layer flag and wait length, for some other in-viewport effect
sequence) without itself being traced. Also called (twice) from it:
`DrawAnimationFrameAndAdvance` (was `sub_2D3FE`) — a small, generic
"draw this animation frame, return the next (wrapping) frame index"
cycler, drawing picture `ax` at x=`bx` and advancing/wrapping the frame
counter within `[word_332EC, word_332EC+word_332EE)`. Also called once
from it: `ResolveAttackAndLatchFirstHit` (was `sub_2D195`) — calls
`ResolveAttack` then latches a value (`word_332E8`) into `word_2E49C`
the first time through (only if it was still 0); exact field identities
not confirmed.

**Ranged attacks and area-effect abilities against a corridor monster**:
`ResolveAttackOrAbilityAction` (called from `sub_1D4B8`, an unnamed
combat-round driver, itself called from `start`) handles two modes,
selected by `word_328C8` bit `0x100` (set by the caller — e.g.
`start`'s loc_10A65 branch, gated on *not* being in formal combat):

- **Set (ranged/thrown weapon)**: finds an equipped item in a party
  member's inventory slot, loads its catalog record for damage-type
  flags, rolls the hit via `ResolveAttack`, applies it via
  `ApplyResolvedDamageWithResistance`.
- **Clear (spell/ability)**: calls `ResolveAbilityEffect` — an 85%
  success roll, then dispatches on `word_32974` (the ability id) to
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
(called from `start`, 2 sites — one sets `word_328C8` bit `0x100`
first): a 3-way combat-action dispatcher. If already in formal combat
(`word_328CA` bit `0x1000`), branches elsewhere (not traced). If the
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
attempts left (`word_2E544`, decremented via `sub_1DCC6`), the loop
continues to the next depth row automatically — a multi-shot
continuation for characters with more than one attack. All paths
converge on a common epilogue: if the loot-staging counter (`0x51B6`)
has accumulated enough, `ShowLootAndAwardExperience` fires, then
`ProcessLevelMonsters` ticks and the screen/minimap redraw.

**The in-combat melee branch** (formal combat, `word_328CA` bit
`0x1000` set) is much simpler: `HighlightSelectedAbilityIcon` marks
the selected ability in the UI, one `AnimateProjectileStep`, then
`ResolveAttackOrAbilityAction` directly against `word_32A1E` (the
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

### Combat: monster slots and turn order

Up to **3 simultaneous active monsters**, `g_monsterSlots` (base
`0x51C0`, 3 × `0x9C`/156-byte records, `[+0]==0` = empty slot;
zeroed wholesale by `InitializeDungeonLevel` when entering/loading a
level, alongside clearing `word_32A1E`, the active-combat-monster
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
`word_32A1E`. **`ProcessCombatRound` confirms the "defeated" flag**:
called every `RunDungeonGameLoop` iteration, it checks every occupied
`g_monsterSlots` entry's HP (`+0x10` <= 0) and, on death, sets its
`g_combatTurnOrder` entry's `0x4000` flag, clears `word_32A1E` if it
was the active target, and calls `GrantMonsterRewards` — closing the
loop from `HandleDungeonInput`'s HP subtraction through to loot. If no
monster died that pass, it instead advances the turn to the next
living combatant in `g_combatTurnOrder`.

`word_32A1E`, the "currently active monster" global read throughout
the combat-adjacent code already documented this session
(`UseAbilityOnTarget`, `ExamineTarget`, `CastSpell`'s target checks,
etc. — not yet cross-referenced against this specific variable, a
good next step). Mouse-clicking a monster panel's icon also sets
`word_32A1E` directly (a target-selection shortcut alongside
`SelectActiveMonster`'s automatic pick).

**Attack-roll formula, found via that click handler**: `ResolveAttack`
(`ax`=target defense, `bx`=attacker accuracy, `cx`=weapon damage power)
— hit if `(accuracy-defense) >= RandomInRange(55)`, damage =
`weaponPower*(accuracy-defense)/100` (minimum 1), else a flat miss.
`UpdateMonsterWoundTier` then classifies a hit into an escalating
visual wound-severity flag on the monster record (`+0xE`: `0x8000`
light, `0x4000` moderate, `0x2000` severe, by percentage of `+0x50` —
plausibly max HP/toughness) plus an unconditional display flag
(`+0xC` `|= 0xA`) — it does not subtract HP itself, but its caller
does immediately afterward: **`[monster+0x10] -= word_2E49C`** (the
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
fields like `errorCode`/`word_32974`). `+0x50` is therefore plausibly
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
per-frame row-depth counter (`word_3292C`) and calls
`RenderDungeonViewRow` six times with decreasing cell counts (`0x11`,
`0x11`, `5`, `3`, `3`, `3`) and a different row-data pointer each time
— the classic "draw each depth row of the visible corridor, near to
far" shape. `RenderDungeonViewRow` draws each cell's picture (a
12-byte-stride lookup table at `0xE551`) and calls
`TryTriggerMonsterEncounterAtCell` once per cell, incrementing/
decrementing `word_3292C` as it goes. `TryTriggerMonsterEncounterAtCell`
only fires for `word_3292C >= 0x11` — since only the first two (and
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
(`word_3292C`) against a per-monster detection-range threshold
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
with the same (picture id `word_2E530`, scale class `word_2E532`,
z-layer/depth `word_32918`, transparency `_font_bgTransparent`)
convention — the perspective/depth-aware counterpart to the simpler
general-purpose `DrawPicture`.

`RenderDungeonViewport`'s 7th and final call is
`RenderDungeonVanishingPoint`, structurally different from the other
six: draws the far-wall/vanishing-point cells at the end of the
visible corridor (a different `0xE551` table field, z-layer 6), then
runs the same per-cell side-feature and encounter checks as
`RenderDungeonViewRow` for the final cell, and (when `word_328CA` bit
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
(the same `word_36CF5` tier bits `ShowCompassDirection` reads) plus
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
`word_36CF7`/`word_36CF9`) and calls `IsMonsterStepBlocked` (was
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
type falls in that range. If `byte_2E400`=='H' (not one of the
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
filtered list of known spells (an eligibility check, `sub_27A66`, not
itself traced) unless the character is incapacitated, then calls
`CheckSpellCastability` (was `sub_1E285`) on each to check whether the
character can currently afford it — enough MP (`+0x54`), MAGIC ORE
(`0x94B7`), and NUORE (`0x94BB`), loading each spell's cost data via
`LoadClueBookSpellEntry` — setting a "castable" icon state for the ones
that qualify. Pagination is 13 spells/page. `DrawAlchemySpellList`
(was `sub_1E3AF`) is the visual counterpart, drawing each page's rows
(name colored by castability, cost values via `DrawSpellCostValue`,
was `sub_1E340`) and highlighting the current selection.
`RunAlchemyScreen` also calls `ShowCompassDirection`, a
"NORTH"/"SOUTH"/"EAST"/"WEST" HUD readout gated on an unidentified
"compass active" mode (`word_328CA` bit `0x1000` clear, `word_36C7F`
bit `0x400` set), drawn at the same screen position as the
material/gold HUD.

Three **global** (not per-party-member) counters at `0x94B3`
(`g_partyGold`), `0x94B7`, `0x94BB` — confirmed **exactly
consecutive**, 4-byte packed-BCD stride, by
`ShowResourceDepletedOverlay`'s scan of all three in one loop.
Individual identities: `0x94BB`/`0x94B7` are used by `CastSpell`'s
`0x1C` alchemy ability (converts 10 units of one into the other —
`NUORE`/`MAGIC ORE`); `0x94B3` is the party's **gold** — **correction**:
first framed as a generic "material counter" (below), but its HUD
label (`ShowMaterialCounterHud`, msg `0x7FC4`) turned out to be a
literal `"$"`, and its two consumer functions were renamed to match:
`TrySellItemForGold` (was `TryConvertItemToMaterial` — a Space-bar
action, main input loop `word_328C6` bit `0x10`, while carrying an
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
`TryRepairItemForGold` (`word_328C6` bit 4), pays gold (cost table
`0x5082`) to repair the held item, rejecting with "I CAN NOT REPAIR
THAT" if ineligible — distinct from the skill-based `RepairItemCommand`
minigame below, which can critically fail and destroy the item. Its
eligibility check is `IsItemEligibleForRepair` (a location/item
flag-pair match); `TryEnhanceItemForGold`'s is `IsItemEligibleForEnhance`
(a location-selected item field checked against a range table at
`0xBCE`). Both have other, untraced callers beyond this Space-bar
cluster. A related classifier: `ClassifyItemServiceTier` (was
`sub_1AE9D`, called 6 times from two other unnamed functions) loads an
item and, based on its `[+0xC]`/`[+2]` flags, returns one of 3 tier
codes (or a 4th "wrong item type" code) — plausibly gating which
service (repair/enhance-style) the item qualifies for, but not
confirmed. The shared "YOU DON'T HAVE ENOUGH GOLD!" rejection is
`ShowInsufficientGoldMessage`. The whole sell-item screen is entered
via `RunSellItemScreen` (from `UseItem`, when the used item's `[+0xE]`
flags have bit `0x4000` set). A sibling branch, gated on the item's
name literally matching `"BUY "` or its `[+0xE]` flags having bit
`0xC000` set, calls `ConfirmAndValidatePartyTarget` — a confirm
prompt to pick a party member, re-prompting with a warning if the
pick is incapacitated (caching the valid choice): sets `word_328C6` bit `0x10` and runs
the main input loop (`sub_1869D`) so Space triggers
`TrySellItemForGold`, then rebuilds/redraws the minimap on exit. Its
two siblings, `RunEnhanceItemScreen` (`UseItem+0x1C1`, sets bit 8) and
`RunRepairItemScreen` (`UseItem+0x1D0`, sets bit 4), are otherwise
identical — completing the shop cluster's three `UseItem`-reachable
entry points (sell/enhance/repair), each just setting a different
`word_328C6` action bit before running the same main input loop.
All three, plus `start` and `HandleDungeonInput` generally, call
`RefreshPartyPortraits`: its core role is refreshing the 4 party
portrait slots, but when a shop action bit is active
(`word_328C6 & 0x1C`) it also draws a context hint — "SPACEBAR TO
ENHANCE ITEM" / "SPACEBAR TO REPAIR ITEM" / default "SPACEBAR TO SELL
ITEM OR ESC TO UNDO". A simpler sibling, `RestoreAllPortraitsFromEMS`
(was `sub_18F6C`, called from the still-untraced `sub_1869D`), does the
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
`sub_191FC`, a large-but-not-full-screen area restore, called from the
still-unnamed `sub_1869D`). Separately, `RestoreAndRedrawFixedStatusIcon`
(was `sub_22402`, 6 call sites incl. `start`) restores a small EMS-
cached area then redraws a fixed picture via `DrawFixedStatusIcon` (was
`sub_225F1`) — a picture from the same directory category (`0x60`)
`DrawPartyMemberPortrait` uses, drawn at a fixed screen position; which
specific HUD icon this is isn't confirmed. `HandlePortraitClick`
is the mouse-click counterpart to the keyboard `1`-`4` selector
(`sub_25B34`): hit-tests the 4 portrait zones and sets the matching
highlight bit when clicked. Both draw via `ShowPartyPortraitForSlot`
→ `DrawPartyMemberPortrait`: the character's icon (`+0x14`), a status
bar, and a condition icon selected by `+0x15C`/`+0x10`, plus
equipped-item icons via `DrawEquippedItemIcons` (reads the
`+0x13A`/`+0x13E`/`+0x142` equipment slot arrays, using an "active"
icon variant when an item's own `[+0xC]` bit `0x400` is set) — a
150-line function whose remaining icon-selection logic isn't
individually traced, plus two small overlay icon drawers,
`DrawPortraitOverlayIconA`/`DrawPortraitOverlayIconB` (each drawing a
"+1" highlighted icon variant at a fixed offset — exact narrative not
confirmed). A separate function, `DrawPartyMemberStatusPanel`
(called from the main input loop `sub_1869D`), draws a fuller
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
for every occupied roster slot.
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
duplication found earlier this session.
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
slot (`word_32924` → a `g_partySlotAssignment` entry → character id →
`SelectPartyRecordById`), calls `CheckForLevelUp`, and shows
`ShowLevelUpMessage` if `+0x1E` came back nonzero.

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
traced real caller passes `si=word_328D4` (the current party member),
inside an item-use dispatcher (`sub_1BBED`) branch gated on having
enough of material `0x94B3` — plausibly per-character one-time-event
flags (quest steps, items read, NPCs met), not confirmed.
`GetRecordFlagBitAndWord_CA` (was `sub_27AC1`) is the same mechanism at
a *different* offset, `+0xCA`, on an unconfirmed record type. **Follow-
up**: found and named the missing Test accessor for the `+0x10C` bank,
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
bits on `word_32DCE`. How much detail is revealed is gated on the
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

**Key items reference locks by their own catalog type value**:
`UseItem`'s `UseKeyItem` branch passes a key item's own type-flags
field directly as `LoadLockState`'s lock id — a key's catalog "type"
*is* the numbered door it opens, no separate item-to-lock lookup
table needed.

**The actual unlock-a-door command**: `UnlockDoorCommand` (a
`HandleGameCommand` handler, `word_32974` `0x21`-`0x2E`/`0x2F`) uses
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
background music tracks based on `word_36D01` (the clock) falling
inside or outside `[0x1A4, 0x474]` (7:00 AM–7:00 PM), via the
already-named `PlayMusicTrack`. `word_3297E` is the "forced track"
override this checks (0 = let the ambient day/night system choose):
`RunTitleScreen` sets it to `1` (title music) on entry and clears it
to `0` right at its `E` ("Enter"/leave-the-title-screen) exit point,
handing music control to the ambient system for the rest of gameplay
— and briefly forces `0` (silence) during character creation, restoring
`1` afterward. The
raw ISR entry itself is embedded in bytes IDA hasn't cleanly separated
from a preceding data declaration, so it's documented here rather than
renamed (renaming risks corrupting the disassembly boundary).

`ShowGameClockCommand` (a `HandleGameCommand` handler, `word_32974==7`)
confirms the game tracks a genuine in-game date and time, not just a
coarse day/night or "time of day" value: it fills two fixed template
strings — `12:12 AM` and `12/12/1212` — with the current hour/minute/
AM-PM and month/day/year, via `ComputeGameClockTime`.

**Full mechanism traced**: `word_36D01` is the master "minutes since
midnight" counter (0–1439), advanced by `AdvanceGameClock` — the
per-minute clock tick. `ComputeGameClockTime` converts it to a 12-hour
display (`word_32934`="AM"/"PM", `word_32948`=hour 1–12,
`word_3295C`=minute). Past 1440, `AdvanceGameClock` rolls the calendar:
day (`word_36CFB`) wraps at 31 into month (`word_36CFD`), which wraps
at 13 into year (`word_36CFF`) — a **30-day-month, 12-month-year**
in-game calendar (new-game start: day 4, month 11, year `0x222`=546).
On the day rollover, `ResetDailyAbilityCharges` also zeroes every
party member's 4 special-ability charge fields (`+0xB6`-`+0xBC`,
see `RevealMapRegion`/`UseAbilityScroll`) — special abilities recharge
once per in-game day.

**The "R rest" command**, `RestPartyAndAdvanceClock` (an action-toolbar
entry from `start`, also reached from `sub_2C0FE`): after an
eligibility check (`IsRestingAllowedHere` — rejects on a global flag,
forbidden map/level id, or a special-cell match via
`IsPositionInTriggerList`, confirmed by the "YOU CAN NOT REST HERE"
message), advances `word_36D01` directly — a flat `+0x1E0`
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
`ClearMessageBoxArea` (shared with `sub_17032` and
`UseAbilityCommand`) to clear the message-box background.
`UseAbilityCommand` also calls `ConsumeAbilityChargeAndRefresh` (was
`sub_17A65`): shows `ShowResourceDepletedOverlay`, then — unless a
flag (`word_32DCE` bit 1) says otherwise — plays a sound, increments a
counter at `[word_32DC4+2]` (plausibly the ability's charge/uses
count, alongside the already-known `word_32DC0`/`word_32DC2`
effect-id/threshold parameters feeding `ApplySavingThrowEffect`), and
refreshes the dungeon screen.

It also fires a dawn event at exactly 6:00 AM and a dusk event at
6:00 PM (`word_36D01`==`0x168`/`0x438`, via `AdvanceDayNightPaletteFade`
— a genuine ambient-lighting system: a gradual 113-step palette fade
through a snapshot table, written into VGA palette entries `0xE0`-
`0xFF` (the last 32 slots, plausibly a dedicated sky/ambient-light
ramp) via `SetPaletteRange`, walked forward from dawn and backward
from dusk), plus a separate 5-minute periodic timer (`word_32954`,
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
by a fixed 8 hours (`word_36D01 += 0x1E0`, matching the classic
"resting takes 8 hours" convention); a separate `+0x3C` (1-hour) advance
exists elsewhere too, context not traced.

A **separate** per-member status icon bar exists alongside
`TickWorldAilments`' 6-slot world table: `TickPartyAilmentIconBar` (was
`sub_1A085`, called from `RunDungeonGameLoop` and
`ApplyMapTriggerEffect`) periodically (a counter reaching 40, or a
much slower ~32768-call wraparound path gated on a 4th `word_36C79`
flag, bit `2`) recomputes each of the 4 roster members' ailment
severity via `PrepareTrapEffectSlots(ax=2` or `0xE)` plus a helper that
checks `+0x1C` status bits — **now identified via `DrawAfflictionsList`**:
`0x2000`/`0x4000`/`0x8000` = DISEASED/POISONED/SICK for the normal
path (id `2`); `0x80`/`0x100`/`0x200` = CURSED/HEXED/JINXED, gated on
having MP (`+0x54 != 0`), for the second helper (id `0xE`) — or, on the
slow path, tiers off the derived stat `+0x58` instead. Whenever
severity is nonzero it populates a per-member icon-bar slot and calls
`ApplyEffectAndDrawIconBar`. The `word_36C79` bit-`2` slow-path
condition still isn't confirmed.

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
fixed status icon), then `RunGameDialog`, then (unless `byte_2E400`==
`0xFF`) `InitializeDungeonLevel` — reading very much like a "whole
party is down → show a screen → reset the level" handler. Bit `6`'s
specific ailment isn't confirmed (it's not one of the 3 timed-ailment
bits), nor is the exact meaning of `byte_2E400`==`0xFF` skipping the
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

**Item data catalog**: `UseItem`'s `LoadItemData` (was `sub_1C890`)
looks up a per-item data block via a fixed catalog record at `0xBCE`
(same shared lookup helper, `sub_27B42`, as the resource-stub family
above — cross-confirming it's a generic "look up catalog entry N in
`WORLD.DAT`" primitive), allocates a buffer sized to fit, and reads the
item's raw data in. Item contents/size/count not yet examined —
`sub_27B42`'s own signature (id in, offset+size out) would be the
fastest way to enumerate the whole catalog if that's wanted later.

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
persisted bulk edit. `BrowseWallTilePalette`/`BrowseFloorTilePalette` (`B`/`F`) jump the
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
