"""
Read-only: dumps sub_2C010's fail/partial/success message strings.

    .\run_ida_script.ps1 dump_skill_check_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860

TARGETS = {
    "0x8304 (cx=6, roll < low threshold -- FAIL)": (0x8304, 6),
    "0x82D8 (cx=4, low<=roll<=high -- PARTIAL)": (0x82D8, 4),
    "0x8349 (cx=2, roll > high threshold -- SUCCESS)": (0x8349, 2),
}

for label, (off, cx) in TARGETS.items():
    ea = DS_BASE + off
    print(f"--- {label}  linear={ea:#x} ---")
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
