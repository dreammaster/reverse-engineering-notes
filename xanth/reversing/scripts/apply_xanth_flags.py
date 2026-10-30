"""
IDAPython: gives the saved story/puzzle variables that still have automatic names (byte_852C0, word_85268, ...) a descriptive name
flag_<ADDR>_<first writer's object> / flag_<ADDR>_conv<N> (conversation callback) / flag_<ADDR>_<function>, and a comment listing the writers and testers,
from analysis/xanth_flags.tsv (made by xanth_flags.py).  Names are heuristic labels, not meanings: they only say where the variable is set first.

    bash reversing/scripts/apply_to_idb.sh xanth.idb reversing/scripts/apply_xanth_flags.py
"""
import os
import re
import idc
import ida_kernwin

TSV = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "analysis", "xanth_flags.tsv")
AUTO = re.compile(r"^(byte_|word_|unk_|dword_)")


def slug(label):
    m = re.match(r"^(\w+?)(?:\((\d+) (.*)\))?$", label)
    fn, name = m.group(1), m.group(3)
    if name:
        return re.sub(r"[^A-Za-z0-9]+", "_", name).strip("_").lower()[:20]
    m2 = re.match(r"conv_cb_(\d+)", fn)
    return "conv%s" % m2.group(1) if m2 else fn


n = 0
with open(TSV, encoding="utf-8") as f:
    next(f)
    for line in f:
        addr, name, size, wr, te = (line.rstrip("\n").split("\t") + ["", ""])[:5]
        ea = int(addr, 16)
        cur = idc.get_name(ea)
        if not wr or not AUTO.match(cur):
            continue
        first = wr.split(";")[0].split("=")[0].strip()
        new = "flag_%s_%s" % (addr.upper(), slug(first))
        if idc.set_name(ea, new, idc.SN_NOWARN):
            idc.set_cmt(ea, "written by: %s | tested by: %s" % (wr[:300], te[:200]), 1)
            n += 1
idc.save_database(idc.get_idb_path(), 0)
print("renamed flags", n)
if not ida_kernwin.is_idaq():
    idc.qexit(0)
