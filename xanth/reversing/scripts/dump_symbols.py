"""
IDAPython: resolves data symbol names to {ea, segment start, item size, struct type} for the assembly lifter (lift_handlers.py), plus every segment's start paragraph,
every structure (size and members with offsets / nested structure types) and every enum member value (IDA prints these as names in operands).

    idat.exe -A -S"dump_symbols.py names.json out.json" copy.idb
"""
import json
import idc
import idautils
import ida_segment
import ida_nalt
import ida_kernwin

names = json.load(open(idc.ARGV[1]))
res, miss = {}, []
import re
DUMMY = re.compile(r"^(loc|locret|sub|byte|word|dword|unk|off|stru|asc|flt|qword|seg)_([0-9A-F]+)$")
for n in names:
    nn = {"g_offset_var": "offset", "g_ptr_var": "ptr"}.get(n, n)
    ea = idc.get_name_ea_simple(nn)
    if ea == idc.BADADDR:
        dm = DUMMY.match(n)
        if dm and ida_segment.getseg(int(dm.group(2), 16)):
            ea = int(dm.group(2), 16)       # IDA's auto-generated names are not in the name table, but encode the address
    if ea == idc.BADADDR and "__" in n:
        ea = idc.get_name_ea_simple(n.replace("__", "::", 1))     # C++ style names (PicFile::_file) are exported with underscores
    if ea == idc.BADADDR:
        s = ida_segment.get_segm_by_name(n)
        if s:
            res[n] = {"kind": "seg", "para": s.start_ea >> 4}
        else:
            miss.append(n)
        continue
    sg = ida_segment.getseg(ea)
    d = {"kind": "data", "ea": ea, "seg": (sg.start_ea >> 4) if sg else 0, "segname": ida_segment.get_segm_name(sg) if sg else "", "size": idc.get_item_size(ea)}
    try:
        sid = ida_nalt.get_strid(ea)
    except Exception:
        sid = -1
    if sid not in (-1, idc.BADADDR, 0xFFFFFFFF):
        d["struct"] = idc.get_struc_name(sid)
    res[n] = d
segs = {}
for i in range(ida_segment.get_segm_qty()):
    s = ida_segment.getnseg(i)
    segs[ida_segment.get_segm_name(s)] = s.start_ea >> 4
structs = {}
for sid_idx, sid, sname in idautils.Structs():
    members = {}
    for off, mname, msize in idautils.StructMembers(sid):
        try:
            sub = idc.get_member_strid(sid, off)
        except Exception:
            sub = -1
        members[mname] = [off, msize, idc.get_struc_name(sub) if sub not in (-1, idc.BADADDR, 0xFFFFFFFF) else None]
    structs[sname] = {"size": idc.get_struc_size(sid), "members": members}
enums = {}
for i in range(idc.get_enum_qty()):
    eid = idc.getn_enum(i)
    masks = [0xFFFFFFFF]
    if idc.is_bf(eid):
        masks = []
        m = idc.get_first_bmask(eid)
        while m not in (idc.BADADDR, 0xFFFFFFFF, -1) and len(masks) < 64:
            masks.append(m)
            nm = idc.get_next_bmask(eid, m)
            if nm == m:
                break
            m = nm
        masks.append(0xFFFFFFFF)
    for mask in masks:
        v = idc.get_first_enum_member(eid, mask)
        n = 0
        while v != idc.BADADDR and n < 5000:
            cid = idc.get_enum_member(eid, v, 0, mask)
            nm_ = idc.get_enum_member_name(cid)
            if nm_:
                enums[nm_] = v
            v = idc.get_next_enum_member(eid, v, mask)
            n += 1
json.dump({"syms": res, "missing": miss, "segs": segs, "structs": structs, "enums": enums}, open(idc.ARGV[2], "w"))
print("resolved", len(res), "missing", len(miss), "structs", len(structs), "enums", len(enums))
if not ida_kernwin.is_idaq():
    idc.qexit(0)
