"""
Names sub_26778, called several times from DrawPartyMemberPortrait
(beyond DrawEquippedItemIcons' calls): given an item id (ax) and
offset (bx,cx, added to word_328BC/word_328C0), loads the item's
catalog record and draws its icon ([+4], or [+4]+1 when [si+0x10]==1
-- a two-variant toggle, same pattern DrawEquippedItemIcons uses).
Bails if ax==0 (empty slot).

-> DrawPortraitAccessoryIcon

Run via:
    .\run_ida_script.ps1 name_draw_portrait_accessory_icon.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x26778
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawPortraitAccessoryIcon", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawPortraitAccessoryIcon': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws one item icon (ax=item id) at an offset position, with a "
    "two-variant toggle ([si+0x10]==1). Bails if ax==0. Called from "
    "DrawPartyMemberPortrait.",
    False,
)
