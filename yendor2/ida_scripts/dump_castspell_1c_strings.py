"""
Read-only: dumps the raw null-terminated message strings CastSpell's
0x1C branch references (sub_23B76(bx, cx) reads cx consecutive
null-terminated strings starting at ds:bx). ds is the main data
segment (word_2D86) throughout this region.

    .\run_ida_script.ps1 dump_castspell_1c_strings.py -NoExport
"""
import idc
import ida_bytes

DS_BASE = 0x2D860  # word_2D86 * 0x10

TARGETS = {
    "0x7D7E (cx=3, invalid target)": (0x7D7E, 3),
    "0x802B (cx=4, resource-check fail)": (0x802B, 4),
    "0x7D2C (cx=2, branch 'answer==5')": (0x7D2C, 2),
    "0x8D8C (cx=2, branch 'answer==7')": (0x8D8C, 2),
}

for label, (off, cx) in TARGETS.items():
    ea = DS_BASE + off
    print(f"--- {label}  linear={ea:#x} ---")
    a = ea
    for i in range(cx):
        s = ida_bytes.get_strlit_contents(a, -1, 0)
        if s is None:
            # fall back to manual scan
            b = bytearray()
            p = a
            while True:
                c = ida_bytes.get_byte(p)
                if c == 0:
                    break
                b.append(c)
                p += 1
            s = bytes(b)
            a = p + 1
        else:
            a += len(s) + 1
        print(f"  [{i}] {s!r}")

ea = DS_BASE + 0xAFAA
print(f"--- 0xAFAA (msg, unconditional) linear={ea:#x} ---")
b = bytearray()
p = ea
while True:
    c = ida_bytes.get_byte(p)
    if c == 0:
        break
    b.append(c)
    p += 1
print(f"  {bytes(b)!r}")
