"""
Names sub_2687B, called from sub_2621C (the container-interaction
input handler, alongside the already-named SwapHeldItemWithSlot and
FinishPlacingHeldItem): plays the same "swap-family" sound (ax=6) as
SwapHeldItemWithSlot, restores the cursor background, then calls
sub_266D4 directly (no pickup step first, unlike SwapHeldItemWithSlot)
and clears the held-item cursor. Reads as "place the held item into an
already-empty slot" -- the simpler sibling of SwapHeldItemWithSlot,
which additionally has to pick up an occupied slot's item first.
sub_266D4 itself is not named: it's a large, multi-branch function
touching the same 3 sub-block offsets WriteContainerSubBlock writes
(+0x17C/+0x1A2/+0x1C8, adjacent to that function's +0x17E/+0x1A4/+0x1CA
fields) plus a running total at +0x118 (plausibly carried weight), too
many unconfirmed pieces to name confidently yet.
-> PlaceHeldItemIntoEmptySlot

Run via:
    .\run_ida_script.ps1 name_place_into_empty_slot.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2687B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PlaceHeldItemIntoEmptySlot", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PlaceHeldItemIntoEmptySlot': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Places the held item into an already-empty slot via sub_266D4 "
    "(no pickup step, unlike SwapHeldItemWithSlot), then clears the "
    "held-item cursor. Called from sub_2621C.",
    False,
)
