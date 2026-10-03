"""
Chapter 2's tile-type legends are flat tables: wall types at DS:0xE551 (12-byte entries = 6 words) and floor/overlay types at
DS:0xE175 (10-byte entries = 5 words), indexed directly by the type (DrawDungeonCellWallTexture, DrawLocalMapCell). Dumps
58 wall and 68 floor entries as words (Chapter 3 pages them: see yendor3's dump_tile_legends.py).

    .\run_ida_script.ps1 dump_tile_legends.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860


def w(off):
    return ida_bytes.get_word(DS_BASE + off)


lines = ["== wall legend 0xE551"]
for idx in range(64):
    lines.append(f"  {idx}: {[w(0xE551 + idx * 12 + 2 * k) for k in range(6)]}")
lines.append("== floor legend 0xE175")
for idx in range(72):
    lines.append(f"  {idx}: {[w(0xE175 + idx * 10 + 2 * k) for k in range(5)]}")
with open(r"C:\dev\yendor\yendor2\ida_scripts\tile_legends.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines[:5]))
