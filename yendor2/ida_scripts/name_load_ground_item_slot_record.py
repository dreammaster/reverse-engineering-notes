"""
Names sub_19091, called once from sub_1869D (the trade/inventory
dispatcher).

Caches ax into word_36863 -- the confirmed ground/world-object item
slot record cache (per an existing comment at 0x18066: "Reads a
ground/world-object item slot record (FileEntry_Read, errorCode=0xB)
into word_36863. Called 3 times from PlaceItemOnGround.") -- then
loads that same record type (FileEntry errorCode 0xB) via sub_27E3A
using the caller's original bx as the record selector.
-> LoadGroundItemSlotRecord

Run via:
    .\run_ida_script.ps1 name_load_ground_item_slot_record.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x19091
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "LoadGroundItemSlotRecord", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'LoadGroundItemSlotRecord': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Caches ax into word_36863 (the confirmed ground/world-object "
    "item slot record cache, also used by PlaceItemOnGround), loads "
    "that record type (FileEntry errorCode 0xB) via sub_27E3A. "
    "Called once from sub_1869D.",
    False,
)
