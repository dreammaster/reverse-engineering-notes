"""
Read-only: dump every function of the extended Wasteland database (ext/WL_ext.idb) with ALL its chunks (code that IDA
attached to a function as tails, such as shared epilogues) to dump_wl_ext.txt. Same line format as dump_funcs.py plus
";---- chunk" separators. Used by engine/src/wasteland1/tools/fn.py.

    .\run_ida_script.ps1 -Idb ida_scripts\ext\WL_ext.idb -ScriptName dump_wl_ext.py -NoExport
"""
import os
import idc
import idautils
import ida_funcs

out_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "dump_wl_ext.txt")


def chunks(fea):
    f = ida_funcs.get_func(fea)
    res = []
    it = ida_funcs.func_tail_iterator_t(f)
    ok = it.main()
    while ok:
        c = it.chunk()
        res.append((c.start_ea, c.end_ea))
        ok = it.next()
    res.sort()
    return res


with open(out_path, "w", encoding="utf-8") as fh:
    for fea in idautils.Functions():
        cs = chunks(fea)
        size = sum(b - a for a, b in cs)
        callers = sorted({idc.get_func_name(x) or ("data_%X" % x) for x in idautils.CodeRefsTo(fea, 0)})
        fh.write("\n;==== %s @ %X size=%d callers=%s\n" % (idc.get_func_name(fea), fea, size, ",".join(callers)))
        first = True
        for a, b in cs:
            if not first:
                fh.write(";---- chunk %X..%X\n" % (a, b))
            first = False
            ea = a
            while ea < b:
                lbl = idc.get_name(ea)
                if lbl and ea != fea and not lbl.startswith("loc_") and not lbl.startswith("sub_") is False:
                    pass
                fh.write("%X  %s\n" % (ea, idc.generate_disasm_line(ea, 0)))
                ea = idc.next_head(ea, b)
print("wrote", out_path)
