"""
CORRECTION: sub_2738B (just named HandleEquipmentIconClick) was
mis-framed. The table it reads (0x9519, 6 entries, stride 4) is NOT
a generic "6-entry item table" -- it's already documented elsewhere in
file-formats.md as the ailment-tracking table TickWorldAilments walks
([+0]=ailment code matching TickStatusEffects' 9/0xF/0xC, [+2]=
remaining duration). The click handler is therefore about clicking an
*active ailment* icon, not a generic equipment slot.

Renaming to reflect this: -> TryCureAilmentFromIconClick

Run via:
    .\run_ida_script.ps1 fix_equipment_icon_click_name.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2738B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TryCureAilmentFromIconClick", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TryCureAilmentFromIconClick': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "CORRECTED: click handler for the 6-slot active-ailment icon bar "
    "(word_36C7F bit 0x1000), not a generic equipment bar. Hit-tests "
    "region table 0x636C, maps to the 6-entry ailment table 0x9519 "
    "(same table TickWorldAilments walks -- [+0]=ailment code matching "
    "TickStatusEffects' 9/0xF/0xC, [+2]=duration; also read by "
    "IsItemRangeAvailable). Loads the ailment code as an item-catalog "
    "record (ailment codes and item ids appear to share a numbering "
    "space elsewhere in this engine too), validates via sub_2D5E0 if "
    "needed, then stages it into the 'carrying' state (errorCode=2) -- "
    "plausibly clicking an active ailment icon to apply a held cure "
    "item to it. Called from `start` and HandleDungeonInput.",
    False,
)
