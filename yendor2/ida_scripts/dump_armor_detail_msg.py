"""
Dump label 0x8A96 for sub_13780 (called from RunClueBookItemCategory,
the ARMOR/RINGS subtype).

Run via:
    .\\run_ida_script.ps1 dump_armor_detail_msg.py -NoExport
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

ea = 0x8A96 + 0x2D860
print(f"0x8a96 -> {ea:#x}: {read_str(ea)!r}")
