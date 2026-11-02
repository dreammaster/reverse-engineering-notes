"""
Read-only report: one line per function with its size, callers, callees and the
strings it references, written to <idb-stem>_functions.txt in the scratch folder
given by the DUMP_DIR environment variable (default: next to the idb).  This is
the main way to find functions by the strings and library calls they use.

    $env:DUMP_DIR = "C:\\temp"; .\run_ida_script.ps1 -Idb u6_game -ScriptName dump_functions.py -NoExport
"""

import os

import idautils
import idc
import ida_bytes
import ida_funcs
import ida_nalt
import ida_segment

out_dir = os.environ.get("DUMP_DIR") or os.path.dirname(idc.get_idb_path())
stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
path = os.path.join(out_dir, stem + "_functions.txt")


def string_at(ea):
    if ea is None or not ida_bytes.is_strlit(ida_bytes.get_flags(ea)):
        return None
    s = ida_bytes.get_strlit_contents(ea, -1, ida_nalt.STRTYPE_C)
    if not s:
        return None
    return s.decode("latin1")


dseg = ida_segment.get_segm_by_name("dseg")


def string_at_dseg_offset(v):
    """Strings are referenced as `mov ax, <offset in dseg>` (large model, no offset xrefs)."""
    if dseg is None or v < 0x10 or dseg.start_ea + v >= dseg.end_ea:
        return None
    ea = dseg.start_ea + v
    if ida_bytes.get_byte(ea - 1) != 0:
        return None
    raw = ida_bytes.get_bytes(ea, 80)
    if not raw:
        return None
    end = raw.find(bytes([0]))
    if end < 4 or any(c < 32 or c > 126 for c in raw[:end] if c not in (9, 10, 13)):
        return None
    return raw[:end].decode("latin1")


lines = []
for ea in idautils.Functions():
    f = ida_funcs.get_func(ea)
    callers = set()
    for x in idautils.CodeRefsTo(ea, 0):
        cf = ida_funcs.get_func(x)
        callers.add(idc.get_func_name(cf.start_ea) if cf else "%X" % x)
    callees = set()
    strings = []
    for head in idautils.FuncItems(ea):
        for x in idautils.CodeRefsFrom(head, 0):
            cf = ida_funcs.get_func(x)
            if cf and cf.start_ea != ea and cf.start_ea == x:
                callees.add(idc.get_func_name(x))
        for x in idautils.DataRefsFrom(head):
            s = string_at(x)
            if s:
                strings.append(s)
        if idc.print_insn_mnem(head) in ("mov", "push", "lea") and idc.get_operand_type(head, 1 if idc.print_insn_mnem(head) != "push" else 0) == idc.o_imm:
            v = idc.get_operand_value(head, 1 if idc.print_insn_mnem(head) != "push" else 0)
            s = string_at_dseg_offset(v)
            if s and s not in strings:
                strings.append(s)
    seg = ida_segment.get_segm_name(ida_segment.getseg(ea))
    lines.append("%05X %s size=%d seg=%s callers=%d\n    calls: %s\n    strings: %s" % (
        ea, idc.get_func_name(ea), f.end_ea - f.start_ea, seg, len(callers),
        ", ".join(sorted(callees)), " | ".join(repr(s) for s in strings[:12])))

with open(path, "w", encoding="utf-8") as fh:
    fh.write("\n".join(lines))
print("wrote %d functions to %s" % (len(lines), path))
