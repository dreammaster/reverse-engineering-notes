"""
Read-only report: lists all still-unnamed (sub_XXXXX) functions in the
current IDB, ranked by how many call sites reference them (most-called
first). High call-count + still-unnamed is the highest-value target --
naming it clarifies the most call sites at once. Mirrors the sweep
approach used in the sibling ultima1 / ultima2 projects.

Run with -NoExport, e.g.:
    .\run_ida_script.ps1 -Idb leglib -ScriptName rank_unnamed_functions.py -NoExport
"""

import idc
import idautils

rows = []
for ea in idautils.Functions():
    fname = idc.get_func_name(ea)
    if not fname.startswith("sub_"):
        continue
    callers = set()
    for xref in idautils.CodeRefsTo(ea, 0):
        caller_func = idc.get_func_name(xref)
        if caller_func:
            callers.add(caller_func)
        else:
            callers.add(f"data/{xref:X}")
    rows.append((len(callers), ea, fname, sorted(callers)))

rows.sort(key=lambda r: -r[0])

print(f"{len(rows)} unnamed functions total\n")
for count, ea, fname, callers in rows:
    caller_str = ", ".join(callers[:6])
    if len(callers) > 6:
        caller_str += f", ... (+{len(callers) - 6} more)"
    size = idc.get_func_attr(ea, idc.FUNCATTR_END) - ea
    print(f"{ea:X}  {fname:12s} callers={count:3d} size={size:4d}  <- {caller_str}")
