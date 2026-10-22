"""
Read-only survey: one line per function, "<start> <size> <sha1 of raw bytes>",
written to func_hashes_<dbname>.txt. Near calls/jumps are IP-relative, so a
function copied unchanged between two programs hashes identically as long as it
contains no relocated segment-immediates or absolute data offsets; this gives a
lower bound on shared library code between the PROGn executables.

    .\run_ida_script.ps1 <name> dump_func_hashes.py -NoExport
"""

import hashlib
import os

import idautils
import ida_funcs
import ida_bytes
import idc

stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"func_hashes_{stem}.txt")
with open(out, "w") as fh:
    for ea in idautils.Functions():
        f = ida_funcs.get_func(ea)
        size = f.end_ea - f.start_ea
        h = hashlib.sha1(ida_bytes.get_bytes(f.start_ea, size)).hexdigest()[:16]
        fh.write(f"{ea:#x} {size} {h}\n")
print(f"{stem}: wrote {out}")
