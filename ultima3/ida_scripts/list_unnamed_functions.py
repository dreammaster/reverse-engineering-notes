"""
Read-only survey: lists every still-unnamed (sub_XXXXX) function in the
current IDB with its size and the names of any ALREADY-NAMED functions
that call it -- lets us pick good next targets (small, called from
well-understood callers) without re-reading huge swaths of .asm by hand.
"""

import idc
import idautils
import ida_funcs
import ida_xref


def callers_of(ea):
    names = []
    for xref in idautils.XrefsTo(ea, 0):
        if xref.type in (ida_xref.fl_CN, ida_xref.fl_CF, ida_xref.fl_JN, ida_xref.fl_JF):
            caller_func = ida_funcs.get_func(xref.frm)
            if caller_func:
                nm = idc.get_func_name(caller_func.start_ea)
                if nm not in names:
                    names.append(nm)
    return names


def main():
    rows = []
    for ea in idautils.Functions():
        fname = idc.get_func_name(ea)
        if not fname.startswith("sub_"):
            continue
        f = ida_funcs.get_func(ea)
        size = f.end_ea - f.start_ea if f else 0
        callers = callers_of(ea)
        named_callers = [c for c in callers if not c.startswith("sub_")]
        rows.append((size, ea, fname, named_callers, callers))

    rows.sort(key=lambda r: r[0])
    for size, ea, fname, named_callers, callers in rows:
        caller_str = ", ".join(named_callers[:4]) if named_callers else ", ".join(callers[:4])
        print(f"{ea:#06x} {fname:12s} size={size:4d}  callers=[{caller_str}]")


if __name__ == "__main__":
    main()
