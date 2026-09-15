"""
Names sub_267A7, called 3x from DrawPartyMemberPortrait (once for the
+0x13A equipment slot when a status flag is set, cx=1; otherwise for
+0x13E/+0x142, cx=3/2). For each nonzero item id in the si array,
loads its catalog record and draws its icon at a position from the di
array offset by word_328BC/word_328C0 -- using the item's [+4] icon
field instead of [+8] when word_328C6 bit 0x8000 and the item's own
[+0xC] bit 0x400 are both set (an "active/in-use" icon variant).

-> DrawEquippedItemIcons

Run via:
    .\run_ida_script.ps1 name_draw_equipped_icons.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x267A7
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawEquippedItemIcons", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawEquippedItemIcons': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws up to cx equipped-item icons next to a portrait: for each "
    "nonzero item id, loads its catalog record and draws its icon at "
    "a position offset by word_328BC/word_328C0, using an 'active' "
    "icon variant ([+4] vs [+8]) when word_328C6 bit 0x8000 and the "
    "item's [+0xC] bit 0x400 are both set. Called from "
    "DrawPartyMemberPortrait.",
    False,
)
