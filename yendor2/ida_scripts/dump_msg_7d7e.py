"""
Dump message 0x7D7E, drawn by sub_222BD (called from ShowLocalAreaMap
and ToggleMapViewMode), cx=3 lines.

Run via:
    .\\run_ida_script.ps1 dump_msg_7d7e.py -NoExport
"""
import ida_bytes

ea = 0x7D7E + 0x2D860
ctx = bytes(ida_bytes.get_byte(ea + i) for i in range(80))
print(f"0x7d7e -> {ea:#x}: {ctx!r}")
