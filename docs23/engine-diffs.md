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
disassembly, just through a raw, not-yet-converted `ds:0xFA00` operand
in yendor3 (attempting `set_name` on it failed: IDA reports the
address as unloaded, meaning the segment:offset needs a proper
"convert to offset" pass before it can be named — not just a plain
linear address the way most other globals in this codebase are).
So `sub_286D8` is a **new "if flag X, poke the sound driver with
sub-function 8" wrapper**, called from at least 3 places — plausibly a
new sound-driver keepalive/reset call added around specific game
events, not a general event/hook system. `ds:0xCF63`'s exact meaning
is still unconfirmed.

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
becomes ineligible mid-action. The underlying table's yendor3 address
(`ds:0xCF81`) is confirmed to be the same `g_partySlotAssignment`
table yendor2 uses (per several already-existing inline comments
describing it exactly that way), but attempting to rename it hit the
same segmented-addressing snag as `g_soundDriverFarPtr` last round
(IDA reports it unloaded) -- needs a proper convert-to-offset pass.

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
gates the pause-menu MUSIC/SOUND FX checkboxes on. Renaming the actual
address failed the same way `g_soundDriverFarPtr` and
`g_partySlotAssignment` did in earlier rounds (IDA reports it
unloaded) -- a growing list of segmented-operand globals that need a
proper convert-to-offset pass, tracked here rather than each rename
attempt being repeated.

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

Still a hypothesis, not confirmed — but three unrelated functions all
pointing at NUORE-related code disappearing or changing shape is
enough to prioritize a dedicated round on this specifically: find
every remaining `CollectNuoreCache`/`AddToBCDCounter`-with-NUORE-offset
call site in yendor3 and see what actually replaced each one.

### Deliberately not renamed: 3 functions too large/changed to trust a quick read

- **`CheckAndPaySpecialItemCost`** (0.65): grew from 45 to 86
  instructions. New calls include `RemoveMultiStatEffect`,
  `RecomputeEquipmentStatBonuses`, and a brand-new BCD helper,
  `ShiftBCD4LeftNibble` (called twice) — not part of the already-named
  BCD family (`bcd4.c`'s reimplementation target list may need to grow
  by one). Reads like a new "pay with an enchanted item, removing its
  stat bonus" payment option, but not confirmed.
- **`DispatchItemAbilityCommand`** (0.60): grew from 69 to 139
  instructions — more than doubled. Several yendor2 branches
  (`ShowVisionAtLocation`, `UseLocationBoundPotion`,
  `CheckQuestItemsCompleted`, `CollectNuoreCache`,
  `CollectMagicOreCache`, `PartyMassHealAndOverheal`,
  `InstantKillActiveMonster`) are entirely absent from yendor3's call
  list, replaced by a much longer sequence built around 5 repeated
  `IsItemRangeAvailable`+`ConsumeItemChargeResource` pairs and several
  `TestGlobalFlag`/`SetGlobalFlag` calls. This is probably the single
  highest-value function left to fully understand, given its
  connection to the NUORE hypothesis above — worth a dedicated round.
- **`CastSpell`** (0.36): a 202-line function became 216 lines with a
  radically different shape — the same 8-call sequence
  (`TickTravelResourceAilments`/`RefreshDungeonMapWindow`/
  `RevealCellsAroundPlayer`/`RestoreFullScreenFromEMS`/
  `RedrawDungeonScreen`/`BuildMinimapTileData`/`DrawMinimap`/
  `DrawMouseCursor`) repeats 5-6 times almost verbatim, suggesting
  several spell-effect branches (teleport home? recall? multiple
  destination options?) each inline what used to be shared
  travel/redraw logic rather than calling a common helper. Modest
  overall size change hides a much bigger shape change — needs a real
  read, not a call-list skim.

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
  is done for now, with two exceptions left deliberately unrenamed:
  the address BinDiff matched to `RunTitleScreen` (0.88 -- still too
  large/noisy to trust without a dedicated read) and the one matched
  to `WaitForTickAndDrawCreationFrame` (0.77 -- confirmed to be a
  generic tick-wait duplicate, not that function, see above). The
  address matched to `PollForEscapeKeyOnlyAlt` (0.90) *was* resolved
  this round -- see the correction above.
- 77 functions at similarity <0.70: 37 checked in round 5, 23 renamed
  (`apply_round5_findings.py`, `apply_round5b_findings.py`). 12 more
  confirmed-bad matches found this round alone (see above), bringing
  the session total to 16-ish confirmed-bad BinDiff labels. 3 large,
  substantially-changed functions deliberately left unrenamed pending
  a dedicated read (`CheckAndPaySpecialItemCost`,
  `DispatchItemAbilityCommand`, `CastSpell` -- see above). ~40 low-
  confidence functions remain unchecked, for future rounds. Treat
  *any* match under roughly 0.5 as unverified until read directly --
  this tier's bad-match rate is now higher than good.
