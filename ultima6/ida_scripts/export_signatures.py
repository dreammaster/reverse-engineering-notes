"""
Read-only: exports a position-independent signature for every function so the
same routine can be recognised in the other executables (Ultima 6 / Martian
Dreams / Savage Empire share most of their engine).  Written to
<DUMP_DIR>/<idb-stem>_sigs.json, consumed by tools/match_functions.py.

The signature is the instruction stream with everything that moves between
builds masked out: mnemonics and operand kinds are kept, small immediates and
displacements are kept, addresses (calls, jumps, data) and large immediates are
not.

    $env:DUMP_DIR = "C:\\temp"; .\run_ida_script.ps1 -Idb u6_game -ScriptName export_signatures.py -NoExport
"""

import hashlib
import json
import os

import idautils
import idc
import ida_funcs
import ida_segment

out_dir = os.environ.get("DUMP_DIR") or os.path.dirname(idc.get_idb_path())
stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]

O_REG, O_MEM, O_PHRASE, O_DISPL, O_IMM, O_FAR, O_NEAR = 1, 2, 3, 4, 5, 6, 7


def operand_token(ea, n):
    t = idc.get_operand_type(ea, n)
    if t == idc.o_void:
        return ""
    if t == O_REG:
        return "r" + idc.print_operand(ea, n)
    if t == O_IMM:
        v = idc.get_operand_value(ea, n)
        return "i%X" % v if v < 0x100 else "I"
    if t == O_DISPL:
        v = idc.get_operand_value(ea, n)
        txt = idc.print_operand(ea, n).split("+")[0]
        return "d%s%X" % (txt[:3], v) if v < 0x100 or v > 0xFF00 else "D" + txt[:3]
    if t in (O_FAR, O_NEAR):
        return "C"
    if t == O_MEM:
        return "M"
    return "p"


def signature(f):
    toks = []
    for ea in idautils.FuncItems(f.start_ea):
        toks.append(idc.print_insn_mnem(ea) + " " + ",".join(operand_token(ea, n) for n in range(2)))
    return toks


def callees(f):
    out = []
    for ea in idautils.FuncItems(f.start_ea):
        for x in idautils.CodeRefsFrom(ea, 0):
            cf = ida_funcs.get_func(x)
            if cf and cf.start_ea == x and x != f.start_ea:
                out.append(x)
    return out


funcs = []
for start in idautils.Functions():
    f = ida_funcs.get_func(start)
    toks = signature(f)
    funcs.append({
        "ea": start,
        "name": idc.get_func_name(start),
        "size": f.end_ea - f.start_ea,
        "nins": len(toks),
        "seg": ida_segment.get_segm_name(ida_segment.getseg(start)),
        "hash": hashlib.sha1("\n".join(toks).encode()).hexdigest()[:16],
        "callees": callees(f),
    })

with open(os.path.join(out_dir, stem + "_sigs.json"), "w") as fh:
    json.dump(funcs, fh)
print("wrote %d signatures" % len(funcs))
