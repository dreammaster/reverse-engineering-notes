"""
Chapter 3's counterpart of yendor2/ida_scripts/dump_document_tables.py --
see that file for the full explanation. This database has no ds segment
register, so the data segment base is taken from seg133's paragraph-aligned
start.

    .\run_ida_script.ps1 dump_document_tables.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF

# (category, offset-table addr, length-table addr, entry count, line width).
# Journal/Note/Document counts come from the gap to the next table in
# memory; Unused's 1 is asserted the same way as in the Chapter 2 script.
CATEGORIES = [
    ("Journal (was _4000)", 0xB5F5, 0xB615, 8, 16),
    ("Note (was _1000)", 0xB625, 0xB649, 8, 23),
    ("Document (was _2000)", 0xB659, 0xB6C1, 26, 22),
    ("Unused (was _800)", 0xB6F5, 0xB6F9, 1, 23),
]

for name, off_table, len_table, count, width in CATEGORIES:
    print("%s: offset table @%#x, length table @%#x, %d entries, %d-byte lines" %
          (name, off_table, len_table, count, width))
    for i in range(count):
        lo = ida_bytes.get_wide_word(DS_BASE + off_table + i * 4)
        hi = ida_bytes.get_wide_word(DS_BASE + off_table + i * 4 + 2)
        length = ida_bytes.get_wide_word(DS_BASE + len_table + i * 2)
        offset = (hi << 16) | lo
        lines = length // width
        remainder = length % width
        print("  id %2d: offset=%#x length=%#x(%d) lines=%d rem=%d" % (i + 1, offset, length, length, lines, remainder))
