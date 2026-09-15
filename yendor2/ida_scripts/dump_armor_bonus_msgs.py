"""
Dump label 0x7B24 and the 9-entry resistance-type table starting at
0x7B31 for sub_149DD (called from ShowArmorDetailRow).

Run via:
    .\\run_ida_script.ps1 dump_armor_bonus_msgs.py -NoExport
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

ea = 0x7B24 + 0x2D860
print(f"0x7b24 -> {ea:#x}: {read_str(ea)!r}")

# dump the raw bytes near 0x7B31 to see the string table layout
base = 0x7B31 + 0x2D860
ctx = bytes(ida_bytes.get_byte(base + i) for i in range(150))
print(f"0x7b31 table raw: {ctx!r}")
