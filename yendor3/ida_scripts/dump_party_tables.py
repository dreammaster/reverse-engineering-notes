"""
Read-only: Chapter 3's counterpart of yendor2/ida_scripts/dump_party_tables.py.
Same tables at different addresses (stat names 0x80F4, class names 0x7CB4/
0x875B/0x87BE). This database has no ds segment register, so the data
segment base is taken from seg133's paragraph-aligned start.

    .\run_ida_script.ps1 dump_party_tables.py -NoExport
"""
import idc
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF


def text(offset, size):
    raw = bytes(idc.get_bytes(DS_BASE + offset, size))
    return raw.split(b"\0")[0].decode("latin1").rstrip()


print("STAT TABLE @0x80F4:")
for i in range(27):
    print("  idx %2d current=+0x%02x max=+0x%02x name=%r" % (i, 0x3C + 2 * i, 0x7C + 2 * i, text(0x80F4 + i * 13, 13)))

for tier, base in enumerate((0x7CB4, 0x875B, 0x87BE)):
    names = [text(base + i * 11, 11) for i in range(9)]
    print("CLASS tier %d (ids %d-%d): %s" % (tier, tier * 10 + 1, tier * 10 + 9, names))
