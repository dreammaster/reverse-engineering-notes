"""
Relocation-independent function fingerprints: for every function, a hash of its
instruction stream with operands normalised (registers kept, immediates -> I,
memory references -> M, near/far code targets -> T). Two copies of the same
routine linked at different addresses/segments hash the same. Written to
norm_<db>.txt as "<start> <size> <ninsn> <hash> <name>".

    .\run_ida_script.ps1 <name> norm_hashes.py -NoExport
"""
import hashlib
import os

import idc
import idautils
import ida_funcs

stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"norm_{stem}.txt")

OPKIND = {idc.o_void: "", idc.o_reg: "R", idc.o_mem: "M", idc.o_phrase: "P", idc.o_displ: "D",
          idc.o_imm: "I", idc.o_far: "T", idc.o_near: "T"}

with open(OUT, "w") as fh:
    for f in idautils.Functions():
        fn = ida_funcs.get_func(f)
        parts = []
        n = 0
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
                    # keep the register part of [bx+si+..] but drop the displacement value
                    txt = idc.print_operand(h, i).lower()
                    keep = "".join(c for c in txt if c.isalpha() or c in "[]+:")
                    ops.append("D:" + keep.replace("byteptr", "").replace("wordptr", ""))
                else:
                    ops.append(OPKIND.get(t, "?"))
            # a call/jmp to another function is just a target
            parts.append(mn + " " + ",".join(ops))
            n += 1
        hsh = hashlib.sha1("\n".join(parts).encode()).hexdigest()[:12]
        fh.write(f"{f:#x} {fn.end_ea - fn.start_ea} {n} {hsh} {idc.get_func_name(f)}\n")
print(stem, "normalised hashes written")
