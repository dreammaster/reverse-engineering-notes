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
`RenderDungeonViewRow`, and `HandleSpecialCellEntry` (dungeon
rendering) plus `DrawFloorTypeLegendRow` and `DrawCellIconPair` (map
editor) — no yendor2 equivalent. Both take a raw tile id in `ax`,
split it into a group (`id/100`) and remainder, index into a lookup
table (stride `0xC` for `sub_1BC98`, `0xA` for `sub_1BCDB` — matching
the already-confirmed 12-byte wall-table/10-byte floor-table entry
sizes), bounds-check against the table's own count field, and either
compute a picture id from the matched entry or fall back to a default
entry while setting `errorCode=1` on out-of-range input. Hypothesis
(not confirmed): Chapter 3 added a validated/clamped tile-id lookup
layer, plausibly because it introduced more wall/floor tile types than
fit Chapter 2's direct-index approach safely.

### Unidentified new call in `MarkIneligiblePartyMembers` (yendor3 `sub_2DCE6`)

Reads a level-like field (`[si+0xE]`), folds it into a 0-9 range
(subtracting 10 twice while >9), then maps values 4-9 to a 6-bit mask
(`0x20`/`0x10`/`8`/`4`/`2`/`1`) and 0-3 to `0`. Called between
`TestRecordFlag_CA` and `DrawPartyMemberStatusPanel`. Purpose not
traced — plausibly a new level-gated eligibility/highlight indicator,
but the consuming code (what reads this bitmask) hasn't been found.

## Review status

- 68 functions bulk-imported at BinDiff similarity >=0.95
  (`yendor3/ida_scripts/apply_bindiff_high_confidence.py`); roughly 40
  of those spot-checked this round (`apply_round1_corrections.py`),
  turning up all of the above. The remaining ~28 are still unverified
  beyond the similarity score.
- 52 functions at similarity 0.70-0.95: not yet reviewed.
- 77 functions at similarity <0.70: not yet reviewed. Many of the
  lowest scores (well under 0.3) may not be genuine matches at all —
  treat the suggested yendor2 name as a weak hint, not a starting
  assumption, when reviewing these.
