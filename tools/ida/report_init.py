"""Read-only first look at a fresh TWW database: segments, function/name counts, RTTI-ish names."""
import idautils, idc, ida_segment
print("processor", idc.get_inf_attr(idc.INF_PROCNAME), "base", hex(idc.get_inf_attr(idc.INF_MIN_EA)))
for s in idautils.Segments():
    print(idc.get_segm_name(s), hex(s), hex(idc.get_segm_end(s)))
funcs = list(idautils.Functions())
print("functions", len(funcs))
names = list(idautils.Names())
print("names", len(names))
print("RTTI-style names", sum(1 for _, n in names if n.startswith("??_7") or n.startswith("??_R")))
for ea, n in names[:40]:
    print(hex(ea), n)
