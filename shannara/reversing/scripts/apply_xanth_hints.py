"""
IDAPython (Shannara IDB): adds Xanth's existing human names as comments on still-unnamed Shannara functions, from
xanth/reversing/analysis/shannara_hints.tsv (produced by make_xanth_names.py).  Comments only - names stay as they are.
"""
import os
import idc
import ida_kernwin

TSV = os.environ.get("XANTH_HINTS", r"C:\dev\legend\xanth\reversing\analysis\shannara_hints.tsv")
n = 0
for line in open(TSV, encoding="utf-8").read().split("\n")[1:]:
    if not line:
        continue
    ea_s, name, cmt = line.split("\t", 2)
    ea = int(ea_s, 16)
    old = idc.get_func_cmt(ea, 0) or ""
    txt = "Companions of Xanth name: %s (%s)" % (name, cmt)
    if txt not in old:
        idc.set_func_cmt(ea, (old + "\n" if old else "") + txt, 0)
        n += 1
idc.save_database(idc.get_idb_path(), 0)
print("hints applied", n)
if not ida_kernwin.is_idaq():
    idc.qexit(0)
