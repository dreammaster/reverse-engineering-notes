"""
Read-only: dumps the message strings sub_17795 selects among, to
understand exactly what "requirement not met" text it shows.

    .\run_ida_script.ps1 dump_ability_gate_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860

TARGETS = {
    "0x7BE8 (cx=2, word_32DCE bit 0x20 branch)": (0x7BE8, 2),
    "0x7BB7 (cx=1, else-branch A)": (0x7BB7, 1),
    "0x7BCE (cx=1, else-branch B)": (0x7BCE, 1),
    "0x7BD5 (cx=2)": (0x7BD5, 2),
    "0x7BF9 (cx=2, header when 0xFE00 set)": (0x7BF9, 2),
    "0x7C0E (bit 0x8000)": (0x7C0E, 1),
    "0x7C18 (bit 0x4000)": (0x7C18, 1),
    "0x7C23 (bit 0x2000)": (0x7C23, 1),
    "0x7C2E (bit 0x1000)": (0x7C2E, 1),
    "0x7C37 (bit 0x800)": (0x7C37, 1),
    "0x7C41 (bit 0x400)": (0x7C41, 1),
    "0x7C4C (bit 0x200)": (0x7C4C, 1),
    "0x7C02 (fallback)": (0x7C02, 1),
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
