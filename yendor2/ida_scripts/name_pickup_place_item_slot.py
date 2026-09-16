"""
Names sub_26864, sub_26B4F, and sub_266D4 -- the item pickup/place
pair underlying SwapHeldItemWithSlot and PlaceHeldItemIntoEmptySlot
(both named earlier this session), now confirmed as exact mirror
images of each other:

sub_26B4F (0x26B4F, called from SwapHeldItemWithSlot and sub_26864):
removes the item at slot di into the held-item globals (word_31948/
word_3194C/word_3194A), loads its catalog record, and -- based on
[+0x15C] condition-icon flag bits selecting one of 3 equipment-section
running totals (+0x180/+0x1A6/+0x1CC, the same 3 offsets
WriteContainerSubBlock's fields sit next to) or the general total
(+0x118) -- SUBTRACTS the item's value (weight? quantity? not fully
confirmed) from the relevant total(s), skipping a section whose "type"
field (+0x17C/+0x1A2/+0x1C8) equals 0x11. -> PickUpItemFromSlot

sub_266D4 (0x266D4, called from SwapHeldItemWithSlot and
PlaceHeldItemIntoEmptySlot): the exact mirror -- places the held item
into slot di and ADDS to the same totals. -> PlaceItemInSlot

sub_26864 (0x26864, called from sub_2621C): the simple "pick up only"
action -- restores the cursor, calls PickUpItemFromSlot (no placement
step), updates the cursor and portrait. The pickup counterpart to
PlaceHeldItemIntoEmptySlot. -> PickUpHeldItemFromSlot

Run via:
    .\run_ida_script.ps1 name_pickup_place_item_slot.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x26B4F, "PickUpItemFromSlot"),
    (0x266D4, "PlaceItemInSlot"),
    (0x26864, "PickUpHeldItemFromSlot"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x26B4F,
    "Removes the item at slot di into the held-item globals, loads its "
    "catalog record, and subtracts its value from one of 3 equipment- "
    "section running totals (+0x180/+0x1A6/+0x1CC, selected by [+0x15C] "
    "flag bits) or the general total (+0x118) -- the exact mirror of "
    "PlaceItemInSlot. Called from SwapHeldItemWithSlot and "
    "PickUpHeldItemFromSlot.",
    False,
)
ida_bytes.set_cmt(
    0x266D4,
    "Places the held item into slot di and adds its value to one of 3 "
    "equipment-section running totals (+0x180/+0x1A6/+0x1CC, selected "
    "by [+0x15C] flag bits) or the general total (+0x118) -- the exact "
    "mirror of PickUpItemFromSlot. Called from SwapHeldItemWithSlot and "
    "PlaceHeldItemIntoEmptySlot.",
    False,
)
ida_bytes.set_cmt(
    0x26864,
    "Restores the cursor, calls PickUpItemFromSlot (no placement step), "
    "updates the cursor and portrait. Called from sub_2621C -- the "
    "pickup counterpart to PlaceHeldItemIntoEmptySlot.",
    False,
)
