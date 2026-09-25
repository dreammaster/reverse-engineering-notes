# Chapter 2 vs. Chapter 3 engine differences

Living reference for confirmed behavioral differences between
`yendor2.idb` (Book I Chapter 2, the fully-documented baseline) and
`yendor3.idb` (Book I Chapter 3, confirmed via BinDiff to share the
same engine). Unlike `overview.md`, this file can be edited/corrected
in place as understanding improves — it's a reference, not a session
log. See `overview.md` for the dated narrative of how each entry was
found.

**Methodology note**: BinDiff similarity scores are a starting filter,
not a verdict — spot-checks during the initial >=0.95-similarity bulk
import already found two real differences at 0.99 and 0.98 similarity
(see below), so a high score only means "almost certainly the same
function," not "identical." Real verification needs pulling the
called-function sequence for each match (mnemonic + call-target diff,
filtering out call targets that only differ because the *other*
function hasn't been renamed yet) and reading anything that doesn't
fully explain away as compiler/register-allocation noise.

## Important caution: some BinDiff matches are simply wrong

Found by direct evidence, not inference: yendor3's `ErrorCheck`
(confirmed byte-for-byte structurally identical to yendor2's, just
using a raw `ds:0x53E0` address instead of the not-yet-created
`errorCode` symbol) was sitting at an address BinDiff had instead
weakly matched (0.04 similarity) to `FileEntry_Close`. This produced a
misleading pattern across many otherwise-clean diffs — `InitGame`,
`InitializeNewGameWorldState`, `SpawnMonsterInFacingDirection`,
`TriggerSoundEvent`, and others all appeared to be missing their
`ErrorCheck` call and gaining a `FileEntry_Close` call instead, in
exactly the position right after a `FileEntry_Read`/`FileEntry_Write`.
That's now fixed (`ErrorCheck` renamed to its correct yendor3 address,
`apply_round1_corrections.py`), but it's a concrete demonstration that
a low-confidence BinDiff match can point at entirely the wrong
function — don't trust one in isolation, verify by reading.
yendor3's real `FileEntry_Close` still isn't identified.

**Two more confirmed-bad matches found in round 2**, both still
unresolved (i.e. don't trust these BinDiff labels going forward):
- The address BinDiff labeled `ShowLocalAreaMap` (similarity 0.01) is
  not that function at all -- reading it directly, it's a small
  wrapper (see `ReassignPartySlotReference` below) with 10 real call
  sites, none of them related to drawing a local map.
- The address BinDiff labeled `IsRestingAllowedHere` (similarity 0.32,
  found via `ProcessLevelMonsters`) is also wrong -- it's monster
  trap/ambush-trigger logic (`IsPositionInTriggerList` +
  `PrepareTrapEffectSlots` + `TriggerSoundEvent`), nothing to do with
  resting eligibility. Not yet identified/named.

## Confirmed differences

### New feature: a barter-pricing preview before purchase confirmation

Found independently at **three separate call sites** so far, all
inserting the same two calls — `LoadItemCatalogRecord` then
`ComputeBarterPricingPreview` — immediately before
`ShowItemPurchaseConfirmPrompt`:
- `TryHandleCatalogSlotClick` (yendor3 `sub_128F4`, called from
  `RunShopScreen` — BinDiff had matched this address only weakly,
  0.09, to the right function despite it being a real match, just
  heavily changed)
- `HandleStatusIconBarClick` (BinDiff similarity 0.98)
- A second, structurally different catalog-click handler (yendor3
  `sub_144D4`, called from `RunPartyInventoryScreen` — this is
  `TryHandleCatalogSlotClick`'s *other* yendor2 call site, but in
  yendor3 it's been forked into a distinct function with its own
  hit-test-table loop, not simply the same code reused — not yet named)

Reads as a deliberate, systematic UX addition: the player now sees a
barter-adjusted price preview before confirming any item purchase,
matching the manual's documented "`BARTERING` skill affects shop
profit margins" mechanic — Chapter 2 computed this only in a couple of
places (e.g. inside `RunPartyInventoryScreen`'s own main loop) but
Chapter 3 surfaces it at every purchase entry point.

### New function: `RefreshMultiStatEffects` (yendor3 `sub_1A5C5`)

Called from `UseTrainingItem`, no yendor2 equivalent. Walks a party
record's two multi-stat-effect field ranges (offset `0x13A` × 6 @
4-byte stride, `0x152` × 5 @ 2-byte stride), removing every
currently-applied effect via `RemoveMultiStatEffect`, calls
`SyncPartyRecordStagedStats`, then re-walks the same ranges reapplying
via `ApplyMultiStatEffectForItem` — a full stat-effect recalculation
pass, plausibly a bug fix for stacking/stale-effect issues when using
a training item.

### `sub_286D8`: a new conditional sound-driver call, not an unidentified hook

Corrects the initial read of this (found via `ProcessMonsterAttackTurn`,
also called from `HighlightSelectedAbilityIcon` and Chapter 3's
alchemy-refinement-yield function): it tests a flag bit (`ds:0xCF63`
bit `0x8`) and, if set, calls a far function pointer at `ds:0xFA00`
with `bx=8`. That function pointer is the same one yendor2's
`TriggerSoundEvent` calls as `g_soundDriverFarPtr` — confirmed by
`TriggerSoundEvent` itself showing the identical call in both games'
disassembly. Both `g_soundDriverFarPtr` (ea `0x3D7B0`) and
`g_driverStateFlags` (`ds:0xCF63`, ea `0x3AD13`) are now properly
named — see the "segmented-addressing blocker resolved" note below.
So `sub_286D8` is a **new "if flag X, poke the sound driver with
sub-function 8" wrapper**, called from at least 3 places — plausibly a
new sound-driver keepalive/reset call added around specific game
events, not a general event/hook system.

### `WaitForSoundDriverIdle` gained many new call sites

Beyond `ApplyEncodedItemEffect` (noted above), also newly called from
`RunDungeonGameLoop` and `HandleDungeonInput` (both confirmed this
round). Chapter 2 only calls it from `TryPlaySoundCueAlt`. Combined
with the `sub_286D8` finding above, Chapter 3 appears to have added
more careful/frequent sound-driver synchronization throughout — a
plausible fix for an audio-timing bug in Chapter 2.

### Likely new tile-classification helpers: `sub_1BC98` / `sub_1BCDB`

Found via `RenderDungeonVanishingPoint`, `BuildMinimapTileData`,
`RenderDungeonViewRow`, `HandleSpecialCellEntry`,
`DrawDungeonFloorAndCeiling`, `DrawDungeonCellWallTexture`, and
`DrawRevealedCellIcon` (dungeon rendering) plus `DrawWallTypeLegendRow`,
`DrawFloorTypeLegendRow`, and `DrawCellIconPair` (map editor) — no
yendor2 equivalent. Consistently inserted immediately before a
`DrawPicture` call each time, confirming these are "validate/convert a
raw tile id, then draw it" wrappers, not incidental. Both take a raw tile id in `ax`,
split it into a group (`id/100`) and remainder, index into a lookup
table (stride `0xC` for `sub_1BC98`, `0xA` for `sub_1BCDB` — matching
the already-confirmed 12-byte wall-table/10-byte floor-table entry
sizes), bounds-check against the table's own count field, and either
compute a picture id from the matched entry or fall back to a default
entry while setting `errorCode=1` on out-of-range input. Hypothesis
(not confirmed): Chapter 3 added a validated/clamped tile-id lookup
layer, plausibly because it introduced more wall/floor tile types than
fit Chapter 2's direct-index approach safely.

### New function: `ReassignPartySlotReference` / `ClearCharacterFromPartySlots`

A small helper pair, confirmed via 10 call sites this round
(`UseAbilityCommand`, `RunConversation`, `HandleSearchCommand`,
`RepairItemCommand`, `RunPartyMemberDetailScreen`, and 5 more).
`ClearCharacterFromPartySlots(bx=character ptr)` loops the 4
`g_partySlotAssignment` entries and zeroes any that equal `bx` --
removing a character's active-roster-slot reference wherever it
appears. `ReassignPartySlotReference(si=optional output ptr, bx=character
ptr)` wraps it: always clears `bx` from the roster first, and if
`si != 0` also writes `bx` into `[si]` -- effectively "move this
character's slot reference." No yendor2 equivalent; plausibly a fix
for a stale-slot-reference bug when a character is reassigned or
becomes ineligible mid-action. The underlying table is
`g_partySlotAssignment`, now correctly named at its real yendor3
address (ea `0x3AE79`, raw offset `0xD0C9` -- round 2's original
`0xCF81` guess for this table was simply wrong; see the
"segmented-addressing blocker resolved" note below).

### New function: `ApplyScriptedMapCellOverrides` (yendor3 `sub_1B704`)

Called from `RefreshDungeonMapWindow`, no yendor2 equivalent. Checks
the current map cell's position against 2 fixed coordinate pairs; if
one matches and a specific global flag is set (checked via
`TestGlobalFlag`), sets a new flag bit (`0x200`) on the cell record and
overwrites its overlay tile id. Reads as a scripted/quest-flag-gated
map decoration system -- e.g. a marker that only appears on the map
after some quest milestone. Exactly which flags/locations, and what
the two tile ids (`0xD2`/`0xDF`) represent, isn't traced.

### New function: `RefreshMultiStatEffectsAlt` (yendor3 `sub_1D76B`)

Called from `ApplyRestEffectsToCharacter`. Same remove-then-reapply
multi-stat-effect pattern as round 1's `RefreshMultiStatEffects`
(identical field offsets/strides), but with an added raw block-copy
step in the middle (an EMS-segment-relative copy, offsets `0x72`->`0x32`)
-- likely syncing an additional field range that
`RefreshMultiStatEffects` doesn't touch. Exact fields being copied not
identified.

### Unidentified new call in `MarkIneligiblePartyMembers` (yendor3 `sub_2DCE6`)

Reads a level-like field (`[si+0xE]`), folds it into a 0-9 range
(subtracting 10 twice while >9), then maps values 4-9 to a 6-bit mask
(`0x20`/`0x10`/`8`/`4`/`2`/`1`) and 0-3 to `0`. Called between
`TestRecordFlag_CA` and `DrawPartyMemberStatusPanel`. Purpose not
traced — plausibly a new level-gated eligibility/highlight indicator,
but the consuming code (what reads this bitmask) hasn't been found.

### `g_driverStateFlags` identified at a new address

Confirmed via `ParseSoundBlasterEnvironmentVariable`, which ANDs/ORs
`ds:0xCF63` with the exact same masks (`0x3FF0`/`0x8000`/`0xC000`)
yendor2's version uses on `g_driverStateFlags`. This also resolves
round 1's open question about `sub_286D8`: it tests bit `0x8` of this
same flags word -- one of the two bits `DrawCheckboxIndicator` already
gates the pause-menu MUSIC/SOUND FX checkboxes on. Now correctly named
at ea `0x3AD13` -- see the "segmented-addressing blocker resolved"
note below.

### Round 10: the "segmented-addressing" global-rename blocker resolved (it was a data-entry bug)

Every global-rename failure noted above (`g_soundDriverFarPtr`,
`g_partySlotAssignment`, `g_driverStateFlags`) turned out to share one
root cause, and it wasn't a real IDA limitation: **the addresses used
in rounds 1-3 were never valid linear addresses in *either* game.**
`0xFA00` was an unrelated literal constant from a nearby `cmp`
instruction; `0xCF63` and `0xCF81` were raw, unresolved `ds:`-relative
offsets misread as if they were flat addresses (0xCF81 was also
simply the wrong offset for `g_partySlotAssignment` -- the real one is
`0xD0C9`). The real yendor2 addresses (found via
`idc.get_name_ea_simple`, where all three are already correctly named)
are `0x3CC78` / `0x36E4B` / `0x36CE5`.

The actual fix: these globals are referenced via unresolved
`ds:0NNNNh` raw offsets in yendor3 because `assume ds:seg133` (set
explicitly near the top of `start`) doesn't propagate into other code
segments (`ShutdownAudioDrivers`, `HandleMovementInput`, etc., each
compiled as their own segment) -- so IDA can't compute a linear
address for those operands and just shows the raw 16-bit immediate.
`idc.to_ea(seg133.sel, offset)` resolves them correctly (plain
`selector*16 + offset` arithmetic is unreliable here -- IDA's internal
selector table doesn't work that way). Cross-validated two ways: (1)
the reference sites' bit masks and call patterns match yendor2's usage
exactly, and (2) a relative-offset check -- yendor2's
`g_partyFacing`/`g_partyRoleAssignment3`/`g_partySlotAssignment` sit
at fixed byte deltas from `g_driverStateFlags` (`+0x10`/`+0x22`/
`+0x166`), and yendor3's raw DS-offsets at the matching code positions
sit at the *exact same* deltas from `0xCF63` (`+0x10 -> 0xCF73`,
`+0x22 -> 0xCF85`, `+0x166 -> 0xD0C9`) -- strong evidence this
near-data cluster's layout is byte-for-byte identical between the two
games. All three now correctly named: `g_driverStateFlags` (ea
`0x3AD13`), `g_soundDriverFarPtr` (ea `0x3D7B0`),
`g_partySlotAssignment` (ea `0x3AE79`).

**This unblocked `HandleRangedOrCombatAction`.** Its yendor2 callers
(both in `start`) gate on `g_uiScratchFlags4` bit `0x1000`; that
global turned out to already be a resolved symbol in yendor3
(`word_33120`, since it's referenced directly inside `start` where
`assume ds:` *is* tracked) -- no offset translation needed. Its two
`test word_33120, 1000h` gates sit at the exact positions of yendor2's
two `HandleRangedOrCombatAction` call sites, both calling the same
`sub_1C13E` -- the address round 9 flagged as a confirmed-bad
`ComputeAlchemyRefinementYield` guess with a mysteriously huge
combat/projectile call list. A direct diff confirms it: 292/292
instructions (delta 0), call list identical except for a few new
`sub_286D8` sound-driver-hook calls (already documented above) and one
new `wait`. Renamed.

**Bonus find while tracing `start`**: `sub_11E56` (round 9's
confirmed-bad `ErrorCheck` guess, empty call list) sits in the exact
`start` call slot yendor2 uses for `ParseCommandLineSwitches`, right
after the DS segment setup. Direct diff: calls match exactly. Renamed
-- this also resolves the long-standing "ParseCommandLineSwitches is
almost certainly a wrong BinDiff label contaminating other diffs"
observation from the mid-confidence rounds; its real address was never
the one BinDiff suggested.

Only one major open thread remains from the whole 197-function review:
`DrawShadowedTextAlt`'s real yendor3 identity. Its only yendor2 caller
is `PlayStudioCreditsIntro` (5 call sites), but that caller's own
yendor3 identity is itself still unresolved (confirmed bad, rounds
7/9), so the caller-structure trick needs `PlayStudioCreditsIntro`
found first -- left for a future round.

### Possible pattern: several UI panels show one fewer value

Four different display functions each lost exactly one repeated
sub-call compared to yendor2, all in the same shape ("draw N of these"
became "draw N-1"):
- `DrawAlchemyStatusPanel`: lost its final `writeString`+
  `FormatAndDrawBCD4` pair (was showing 2 BCD-formatted resource
  values, now 1).
- `DrawTrainingScreenStatSheet`: 8 `DrawStatValueWithCapColor` calls
  became 7.
- `DrawAlchemySpellList`: 3 `DrawSpellCostValue` calls became 2.
- `ComputeDerivedCharacterStats`: 19 `ScaleByPercentRounded` calls
  became 17 (two fewer, not one -- the outlier, or two separate
  removals).

Hypothesis, not confirmed: Chapter 3 may have consolidated or removed
a resource/stat field (a leading candidate given the manual's
documented `NUORE`/`ORE`/gold 3-currency economy: dropping `NUORE` as
a separate currency would explain the alchemy-panel and
spell-cost-list reductions specifically). Worth a dedicated pass:
finding exactly *which* value each function stopped drawing, and
whether `ComputeDerivedCharacterStats`'s two missing scale operations
correspond to the same removed field(s).

### `CheckSpellCastability`: one resource check replaced with a flag check

Was `[LoadClueBookSpellEntry, IsBCDCounterAtLeast, IsBCDCounterAtLeast]`
(two resource-sufficiency checks, presumably MP and a material cost);
now `[LoadClueBookSpellEntry, TestGlobalFlag, IsBCDCounterAtLeast]` --
one of the two checks now tests a global flag instead of a BCD
counter threshold. Plausibly related to the "one fewer value" pattern
above (if a cost resource was removed, its check would change shape
like this), or a new "spell already known"/cheat-flag gate. Not traced
further.

### New pre-redraw processing step: `sub_2BA9C`

Called from both `RedrawDungeonScreen` and `RefreshDungeonScreen`, no
yendor2 equivalent. Saves a large set of registers/globals, clears a
flag bit, loops 6 times over a table calling another new function
(`sub_2BB3E`, not yet examined), then for each of the 4
`g_partySlotAssignment` members with a valid record, walks 8 entries
of their inventory (`[+0x11A]`, the confirmed main-inventory field).
Substantial new logic (73 lines) whose exact purpose isn't traced --
plausibly a new per-item tick (degradation, curse, or quest-flag
check) run before every dungeon-screen redraw. Worth a dedicated pass.

### `ApplyMapTriggerEffect` gained new calls

Two real additions found: `RevealCellsAroundPlayer` (already a known
function, newly called here) and `TickTravelResourceAilments` +
`TriggerSoundEvent` (paired, inserted after `TickPartyAilmentIconBar`).
Not traced further, but consistent with the same "more careful
sound-driver/ailment bookkeeping" theme as round 2's
`WaitForSoundDriverIdle` findings.

### Not yet resolved: `PollForEscapeKeyOnlyAlt` and `RunTitleScreen`

Deliberately **not renamed** this round -- their called-target
sequences differ too much from yendor2's to confirm via the diff
heuristic alone, unlike everything else in this batch:
- The address BinDiff matched to `PollForEscapeKeyOnlyAlt` (0.90
  similarity) calls `WaitForSoundDriverIdle` and (via a weak,
  unverified resolution) `WaitForTickAndDrawCreationFrame` -- nothing
  resembling yendor2's simple single-`PollKeyboardInput` body. Possibly
  a real match with a heavily changed body, possibly a bad match
  (BinDiff has been wrong before this session, see above) -- needs a
  direct read.
- The address matched to `RunTitleScreen` (0.88) is 40 instructions
  longer, with a new early resource-load
  (`LoadMasterPalette`/`FileEntry_Read`/`ErrorCheck`) and calls that
  resolve (with varying confidence) to `PlayTitleScreenSequence`,
  `RunMapEditorScreen`, and `PlayCreditsWipeAnimation` -- the latter
  two are almost certainly further bad low-confidence matches (0.09
  and 0.08 in the original list) contaminating the picture, similar to
  this session's other bad-match findings. Needs a direct read before
  trusting any conclusion about what changed.

### `ShowClueBook`: mostly noise from other bad matches, one real addition

Most of this diff is contamination from the same
`ParseCommandLineSwitches`-labeled address appearing repeatedly where
`RunClueEntryMenu` should be (that BinDiff label is almost certainly
wrong too, given it's a 0.28-similarity match being asked to explain 9
different call sites in a simple category-dispatch loop). One clean,
unambiguous real addition: a new `UpdateAmbientMusic` call right after
opening the clue book background -- plausibly fixing music not
continuing correctly while the clue book is open.

### `DrawLocalMapCell` and `DrawDungeonCellSideFeature` gained extra draws

`DrawLocalMapCell` gained a new call, `sub_1B751` (not yet examined),
alongside the by-now-expected tile-classification helpers.
`DrawDungeonCellSideFeature` gained a third `DrawViewportSprite` call
(yendor2 draws 2, yendor3 draws 3) alongside its tile-classification
helper -- plausibly an added decorative layer for side features.

### Corrects a 0.90-similarity BinDiff match -- the highest-confidence wrong one found this session

BinDiff labeled this address `PollForEscapeKeyOnlyAlt` (0.90
similarity -- every other wrong match found this session scored under
0.5). Read directly, its actual body is: call `WaitForSoundDriverIdle`;
if not idle, or if `cx==0xFFFF`, return immediately; otherwise wait
`cx` ticks via the generic tick-wait-and-clear primitive. Nothing
resembling Escape-key polling. Renamed to **`WaitForSoundDriverThenTicks`**
(confirmed via its actual caller, the new animation sequence below, which
calls it with tick counts and no Escape-related follow-up). Lesson:
even a 0.90 match needs verification before trusting it, not just the
low ones.

A related function this same investigation turned up,
**deliberately left unnamed**: the address BinDiff separately labeled
`WaitForTickAndDrawCreationFrame` (0.77 similarity) is a bare
tick-wait-and-clear primitive with *no* frame-drawing call at all --
matching the shape of the already-named
`WaitForTickFlagAndClear`/`WaitForTickFlagAndClearAlt` overlay-segment
duplicates (this codebase has several near-identical copies of this
same primitive compiled into different overlay segments) rather than
the frame-drawing function BinDiff suggested. Yet another such
duplicate, not confirmed to deserve a distinct name of its own yet.

### New function pair: a character-creation opening sequence

`RunCharacterCreation` gained 2 new calls prepended before its original
4 (`ComposeCharacterPortrait`/`PlayCharacterCreationIntroAnimation`/
`RunCharacterCreationSelectionStep`/`FinalizeCharacterCreation`, all
still present and in the same order):
- **`PlayCharacterCreationOpeningSetup`**: loads a new palette/picture
  pair, allocates and zeroes a ~64000-byte buffer, sets a couple of
  new flag bits, and plays a music track (9).
- **`PlayCharacterCreationOpeningSequence`**: draws the picture from
  the setup step, then drives a two-phase sound-and-animation loop (3
  iterations, then 5 more, incrementing a sound-cue id each time),
  checking a per-frame completion flag to bail early and calling
  `WaitForSoundDriverThenTicks` between frames.

Reads as a new animated intro sequence played before the existing
character-creation flow starts -- a nice-to-have visual addition, not
a mechanical change to character creation itself. One resolved call in
`RunCharacterCreation`'s diff (`DebugToggleViewportCellHidden`, at a
position that makes no sense in this context) is almost certainly yet
another bad low-confidence match, not investigated further.

### `ExamineTarget` gained a new item-requirement gate

All of yendor2's original calls (`TravelToDestination`,
`ShowAbilityDescriptionColumn` ×2, `RestoreCursorBackgroundIfDirty`,
`UpdateCursorForHeldItem`, `DrawMouseCursorAlt`) are still present in
the same order, with 4 new calls prepended: `TestGlobalFlag`,
`IsItemRangeAvailable`, `ClearGlobalFlag`, `LoadItemCatalogRecord`.
Plausibly a new "you need a specific item to examine/search this
target" requirement check. Not traced further.

### Starting the low-confidence tier: bad matches are now the majority, not the exception

Round 5 checked ~37 of the 77 low-confidence (<0.70) functions. Of a
20-function batch in the 0.33-0.51 range, **12 were confirmed bad**
(call lists bearing no resemblance at all, several with zero calls on
one side and several on the other, or vice versa) — a much higher
proportion than the high/mid tiers. Confirmed bad, left unrenamed:
`IsPairedValueMatch`, `SetPaletteToWhite`, `CollectNuoreCache`,
`InstantKillActiveMonster`, `PlaySoundSequenceGH`,
`TriggerPaletteRange16FadeUpAlt`, `ShowVisionAtLocation`,
`ClearVideoBackBufferLowerRegion`, `StepPaletteRange16FadeDown`,
`StepPaletteRange16FadeDownAlt`, `TriggerPaletteRange16FadeDownAlt`,
`PlayTitleScreenSequence`. `RestoreWipeEffectPixel` and
`PlayCreditsFrameAnimation` are also suspect (near-empty or
heavily-contaminated call lists) but not conclusively wrong.
**Practical upshot for future rounds**: below ~0.5 similarity, assume
the match is wrong until a direct read says otherwise — it's now the
more likely outcome, not the exception.

### Growing evidence for the "NUORE removed/restructured" hypothesis

Three independent data points now point the same direction:
- `DeductAlchemySpellCosts` lost one of its two
  `SubtractFromBCDCounter` calls (was deducting 2 resources per spell
  cast, now 1).
- The address matched to `CollectNuoreCache` (0.44, confirmed bad
  above) doesn't resemble a resource-collection function at all
  anymore — it plays a picture/sound/music sequence instead. If
  that's really what replaced the old `CollectNuoreCache` call site,
  NUORE collection itself may have been removed or replaced by a
  different mechanic (a triggered event/cutscene?).
- `DispatchItemAbilityCommand`'s call list (see below) no longer
  includes `CollectNuoreCache` or `CollectMagicOreCache` at all, while
  growing by 70 instructions overall.

**Correction (round 6)**: this hypothesis needs a partial retraction.
All 3 deferred functions from round 5 were fully read this round, and
2 of the 3 turned out to be bad BinDiff matches entirely unrelated to
the functions they were compared against — see below. That means the
`DispatchItemAbilityCommand` data point above wasn't real evidence: it
wasn't actually that function, so its "missing `CollectNuoreCache`
call" observation is meaningless (it was never going to have one). The
other two data points (`DeductAlchemySpellCosts` losing a call, and
the confirmed-bad `CollectNuoreCache` match itself having an unrelated
body) still stand, but the hypothesis is weaker than round 5 made it
sound — down to 2 data points, one of which (the bad match itself)
only shows *that specific address* isn't `CollectNuoreCache`, not that
NUORE collection was removed from the game. Worth still checking, but
with reduced confidence.

### Round 5's 3 deferred functions, resolved: 2 more bad matches, 1 real one substantially enhanced

- **`CheckAndPaySpecialItemCost`** (0.65) — **confirmed real**, genuinely
  enhanced. Gained a 4th special-cost type (tag `0x270F`, alongside the
  existing gold/NUORE/magic-ore tags `1`/`2`/`3`): converts the cost to
  BCD then applies `ShiftBCD4LeftNibble` twice (a ×100 scale) before
  comparing against a shared price table — a new cost type using a
  different unit scale from the other three. Also: the generic
  "pay with a specific inventory item" branch now checks whether the
  consumed item occupied one of the 6 confirmed multi-stat-effect
  equipment slots (the exact same 6 offsets `RefreshMultiStatEffects`
  walks) and, if so, calls `RemoveMultiStatEffect` +
  `RecomputeEquipmentStatBonuses` — reads as a bug fix: previously,
  spending an equipped magic item as a special payment might not
  correctly remove the stat bonus it was granting.
  **Correction to round 5's writeup**: `ShiftBCD4LeftNibble` is *not*
  a brand-new BCD helper — it already exists in both games (yendor2:
  called from `MulBCD4ByWord`; yendor3: also called from
  `PromptBuyOreQuantity`). Round 5 assumed "new" without checking
  yendor2 first. `bcd4.c`'s reimplementation scope should still grow
  to include the wider BCD family (`MulBCD4ByWord`,
  `ShiftBCD4LeftNibble`) when that work resumes — just not because
  they're Chapter-3-only.
- **`DispatchItemAbilityCommand`** (0.60) — **confirmed bad match**.
  The real body has nothing to do with relic-item dispatch: it checks
  a 3-way key against `0x176`/`0x17E`/`0x277`, handling a plain
  ability-on-target passthrough, a specific-target ability check, and
  (the interesting branch) gating on 5 specific item ids
  (`0xD6`/`0x12C`/`0x20B`/`0x224`/`0x276`) all being available,
  consuming all 5, then teleporting the party to a fixed new location.
  Reads as a genuinely new **"collect 5 quest artifacts, then unlock a
  new area"** mechanic. Renamed **`HandleSpecialQuestCommand`**. The
  real yendor3 equivalent of yendor2's `DispatchItemAbilityCommand`
  (the NUORE/ore/heal/kill relic dispatcher) is still unidentified.
- **`CastSpell`** (0.36) — **confirmed bad match**, and a structural
  tell should have caught it sooner: yendor2's `CastSpell` is called
  from `HandleGameCommand`'s action-id dispatch, but this address is
  called directly from `start` — a fundamentally different call site.
  Reading it: dispatches on a record field (`[si+4]` = 1, 2, ...),
  each branch gated on a distinct one-time global flag, triggering a
  teleport-and-message sequence on first activation. Reads as a shared
  **"resolve one scripted one-time story/world-unlock event"**
  handler — structurally a sibling to the already-known
  `ApplyMapTriggerEffect`, but for one-time story beats instead of
  repeatable map triggers. Renamed **`HandleScriptedStoryEventTrigger`**.
  The real yendor3 `CastSpell` is still unidentified.

**Takeaway**: a mismatched call-site *shape* (different caller,
different calling convention) is as strong a red flag as a low
similarity score — worth checking `CODE XREF` on any deferred function
before spending time reading its body.

### Round 8: found the real `CastSpell` by structural analysis, and confirmed `DispatchItemAbilityCommand`'s command slot was genuinely replaced

Rather than keep diffing candidate addresses, read yendor2's
`HandleGameCommand` to find `CastSpell`'s exact dispatch gate shape — a
distinctive 3-comparison pattern (range-low check, range-high-or-call
check, then a variable-equality exception: `g_currentActionId` in
`[0x12,0x1D]` or `== _val46`). The identical shape exists in yendor3's
already-renamed `HandleGameCommand`: `ds:0x5426` in `[0x34,0x3F]` or
`== ds:0x5464` → call `sub_2AE87`. That address is the one BinDiff had
separately (and wrongly, 0.17 similarity) labeled
`PartyMassHealAndOverheal` — and its own call list independently
matches `CastSpell`'s self-heal/target-effect shape
(`RedrawItemDescriptionAndMaterials`/`ConfirmAndSelectPartyTarget`/
`ConsumeItemChargeResource`/`RedrawAllPartyStatusPanels`/
`ApplyMapTriggerEffect`), two independent confirmations agreeing.
Renamed **`CastSpell`**. The real yendor3 `PartyMassHealAndOverheal`
is now unidentified in turn.

The same `HandleGameCommand` read also settles the
`DispatchItemAbilityCommand` question for good: its exact old tail
dispatch position (`call X; call DrawMouseCursorAlt; retf`, the very
last thing in the function) now calls `HandleSpecialQuestCommand` in
yendor3, not some other, still-hidden function. **There is no separate
"real" `DispatchItemAbilityCommand` to find** — Chapter 3 replaced
that whole command slot (and, with it, the NUORE/magic-ore/heal/kill
relic-item mechanic) with the new 5-artifact quest mechanic. This is
meaningfully stronger evidence for the NUORE hypothesis than round 5's
original (now-corrected) framing: it's not just that one address
doesn't look like `CollectNuoreCache` anymore — the entire game
command that used to invoke it is gone.

**Takeaway for future rounds**: when a target function is dispatched
from an already-identified, structurally-distinctive caller (a
`HandleGameCommand`-style command table, a fixed tail pattern, etc.),
finding its exact dispatch gate in the *caller* is often faster and
more reliable than diffing candidate callee addresses one at a time.

### New failure mode: BinDiff swapping two related functions with each other

Round 7 found something beyond "matched to something unrelated": two
cases where BinDiff appears to have **crossed the wires between two
real, related functions** rather than picking a random wrong one:
- The address labeled `HandleRangedOrCombatAction` (0.26 similarity)
  is actually `RestPartyAndAdvanceClock` -- its call sequence matches
  yendor2's `RestPartyAndAdvanceClock` almost verbatim
  (`ClearStatusPanelIfDirty`/.../`MaybeForceTickWorldAilments`/
  `ProcessLevelMonsters`/`ResetDailyAbilityCharges`/.../
  `ApplyRestEffectsToCharacter`/.../`TickTravelResourceAilments`),
  nothing like the combat/`AnimateProjectileStep` logic
  `HandleRangedOrCombatAction` should have. Corrected. The real
  yendor3 `HandleRangedOrCombatAction` is still unidentified -- not
  necessarily at the address BinDiff originally called
  `RestPartyAndAdvanceClock` either; that wasn't checked.
- The address labeled `DrawShadowedTextAlt` (0.04 similarity) is
  actually `RunCharacterDetailOverlay` -- its call sequence matches
  yendor2's version call-for-call in order, plus one new
  `ReassignPartySlotReference` call. Corrected. The real yendor3
  `DrawShadowedTextAlt`, and the real identity of the address BinDiff
  mislabeled `RunCharacterDetailOverlay` (a much smaller function
  involving `LoadItemCatalogRecord` and file I/O), are both still
  unknown.

Also confirmed as a real match this round: **`ClearPartySlotReferenceOnDamage`**
gained exactly the call its own name always implied it needed
(`ReassignPartySlotReference`), even though yendor2's version had no
sub-calls at all (fully inlined) -- a clean example of the
"a function with zero calls doesn't necessarily mean a bad match"
exception to the usual heuristic.

**Practical implication**: when a function's call-list diff looks
totally unrelated, it's worth a quick check of whether *another*
low-confidence match nearby (especially one with a similarly wrong-
looking diff) might actually be describing this function's real body --
not just assuming both are independently bad.

### Confirmed bad in this round's large batch (not renamed)

`ShowIntroPicture`, `CheckQuestItemsCompleted`, `EnforceDemoBoundary`,
`ShowClueBookHelpScreen`, `PartyMassHealAndOverheal`,
`CollectMagicOreCache`, `DrawShadowedText`, `RunPaletteRange16FadeDown`,
`RunPaletteRange16FadeDownAlt`, `TickPerceptionGatedAilmentSlot`
(see below), `ToggleMapViewMode`, `PlayStudioCreditsIntro`,
`DebugTeleportToCoordinates`, `PlayClueBookOpenAnimation`,
`UseLocationBoundPotion`, `DebugSetFloorTileByNumber`, `ShowWorldMap` --
17 more confirmed-bad matches, on top of the ~18 from round 5. This
tier's true bad-match rate is looking like it may exceed 60-70%
overall once fully reviewed.

### Open lead: `TickPartyAilmentIconBar`'s two worker functions

While re-verifying `TickPartyAilmentIconBar` (confirmed still correctly
named -- its real body is a small gate/counter wrapper calling two
worker sub-functions, `sub_1AF5C` on the normal-frequency path and
`sub_1B085` on a separate slow/random path), found that the address
BinDiff separately (and wrongly, 0.01 similarity) labeled
`TickPerceptionGatedAilmentSlot` is actually `sub_1AF5C` -- the normal-
path worker, which itself calls the real (already-named)
`TickDiseasePoisonSickAilmentSlot` and `TickCurseHexJinxAilmentSlot`.

**`sub_1B085` traced further**: it's a genuinely new "random party
ailment" mechanic, no yendor2 equivalent. Prepares three trap-effect
slot records via `PrepareTrapEffectSlots` (ids `0x2E`, `0x2F`, `0x30`
-- new ids, not seen elsewhere), rolls a random 1-3 value, then loops
the 4 `g_partySlotAssignment` entries (confirmed by the loop init
using `g_partySlotAssignment`'s real address directly) calling
`SelectPartyRecordById` + a second worker, `sub_1B10C`, per member.
`sub_1B10C` skips members that are dead/incapacitated (status bits
`0x1C40`) or a specific class id (`0x10B`); for the one member whose
position happens to match the random 1-3 roll, it applies the `0x2F`
trap-effect slot *plus* an extra side effect (writes
`[member+0x16]+4` to a scratch global, `ds:0x5882`); every other
member gets the `0x2E` slot instead. Both cases mark the party status
icon bar dirty (`ds:0x5370` bit `0x100`) so `ApplyEffectAndDrawIconBar`
picks it up. The third prepared slot (`0x30`) isn't consumed anywhere
in this function -- possibly used by `PrepareTrapEffectSlots` itself
for a shared/aggregate record, not confirmed. Reads as "each tick, one
random party member takes a slightly worse version of a periodic
ailment/hazard than the rest of the party" -- plausibly a new
difficulty mechanic (a roaming curse, environmental hazard, or
group-trap effect) rather than anything tied to a specific spell or
item. Exact gameplay meaning of ids `0x2E`/`0x2F`/`0x30` not decoded
(would need reading `PrepareTrapEffectSlots`' own effect-definition
table). Left unrenamed pending that -- a description this specific
without full certainty on the trap-effect semantics risks a
misleading name.

### Round 9: closed out the low-confidence tier, two more real matches found via caller context

The last ~14 unverified low-confidence functions were checked this
round. 12 confirmed bad (wildly different call lists/instruction
counts from their BinDiff-suggested yendor2 counterpart -- consistent
with this tier's established pattern). Two real matches found, both by
caller-context confirmation rather than trusting the raw call-list
diff, since Chapter 3 restructured their internals heavily:

- **`RunClueEntryMenu`**: the address BinDiff called
  `ParseCommandLineSwitches` (0x16654, 0.28 similarity) is actually the
  real `RunClueEntryMenu`. Its call list is an exact 17-call sequence
  match to yendor2's `RunClueEntryMenu`
  (`ShowClueCategoryEntries`/`PollKeyboardInput`/
  `HandleClueCategorySelection`/.../`ShowClueBookRegistrationNag`),
  and it's called from `ShowClueBook`, exactly like yendor2's version
  (13 call sites there). The address BinDiff itself suggested for
  `RunClueEntryMenu` (0x2566C, 0.12 similarity) is unrelated (near-
  empty, a single `StepPaletteFadeRange` call) -- still unidentified.
- **`RunCharacterCreationSelectionStep`**: the address BinDiff
  suggested (0x2BF44, 0.27 similarity) is actually correct, confirmed
  by caller position -- it's called from `RunCharacterCreation` in the
  same slot yendor2's version occupies. Its raw call list looks
  totally different (623 yendor2 instructions collapse to 140 in
  yendor3) because Chapter 3 factored the screen's many repeated
  `StepPaletteFadeRange`/`TriggerPaletteRange16FadeUp`/`Down`/
  `PollForEscapeKeyOnlyAlt` animation sequences into new shared
  helpers (a `RunPaletteFadeSequence`-style wrapper, plus `sub_2C11E`,
  `sub_2C376`). A genuine internal rewrite, not a bad match -- another
  example of the "consolidate repeated sequences into shared helpers"
  pattern.

Confirmed bad, left unrenamed: `TriggerFullPaletteFadeIn` (0x18691),
the address separately mislabeled `RunCharacterDetailOverlay`
(0x11778 -- the real one is at 0x237BA, already renamed round 7; this
address is still unidentified, small, uses `LoadItemCatalogRecord`/
file I/O, matching round 7's prediction exactly), `ComposeCharacterPortrait`
(0x2BD4A), `RunMapEditorScreen` (0x2B7AE), the address separately
mislabeled `TryHandleCatalogSlotClick` (0x144D4 -- the real one is at
0x128F4, already renamed round 1), `PlayCreditsWipeAnimation`
(0x14F40), the address separately mislabeled `ErrorCheck` (0x11E56 --
the real one is at 0x28C1A, already renamed round 1),
`DrawCharacterCreationAnimationFrame` (0x2BBB5),
`ComputeAlchemyRefinementYield` (0x1C13E -- y3 side is a huge
combat/projectile function, nothing like BCD refinement math),
`DebugToggleViewportCellHidden` (0x2BC75), `DebugSetOverlayTileByNumber`
(0x2147B -- y3 side handles clue-location string formatting, possibly
a new "show location" feature, worth a dedicated future look).

**Cluster observation**: several of the still-unidentified addresses
in this batch (0x2BD4A, 0x2BBB5, 0x2BC75, plus the now-confirmed
`RunCharacterCreationSelectionStep` at 0x2BF44) all sit in the same
0x2BB7C-0x2BF44 code region and share calls to a small set of new
helpers (`~DrawCharacterCreationAnimationFrame`, `StepPaletteFadeRange`,
`DrawPicture`, `wait`). This whole region looks like Chapter 3's
restructured character-creation screen code -- likely worth a
dedicated future pass rather than one-off diffing.

This closes out the full 77-function low-confidence tier: all three
BinDiff match tiers (68 high, 52 mid, 77 low = 197 total) have now
been spot-checked at least once.

### Round 12: a new quest-item-conversion utility chain (ruled out `RunCharacterDetailOverlay` for good)

Traced the address BinDiff suggested for `RunCharacterDetailOverlay`
(0x11778, 0.15 sim -- the real one is at 0x237BA, round 7) all the way
through its call chain. It's unrelated: a genuinely new Chapter 3
item-conversion system with no yendor2 equivalent, reached from
`HandleScriptedStoryEventTrigger` (the new 5-artifact quest handler).
`SwapItemInstanceEverywhere` (yendor3 `sub_1161C`) takes a target item
id and a replacement item id: checks a 6-entry shared table first,
then loops all 4 party members via `g_partySlotAssignment` calling
`SwapItemInMemberInventory` (`sub_116EF`) on each, which searches that
member's 14-slot inventory (plus one equipped-item slot) for the
target item and swaps it in place (reapplying multi-stat effects).
`HandleScriptedStoryEventTrigger` calls it in a loop (item `0xC6` ->
`0xCD`) tallying conversions -- "convert every instance of item X the
party is carrying into item Y," presumably a quest-artifact
transformation step. Both renamed.

A third helper in the chain, `sub_11778` (called for *catalogued
limited* items, catalog flag `0x2000`), manages an 8-slot table
(base `ds:0xA5A6`, referenced widely elsewhere in the codebase, likely
an existing yendor2 global not yet cross-referenced) and persists it
to disk via `FileEntry_Write` -- looks like a registry of
limited/unique item instances currently in play, but left unnamed;
its exact purpose needs the `ds:0xA5A6` table's yendor2 identity
confirmed first.

### Round 13: two more low-confidence guesses ruled out / resolved

- The address BinDiff suggested for `RunClueEntryMenu` (0x2566C, 0.12
  sim) is conclusively NOT that function -- its body is a tiny 2-
  instruction stub (`StepPaletteFadeRange` with fixed params, then set
  a screen-dirty flag bit) called from 14 places scattered across
  totally unrelated code, not anything ShowClueBook-specific. A
  small, generic, widely-reused utility -- left unnamed as too generic
  to confidently map to one yendor2 identity.
- The address BinDiff suggested for `TryHandleCatalogSlotClick`
  (0x144D4, 0.09 sim -- the real one is at 0x128F4, round 1) turned
  out to be a genuinely new helper: it factors out a sequence that's
  inlined directly in yendor2's `RunPartyInventoryScreen` (hit-test a
  slot, then `ComputeBarterPricingPreview` + `ShowItemPurchaseConfirmPrompt`
  in the same order) into its own function, called from
  `RunPartyInventoryScreen` in yendor3. Renamed
  `TryHandlePartyInventorySlotClick` (moderate confidence -- the
  hit-test/8-entry-table wrapper itself isn't present verbatim in
  yendor2, only the barter-preview-then-confirm tail is a confirmed
  match).

### Round 14: the character-creation-region cluster resolved

Read `RunCharacterCreation`'s full yendor3 call sequence directly
rather than diffing candidates one at a time:
`PlayCharacterCreationOpeningSetup` -> `sub_2BC75` ->
`PlayCharacterCreationOpeningSequence` -> `sub_2BD4A` ->
`PlayCharacterCreationIntroAnimation` -> `RunCharacterCreationSelectionStep`
-> `FinalizeCharacterCreation`. Notably, yendor2's `ComposeCharacterPortrait`
does not appear anywhere in this sequence -- it looks to have been
dropped in favor of extra opening-screen content instead (its
suggested address, 0x2BD4A, turned out to be something else entirely,
see below).

- `sub_2BC75` -> **`PlayCharacterCreationOpeningPicture`**: draws
  picture id `8`, then an escape-pollable wait loop, no sound sequence
  -- the same "one static picture + wait" shape as
  `PlayCharacterCreationOpeningSetup` right before it.
- `sub_2BD4A` -> **`PlayCharacterCreationOpeningSequenceAlt`**: draws
  picture id `0xA`, then a `TriggerSoundEvent`/`WaitForSoundDriverThenTicks`
  cue loop -- byte-for-byte the same shape as
  `PlayCharacterCreationOpeningSequence` right before it, just
  different picture/sound ids and loop counts. A structural clone,
  matching this project's established `...Alt` convention. (This is
  the address BinDiff had suggested for `ComposeCharacterPortrait` --
  ruled out once the full body was read: nothing here resembles
  portrait composition, it's another title-card screen.)
- `sub_2BBB5` -> **`PollForEscapeKeyOnlyAlt`** (the escape-poll gate
  both of the above call repeatedly): byte-for-byte identical to
  yendor2's `PollForEscapeKeyOnly`/`PollForEscapeKeyOnlyAlt` (both are
  byte-identical overlay-segment duplicates in yendor2 too). **Correction**:
  first applied as plain `PollForEscapeKeyOnly`, which silently
  collided with an already-correctly-named function at a *different*
  yendor3 address (0x15044, unrelated to this cluster) -- IDA
  auto-suffixed it to `PollForEscapeKeyOnly_0` without erroring, caught
  and fixed in a follow-up check. The caller
  (`PlayCharacterCreationIntroAnimation` calls
  `PollForEscapeKeyOnlyAlt` in yendor2) confirms `PollForEscapeKeyOnlyAlt`
  is the correct one of the two names for this address.

This resolves the whole cluster flagged since round 9. Reads as:
Chapter 3 added two more opening/title-card screens (one silent
picture, one picture-plus-sound-cue) to the character creation intro
sequence, and removed the portrait-composition step entirely (or moved
it elsewhere, not found).

### Round 15: found the real `ShowIntroPicture` and confirmed `RunTitleScreen` at last

Continued chasing `DrawShadowedTextAlt` by tracing its blocker
(`PlayStudioCreditsIntro`) from a different angle -- found something
even more valuable along the way. `sub_14F40` (round 9's confirmed-bad
`PlayCreditsWipeAnimation` guess) is actually the real
`ShowIntroPicture`: draws a picture, reloads the master palette
(`LoadMasterPalette`/`FileEntry_Read`/`ErrorCheck`/
`TriggerFullPaletteFadeOut`, via a tail helper reordered to run after
the main loop instead of before), then loops a data table of text
entries calling `DrawStringColumn` with `PollForEscapeKeyOnly` checks
and fade transitions between entries -- the exact shape of yendor2's
`ShowIntroPicture` (whose "shadow text" effect turns out to be two
manual offset `DrawStringColumn` calls, not `DrawShadowedTextAlt` --
confirming `ShowIntroPicture` never used `DrawShadowedTextAlt` in
either game, only `PlayStudioCreditsIntro` does). Clinched by
`WaitFrameTicksOrEscape`'s inherited comment ("Called from
ShowIntroPicture") matching its only yendor3 caller. Renamed.

That let a much bigger fish get landed: `sub_14F40`'s caller,
`sub_1BE7E`, is called from the already-established `ConfirmNewGame`
-- and a direct diff against yendor2's `RunTitleScreen` (BinDiff's own
0.88-similarity suggestion for this exact address, left deliberately
unrenamed back in round 4 as "too large/noisy to trust without a
dedicated read") shows the same ~20-call core sequence in the same
order in both games, including both `ShowIntroPicture` and
`RunCharacterCreation` called twice each, matching exactly. **Confirmed
and renamed `RunTitleScreen`** -- closing out one of round 4's two
long-deferred mid-confidence exceptions (the other,
`WaitForTickAndDrawCreationFrame`'s suggested address, was already
separately resolved as a generic tick-wait duplicate).

`PlayStudioCreditsIntro` (and therefore `DrawShadowedTextAlt`) is
still not found -- `RunTitleScreen` doesn't call it, consistent with
yendor2 where it's only reached from one specific `start`-tail easter
egg (a hidden 3-flag check) that still hasn't turned up an equivalent
in yendor3.

## Savegame (`CURGAME`/`SAVGAMEn`) layout differs in section sizes

Found while writing `src23/savegame.c` (2026-09-19). The save/load code
(`SaveCurrentGameToSlot`, `RunGameDialog`'s load path) has an identical
structure in both games, and the same 7 contiguous sections, but four
section sizes grow in Chapter 3 (the larger world), each read from the
`InitGlobals` constants and cross-checked against the stub offset
tables: explored-map rows 144 -> 168 (x100 bytes), the `DC6` state
section 644 -> 1059, the `DCA` state section 608 -> 1008, and the
monster-spawned bitmap 313 -> 626. Item instances (1296 x 34), the
80 x 156 monster block, the 5000-byte header/party block and the 500-byte
party record stride are unchanged. Total file size 77,509 -> 81,037. See
`file-formats.md`'s byte-layout table. `src23/savegame.c` selects between
the two layouts by exact file size.

## Party record: same layout, three small differences

Found while writing `src23/party.c` (2026-09-19). Chapter 3's record uses
the same offsets (its `GetInventorySlotPtr` offsets, class tables and
27-entry stat-name table all index identically, and it reads bartering at
`+0x68`), with these differences:
- The stat table calls hit points `HEALTH` instead of `HIT POINTS`.
- The `CHEMISTRY` entry (`+0x70`/`+0xB0`) is blank: the chemistry skill has
  no name in Chapter 3, consistent with the alchemy changes above.
- `GetInventorySlotPtr` has no case for equipment code `0x12`, so it falls
  through to `+0x15A` (the same as code `0x14`) and slot `+0x156` cannot be
  reached through it; Chapter 2 maps `0x12` to `+0x156`. Looks like an
  accidental omission rather than a design change.
`partyStatName` and `partyEquipmentSlot` take a `GameKind` for these.

## Item catalog: different contents and sizes, same record format

Found while writing `src23/item.c` (2026-09-19); checked against both real
`WORLD.DAT` files. The record format, table selection rules and effect-pair
format are identical. What differs:
- **Size**: 631 items (all real) vs. Chapter 2's 744 real items (759 loaded);
  the effect/wearable/consumable/weapon tables are 148/221/151/210 entries vs.
  175/275/250/190. Table offsets are in `file-formats.md`.
- **Ids are renumbered** (Ch2 id 2 MAGIC ORE, 6 BAG, 55 BREAD; Ch3 id 2 FOOD,
  6 CLOTHES +2, 55 PINK POTION), so item ids must never be shared between games.
- **The resource items changed**: Ch2's first three are GOLD COINS, MAGIC ORE
  and NUORE; Ch3's are GOLD COINS, **FOOD** and NUORE. So Chapter 3 swapped
  MAGIC ORE for FOOD and *kept NUORE*, which weakens the earlier hypothesis
  here that Chapter 3 removes the NUORE mechanic (the new 5-artifact quest
  system may sit alongside it, not replace it).
- `SAPHIRE` (sic) is spelled `SAPPHIRE` in Chapter 3.

## Monster catalog: same block format, bigger roster

Found while writing `src23/monster.c` (2026-09-19), checked against both real
`WORLD.DAT` files. The 106-byte stat block, its field offsets, the two-level
lookup (type id -> block index -> block) and the spawn code are identical.
Differences: **73 blocks vs. Chapter 2's 62** (a different monster list: Ch2
starts ALLIGATOR, ARMY ANT, BATS...; Ch3 starts WASP, CENTIPEDE, WASP QUEEN...
and ends PURPLE DRAGON), one `NOT USED` filler block (Ch3 block 62), a lookup
covering types up to 1862 instead of 2143, and **23 boss/quest death flags vs.
17**. Monster type ids and block indices, like item ids, are per-game.

## Trap/status effect table: same record format, more effects

Found while writing `src23/effect.c` (2026-09-22). The 12-byte definition
format, `PrepareTrapEffectSlots`'s indexing, and the whole cost/resistance/
magnitude pipeline (`ApplyEffectCost`, `RollEffectMagnitude`,
`RollEffectResistance`, `ApplyIconBarStatDelta`) are identical between the
games. Chapter 3 defines **49 effects vs. Chapter 2's 45**, at a different
base address (`DS:0x96DA` vs `DS:0x905B`) with its own sound/icon ids per
effect — confirmed by round-tripping every monster's attack/special-attack
effect id against its own game's table with no exceptions.

## World map: same format, bigger grid — and a genuinely different tile-legend mechanism

Found while writing `src23/worldmap.c` (2026-09-22), checked against both
real `WORLD.DAT` files. The map is one continuous 800-column-wide tile
grid starting at file offset 0 in both games, same row byte format (two
`u16` tile-type indices per column), same row size (3200 bytes,
`4*_blockSize3` with `_blockSize3=0x320` in both games). Chapter 3 is
**168 rows tall vs. Chapter 2's 144** (cross-checked against each game's
own `CURGAME` fog-of-war bitmap row count, which matches exactly).

**The wall/floor tile-legend tables are not just relocated — Chapter 3
uses a different, paged mechanism.** Chapter 2's are flat arrays
(`id*stride + base`). Chapter 3's legend-row drawers instead call
`sub_1BC98`/`sub_1BCDB`, which divide the tile-type id by 100 and select
a **page** before indexing — the same style of scaling-up seen elsewhere
for Chapter 3's larger resources (bigger item/monster/effect tables), but
here it's a structural change, not just bigger constants. Not fully
traced (see `file-formats.md`'s "World map" section) — `src23/worldmap.c`
only resolves wall/floor picture offsets for Chapter 2 as a result.

## In-world readable text: same mechanism, very different content mix

Found while writing `src23/document.c` (2026-09-23), checked against both
real `WORLD.DAT` files. The mechanism is identical: four id-indexed text
pools, in-EXE offset/length index tables (different addresses, same
shape), fixed-width NUL-terminated lines, all four categories' real
entries tiling one small contiguous `WORLD.DAT` span exactly. What
differs is which categories actually have content:
- **Note** (`_1000`): empty in Chapter 2 (1 placeholder entry, length 0)
  but has **8 real entries** in Chapter 3 (short messages, including a
  spoken password signed by "Queen Obversia").
- **Document** (`_2000`): Chapter 2 uses all 26 reserved slots; Chapter 3
  reserves the same 26 slots but only **6** have real content (7-26 are
  present, real, and legitimately zero-length).
- **Journal** (`_4000`) grew from 6 entries to 8.
Net effect: Chapter 3 shifted some of its "found text" content from the
Document pool into the previously-unused Note pool, rather than simply
having more text overall. `src23/document.c` embeds both games' full
tables as compile-time data (like `effect.c`'s `g_trapEffectDefs`) since,
unlike every other per-game difference documented here, this data lives
in the executable itself, not `WORLD.DAT`.

## Movement/passability: identical algorithm, unrelated numeric thresholds

Found while writing `src23/movement.c` (2026-09-23), from direct
instruction-level reading of both games' `HandleMovementInput`,
`ClassifyFloorType`, and `IsCellTypeImpassable` (see `file-formats.md`'s
"Movement and cell passability" for the full per-cell decision). The
direction/turn/strafe math is instruction-for-instruction identical —
every facing-bit remap and every `(Δcol, Δrow)` pair matches exactly
between the two games' disassembly, nothing shared here needed a
game-aware branch. What differs is purely the numeric classification
thresholds fed into that shared shape, and they are **not** simply
Chapter 3's numbers scaled up by some constant factor — the band
structure itself changes shape:

- **Playable bounding box**: same column range (`0x28`-`0x2F7`) both
  games; row range grows from Chapter 2's `0x18`-`0x77` to Chapter 3's
  `0x18`-`0x8F`, matching the already-documented 144-vs-168-row map
  height above.
- **`ClassifyFloorType`** (wall-type → void/blocked/normal): Chapter 2
  is void/blocked/normal/blocked-again in four bands (`{0,1}`,
  `[2,15]`, `[16,57]`, `58+`); Chapter 3 is void/blocked/normal/blocked/
  normal in five (`{0,1}`, `[2,99]`, `[100,199]`, `[200,299]`, `300+`).
  Chapter 3's isn't Chapter 2's shape at 100x scale — Chapter 2 ends on
  an unbounded *blocked* band past its table, Chapter 3 ends on an
  unbounded *normal* one.
- **`IsCellTypeImpassable`** (floor/overlay-type impassability):
  Chapter 2's impassable set is three disjoint pieces — `[21,35]`, the
  single value `37`, and `[39,42]` — not a range at all; Chapter 3's is
  one contiguous range, `[200,399]`.
- **Special-cell-entry range** (bypasses passability, always enters):
  Chapter 2 is `6`-`11` (`_val32`/`_val31`), sitting inside its own
  `ClassifyFloorType`'s first blocked band; Chapter 3 is `200`-`299`
  (`ds:5450h`/`5452h`), which is *exactly* its `ClassifyFloorType`'s
  second blocked band, not a narrow sub-range within a bigger one —
  the "special" and "blocked-by-default" concepts fully coincide for
  that one page in Chapter 3, where in Chapter 2 they're a small
  special carve-out inside a wider blocked range. Consistent with (but
  not fully explained by) Chapter 3's already-documented EMS-paged,
  100-entries-per-page tile-legend mechanism above.

Net effect for anyone touching this code later: don't assume any
Chapter 3 passability constant is a Chapter 2 constant times 100 —
check the real disassembly range for each function independently, the
same way `src23/movement.c`'s two classification functions had to be
written as explicit per-game band lists rather than one shared formula.

## World object index: same format and offset-finding method, different content mix

Found while writing `src23/worldobjects.c` (2026-09-23), checked
against both real `WORLD.DAT` files. Same as the in-memory dungeon
grid immediately below: the format itself (720-column offset array,
6-byte sorted-by-row records, `0xFFFF` terminator, the flag-bit
dispatch order) is instruction-identical between the two games — only
the block's `WORLD.DAT` offset (`0x1A1141` Chapter 2 vs. `0x41090D`
Chapter 3, both discovered the same way, via
`extract_resource_stubs.py`) and its content differ:
- Chapter 2: 3108 reachable records; Chapter 3: 2572 — fewer overall
  despite Chapter 3's larger map, consistent with the already-documented
  broader pattern of Chapter 3 redistributing content rather than
  simply scaling everything up (see "In-world readable text" above).
- The relative mix shifts too: Chapter 3 has proportionally more
  monster-spawn markers (flag `0x800`: 72% of its records vs. Chapter
  2's 69%); door/lock markers (flag `0x8000`) stay close to the same
  proportion in both (~14%); the bigger shift is curgame-record
  markers, flag `0x4000`, dropping from 231 records (7.4% of the
  total) in Chapter 2 to just 71 (2.8%) in Chapter 3.
- Chapter 2's raw on-disk data has 3 dead records with rows outside
  the playable bounding box (unreachable via the real lookup code,
  since the bounds check runs before the scan); Chapter 3's real data
  has none.

## Lock/door catalog: same format and loading mechanism, very different key-bit usage

Found while writing `src23/lockcatalog.c` (2026-09-23), checked against
both real `WORLD.DAT` files. The format (26-byte records, same two
confirmed fields), the loading mechanism (EMS-paged via the identical
`loadWorldDat2`+`loadWorldDat3` two-chunk pattern), and the message
dispatch logic (`ShowLockStatus`) are all instruction-identical between
the games — only the offset (`0x7E5BA` Chapter 2 / `0x8F00A` Chapter 3)
and the content differ. The content difference here is sharper than
most: **Chapter 2's 7 key-requirement bits are essentially a one-hot
selector (0 of 608 records have more than one set) while Chapter 3's
are a genuine bitmask (123 of 1008 records have more than one)** —
different enough in kind, not just degree, that reimplementing this
from Chapter 2's data alone would have produced a wrong model
(`lockRequiredKeyType`'s "first match wins" resolution only matters at
all because of Chapter 3's real data). Chapter 3 also uses one specific
key type, Copper, far more than the others (178/1008, ~18%, vs. its
next-most-common bit at 16) where Chapter 2's distribution across the
7 types is comparatively even — not investigated further, but worth
noting if a puzzle/quest-design question about Chapter 3 comes up
later. Magical locks are also proportionally far more common in
Chapter 3 (214/1008, ~21%) than Chapter 2 (17/608, ~3%).

## Monster distance activation: no behavioral difference found

Found while writing `src23/monster.c`'s `monsterTryActivateByDistance`
(2026-09-23). Instruction-identical between the two games, including
every threshold (`0x21`/`0x2C`/`0x29`/`0x26`), confirmed by direct
comparison.

## Monster tick-timer state machine and per-slot processing: no behavioral difference found

Found while writing `src23/monster.c`'s `monsterTickTimer` and
`src23/monsterpool.c`'s `monsterPoolProcessSlot` (2026-09-23). Both
`TickMonsterTimer` and the surrounding per-slot flow in
`ProcessLevelMonsters` are instruction-identical between the two
games, confirmed by direct comparison, not assumed from the pattern
established elsewhere this session.

## Monster approach/ambush check: Chapter 3 fixes a Chapter 2 bug

Found while writing `src23/monsterpool.c`'s `monsterApproachParty`
(2026-09-23), from direct side-by-side comparison of both games'
`ProcessLevelMonsters`. The alignment/direction/ambush logic is
otherwise instruction-identical, but the scan-ahead step limit differs
in a way that looks like an unintentional bug in Chapter 2, corrected
in Chapter 3:
- **Chapter 2** only explicitly bounds the obstacle scan to 5 steps
  (`mov cx, 5`) when the monster is on the "far" side of the party
  along the aligned axis (south or east of it). On the "near" side
  (north or west), the code never (re)initializes `cx` before using it
  as the scan's loop counter — it inherits whatever value is left in
  `ProcessLevelMonsters`' own unrelated 80-slot outer-loop counter,
  meaning the real scan depth on that side depends on which pool slot
  index the monster currently occupies (anywhere from ~1 to 80), not a
  deliberate game-design value.
- **Chapter 3** adds the missing `mov cx, 5` unconditionally, before
  the near/far branch, so both sides get the same explicit 5-step
  bound.

`src23/monsterpool.c` always uses the corrected 5-step bound for both
games (see its own doc comment for the reasoning) rather than
replicate Chapter 2's pool-slot-index-dependent quirk, which isn't
meaningful game content to preserve.

Otherwise, the wall/floor obstacle-classification thresholds
(`monsterClassifyObstacle`) and the ambush-roll thresholds both differ
between the games in the usual way (see `file-formats.md`'s "Monster
approach and ambush check" section for the exact bands) — same shape
of difference already established for the movement/passability system.

## Monster death (rewards/removal) and global flags: no behavioral difference found

Found while writing `src23/monsterpool.c`'s reward/removal additions
and `src23/globalflags.c` (2026-09-23). `GrantMonsterRewards`,
`RemoveMonsterFromMap`, and `GetGlobalFlagBitAndWord`/`Set`/`Clear`/
`TestGlobalFlag` are all instruction-identical between the two games
(spot-checked directly, not assumed from the pattern established by
the rest of this session's monster-pool work).

## Monster spawning: no behavioral difference found

Found while writing the spawn half of `src23/monsterpool.c`
(2026-09-23), from a full instruction-level trace of both games'
`SpawnMonsterInFacingDirection`. Same as the scroll relink/despawn
half below — instruction-identical, including all 4 of its
facing-dependent position-offset tables (204 signed-byte-pair entries
total), confirmed byte-for-byte identical between the two games'
executables via direct extraction, not inferred from structural
similarity alone.

## Monster pool scroll relink/despawn: no behavioral difference found

Found while writing `src23/monsterpool.c` (2026-09-23), from a full
instruction-level trace of both games' `RefreshDungeonMapWindow`
continuation (the part after the base grid build documented above).
Like the in-memory dungeon grid itself, this is instruction-identical
between the two games — same 80-slot pool, same inclusive
`[origin, origin+78]` bounds check, same cell-offset formula, same
`0x400` overlay bit, same `ClearCellMonsterSpawnedFlag` call on
despawn. No game-specific behavior to record.

## In-memory dungeon grid: no behavioral difference found

Found while writing `src23/dungeongrid.c` (2026-09-23), from a full
instruction-level trace of both games' `RefreshDungeonMapWindow`
(`yendor2.asm:29286`, `yendor3.asm:28425`). Unlike every other module
documented in this file, **no difference at all** turned up here — the
78x78 window size, the origin-clamp formula and its constants (39,
15), the per-cell copy shape (two tile-type words, a zeroed third
word, an explored-bit word), and the MSB-first explored-bitmap
bit-packing are all instruction-for-instruction identical between the
two games. The only per-game input is the playable bounding box
(`movement.h`'s `MovementBounds`, already documented above as
differing only in Chapter 3's taller row range) that the origin clamps
against — reused directly rather than duplicated. Recorded here mainly
so a future session doesn't waste time re-checking a game-diff that
was already ruled out.

## `TryInteractAtPosition`: identical dispatch logic, but Chapter 3 disables the shared-bitmap id offset

Found while writing `src23/interact.c` (2026-09-24), from a full
instruction-level trace of both games' `TryInteractAtPosition`
(`yendor2.asm:30813`, `yendor3.asm`'s equivalent). The dispatch logic
itself -- all 11 `errorCode` branches, the flag-priority order, the
door/curgame-record "already resolved" bitmap check -- is
instruction-identical, same as most of this project's dungeon-loop
modules.

The one real difference is in the data, not the code: the shared
"already unlocked/triggered" bitmap's curgame-record id offset
(`interactCurgameIdOffset` in `interact.h`) is a live, correctly-set
constant in Chapter 2 (`_val10 = 608`, exactly its lock count) but a
global that's **read, never written, always 0** in Chapter 3
(`word_2ECF8`) -- meaning Chapter 3's curgame-record ids share bit
positions with its own lowest lock ids in this bitmap, while Chapter
2's two id spaces are kept cleanly separate. This is the *second*
always-zero offset global found in `LoadCurgameRecord` specifically
(the first, `word_3320E`/`_val9`, is a separate multiplier for the
still-undecoded 4-byte EMS record -- see `lockcatalog.h`'s
"LoadCurgameRecord" note) -- not proof of a genuine bug, but suggestive
enough to flag rather than dismiss as isolated noise.

## Character leveling: identical mechanism and cascade logic, a genuinely different XP curve

Found while writing `src23/party.c`'s `partyCheckForLevelUp` (2026-09-24),
from extracting both games' XP-threshold tables directly via IDA
(`ida_scripts/dump_xp_threshold_table.py`, both games) rather than
assuming they matched. `CheckForLevelUp`'s own code — the threshold
walk, the `>=`-then-`>` comparison asymmetry, the 89-entry/level-90 cap
shape — is instruction-identical between the two games, same as most
of this project's non-combat mechanics.

The table contents are not shared, though: nearly every one of the 39
real per-level jumps differs between the games (both start at 680 XP
for level 1->2, but diverge immediately afterward), and even the
"effectively unreachable" sentinel value repeated for the remaining
50 entries (levels 40-89, past each game's real curve) differs --
`90,000,000` in Chapter 2 vs. `99,999,999` in Chapter 3. A real
balance/content difference, not an artifact of a shared table read at
different addresses.

`ShowLootAndAwardExperience`'s own staging-drain/award logic
(`monsterRewardsAward`) is likewise instruction-identical between the
two games -- checked directly, not assumed.

## Character training/leveling (`UseTrainingItem`): identical growth formulas, Chapter 3 disables secondary-class promotion

Found while writing `src23/party.c`'s `partyApplyTraining` (2026-09-24).
The cost gate, level increment/cap, HP/MP growth formulas (including
the full per-class-base MP-growth weighting table), and the two
flat-`+2` stat-growth loops are all instruction-identical between the
two games — same pattern as most of this project's character/combat
mechanics. `SyncPartyRecordStagedStats` and
`RefreshCarryCapacityAndAttributeBonuses` (the two functions
`partyApplyTraining` calls at its tail, added same day) are
instruction-identical too, same constants (72 threshold, 20% scale)
and structure, checked directly. `RecomputeEquipmentStatBonuses`
(`partyRecomputeEquipmentStatBonuses`, same round) is likewise
instruction-identical — same baseline-field layout including the
EquipRating2/3 swap, same per-slot skill selection.

The secondary-class promotion step is not, though: Chapter 2 promotes
a character's class (`PartyFieldClass += 10`) at levels 10 and 30
(`_val25`/`_val26`, live constants). Chapter 3's equivalent globals
(`word_331F8`/`word_331FA`) are read but never written anywhere in the
disassembly — always 0 — so the comparison against a real level
(always ≥ 1) can never match. **Secondary-class promotion via training
is therefore effectively disabled in Chapter 3.** This is the third
always-zero-global quirk found in this project, each in an unrelated
subsystem (see `lockcatalog.h`'s "LoadCurgameRecord" note and
`interact.h`'s `curgameIdOffset` note for the other two) — a repeated
pattern across three independent subsystems is more suggestive of a
systemic Chapter 3 change (or a shared code-generation/init bug) than
three isolated coincidences, but that remains unconfirmed.

## Ability/spell-unlock table: identical shape, different ids, same latent physical-class bug

Found while writing `src23/party.c`'s `partyAbilityUnlocksAtLevel`
(2026-09-24). The table's shape (6 rows, 20 columns, 2 `u16` slots) and
`UseTrainingItem`'s walk logic are identical between the games, but the
ability ids themselves genuinely differ — e.g. MONK's level-4 unlocks
are ids `7`/`0xb` in Chapter 2 but `6`/`7` in Chapter 3 — the usual
per-game id-space difference this project has found everywhere else
(monster types, item ids, block indices, ...).

One thing that is *not* a Ch2/Ch3 difference, confirmed by checking
both: the apparent original-engine bug where class base 1-3
(FIGHTER/MERCHANT/ROGUE, at any tier) reads out of the ability table's
bounds into `TravelToDestination`'s own destination-table data — the
row-index arithmetic and the destination-table base addresses both
differ between the games (`0xD40B` vs. `0xBA95`), but the *relationship*
(`abilityTable + 6*0x50 == destinationTable`, so any row beyond 5 reads
into it) holds identically in both. See `file-formats.md`'s "ability/
spell-unlock table" section for the full derivation.

## Attack resolution primitives: identical, but Chapter 3 adds a resisted-special-attack fallback

`ResolveAttack` and `FailsSavingThrow` themselves are instruction-
identical between the two games — checked directly, `yendor2.asm:
38488`/`:41947` vs. `yendor3.asm:38492`/`:42293`. Same damage/chance
formulas, same RNG bounds (55 and 100 respectively), same
minimum-damage/chance-floor clamps. Reimplemented once in
`src23/combat.c`, shared by both games.

**`ResolveAttackerActionOutcome` itself is *not* instruction-identical
— a real, previously-undocumented Chapter 3 behavioral addition**,
found by reading `yendor3.asm:2150` directly rather than assuming a
match from the shared branch structure. In both games, branches 2
("status-effect application") and 3 ("equipment corrosion") each gate
their effect on a `FailsSavingThrow` roll. **In Chapter 2, a resisted
saving throw for either branch is a clean miss** — the function
returns immediately with nothing staged (`locret_16CC0`/`locret_16D4C`).
**In Chapter 3, a resisted saving throw for either branch instead
falls back to a normal damage roll**: it clears `g_uiScratchFlags4`
bit `0x200` (the very flag that selected branch 2/3 over branch 1 in
the first place), loads the attacker's `MonsterFieldAttackEffect`
through `PrepareTrapEffectSlots` (`yendor2.asm:13722`, the shared
effect-table lookup `effect.h` already decomposes into
`effectGetDef`/`effectMagnitude`/etc., not reimplemented as a single
function under that name), and jumps back into branch 1's own
`ResolveAttack` damage-roll code (`loc_11346`) — so a resisted special
attack in Chapter 3 still deals ordinary damage instead of doing
nothing. This reads as a deliberate game-feel fix (a resisted special
attack no longer wastes the monster's whole turn) rather than an
incidental difference. Not reimplemented yet, since
`ResolveAttackerActionOutcome` as a whole is deferred — see
`file-formats.md`'s "Attack resolution" section for the full branch
breakdown and what's still open.

## The icon-bar effect-application pipeline: no behavioral difference found

`ProcessMonsterAttackTurn`, `SelectTrapEffectVariant`,
`PrepareTrapEffectSlots`, `ApplyEffectAndDrawIconBar`,
`DeductHPClamped`, `DeductMPClamped`, `ApplyEffectCost`,
`RollEffectResistance`, and `RollEffectMagnitude` are all
instruction-identical between the two games — checked directly against
`yendor3.asm` (`:1744`, `:2358`, `:6287`, `:6322`, `:6522`, `:6541`,
`:6556`, `:6688`, `:6775` respectively). Same field offsets, same mode
dispatch, same clamp/death-flag behavior. Reimplemented once
(`effectRollMagnitude`/`effectResolveInflictedStatus` in
`src23/effect.c`, `combatApplyEffect` in `src23/combat.c`,
`partyDeductHp`/`partyDeductMp` in `src23/party.c`), shared by both
games.

## Turn-based combat turn order and round processing: no behavioral difference found

`BuildCombatTurnOrder`, `SelectActiveMonster`, and `ProcessCombatRound`
are all instruction-identical between the two games — checked
directly, `yendor2.asm:10952`/`:11340`/`:11094` vs.
`yendor3.asm:1929-2018`/(`SelectActiveMonster`'s and
`ProcessCombatRound`'s own matches). No Ch2/Ch3 divergence of any
kind: same sort logic, same stable-tie behavior, same random-target
retry, same dead pre-check branches (not reproduced in either game's
reimplementation — see `file-formats.md`), same death-scan/turn-advance
gating, same `CompactMonsterSlots`/`RelocateActiveMonsterPointer`
pointer-fixup mechanism (also not reproduced — this reimplementation's
index-based turn-order entries have no stale-pointer problem to fix).
Reimplemented once in `src23/combat.c`, shared by both games via the
existing `GameKind` plumbing already threaded through `party.c`/
`monster.c`/`savegame.c`.

## Review status

- 68 functions bulk-imported at BinDiff similarity >=0.95
  (`yendor3/ida_scripts/apply_bindiff_high_confidence.py`); all 68 now
  spot-checked across rounds 1-2 (`apply_round1_corrections.py`,
  `apply_round2_findings.py`), turning up all of the findings above.
  This tier is done for now, though "spot-checked via call-target diff"
  is not the same guarantee as a full instruction-by-instruction read
  -- treat as high-confidence, not certain.
- 52 functions at similarity 0.70-0.95: **all 52 now spot-checked**
  across rounds 3-4 (`apply_round3_findings.py`,
  `apply_round3b_findings.py`, `apply_round4_findings.py`,
  `apply_round4b.py`), turning up all of the findings above. This tier
  is done for now. Of the two exceptions originally left deliberately
  unrenamed, `RunTitleScreen` (0.88) was confirmed and renamed in
  round 15 (a direct diff finally justified the trust); the one
  matched to `WaitForTickAndDrawCreationFrame` (0.77) remains
  unrenamed, confirmed to be a generic tick-wait duplicate, not that
  function (see above). The address matched to `PollForEscapeKeyOnlyAlt`
  (0.90) *was* resolved in round 4 -- see the correction above.
- 77 functions at similarity <0.70: **all 77 now checked**, 34 renamed
  to their real (sometimes non-obvious) identity across rounds 5-10
  (`apply_round5_findings.py`, `apply_round5b_findings.py`,
  `apply_round6_findings.py`, `apply_round6b.py`, `apply_round6c.py`,
  `apply_round7_findings.py`, `apply_round8_findings.py`,
  `apply_round9_findings.py`, `apply_round10_findings.py`). ~44
  confirmed-bad BinDiff matches found in this tier -- a roughly 55-60%
  bad-match rate once fully reviewed, confirming the "below 0.5
  similarity, assume wrong until proven" rule of thumb. This tier is
  now closed out.
- **The segmented-addressing global-rename blocker (rounds 1-3) is
  resolved** (round 10) -- it was a data-entry bug (wrong/misread
  addresses), not a real IDA limitation. `g_soundDriverFarPtr`,
  `g_partySlotAssignment`, and `g_driverStateFlags` are all correctly
  named now. This directly unblocked `HandleRangedOrCombatAction`,
  found via the caller-structure technique once `g_uiScratchFlags4`
  (already resolved in yendor3 as `word_33120`) made the search
  possible; `ParseCommandLineSwitches` was found as a bonus in the
  same pass.
- **`sub_156C9` resolved** (round 11) -- a verbatim structural match to
  yendor2's `CheckAndTickAvailableAilment` (identical loop shape,
  identical `IsItemRangeAvailable`/`g_currentActionId`-gated
  `TickStatusEffects` call, called from the same position right after
  `TickTravelResourceAilments`). Not `RestPartyAndAdvanceClock`, as
  originally guessed -- that turned out to be a different address
  entirely (round 7). Also traced `sub_1B085`'s new ailment mechanic in
  more depth (round 11): a genuinely new "random party member takes a
  worse periodic effect than the rest" mechanic, no yendor2 equivalent
  -- see the dedicated write-up above. Left unrenamed pending full
  decoding of its trap-effect ids.

- **Rounds 12-13**: ruled out (or resolved) all remaining low-priority
  suggested-address leads. `RunCharacterDetailOverlay`'s suggested
  address (0x11778) turned out to be part of a new quest-item-
  conversion chain, `RunClueEntryMenu`'s (0x2566C) a generic
  widely-reused stub, and `TryHandleCatalogSlotClick`'s (0x144D4) a
  new factored-out helper -- see the round 12/13 write-ups above.
  `SwapItemInstanceEverywhere`, `SwapItemInMemberInventory`, and
  `TryHandlePartyInventorySlotClick` are now named; `sub_2566C` and
  `sub_11778` (the `ds:0xA5A6`-table registry) are left unnamed as
  genuinely too uncertain/generic to name with confidence.

- **Round 14**: resolved the character-creation-region cluster
  (0x2BD4A/0x2BBB5/0x2BC75) -- see the dedicated write-up above.
  `PlayCharacterCreationOpeningPicture`, `PlayCharacterCreationOpeningSequenceAlt`,
  and `PollForEscapeKeyOnlyAlt` are now named (the last one corrected
  from an initial `PollForEscapeKeyOnly` naming collision -- see the
  correction note above).
- **Round 15**: found the real `ShowIntroPicture` (round 9's
  confirmed-bad `PlayCreditsWipeAnimation` guess) and, via its caller
  chain, finally confirmed `RunTitleScreen` at the address BinDiff
  suggested all along -- closing out the last deliberately-deferred
  mid-confidence exception from round 4. See the dedicated write-up
  above.

Remaining open leads for future sessions (not blocking, just
unresolved identities): `DrawShadowedTextAlt`/`PlayStudioCreditsIntro`
(its only caller in yendor2 is one specific `start`-tail easter egg --
a hidden 3-flag check -- that hasn't turned up an equivalent in
yendor3, and `RunTitleScreen` doesn't call it either), `sub_1B085`'s
exact trap-effect-id semantics, `sub_11778`'s exact purpose (the
`ds:0xA5A6` 8-slot "limited item" registry), `sub_2566C`'s yendor2
identity (a generic palette-fade-and-mark-dirty stub, 14 call sites),
and the still-unverified `PlayTitleScreenSequence` guess (0x20DC4 --
now a confirmed real call from `RunTitleScreen`, but not independently
diffed).
