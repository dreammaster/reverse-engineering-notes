"""
Read-only report: which executable an .idb corresponds to and how far the
analysis has got. Run with -NoExport so it doesn't touch the database.

    .\run_ida_script.ps1 -Idb u6_game -ScriptName identify.py -NoExport
"""

import idc
import idautils
import ida_segment
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

total = named = 0
for ea in idautils.Functions():
    total += 1
    if not idc.get_func_name(ea).startswith("sub_"):
        named += 1
print(f"\nfunctions: {total} total, {named} named, {total - named} still sub_XXXXX")
