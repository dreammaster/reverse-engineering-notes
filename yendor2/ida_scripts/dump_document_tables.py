"""
Read-only: dumps the four in-EXE index tables RunConversation's four
branches (LoadConversationText_4000/_2000/_1000/_800) use to find their
text in WORLD.DAT, for src23/document.c.

Each category has two small tables baked into the executable itself (not
WORLD.DAT): a dword (offset-lo, offset-hi) table and a word (byte length)
table, both indexed by a 1-based id from LookupConversationTextBlockOffset_*
(yendor2.asm:42441 on). This script locates each category's entry count by
the gap to the next table in memory, then prints (id, absolute WORLD.DAT
offset, byte length, line count) for every entry -- including the empty
placeholder entries (length 0), which are real data, not missing tables.

    .\run_ida_script.ps1 dump_document_tables.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860

# (category, offset-table addr, length-table addr, entry count, line width in
# bytes). Entry counts for Journal/Note/Document come from the gap to the
# next table in memory (Note's and Document's offset tables sit right where
# the previous category's length table ends); Unused's isn't bounded that
# way (what follows it isn't identified), so its count of 1 is asserted
# from the driver code's own div-by-width sanity check instead (a 2nd
# "entry" here would read into unrelated data, not a real 2nd id).
CATEGORIES = [
    ("Journal (was _4000)", 0xD0D9, 0xD0F1, 6, 16),
    ("Note (was _1000)", 0xD0FD, 0xD101, 1, 23),
    ("Document (was _2000)", 0xD103, 0xD16B, 26, 22),
    ("Unused (was _800)", 0xD19F, 0xD1A3, 1, 23),
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
