"""
Read-only helper: full dump of Savegame plus every struct-typed member's
own layout (one level of nesting), for cross-IDB comparison. Also lists
every struct name defined in the IDB.

    .\\run_ida_script.ps1 -Idb <stem> -ScriptName dump_savegame_full.py -NoExport
"""

import ida_struct
import idc


def dump_struct(sid_or_name, indent=""):
    if isinstance(sid_or_name, str):
        sid = ida_struct.get_struc_id(sid_or_name)
        if sid == idc.BADADDR:
            print(f"{indent}[!] struct {sid_or_name!r} not found")
            return
    else:
        sid = sid_or_name
    struc = ida_struct.get_struc(sid)
    name = ida_struct.get_struc_name(sid)
    size = ida_struct.get_struc_size(sid)
    print(f"{indent}=== {name}  size={size:#x} ({size}) ===")
    for idx in range(struc.memqty):
        m = struc.get_member(idx)
        mname = ida_struct.get_member_name(m.id)
        msize = m.eoff - m.soff
        cmt = ida_struct.get_member_cmt(m.id, 0) or ""
        print(f"{indent}  {m.soff:#06x}  size={msize:#04x} ({msize:3d})  {mname}" + (f"   ; {cmt}" if cmt else ""))


print("Root:", idc.get_root_filename())
print("\nAll structs in this IDB:")
idxx = ida_struct.get_first_struc_idx()
names = []
while idxx != idc.BADADDR and idxx != -1:
    tid = ida_struct.get_struc_by_idx(idxx)
    n = ida_struct.get_struc_name(tid)
    names.append(n)
    idxx = ida_struct.get_next_struc_idx(idxx)
print(" ", ", ".join(names))

for n in ["Savegame", "Creature", "LocationWidget", "Point", "Rect", "STR15"]:
    if n in names:
        print()
        dump_struct(n)
