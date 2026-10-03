"""Dump the user-assigned names (and function comments) of mm3.idb to mm3/names/mm3.tsv, merging with the
file's existing entries (existing comments are kept).  Run read-only:
  run_ida_script.ps1 -Idb mm3\mm3.idb -ScriptName mm3\ida_scripts\export_names.py -NoExport
Format: <hex linear ea>\t<name>\t<comment>"""
import os
import ida_bytes, ida_funcs, idautils, idc

path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "names", "mm3.tsv")
old, head = {}, []
if os.path.exists(path):
    for line in open(path, encoding="utf-8"):
        line = line.rstrip("\n")
        if line.startswith("#") or not line.strip():
            head.append(line)
            continue
        p = line.split("\t")
        old[int(p[0], 16)] = (p[1] if len(p) > 1 else "-", p[2] if len(p) > 2 else "")
names = dict(old)
for ea, name in idautils.Names():
    if not ida_bytes.has_user_name(ida_bytes.get_flags(ea)):
        continue
    if name.startswith(("def_", "jpt_", "loc_")):
        continue   # IDA-generated switch labels
    if name.startswith("j_") and idc.get_segm_name(ea).startswith("stub"):
        continue   # thunk names are derived by apply_names.py
    names[ea] = (name, old.get(ea, ("", ""))[1])
for ea in idautils.Functions():
    c = idc.get_func_cmt(ea, 0)
    if c:
        names[ea] = (names.get(ea, ("-", ""))[0], c.replace("\n", " "))
with open(path, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(head) + ("\n" if head else ""))
    for ea in sorted(names):
        n, c = names[ea]
        f.write("%05X\t%s\t%s\n" % (ea, n, c) if c else "%05X\t%s\n" % (ea, n))
print(len(names), "names written")
