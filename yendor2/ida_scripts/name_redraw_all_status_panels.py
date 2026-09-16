"""
Names sub_2ADD0, called from ApplyMultiStatEffect and RestCharacter
(among others): iterates all 4 g_partySlotAssignment slots, calling
DrawPartyMemberStatusPanel for each occupied one -- a batch "redraw
every party member's status panel" helper. -> RedrawAllPartyStatusPanels

Run via:
    .\run_ida_script.ps1 name_redraw_all_status_panels.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2ADD0
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RedrawAllPartyStatusPanels", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RedrawAllPartyStatusPanels': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Calls DrawPartyMemberStatusPanel for each occupied "
    "g_partySlotAssignment slot -- redraws every party member's status "
    "panel. Called from ApplyMultiStatEffect, RestCharacter, and others.",
    False,
)
