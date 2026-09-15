"""
Followed the two BCD material counters (0x94B7/0x94BB, already tied to
CastSpell's 0x1C alchemy ability) to their other ~14 call sites and
found a third sibling counter, 0x94B3, plus a shared "deduct one of
several resource types, clamped at zero" dispatcher (sub_18257, called
from sub_180BA -- an icon-bar loop over 4 fixed slots, not yet fully
understood; possibly per-tick upkeep for active effects rather than a
one-time item cost, so sub_18257 itself is NOT renamed this round).

Three callees of sub_18257 ARE clearly understood on their own and
confidently named:

- sub_18205 -> SpendMaterialCounterClamped(ax=BCD counter address,
  bx=pointer to a 4-byte BCD amount): if the counter is greater than
  the amount (CompareBCD4+ja), subtracts normally (SubBCD4). Otherwise
  the counter can't cover the cost -- zeroes it outright (doesn't go
  negative) and calls sub_2704C (not traced; presumably a "this
  resource just hit zero" notification/redraw hook).

- sub_1822A -> DeductHPClamped(ax=amount, bx=party-member record):
  `[bx+0x52] -= ax` (the same HP-current field CastSpell/RestCharacter
  already confirmed), clamped at 0. If it reaches 0, sets status bit
  0x40 in `[bx+0x1C]` and calls two further functions (sub_18095,
  sub_1AB26 -- not traced, plausibly death/incapacitation handling).

- sub_18248 -> DeductMPClamped(ax=amount, bx=party-member record):
  `[bx+0x54] -= ax` (the MP-current field), clamped at 0, no further
  side effect.

sub_18257 dispatches to one or more of these (plus the BCD spend, on
one of the three counters 0x94B3/0x94B7/0x94BB, selected by flag bits
in a caller-supplied record) based on flag bits at `[di+8]` -- reads as
a general "pay this cost, whatever form it takes" helper, but whether
its caller (sub_180BA) represents one-time item use or recurring
effect upkeep isn't settled, so it and 0x94B3's identity are left
unnamed/open.

Run via:
    .\run_ida_script.ps1 name_resource_deduction.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x18205: "SpendMaterialCounterClamped",
    0x1822A: "DeductHPClamped",
    0x18248: "DeductMPClamped",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x18205,
    "SpendMaterialCounterClamped(ax=BCD counter addr, bx=ptr to 4-byte "
    "BCD amount): if counter > amount, SubBCD4 normally; otherwise the "
    "counter can't cover it -- zeroed outright (never negative), then "
    "sub_2704C is called (presumably a 'resource depleted' hook).",
    False,
)
ida_bytes.set_cmt(
    0x1822A,
    "DeductHPClamped(ax=amount, bx=party-member record): "
    "[bx+0x52] -= ax (HP-current), clamped at 0. At 0, sets status bit "
    "0x40 in [bx+0x1C] and calls sub_18095+sub_1AB26 (not traced, "
    "plausibly death/incapacitation handling).",
    False,
)
ida_bytes.set_cmt(
    0x18248,
    "DeductMPClamped(ax=amount, bx=party-member record): "
    "[bx+0x54] -= ax (MP-current), clamped at 0.",
    False,
)
