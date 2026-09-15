"""
Read-only: dumps sub_2AF2E's two location-gated message strings.

    .\run_ida_script.ps1 dump_location_event_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860

for label, off, cx in [("0x8DBE (at location)", 0x8DBE, 3), ("0x8DA3 (elsewhere)", 0x8DA3, 3)]:
    ea = DS_BASE + off
    print(f"--- {label} linear={ea:#x} ---")
    a = ea
    for i in range(cx):
        b = bytearray()
        p = a
        while True:
            c = ida_bytes.get_byte(p)
            if c == 0:
                break
            b.append(c)
            p += 1
        a = p + 1
        print(f"  [{i}] {bytes(b)!r}")
