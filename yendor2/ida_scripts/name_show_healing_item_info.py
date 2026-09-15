"""
Names sub_137C3, RunClueBookItemDetailWithAbilityInfo's overlay for
items whose id falls in RestCharacter's dispatch range (0x36-0x46).
Shows "HEALTH-" or "MAGIC-" (msg 0x8B57/0x8B5F, selected by the held
item's [+2] bit 0x8000 -- matching UseHealingItem's own HP/MP flag
convention) plus a percentage value ([+4]) and "PERCENT" (msg 0x8B66)
-- confirmed via message dump. A healing/restore item's clue-book
description, e.g. "HEALTH- 25 PERCENT".

-> ShowHealingItemPercentInfo

Run via:
    .\run_ida_script.ps1 name_show_healing_item_info.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x137C3
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowHealingItemPercentInfo", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowHealingItemPercentInfo': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Shows a healing/restore item's clue-book description: 'HEALTH-' "
    "or 'MAGIC-' (selected by [+2] bit 0x8000) plus a percentage "
    "value and 'PERCENT'. Called from "
    "RunClueBookItemDetailWithAbilityInfo for items in RestCharacter's "
    "dispatch range.",
    False,
)
