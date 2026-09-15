"""
Read-only: dumps sub_2819F's two fixed label strings.

    .\run_ida_script.ps1 dump_status_command_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860

for label, off in [("0x7934", 0x7934), ("0x793D", 0x793D)]:
    ea = DS_BASE + off
    b = bytearray()
    p = ea
    while True:
        c = ida_bytes.get_byte(p)
        if c == 0:
            break
        b.append(c)
        p += 1
    print(f"{label} linear={ea:#x}: {bytes(b)!r}")
