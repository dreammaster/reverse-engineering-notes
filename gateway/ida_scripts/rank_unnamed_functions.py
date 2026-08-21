"""
Read-only report: lists all still-unnamed (sub_XXXXX) functions in the
current IDB, ranked by how many call sites reference them (most-called
first). High call-count + still-unnamed is the highest-value target --
naming it clarifies the most call sites at once. Ported from the sibling
ultima1 project's ida_scripts.

If the current IDB defines a function named "rtlink_thunk" (true for
gatemain.idb -- the commercial RTLink DOS overlay linker's call-gate,
see docs/overview.md), this also skips any small sub_ function whose
first instruction calls it: these are linker-generated
"call rtlink_thunk; jmp <real target>" trampolines, one per
cross-overlay call site, not independent logic -- 955 of gatemain.idb's
2481 unnamed functions matched this shape in one survey
(find_rtlink_thunks.py), badly polluting the ranking otherwise. Silently
does nothing on IDBs without that symbol (e.g. gate.idb, ultima1's).

Run with -NoExport, e.g.:
    .\\run_ida_script.ps1 -Idb gatemain -ScriptName rank_unnamed_functions.py -NoExport
"""

import idc
import idautils
import ida_bytes

rtlink_thunk_ea = idc.get_name_ea_simple("rtlink_thunk")


def is_rtlink_thunk(ea, end):
    if rtlink_thunk_ea == idc.BADADDR:
        return False
    size = end - ea
    if size > 12 or size < 5:
        return False
    if ida_bytes.get_byte(ea) != 0xE8:  # call rel16
        return False
    if rtlink_thunk_ea not in idautils.CodeRefsFrom(ea, 0):
        return False
    jmp_ea = ea + 3
    jmp_targets = list(idautils.CodeRefsFrom(jmp_ea, 0))
    if not jmp_targets:
        return False
    # Some far jmps land in a tail chunk IDA attributes back to this SAME
    # function (a relocated continuation, not a call to a different
    # function) -- that's a real function with a split body, not a thunk.
    return idc.get_func_attr(jmp_targets[0], idc.FUNCATTR_START) != ea


rows = []
thunks_skipped = 0
for ea in idautils.Functions():
    fname = idc.get_func_name(ea)
    if not fname.startswith("sub_"):
        continue
    end = idc.get_func_attr(ea, idc.FUNCATTR_END)
    if is_rtlink_thunk(ea, end):
        thunks_skipped += 1
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

if thunks_skipped:
    print(f"(skipped {thunks_skipped} rtlink_thunk-shaped trampolines)")
print(f"{len(rows)} unnamed functions total\n")
for count, ea, fname, callers in rows:
    caller_str = ", ".join(callers[:6])
    if len(callers) > 6:
        caller_str += f", ... (+{len(callers) - 6} more)"
    size = idc.get_func_attr(ea, idc.FUNCATTR_END) - ea
    print(f"{ea:X}  {fname:12s} callers={count:3d} size={size:4d}  <- {caller_str}")
