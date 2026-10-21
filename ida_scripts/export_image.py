"""
Read-only: writes the loaded memory image of the database (linear 0x10000.. end) to wl_image.bin next to this script,
plus wl_image.txt with the segment table. Run with -NoExport.
"""
import os
import idc
import ida_segment
import ida_bytes

here = os.path.dirname(os.path.abspath(__file__))
lo = 0x10000
hi = 0
segs = []
for i in range(ida_segment.get_segm_qty()):
    s = ida_segment.getnseg(i)
    segs.append((ida_segment.get_segm_name(s), s.start_ea, s.end_ea))
    hi = max(hi, s.end_ea)
data = bytearray(hi - lo)
for name, a, b in segs:
    for ea in range(a, b):
        if ida_bytes.is_loaded(ea):
            data[ea - lo] = ida_bytes.get_byte(ea) & 0xFF
with open(os.path.join(here, "wl_image.bin"), "wb") as f:
    f.write(bytes(data))
with open(os.path.join(here, "wl_image.txt"), "w") as f:
    f.write("base %#x size %#x\n" % (lo, hi - lo))
    for name, a, b in segs:
        f.write("%s %#x %#x\n" % (name, a, b))
    f.write("entry cs=%#x ip=%#x\n" % (idc.get_inf_attr(idc.INF_START_CS), idc.get_inf_attr(idc.INF_START_IP)))
