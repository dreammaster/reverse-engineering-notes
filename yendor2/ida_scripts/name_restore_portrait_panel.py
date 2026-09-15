"""
Names sub_1922C, called from RefreshPartyPortraits and sub_18504: if
none of the portrait-dirty bits (word_328C6 & 0x7800 -- the same bits
RefreshPartyPortraits sets per occupied slot) are set, blits a cached
background region from EMS-paged memory (MapUnmapPages, then a
136-row x 224-byte copy from the EMS page frame to the video buffer at
the same offsets) back into the screen -- restoring the portrait panel
area when nothing needs redrawing from scratch.

-> RestorePortraitPanelFromEMS

Run via:
    .\run_ida_script.ps1 name_restore_portrait_panel.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1922C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RestorePortraitPanelFromEMS", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RestorePortraitPanelFromEMS': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "If no portrait-dirty bits are set (word_328C6 & 0x7800), blits a "
    "cached background region from EMS-paged memory back into the "
    "video buffer (136 rows x 224 bytes) -- restores the portrait "
    "panel area without a full redraw. Called from "
    "RefreshPartyPortraits and sub_18504.",
    False,
)
