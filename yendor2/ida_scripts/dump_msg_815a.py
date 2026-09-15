"""
Dump the message at DS:0x815A (sub_18FDA's ineligibility rejection,
cx=2 lines), plus a wider window to also see sub_190AF's likely
"not enough material" message nearby if adjacent. Read-only.

Run via:
    .\\run_ida_script.ps1 dump_msg_815a.py -NoExport
"""
import ida_bytes

ea = 0x815A + 0x2D860
ctx = bytes(ida_bytes.get_byte(ea + i) for i in range(200))
print(f"0x815a -> {ea:#x}: {ctx!r}")
