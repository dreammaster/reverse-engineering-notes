"""
Apply mm2/names/<db stem>.tsv to the open database.  One entry per line:

    <hex ea>\t<name>\t<comment>          (comment optional; '#' lines and blanks are ignored)
    <hex ea>\t-\t<comment>               (comment only, keep the name)

Names of code addresses become functions if none exists yet.  Comments on a function start
are stored as the function comment.  Safe to re-run.  The main database also picks up
names/shared.tsv (resident names apply there); overlay databases only get their own file.
"""
import os, sys
import ida_funcs, ida_bytes, idc
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_ida_common import *

NAMES = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "names")
stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
n = c = 0
for fn in (stem + ".tsv",):
    path = os.path.join(NAMES, fn)
    if not os.path.exists(path):
        print("no", path); continue
    for line in open(path, encoding="utf-8"):
        line = line.rstrip("\n")
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        parts = line.split("\t")
        ea = int(parts[0], 16)
        name = parts[1] if len(parts) > 1 else "-"
        cmt = parts[2] if len(parts) > 2 else ""
        if name != "-":
            if ida_bytes.is_code(ida_bytes.get_flags(ea)) and not ida_funcs.get_func(ea):
                ida_funcs.add_func(ea)
            if idc.set_name(ea, name, idc.SN_NOCHECK | idc.SN_NOWARN):
                n += 1
            else:
                print("name failed:", hex(ea), name)
        if cmt:
            f = ida_funcs.get_func(ea)
            if f and f.start_ea == ea:
                idc.set_func_cmt(ea, cmt, 0)
            else:
                idc.set_cmt(ea, cmt, 0)
            c += 1
print(f"{stem}: applied {n} names, {c} comments")
