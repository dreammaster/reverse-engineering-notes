"""
Names sub_18504, called from `start` and HandleDungeonInput: the
mouse-click counterpart to sub_25B34 (the keyboard 1-4 party-panel
selector). Hit-tests region table 0x61C2 for a click on one of the 4
portrait zones (values 1/0xB/0x15/0x1F), maps it to a
g_partySlotAssignment slot and its word_328C6 highlight bit (matching
RefreshPartyPortraits' own bit assignment), and if that slot is
occupied, sets the bit, redraws the portrait via sub_19133, shows a
message, and redraws the cursor.

-> HandlePortraitClick

Run via:
    .\run_ida_script.ps1 name_handle_portrait_click.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x18504
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandlePortraitClick", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandlePortraitClick': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Mouse-click counterpart to sub_25B34 (keyboard 1-4 selection): "
    "hit-tests region table 0x61C2 for one of the 4 portrait zones, "
    "sets the matching word_328C6 highlight bit (same bits "
    "RefreshPartyPortraits uses) if that slot is occupied, redraws via "
    "sub_19133. Called from `start` and HandleDungeonInput.",
    False,
)
