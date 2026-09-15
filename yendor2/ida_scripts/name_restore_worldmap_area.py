"""
Names sub_2438B, called from sub_23C18 (ShowWorldMap's interaction
handler): maps EMS page 0x55D8 (same page as the other portrait/
dialog EMS restores) and blits a large region (offset 0x1F40, 175
rows x 54 words, stride 0xD4) from the EMS page frame into the video
buffer -- restoring the world map display area without a full redraw.

-> RestoreWorldMapAreaFromEMS

Run via:
    .\run_ida_script.ps1 name_restore_worldmap_area.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2438B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RestoreWorldMapAreaFromEMS", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RestoreWorldMapAreaFromEMS': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Blits a large cached region (offset 0x1F40, 175x54 words) from "
    "EMS page 0x55D8 into the video buffer -- restores the world map "
    "display area. Called from sub_23C18 (ShowWorldMap's interaction "
    "handler).",
    False,
)
