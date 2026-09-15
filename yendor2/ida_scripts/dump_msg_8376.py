"""
Dump the message at DS:0x8376 (sub_190AF's "insufficient" rejection,
cx=3 lines -- shared by sub_18FDA and sub_19140's failure paths).

Run via:
    .\\run_ida_script.ps1 dump_msg_8376.py -NoExport
"""
import ida_bytes

ea = 0x8376 + 0x2D860
ctx = bytes(ida_bytes.get_byte(ea + i) for i in range(120))
print(f"0x8376 -> {ea:#x}: {ctx!r}")
