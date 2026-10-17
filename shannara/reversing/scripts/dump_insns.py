"""
IDAPython: dumps the decoded instructions of a list of functions as JSON for the handler transpiler (transpile_handlers.py).

    idat.exe -A -S"reversing/scripts/dump_insns.py closure.txt out.json" copy-of-Shannara.idb
        closure.txt: lines "name hexaddress"

Per function: every instruction of every chunk in address order with its operands decoded structurally
(type, size, register, displacement, SIB, immediate), jump-table targets for `jmp [table + reg*4]`, and the IDA text.
"""
import json
import sys

import ida_ua
import idautils
import idc
import ida_bytes
import ida_nalt

O_VOID, O_REG, O_MEM, O_PHRASE, O_DISPL, O_IMM, O_FAR, O_NEAR = 0, 1, 2, 3, 4, 5, 6, 7


def operand(insn, ea, i):
    op = insn.ops[i]
    d = {"t": op.type, "dt": op.dtype}
    if op.type == O_REG:
        d["reg"] = op.reg
    elif op.type in (O_MEM, O_PHRASE, O_DISPL):
        has_sib = bool(op.specflag1 & 1)
        d["addr"] = op.addr & 0xFFFFFFFF
        if has_sib:
            sib = op.specflag2 & 0xFF
            base, index, scale = sib & 7, (sib >> 3) & 7, 1 << (sib >> 6)
            d["sib"] = [base, index, scale]
        else:
            d["phrase"] = op.phrase
    elif op.type == O_IMM:
        d["val"] = op.value & 0xFFFFFFFF
    elif op.type in (O_NEAR, O_FAR):
        d["addr"] = op.addr & 0xFFFFFFFF
    return d


def main():
    names = [l.split() for l in open(idc.ARGV[1]).read().split("\n") if l.strip()]
    out = {}
    for name, hx in names:
        fea = int(hx, 16)
        insns = []
        for h in idautils.FuncItems(fea):
            insn = ida_ua.insn_t()
            n = ida_ua.decode_insn(insn, h)
            if n == 0:
                insns.append({"ea": h, "size": 1, "mn": "db", "ops": [], "text": idc.generate_disasm_line(h, 0)})
                continue
            mn = idc.print_insn_mnem(h)
            ops = []
            for i in range(6):
                if insn.ops[i].type == O_VOID:
                    break
                ops.append(operand(insn, h, i))
            rec = {"ea": h, "size": insn.size, "mn": mn, "ops": ops, "text": idc.generate_disasm_line(h, 0)}
            if insn.get_canon_mnem() in ("rep", "repe", "repne") or "rep" in mn:
                rec["prefix"] = mn
            insns.append(rec)
        # jump tables: `jmp cs:table[reg*4]`; list the targets IDA resolved
        for r in insns:
            if r["mn"] == "jmp" and r["ops"] and r["ops"][0]["t"] in (O_MEM, O_DISPL, O_PHRASE) and "sib" in r["ops"][0]:
                base = r["ops"][0]["addr"]
                si = ida_nalt.get_switch_info(r["ea"])
                n = si.get_jtable_size() if si else 0
                if n:
                    r["jt"] = [idc.get_wide_dword(base + 4 * k) for k in range(n)]
                    r["jt_lo"] = si.lowcase
        out[name] = {"ea": fea, "insns": insns}
    json.dump(out, open(idc.ARGV[2], "w"))
    idc.qexit(0)


main()
