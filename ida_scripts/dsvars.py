"""
Read-only: profile the DGROUP (module-scope) variables a compiled-BASIC
module's code touches.

Compiled BASIC keeps its module-scope variables at fixed offsets in the
DGROUP data segment, so in the listing they show up as `ds:<off>` memory
operands. This walks every instruction in the code segment, and for each
distinct `ds:<off>` that lands in the DGROUP segment reports:

  * read count / write count
  * the immediate constants stored into it (with tallies)
  * which named functions reference it

High read-count vars shared across many functions are the real engine
state; a var written once with a constant by a single function is almost
always a per-call BASIC scratch temp (the compiler stages an argument in
DGROUP, pushes its address, and calls a runtime routine).

Feeds `apply_dsvars_<module>.py`. Segment names default to
seg000 (code) / seg003 (DGROUP); override via the env vars
DSV_CODE_SEG / DSV_DATA_SEG.

    .\run_ida_script.ps1 -Idb out -ScriptName dsvars.py -NoExport
"""

import os
from collections import defaultdict

import idc
import idautils
import ida_bytes
import ida_segment
import ida_ua

CODE_SEG = os.environ.get("DSV_CODE_SEG", "seg000")
DATA_SEG = os.environ.get("DSV_DATA_SEG", "seg003")

WRITE_MNEMS = ("mov", "and", "or", "add", "sub", "xor", "inc", "dec",
               "shl", "shr", "neg", "not", "adc", "sbb")


def main():
    s0 = ida_segment.get_segm_by_name(CODE_SEG)
    s3 = ida_segment.get_segm_by_name(DATA_SEG)
    if not s0 or not s3:
        print(f"missing segment ({CODE_SEG!r} / {DATA_SEG!r})")
        return
    dg_base = s3.start_ea
    print(f"{CODE_SEG} {s0.start_ea:#x}-{s0.end_ea:#x}   "
          f"{DATA_SEG}(DGROUP) {s3.start_ea:#x}-{s3.end_ea:#x}")

    info = defaultdict(lambda: {"r": 0, "w": 0, "funcs": set(),
                                "consts": defaultdict(int)})

    for f in idautils.Functions(s0.start_ea, s0.end_ea):
        fn = idc.get_func_name(f)
        for ea in idautils.Heads(f, idc.get_func_attr(f, idc.FUNCATTR_END)):
            if not ida_bytes.is_code(ida_bytes.get_full_flags(ea)):
                continue
            insn = ida_ua.insn_t()
            if ida_ua.decode_insn(insn, ea) == 0:
                continue
            mnem = insn.get_canon_mnem()
            for i, op in enumerate(insn.ops):
                if op.type == ida_ua.o_void:
                    break
                if op.type != ida_ua.o_mem:
                    continue
                off = op.addr
                if not (0 <= off < 0x8000 and s3.start_ea <= dg_base + off < s3.end_ea):
                    continue
                d = info[off]
                d["funcs"].add(fn)
                if i == 0 and mnem in WRITE_MNEMS:
                    d["w"] += 1
                    if mnem == "mov" and insn.ops[1].type == ida_ua.o_imm:
                        d["consts"][insn.ops[1].value] += 1
                else:
                    d["r"] += 1

    rows = sorted(info.items(), key=lambda kv: -(kv[1]["r"] + kv[1]["w"]))
    print(f"\n{len(rows)} distinct DGROUP vars referenced from {CODE_SEG} code\n")
    for off, d in rows:
        consts = ",".join(f"{c:#x}x{n}" for c, n in
                          sorted(d["consts"].items(), key=lambda x: -x[1])[:6])
        fns = sorted(d["funcs"])
        tag = ",".join(fns[:6]) + ("..." if len(fns) > 6 else "")
        cur = idc.get_name(dg_base + off)
        nm = f" {cur}" if cur else ""
        print(f"  ds:{off:04X}{nm}  r{d['r']:<3} w{d['w']:<3}  "
              f"[{len(d['funcs'])} fn]  {{{consts}}}  {tag}")


main()
