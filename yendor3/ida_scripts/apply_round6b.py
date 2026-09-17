"""
Round 6 continued: full read of CheckAndPaySpecialItemCost (0.65
similarity, deferred in round 5). Confirms it IS the same function,
substantially enhanced. See docs23/engine-diffs.md.

Corrects round 5's docstring claim that ShiftBCD4LeftNibble is a
"brand-new BCD helper" -- it already exists in BOTH games (yendor2:
called from MulBCD4ByWord; yendor3: called from PromptBuyOreQuantity
too), just wasn't part of the original bcd4.c reimplementation scoping.

Confirmed real additions:
- A 4th special-cost type (tag 0x270F, alongside the existing gold/
  NUORE/magic-ore tags 1/2/3): converts the cost to BCD then applies
  ShiftBCD4LeftNibble twice (a x100 scale) before comparing against a
  shared price table -- a new cost type using a different unit scale
  from the other three.
- The generic "pay with a specific inventory item" branch now checks
  whether the consumed item occupied one of the 6 confirmed multi-
  stat-effect equipment slots (offsets 0x13A/0x13E/0x142/0x146/0x14A/
  0x14E, stride 4 -- the exact same slots RefreshMultiStatEffects
  walks) and, if so, calls RemoveMultiStatEffect +
  RecomputeEquipmentStatBonuses. Reads as a bug fix: previously,
  spending an equipped magic item as a special payment might not
  correctly remove the stat bonus it was granting.

Run via:
    .\run_ida_script.ps1 apply_round6b.py
"""
import idc
import ida_name

ea = 0x1A09D
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CheckAndPaySpecialItemCost", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CheckAndPaySpecialItemCost': {'ok' if ok else 'FAILED'}")
