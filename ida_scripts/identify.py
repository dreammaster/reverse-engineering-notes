"""
Read-only discovery script: reports what executable an .idb corresponds to
and how much analysis work has already been done on it. Meant to be run via
run_ida_script.ps1 with -NoExport so it doesn't touch the database.

    .\run_ida_script.ps1 -Idb ultima1_gen -ScriptName identify.py -NoExport
"""

import idc
import idautils
import ida_segment
import ida_funcs
import ida_bytes
import ida_nalt

print(f"root filename : {idc.get_root_filename()}")
print(f"input file    : {idc.get_input_file_path()}")
print(f"entry point   : {idc.get_inf_attr(idc.INF_START_IP):#x} (cs={idc.get_inf_attr(idc.INF_START_CS):#x})")

md5 = ida_nalt.retrieve_input_file_md5()
if md5:
    print(f"input md5     : {md5.hex() if isinstance(md5, bytes) else md5}")

print("\nsegments:")
for i in range(ida_segment.get_segm_qty()):
    seg = ida_segment.getnseg(i)
    name = ida_segment.get_segm_name(seg)
    print(f"  {name:12s} {seg.start_ea:#08x}-{seg.end_ea:#08x}  ({seg.end_ea - seg.start_ea} bytes)")

total = 0
named = 0
sub_named = 0
for ea in idautils.Functions():
    total += 1
    fname = idc.get_func_name(ea)
    if fname.startswith("sub_"):
        sub_named += 1
    else:
        named += 1

print(f"\nfunctions: {total} total, {named} named, {sub_named} still sub_XXXXX")

strucs = []
idx = idc.get_first_struc_idx() if hasattr(idc, "get_first_struc_idx") else None
try:
    import ida_struct
    sid = ida_struct.get_first_struc_idx()
    while sid != idc.BADADDR and sid != -1:
        tid = ida_struct.get_struc_by_idx(sid)
        sname = ida_struct.get_struc_name(tid)
        strucs.append(sname)
        sid = ida_struct.get_next_struc_idx(sid)
except ImportError:
    pass

print(f"\nstructs ({len(strucs)}): {', '.join(strucs)}")
