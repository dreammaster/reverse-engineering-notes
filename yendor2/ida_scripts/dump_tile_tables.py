"""
Read-only: dumps the two minimap tile-type lookup tables
BuildMinimapTileData reads (DS-relative 0xE551, 12 bytes/entry, field
+0xA used; and 0xE175, 10 bytes/entry, field +8 used), to see their
structure and how many entries they have before values look implausible
as picture ids (should be roughly in g_pictureDir's 0-9 range, or
reference some other picture set entirely -- worth checking).

    .\run_ida_script.ps1 dump_tile_tables.py -NoExport
"""
import ida_bytes
import idc

DS_BASE = 0x2D860

def dump(base, stride, field_off, count, label):
    print(f"\n{label} @ DS:{base:#x} (linear {DS_BASE+base:#x}), stride={stride:#x}, field@+{field_off:#x}")
    for i in range(count):
        ea = DS_BASE + base + i * stride
        raw = ida_bytes.get_bytes(ea, stride)
        val = ida_bytes.get_word(ea + field_off)
        hexs = " ".join(f"{b:02x}" for b in raw)
        name = idc.get_name(ea)
        print(f"  [{i:2d}] ea={ea:#x} name={name!r:20s} field={val:#06x}  raw={hexs}")

dump(0xE551, 0xC, 0xA, 12, "table @ 0xE551 (12 bytes/entry)")
dump(0xE175, 0xA, 0x8, 12, "table @ 0xE175 (10 bytes/entry)")
