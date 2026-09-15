"""
Dump label 0x8AAB and the 27-entry table starting at 0x7DC7 for
sub_148EA (the other ShowArmorDetailRow callee).

Run via:
    .\\run_ida_script.ps1 dump_armor_bonus2_msgs.py -NoExport
"""
import ida_bytes

def read_str(ea, maxlen=30):
    out = bytearray()
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(b)
    return bytes(out)

ea = 0x8AAB + 0x2D860
print(f"0x8aab -> {ea:#x}: {read_str(ea)!r}")

base = 0x7DC7 + 0x2D860
ctx = bytes(ida_bytes.get_byte(base + i) for i in range(340))
print(f"0x7dc7 table raw: {ctx!r}")
