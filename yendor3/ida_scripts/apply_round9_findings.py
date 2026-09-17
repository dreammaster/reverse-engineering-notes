"""
Round 9: closed out the last ~14 unchecked functions in the <0.70
low-confidence tier. 12 of 14 confirmed bad (wildly different call
lists/instruction counts from their BinDiff-suggested yendor2 name,
consistent with this tier's established >60% bad-match rate). Two
real matches found via caller-context confirmation rather than raw
call-list diffing (the call-list diff alone was misleading for both,
since Chapter 3 restructured their internals):

- `RunClueEntryMenu`: the address BinDiff called `ParseCommandLineSwitches`
  (0x16654, 0.28 similarity) is actually the real `RunClueEntryMenu`.
  Its call list is an exact 17-call sequence match to yendor2's
  `RunClueEntryMenu` (ShowClueCategoryEntries/PollKeyboardInput/
  HandleClueCategorySelection/.../ShowClueBookRegistrationNag), and
  it's called from `ShowClueBook`, exactly like yendor2's version (13
  call sites). The address BinDiff itself suggested for
  `RunClueEntryMenu` (0x2566C, 0.12 similarity) is unrelated (near-
  empty, one `StepPaletteFadeRange` call) -- still unidentified.

- `RunCharacterCreationSelectionStep`: the address BinDiff suggested
  (0x2BF44, 0.27 similarity) IS correct, confirmed by caller position
  -- it's called from `RunCharacterCreation` in the same slot yendor2's
  version occupies. Its raw call list looks totally different (623
  yendor2 instructions collapse to 140 in yendor3) because Chapter 3
  factored the screen's many repeated
  StepPaletteFadeRange/TriggerPaletteRange16FadeUp/Down/
  PollForEscapeKeyOnlyAlt animation sequences into new shared helpers
  (a `RunPaletteFadeSequence`-style wrapper, `sub_2C11E`, `sub_2C376`).
  This is a genuine internal rewrite, not a bad match -- a further
  example of the "consolidate repeated sequences into shared helpers"
  pattern already seen elsewhere in the Chapter 3 engine.

Confirmed bad, left unrenamed (call lists bear no resemblance to their
BinDiff-suggested yendor2 counterpart): `TriggerFullPaletteFadeIn`
(0x18691), the address separately mislabeled `RunCharacterDetailOverlay`
(0x11778 -- the real one is at 0x237BA, already renamed round 7; this
address is still unidentified, small, uses LoadItemCatalogRecord/file
I/O, matching round 7's prediction exactly), `ComposeCharacterPortrait`
(0x2BD4A), `RunMapEditorScreen` (0x2B7AE), the address separately
mislabeled `TryHandleCatalogSlotClick` (0x144D4 -- the real one is at
0x128F4, already renamed round 1), `PlayCreditsWipeAnimation` (0x14F40),
the address separately mislabeled `ErrorCheck` (0x11E56 -- the real one
is at 0x28C1A, already renamed round 1), `DrawCharacterCreationAnimationFrame`
(0x2BBB5), `ComputeAlchemyRefinementYield` (0x1C13E -- y3 side is a huge
combat/projectile function, nothing like BCD refinement math),
`DebugToggleViewportCellHidden` (0x2BC75), `DebugSetOverlayTileByNumber`
(0x2147B -- y3 side handles clue-location string formatting, possibly a
new "show location" feature, worth a dedicated future look).

**Cluster observation**: several of the still-unidentified addresses in
this batch (0x2BD4A, 0x2BBB5, 0x2BC75, plus the confirmed
`RunCharacterCreationSelectionStep` at 0x2BF44) all sit in the same
0x2BB7C-0x2BF44 code region and share calls to a small set of new
helpers (`~DrawCharacterCreationAnimationFrame`, `StepPaletteFadeRange`,
`DrawPicture`, `wait`). This whole region looks like Chapter 3's
restructured character-creation screen code -- likely worth a dedicated
future pass rather than one-off diffing, now that the low-confidence
tier's individual review is complete.

This closes out the full 77-function low-confidence tier: all three
BinDiff match tiers (68 high, 52 mid, 77 low = 197 total) have now been
spot-checked at least once.

Run via:
    .\run_ida_script.ps1 apply_round9_findings.py
"""
import idc
import ida_name

renames = [
    (0x16654, "RunClueEntryMenu"),
    (0x2BF44, "RunCharacterCreationSelectionStep"),
]

for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
