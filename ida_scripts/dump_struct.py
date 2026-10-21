"""
Read-only: dumps all structs (name, size, members) to dump_structs_<root>.txt. Run with -NoExport.
"""
import os
import idc
import ida_struct
root = os.path.splitext(idc.get_root_filename())[0]
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "dump_structs_%s.txt" % root)
with open(out, "w") as fh:
    sid = ida_struct.get_first_struc_idx()
    while sid != idc.BADADDR and sid != -1:
        tid = ida_struct.get_struc_by_idx(sid)
        st = ida_struct.get_struc(tid)
        fh.write("== %s size=%d\n" % (ida_struct.get_struc_name(tid), ida_struct.get_struc_size(st)))
        for off in range(0, ida_struct.get_struc_size(st)):
            m = ida_struct.get_member(st, off)
            if m and m.soff == off:
                fh.write("  +%02X %s size=%d\n" % (off, ida_struct.get_member_name(m.id), ida_struct.get_member_size(m)))
        sid = ida_struct.get_next_struc_idx(sid)
