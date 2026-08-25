"""
Read-only helper: dumps full member layout (offset, name, size, type)
for one or more named structs in the currently-open IDB.

Edit STRUCT_NAMES below (or leave empty to dump ALL structs) and re-run.

    .\\run_ida_script.ps1 -Idb <stem> -ScriptName dump_struct.py -NoExport
"""

import ida_struct
import ida_bytes
import idc

STRUCT_NAMES = ["Savegame"]  # empty list = dump every struct

def dump_one(sid, sname):
    size = ida_struct.get_struc_size(sid)
    print(f"\n=== struct {sname}  (id={sid:#x}, size={size:#x} = {size} bytes) ===")
    struc = ida_struct.get_struc(sid)
    off = 0
    m = ida_struct.get_member(struc, 0) if struc.memqty else None
    idx = 0
    while idx < struc.memqty:
        member = struc.get_member(idx)
        moff = member.soff
        msize = member.eoff - member.soff
        mname = ida_struct.get_member_name(member.id)
        # try to get a type string
        tif = idc.get_member_tinfo(struc, member) if hasattr(idc, "get_member_tinfo") else None
        flag = member.flag
        cmt = ida_struct.get_member_cmt(member.id, 0) or ""
        print(f"  {moff:#06x}  size={msize:#04x} ({msize:3d})  {mname}" + (f"   ; {cmt}" if cmt else ""))
        idx += 1

names = STRUCT_NAMES
if not names:
    names = []
    idx = ida_struct.get_first_struc_idx()
    while idx != idc.BADADDR and idx != -1:
        tid = ida_struct.get_struc_by_idx(idx)
        names.append(ida_struct.get_struc_name(tid))
        idx = ida_struct.get_next_struc_idx(idx)

for sname in names:
    sid = ida_struct.get_struc_id(sname)
    if sid == idc.BADADDR:
        print(f"[!] struct {sname!r} not found")
        continue
    dump_one(sid, sname)
