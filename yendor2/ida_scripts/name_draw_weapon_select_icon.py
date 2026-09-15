"""
Names sub_2BAD5, called 4x from HandleRangedOrCombatAction (once per
weapon slot, word_328D8/DA/DC/DE, at x=0/0x36/0x69/0x9D): draws one
weapon-select icon. Bails if the slot is empty (bx=0). Otherwise draws
the item's icon (bx) at the slot position, unless word_328C8 bit 8 is
set, in which case an item id within a highlighted range
(word_3292A..word_32928) uses a different icon (word_329CE) instead --
a "currently selected weapon" highlight.

-> DrawWeaponSelectIcon

Run via:
    .\run_ida_script.ps1 name_draw_weapon_select_icon.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2BAD5
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawWeaponSelectIcon", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawWeaponSelectIcon': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws one weapon-select slot icon (bx=item id), bailing if empty. "
    "When word_328C8 bit 8 is set, uses a highlighted icon variant for "
    "item ids in range word_3292A..word_32928 (the selected weapon). "
    "Called 4x from HandleRangedOrCombatAction.",
    False,
)
