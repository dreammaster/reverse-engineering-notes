"""Read-only: per-function summary (size, callees, strings, ints, ports) of the loaded code.
Writes mm2/build/funcs_<stem>.txt.  Works in main and overlay databases."""
import os, idautils, idc, ida_funcs, ida_bytes, ida_nalt, ida_segment
stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
lines = []
def in_code(ea):
    s = ida_segment.getseg(ea); return s and ida_segment.get_segm_class(s) == "CODE"
for fea in idautils.Functions():
    f = ida_funcs.get_func(fea)
    if not in_code(fea) or (stem != "mm2" and fea < 0x17E10): continue
    callees, strs, ints = [], [], []
    for h in idautils.FuncItems(fea):
        m = idc.print_insn_mnem(h)
        if m in ("call",):
            for r in idautils.CodeRefsFrom(h, 0):
                n = idc.get_name(r)
                if n not in callees: callees.append(n)
        elif m == "int":
            ints.append("int%X" % idc.get_operand_value(h, 0))
        for r in idautils.DataRefsFrom(h):
            if ida_bytes.is_strlit(ida_bytes.get_flags(r)):
                s = idc.get_strlit_contents(r, -1, 0)
                if s: strs.append(s.decode("latin1")[:40])
    lines.append("%05X %-22s size=%4d callers=%2d | calls: %s | strs: %s | %s" % (
        fea, idc.get_func_name(fea), f.end_ea - fea, len(list(idautils.CodeRefsTo(fea, 0))),
        ",".join(callees), "; ".join(strs[:6]), " ".join(sorted(set(ints)))))
p = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build", f"funcs_{stem}.txt")
open(p, "w").write("\n".join(lines))
print(len(lines), "functions ->", p)
