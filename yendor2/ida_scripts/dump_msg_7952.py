"""
Dump message 0x7952 -- the label sub_1CC2E draws before showing
g_partyGold via FormatAndDrawBCD4. Read-only.

Run via:
    .\\run_ida_script.ps1 dump_msg_7952.py -NoExport
"""
import ida_bytes

ea = 0x7952 + 0x2D860
ctx = bytes(ida_bytes.get_byte(ea + i) for i in range(60))
print(f"0x7952 -> {ea:#x}: {ctx!r}")
