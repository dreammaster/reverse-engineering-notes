"""
Read-only: settles where Chapter 3's LoadCurgameRecord gets its two constants. The IDA names word_3320E (the record-block multiplier,
read by LoadCurgameRecord) and word_2ECF8 (the bit offset of curgame ids in the shared event bitmap) have no named writer, but
InitGlobals writes the data-segment offsets DS:0x545E (= 0x3E8, 1000) and DS:0x0F48 (= 0x3F0, 1008) with raw `ds:` operands -- the
Chapter 3 equivalents of Chapter 2's _val9 = 600 and _val10 = 608. This prints the DS base and the linear addresses of those two
offsets so the names can be matched (expected: 0x3320E and 0x2ECF8).

    .\run_ida_script.ps1 check_curgame_globals.py -NoExport
"""
import idc
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF

for name, offset in (("multiplier (DS:0x545E)", 0x545E), ("bit offset (DS:0x0F48)", 0x0F48)):
    ea = DS_BASE + offset
    print("%s -> linear 0x%X, IDA name %s" % (name, ea, idc.get_name(ea)))
print("DS base 0x%X" % DS_BASE)
