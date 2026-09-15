"""
Names sub_17B67, called from sub_17032 (the shop-catalog click
handler) and sub_17270 (not traced): hit-tests region table 0x63C8
for a catalog-slot click, returning the region index in ax (0 = no
hit). If a hit, also checks it against an 8-entry exclusion list
(table 0x558A) and returns early (same ax) if it matches one of those
-- a "is this a clickable catalog slot" gate.

-> HitTestCatalogSlot

Run via:
    .\run_ida_script.ps1 name_hit_test_catalog_slot.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x17B67
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HitTestCatalogSlot", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HitTestCatalogSlot': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Hit-tests region table 0x63C8 for a catalog-slot click; if hit, "
    "also checks an 8-entry table (0x558A) for a match. Returns the "
    "region index (0 = no hit) in ax. Called from sub_17032 and "
    "sub_17270.",
    False,
)
