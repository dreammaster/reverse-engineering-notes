"""
Read-only: lists all remaining sub_XXXXX functions with their sizes
and call-site counts, sorted by size ascending (smallest/easiest
first), to pick the next naming targets once the named-neighbor
heuristic in rank_naming_candidates.py runs dry.

    .\run_ida_script.ps1 list_remaining_unnamed.py -NoExport
"""
import ida_funcs
import idc
import idautils

rows = []
for ea in idautils.Functions():
    name = idc.get_func_name(ea)
    if not name.startswith("sub_"):
        continue
    f = ida_funcs.get_func(ea)
    size = f.end_ea - f.start_ea if f else 0
    nrefs = len(list(idautils.CodeRefsTo(ea, 0))) + len(list(idautils.CodeRefsTo(ea, 1)))
    rows.append((size, nrefs, ea, name))

rows.sort(key=lambda r: r[0])
for size, nrefs, ea, name in rows:
    print(f"{ea:#x}  size={size:4d}  refs={nrefs:3d}  {name}")
print(f"total: {len(rows)}")
