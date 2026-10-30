"""
IDAPython: applies analysis/xanth_names.tsv (made by make_xanth_names.py) to the Xanth IDB.

  * a function or data item whose current name is automatic (sub_/nullsub_/loc_/dword_/word_/byte_/unk_/seg_/off_ ...) is renamed to the
    transferred Shannara name (which ends in '?': "same code in Shannara, matched by strings/call graph/order, not individually verified");
  * anything else keeps its name and just receives the evidence as a comment;
  * a name that is already taken gets a numeric suffix.
Idempotent.  Run with apply_to_idb.sh from the xanth folder:  bash reversing/scripts/apply_to_idb.sh xanth.idb reversing/scripts/apply_xanth_names.py
"""
import os
import re
import idc
import ida_funcs
import ida_kernwin

HERE = os.path.dirname(os.path.abspath(__file__))
TSV = os.path.join(HERE, "..", "analysis", "xanth_names.tsv")
AUTO = re.compile(r"^(sub_|nullsub_|loc_|j_|dword_|word_|byte_|unk_|seg_|off_|stru_|asc_|flt_|qword_|jpt_|byte|word)")
done = renamed = 0
with open(TSV, encoding="utf-8") as f:
    next(f)
    for line in f:
        ea_s, name, cmt = line.rstrip("\n").split("\t", 2)
        ea = int(ea_s, 16)
        fn = ida_funcs.get_func(ea)
        is_func = fn is not None and fn.start_ea == ea
        cur = idc.get_name(ea)
        if name != "-" and (AUTO.match(cur) or not cur):
            base, n = name, 1
            while idc.get_name_ea_simple(name) not in (idc.BADADDR, ea):
                n += 1
                name = "%s_%d" % (base, n)
            if idc.set_name(ea, name, idc.SN_NOWARN):
                renamed += 1
        if is_func:
            old = idc.get_func_cmt(ea, 0) or ""
            if cmt not in old:
                idc.set_func_cmt(ea, (old + "\n" if old else "") + cmt, 0)
        else:
            old = idc.get_cmt(ea, 1) or ""
            if cmt not in old:
                idc.set_cmt(ea, (old + " | " if old else "") + cmt, 1)
        done += 1
idc.save_database(idc.get_idb_path(), 0)
print("applied", done, "renamed", renamed)
if not ida_kernwin.is_idaq():
    idc.qexit(0)
