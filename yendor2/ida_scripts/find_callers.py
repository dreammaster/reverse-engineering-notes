"""
Read-only: lists callers of hardcoded target addresses (edit TARGETS).

    .\run_ida_script.ps1 find_callers.py -NoExport
"""
import idautils
import idc
import ida_funcs

TARGETS = [0x24FFC]

for t in TARGETS:
    callers = set()
    for x in idautils.CodeRefsTo(t, 0):
        f = ida_funcs.get_func(x)
        if f:
            callers.add(f.start_ea)
    names = sorted(idc.get_func_name(c) for c in callers)
    print(f"{t:#x} ({idc.get_func_name(t)}): {len(names)} callers: {names[:15]}")
