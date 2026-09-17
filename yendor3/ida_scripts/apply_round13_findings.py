"""
Round 13: investigated two more low-confidence-tier BinDiff guesses
that had no clear caller-structure lead in round 9.

- `RunClueEntryMenu`-suggested address (0x2566C, originally
  `ParseCommandLineSwitches` guess at 0.28 sim -- already ruled out
  round 9): read its body directly. It's a tiny 2-instruction stub
  (a fixed `StepPaletteFadeRange` call plus setting a screen-dirty
  flag bit). Its `CODE XREF` list made it look ShowClueBook-specific
  at first glance, but it actually has 14 call sites scattered across
  totally unrelated code regions throughout the binary -- a small,
  generic "trigger this palette fade + mark dirty" utility reused
  everywhere, not a ShowClueBook/RunClueEntryMenu-specific helper.
  Conclusively NOT `RunClueEntryMenu`. Left unnamed -- too generic and
  widely reused to confidently map to one yendor2 name; not chased
  further.

- `TryHandleCatalogSlotClick`-suggested address (0x144D4, 0.09 sim --
  the real `TryHandleCatalogSlotClick` is at 0x128F4, already renamed
  round 1): its body (hit-test region 0x6700, search an 8-entry table
  for the clicked item, then `ComputeBarterPricingPreview` +
  `ShowItemPurchaseConfirmPrompt`) matches the *feature* of a code
  block that's inlined directly in yendor2's `RunPartyInventoryScreen`
  (same three calls in the same order, around
  `RunPartyInventoryScreen+0xB2`), but not a dedicated yendor2 function
  -- this is genuinely new: Chapter 3 factored that inline sequence
  out into its own helper, called from `RunPartyInventoryScreen`.
  Renamed descriptively (moderate confidence -- the exact hit-test/
  8-entry-table wrapper isn't present verbatim in yendor2, only the
  barter-preview-then-confirm tail is a confirmed match).

Run via:
    .\run_ida_script.ps1 apply_round13_findings.py
"""
import idc
import ida_name

ea = 0x144D4
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TryHandlePartyInventorySlotClick", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TryHandlePartyInventorySlotClick': {'ok' if ok else 'FAILED'}")
