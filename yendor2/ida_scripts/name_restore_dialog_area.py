"""
Names sub_28246, called from RestPartyAndAdvanceClock and
RunGameDialog: maps EMS page 0x55D8 (same page RestorePortraitPanelFromEMS
uses, different region/offset) and blits a 60-row x 37-word region at
offset 0x78F0 from the EMS page frame into the video buffer --
restoring a cached screen area (the game-dialog/status region) without
a full redraw.

-> RestoreDialogAreaFromEMS

Run via:
    .\run_ida_script.ps1 name_restore_dialog_area.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x28246
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RestoreDialogAreaFromEMS", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RestoreDialogAreaFromEMS': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Blits a cached 60x37-word screen region (offset 0x78F0) from EMS "
    "page 0x55D8 into the video buffer -- restores the game-dialog/"
    "status area without a full redraw. Called from "
    "RestPartyAndAdvanceClock and RunGameDialog.",
    False,
)
