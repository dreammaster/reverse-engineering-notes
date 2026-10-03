"""
Chapter 3's tile-type legends are paged (sub_1BC98 for wall types, sub_1BCDB for floor/overlay types): a type is
page * 100 + index; the page table (4 bytes per page: a pointer to the entries and the highest valid index) is at
DS:0x2 for walls (12-byte entries) and DS:0xC8E7 for floors (10-byte entries). Dumps the page tables and every entry
as words.

    .\run_ida_script.ps1 dump_tile_legends.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF


def w(off):
    return ida_bytes.get_word(DS_BASE + off)


lines = []
for name, table, size in (("wall", 0x2, 12), ("floor", 0xC8E7, 10)):
    lines.append(f"== {name} legend: page table at 0x{table:X}")
    for page in range(6):
        ptr = w(table + page * 4)
        top = w(table + page * 4 + 2)
        lines.append(f"page {page}: ptr=0x{ptr:X} maxIndex={top}")
        if ptr and top < 200 and ptr < 0xF000:
            for idx in range(top + 1):
                words = [w(ptr + idx * size + 2 * k) for k in range(size // 2)]
                lines.append(f"  {page * 100 + idx}: {words}")
with open(r"C:\dev\yendor\yendor3\ida_scripts\tile_legends.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines[:60]))
