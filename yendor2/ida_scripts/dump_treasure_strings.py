"""
Read-only: dumps message strings for sub_2B029/sub_2AFB8/sub_2B09A
(item-icon-dispatch codes 0x246/0x247/0x248) and the shared
"unavailable" message at 0x8E1A.

    .\run_ida_script.ps1 dump_treasure_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860

TARGETS = {
    "0x8E1A (cx=2, shared 'unavailable')": (0x8E1A, 2),
    "0x8E30 (cx=2, item 0x246)": (0x8E30, 2),
}

for label, (off, cx) in TARGETS.items():
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
