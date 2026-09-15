"""
Traced ShowCharacterInventory's item-selection path (sub_26415, a
generic item-interaction handler) into sub_26954 -- finally locates
the party record's "8 item slots", the other TODO flagged since early
in the session (alongside the skill array, found last round).

sub_26954(ax=slot index 1-9, si/di=word_328D4): picks an inventory
GROUP base -- the default is +0x118, but if any of 3 "alternate bag"
marker fields are populated ([+0x17C]/[+0x1A2]/[+0x1C8], each nonzero
selecting +0x180/+0x1A6/+0x1CC respectively and setting a
corresponding flag in [+0x15C]), that group is used instead. Within
the chosen group, slot `ax` (1-9) maps to a 4-byte-stride entry
starting at group-base+2 (ax=1 -> +2, ax=2 -> +6, ax=3 -> +0xA, ...).
So each party member has up to 4 separate 8(-ish)-slot inventories
(one main, three alternates/bags), not fields directly on the base
record -- explaining why they weren't found by scanning
ShowCharacterInventory's own body (which reads generic `_valN` catalog
ids for its drawn labels, not per-character storage directly).

Individual slot content encoding (item id vs. quantity, 4 bytes/slot)
not decoded this round -- named on the confirmed addressing mechanism.

-> GetInventorySlotPtr

Run via:
    .\run_ida_script.ps1 name_get_inventory_slot_ptr.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x26954
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "GetInventorySlotPtr", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'GetInventorySlotPtr': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "GetInventorySlotPtr(ax=slot index 1-9): di = word_328D4 + group "
    "base + 2 + (ax-1)*4. Group base is +0x118 by default, or "
    "+0x180/+0x1A6/+0x1CC if the matching alternate-bag marker "
    "([+0x17C]/[+0x1A2]/[+0x1C8]) is nonzero (also sets a flag in "
    "[+0x15C]) -- up to 4 separate inventories per character (1 main "
    "+ 3 alternates/bags). Slot content encoding (4 bytes each) not "
    "decoded.",
    False,
)
