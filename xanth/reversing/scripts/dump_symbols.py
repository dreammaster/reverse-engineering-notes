"""
IDAPython: resolves data symbol names to {ea, segment start, item size} for the assembly lifter (lift_handlers.py), plus every segment's start paragraph.

    idat.exe -A -S"dump_symbols.py names.json out.json" copy.idb
"""
import json
import idc
import ida_segment
import ida_kernwin

names = json.load(open(idc.ARGV[1]))
res, miss = {}, []
for n in names:
    ea = idc.get_name_ea_simple(n)
    if ea == idc.BADADDR:
        s = ida_segment.get_segm_by_name(n)
        if s:
            res[n] = {"kind": "seg", "para": s.start_ea >> 4}
        else:
            miss.append(n)
        continue
    sg = ida_segment.getseg(ea)
    res[n] = {"kind": "data", "ea": ea, "seg": (sg.start_ea >> 4) if sg else 0, "segname": ida_segment.get_segm_name(sg) if sg else "", "size": idc.get_item_size(ea)}
segs = {}
for i in range(ida_segment.get_segm_qty()):
    s = ida_segment.getnseg(i)
    segs[ida_segment.get_segm_name(s)] = s.start_ea >> 4
json.dump({"syms": res, "missing": miss, "segs": segs}, open(idc.ARGV[2], "w"))
print("resolved", len(res), "missing", len(miss))
if not ida_kernwin.is_idaq():
    idc.qexit(0)
