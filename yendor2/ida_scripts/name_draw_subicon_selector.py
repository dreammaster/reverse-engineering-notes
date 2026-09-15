"""
Names sub_14994, called from ShowArmorDetailRow and
ShowWeaponDetailRow: draws a row of clickable sub-icon selector
indicators, positioned by the same region table (0x6976) used for
sub-icon click hit-testing in RunClueBookItemCategory/
RunClueBookMapCategory, with each icon toggled between two picture
variants (0x155/0x156) based on the matching bit in word_328FE (the
same "selected sub-icon" bitmask RunClueBookItemCategory's click
handler sets).

-> DrawSubIconSelectorRow

Run via:
    .\run_ida_script.ps1 name_draw_subicon_selector.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x14994
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawSubIconSelectorRow", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawSubIconSelectorRow': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws the row of clickable sub-icon selector indicators (region "
    "table 0x6976, same as RunClueBookItemCategory's click "
    "hit-testing), toggling each between two picture variants based "
    "on word_328FE bits. Called from ShowArmorDetailRow/"
    "ShowWeaponDetailRow.",
    False,
)
