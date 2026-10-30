"""
IDAPython: applies analysis/manual_names.tsv to the Xanth IDB - names/comments established by reading the code (as opposed to the
'?' names transferred from Shannara by apply_xanth_names.py).  Columns: current name, new name, comment.  Idempotent (skips rows whose
old name no longer exists).

    bash reversing/scripts/apply_to_idb.sh xanth.idb reversing/scripts/apply_xanth_manual.py
"""
import os
import idc
import ida_funcs
import ida_kernwin

TSV = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "analysis", "manual_names.tsv")
done = 0
with open(TSV, encoding="utf-8") as f:
    next(f)
    for line in f:
        old, new, cmt = line.rstrip("\n").split("\t", 2)
        ea = idc.get_name_ea_simple(old)
        if ea == idc.BADADDR:
            ea = idc.get_name_ea_simple(new)
            if ea == idc.BADADDR:
                print("WARN missing", old)
                continue
        idc.set_name(ea, new, idc.SN_NOWARN)
        fn = ida_funcs.get_func(ea)
        if fn and fn.start_ea == ea:
            if cmt not in (idc.get_func_cmt(ea, 0) or ""):
                idc.set_func_cmt(ea, cmt, 0)
        else:
            idc.set_cmt(ea, cmt, 1)
        done += 1
idc.save_database(idc.get_idb_path(), 0)
print("renamed (manual)", done)
if not ida_kernwin.is_idaq():
    idc.qexit(0)
