"""
Dump labels for sub_1385C (called from RunClueBookWeaponCategory):
0x7C81, 0x8ADE, 0x8AE8, 0x8AEC.

Run via:
    .\\run_ida_script.ps1 dump_weapon_detail_msgs.py -NoExport
"""
import ida_bytes

def read_str(ea, maxlen=40):
    out = bytearray()
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(b)
    return bytes(out)

for off in (0x7C81, 0x8ADE, 0x8AE8, 0x8AEC):
    ea = off + 0x2D860
    print(f"{off:#x} -> {ea:#x}: {read_str(ea)!r}")
