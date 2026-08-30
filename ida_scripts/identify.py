"""
Read-only discovery script: reports what executable an .idb corresponds to
and how much analysis work has already been done on it. Meant to be run via
run_ida_script.ps1 with -NoExport so it doesn't touch the database.

    .\run_ida_script.ps1 -Idb leglib -ScriptName identify.py -NoExport
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

# --- Legacy-of-the-Ancients-specific: this game is compiled Microsoft
# BASIC 6.0, not C. The per-module .EXEs are thin: nearly every source
# statement is a far call routed (via the seg with the int 3Fh thunk
# table) into leglib.exe's run-time module. Rough coverage numbers below
# help spot how much of a module is still raw bytes vs. real code.
code_bytes = 0
undef_in_code_seg = 0
int3f = 0
for i in range(ida_segment.get_segm_qty()):
    seg = ida_segment.getnseg(i)
    if seg.type != ida_segment.SEG_CODE and ida_segment.get_segm_name(seg) not in ("seg000",):
        # still sweep UNK segs -- IDA leaves LotA code segs as 'UNK'
        pass
    ea = seg.start_ea
    while ea < seg.end_ea:
        f = ida_bytes.get_full_flags(ea)
        if ida_bytes.is_code(f):
            code_bytes += 1
        elif not ida_bytes.is_data(f):
            undef_in_code_seg += 1
        ea += 1

print(f"\nbyte coverage : {code_bytes} code, {undef_in_code_seg} undefined (raw)")
