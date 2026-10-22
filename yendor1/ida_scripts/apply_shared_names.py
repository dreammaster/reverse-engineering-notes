"""
Rename functions in this database that match an entry of shared_names.json by their
normalised instruction fingerprint (the same function the other programs carry).
Only functions still named sub_XXXXX are renamed; existing names and comments win.
Regenerate shared_names.json after naming more of SW's runtime (see docs1/program-graph.md).

    .\run_ida_script.ps1 <name> apply_shared_names.py
"""
import hashlib
import json
import os

import idc
import idautils
import ida_funcs
import ida_name

here = os.path.dirname(os.path.abspath(__file__))
names = json.load(open(os.path.join(here, "shared_names.json")))

OPKIND = {idc.o_reg: "R", idc.o_mem: "M", idc.o_phrase: "P", idc.o_displ: "D", idc.o_imm: "I", idc.o_far: "T", idc.o_near: "T"}


def fingerprint(f):
    parts = []
    for h in idautils.FuncItems(f):
        mn = idc.print_insn_mnem(h).lower()
        ops = []
        for i in range(3):
            t = idc.get_operand_type(h, i)
            if t == idc.o_void:
                break
            if t == idc.o_reg:
                ops.append("R:" + idc.print_operand(h, i).lower())
            elif t in (idc.o_phrase, idc.o_displ):
                txt = idc.print_operand(h, i).lower()
                keep = "".join(c for c in txt if c.isalpha() or c in "[]+:")
                ops.append("D:" + keep.replace("byteptr", "").replace("wordptr", ""))
            else:
                ops.append(OPKIND.get(t, "?"))
        parts.append(mn + " " + ",".join(ops))
    return hashlib.sha1("\n".join(parts).encode()).hexdigest()[:12]


n = 0
for f in list(idautils.Functions()):
    cur = idc.get_name(f)
    if not cur.startswith("sub_"):
        continue
    e = names.get(fingerprint(f))
    if not e:
        continue
    new = e["name"]
    if ida_name.set_name(f, new, ida_name.SN_NOWARN | ida_name.SN_NOCHECK):
        n += 1
    elif ida_name.set_name(f, new + "_%X" % f, ida_name.SN_NOWARN):
        n += 1
print("renamed", n, "functions from shared_names.json")
