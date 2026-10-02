"""Apply mm3/names/mm3.tsv (see export_names.py) to the open database; safe to re-run."""
import os
import ida_bytes, ida_funcs, idc

path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "names", "mm3.tsv")
n = c = 0
for line in open(path, encoding="utf-8"):
    line = line.rstrip("\n")
    if not line.strip() or line.startswith("#"):
        continue
    p = line.split("\t")
    ea, name, cmt = int(p[0], 16), p[1] if len(p) > 1 else "-", p[2] if len(p) > 2 else ""
    if name != "-":
        if ida_bytes.is_code(ida_bytes.get_flags(ea)) and not ida_funcs.get_func(ea):
            ida_funcs.add_func(ea)
        ok = idc.set_name(ea, name, idc.SN_NOCHECK | idc.SN_NOWARN)
        if not ok and not ida_bytes.is_code(ida_bytes.get_flags(ea)):
            # inside a larger data item: split it so the address becomes an item head
            ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 1)
            ok = idc.set_name(ea, name, idc.SN_NOCHECK | idc.SN_NOWARN)
        if ok:
            n += 1
        else:
            print("name failed:", hex(ea), name)
    if cmt:
        f = ida_funcs.get_func(ea)
        (idc.set_func_cmt(ea, cmt, 0) if f and f.start_ea == ea else idc.set_cmt(ea, cmt, 0))
        c += 1
print("applied %d names, %d comments" % (n, c))
