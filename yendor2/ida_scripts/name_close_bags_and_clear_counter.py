"""
Names two more small helpers found this round.

sub_266A9, called once from RestorePortraitAreaAtPosition: clears
high bits (above 0xFFF) of the current party member's [+0x15C] status
word, then calls SaveAndCloseContainer for each of the 3 documented
"alternate bag" marker offsets (0x17C, 0x1A2, 0x1C8) -- closing every
open alternate bag for that member, e.g. when leaving the inventory
screen. -> CloseAllAlternateBags

sub_26928, called from PlaceItemInSlot and PickUpItemFromSlot: zeroes
one of [si+0xBE]/[si+0xC0]/[si+0xC2] depending on the current command
code (word_2E40A: 0xA/0xC/0xD respectively) -- the same 3 fields
sub_274B4 (the still-untraced item-use dispatcher) clears for item
types 0x13A/0x142/0x146 via its own ShowResourceDepletedOverlay path.
Plausibly resets a per-item-type "resource depleted" charge counter
whenever the item is picked up or placed via a slot with a matching
command code. -> ClearDepletedResourceCounterForCommand

Run via:
    .\run_ida_script.ps1 name_close_bags_and_clear_counter.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x266A9: "CloseAllAlternateBags",
    0x26928: "ClearDepletedResourceCounterForCommand",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x266A9,
    "Clears [si+0x15C] high bits (above 0xFFF) then calls "
    "SaveAndCloseContainer for each of the 3 alternate-bag marker "
    "offsets (0x17C/0x1A2/0x1C8) -- closes every open bag for the "
    "current party member. Called from "
    "RestorePortraitAreaAtPosition.",
    False,
)
ida_bytes.set_cmt(
    0x26928,
    "Zeroes [si+0xBE]/[si+0xC0]/[si+0xC2] depending on word_2E40A "
    "(0xA/0xC/0xD) -- the same 3 fields sub_274B4 clears for item "
    "types 0x13A/0x142/0x146. Called from PlaceItemInSlot and "
    "PickUpItemFromSlot.",
    False,
)
