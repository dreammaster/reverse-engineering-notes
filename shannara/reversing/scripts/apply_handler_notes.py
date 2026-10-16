"""
IDAPython: writes the LOCAL notes produced by make_handler_notes.py into function comments of the retail IDB
(objects owning each handler + the first message texts).  Run through apply_to_idb.sh (analysis dir must contain handler_notes.tsv,
which is copied from game_out by passing the path, or edit NOTES below).

    idat.exe -A -S"reversing/scripts/apply_handler_notes.py" Shannara.idb
"""
import os
import idc
import ida_kernwin

NOTES = os.environ.get("SHAN_NOTES", r"C:\dev\legend\shannara\game_out\handler_notes.tsv")
n = 0
if os.path.exists(NOTES):
    for line in open(NOTES, encoding="utf-8"):
        name, _, text = line.rstrip("\n").partition("\t")
        ea = idc.get_name_ea_simple(name)
        if ea != idc.BADADDR and text:
            old = idc.get_func_cmt(ea, 0) or ""
            marker = "\n--- game text ---\n"
            base = old.split(marker)[0]
            idc.set_func_cmt(ea, base + marker + text, 0)
            n += 1
idc.save_database(idc.get_idb_path(), 0)
print("notes applied:", n)
if not ida_kernwin.is_idaq():
    idc.qexit(0)
