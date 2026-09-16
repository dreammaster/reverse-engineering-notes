"""
Names sub_13707, called once from ShowClueBookItemDetail.

Draws "FITS IN-" (confirmed via string dump at 0x8AA2), then, based
on the loaded item record's [si+0xE]/[si+0xC] flag bits, either:
"ANY PANEL" (no 0xE000 bits set, [si+0xC] bit 0x2000 clear -- a
general item usable in any inventory panel), "CHARACTER PANEL"
([si+0xC] bit 0x2000 set, 0xE000 clear -- an equipment-only item), or
a concatenation of "BACKPACK "/"BOX "/"BAG" for whichever of
[si+0xE]'s bits 0x8000/0x4000/0x2000 are set -- the storage-container
types the item can be placed into. Identifies [si+0xE]'s top 3 bits
as container-compatibility flags (backpack/box/bag) and [si+0xC] bit
0x2000 as an "equipment/character-panel only" flag.
-> DrawItemContainerCompatibilityRow

Run via:
    .\run_ida_script.ps1 name_draw_item_container_compatibility_row.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x13707
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawItemContainerCompatibilityRow", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawItemContainerCompatibilityRow': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws 'FITS IN-' then either 'ANY PANEL' / 'CHARACTER PANEL' "
    "(based on [si+0xC] bit 0x2000) or a concatenation of "
    "'BACKPACK '/'BOX '/'BAG' for [si+0xE] bits 0x8000/0x4000/0x2000 "
    "-- identifies these as item container-compatibility flags. "
    "Called once from ShowClueBookItemDetail.",
    False,
)
