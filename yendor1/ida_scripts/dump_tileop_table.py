"""Dump the tile-cache operation table (6 near offsets at DS:1DC5h) in the open database. Read-only."""
import idc, ida_segment
seg = ida_segment.get_segm_by_name("seg019")
base = (seg.start_ea >> 4) << 4
for i in range(8):
    w = idc.get_wide_word(base + 0x1DC5 + 2 * i)
    print(f"op {i}: {w:#06x}")
for off in (0x1DC0, 0x1DC1, 0x1DC3):
    print(f"ds:{off:#x} = {idc.get_wide_word(base+off):#06x}")
