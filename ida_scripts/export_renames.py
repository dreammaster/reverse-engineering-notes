r"""
Dump every user-given name in the open database (anything that is not one of
IDA's auto-generated names) to renames/<game>/<stem>.export.tsv.  Read-only:
run with -NoExport.

    .\run_ida_script.ps1 -Idb cadaver1\cad -ScriptName export_renames.py -NoExport

then on any machine with the repo:

    python tools/renames.py merge cadaver1/cad

which adds rows the table does not have yet (conf = likely, so review them),
fills segoff/sig, and reports names that disagree with the table.  This is how
renames made by hand in IDA get into the repo, so they can be re-applied to a
fresh database later with apply_renames.py.
"""
import os
import sys

import idc
import idautils
import ida_funcs
import ida_bytes

_here = os.path.dirname(os.path.abspath(__file__))
_root = os.path.dirname(_here)
sys.path.insert(0, os.path.join(_root, "tools"))
import renames_lib as R  # noqa: E402


def main():
    idb = idc.get_idb_path()
    game = os.path.basename(os.path.dirname(idb))
    stem = os.path.splitext(os.path.basename(idb))[0]
    out = os.path.join(_root, "renames", game, stem + ".export.tsv")
    rows = []
    for ea, name in idautils.Names():
        if R.AUTO_NAME_RE.match(name):
            continue
        f = ida_funcs.get_func(ea)
        if f is not None and f.start_ea == ea:
            kind, size = "func", 0
        elif ida_bytes.is_code(ida_bytes.get_flags(ea)):
            kind, size = "label", 0
        else:
            kind, size = "data", ida_bytes.get_item_size(ea)
        com = (idc.get_func_cmt(ea, 1) if kind == "func" else idc.get_cmt(ea, 1)) or ""
        rows.append(R.Row(ea, "", kind, "", name, "likely", size, "", com.replace("\t", " ").replace("\n", " ")))
    R.write_tsv(out, rows, "names exported from %s/%s.idb by export_renames.py - merge with tools/renames.py merge" % (game, stem))
    print("wrote %d name(s) to %s" % (len(rows), out))


main()
