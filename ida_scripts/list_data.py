"""
Read-only: list every data item in dseg with name, size and xref count,
ranked by xrefs, to dump_data_<root>.txt. Run with -NoExport.
"""
import os
import idc
import idautils
import ida_bytes
import ida_name
import ida_segment

root = os.path.splitext(idc.get_root_filename())[0]
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "dump_data_%s.txt" % root)
seg = ida_segment.get_segm_by_name("dseg")
rows = []
ea = seg.start_ea
while ea < seg.end_ea:
    nm = ida_name.get_name(ea)
    if nm:
        n = len(list(idautils.XrefsTo(ea, 0)))
        sz = idc.get_item_size(ea)
        rows.append((ea, nm, sz, n))
    ea = idc.next_head(ea, seg.end_ea)
    if ea == idc.BADADDR:
        break
base = seg.start_ea
with open(out, "w") as fh:
    for ea, nm, sz, n in rows:
        fh.write("%X off=%04X %-28s size=%-5d xrefs=%d\n" % (ea, ea - base, nm, sz, n))
print("wrote", out, len(rows))
