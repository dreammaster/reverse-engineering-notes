"""
Read-only: dumps the two tile-type legend tables a map cell's tileA/tileB
indices point into (0xE551, 12-byte stride, "wall" type; 0xE175, 10-byte
stride, "floor/overlay" type), for src23/worldmap.c.

Only field +0xA (0xE551) / +8 (0xE175) is confirmed (the g_pictureDir
picture offset DrawWallTypeLegendRow/DrawFloorTypeLegendRow draw) -- see
file-formats.md. This dumps the full 6-word/5-word rows anyway so the other
fields are available if a future session traces them; only the confirmed
picture-offset column is currently reproduced in src23/worldmap.c.

Real map data never uses a tileA index above 57 or a tileB index above 67
(checked against both real WORLD.DAT files); this script dumps a few rows
past those to confirm the tables really do go blank/reserved there rather
than the real data simply not exercising the tail of a longer table.

    .\run_ida_script.ps1 dump_tile_type_tables.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860


def dump(label, base, stride, words, count):
    print("%s @DS:%#x (%d-byte stride):" % (label, base, stride))
    for i in range(count):
        row = [ida_bytes.get_wide_word(DS_BASE + base + i * stride + 2 * k) for k in range(words)]
        print("  %2d: %s" % (i, " ".join("%04x" % w for w in row)))


dump("wall type table (0xE551)", 0xE551, 12, 6, 65)
print()
dump("floor/overlay type table (0xE175)", 0xE175, 10, 5, 75)
