"""
Label every `int 62h` call site in the open database with the VID16.DRV function
it invokes (comment "VID fnNN name"), by scanning backwards for the instruction
that last loads AH. Writes int62_report_<db>.txt next to this script:
a per-function histogram plus the sites whose AH could not be resolved.

Resolved forms: mov ah,imm / mov ax,imm / xor ah,ah / xor ax,ax / sub ah,ah.
Anything else that writes AH/AX (mov ah,mem, a call, a return, an unconditional
jump) ends the scan as unresolved. Conditional jumps and labels are scanned
through; a site reached through a label is flagged "(shared)" in the report
since a second path into it could load a different AH.

    .\run_ida_script.ps1 <name> annotate_int62.py        (re-exports the db)
"""
import collections
import os

import idc
import idautils
import ida_funcs
import ida_xref

from vid_api import VID_FUNCS

stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"int62_report_{stem}.txt")

STOP = {"call", "retn", "retf", "ret", "jmp", "iret", "int"}


def reg(ea, n):
    return idc.print_operand(ea, n).strip().lower()


def find_ah(ea):
    """Return (ah or None, why, shared)."""
    cur = ea
    shared = False
    for _ in range(24):
        cur = idc.prev_head(cur)
        if cur == idc.BADADDR:
            return None, "start of segment", shared
        # a label with an incoming jump between here and the int
        nxt = idc.next_head(cur)
        for xr in idautils.XrefsTo(nxt, ida_xref.XREF_FAR):
            if xr.type in (ida_xref.fl_JN, ida_xref.fl_JF):
                shared = True
        mn = idc.print_insn_mnem(cur).lower()
        if mn in STOP:
            # a `jmp` or `call` before us means the load is on another path
            return None, f"hit {mn} at {cur:#x}", shared
        if mn not in ("mov", "xor", "sub", "lea", "pop", "inc", "dec", "add", "and", "or", "shl",
                      "shr", "xchg", "cbw", "cwd", "lodsb", "lodsw", "in", "neg", "not", "adc", "sbb", "imul", "mul", "div"):
            continue
        d = reg(cur, 0)
        if mn in ("cbw",):
            return None, f"cbw at {cur:#x}", shared
        if d not in ("ah", "ax", "eax"):
            if mn in ("xchg",) and reg(cur, 1) in ("ah", "ax"):
                return None, f"xchg at {cur:#x}", shared
            if mn in ("lodsb", "lodsw", "mul", "div", "imul", "in"):
                return None, f"{mn} at {cur:#x}", shared
            continue
        if mn == "mov" and idc.get_operand_type(cur, 1) == idc.o_imm:
            v = idc.get_operand_value(cur, 1)
            return (v if d == "ah" else (v >> 8) & 0xFF), "", shared
        if mn in ("xor", "sub") and reg(cur, 1) == d:
            return 0, "", shared
        return None, f"{mn} {d} at {cur:#x}", shared
    return None, "no load found in 24 insns", shared


hist = collections.Counter()
bad = []
sites = 0
for seg_ea in idautils.Segments():
    for h in idautils.Heads(seg_ea, idc.get_segm_end(seg_ea)):
        if idc.print_insn_mnem(h).lower() != "int" or idc.get_operand_value(h, 0) != 0x62:
            continue
        sites += 1
        ah, why, shared = find_ah(h)
        fn = ida_funcs.get_func(h)
        fname = idc.get_func_name(h) if fn else "?"
        if ah is None:
            bad.append((h, fname, why))
            idc.set_cmt(h, "VID fn?? (AH not resolved: %s)" % why, 0)
            continue
        hist[ah] += 1
        name = VID_FUNCS.get(ah, ("fn%02X_unknown" % ah, ""))[0]
        idc.set_cmt(h, "VID fn%02X %s%s" % (ah, name, " (shared)" if shared else ""), 0)

with open(OUT, "w") as fh:
    fh.write(f"{stem}: {sites} int 62h sites, {sum(hist.values())} resolved, {len(bad)} unresolved\n")
    for ah, n in sorted(hist.items()):
        fh.write(f"  fn{ah:02X} {VID_FUNCS.get(ah, ('?',))[0]:28s} {n}\n")
    if bad:
        fh.write("unresolved:\n")
        for ea, fname, why in bad:
            fh.write(f"  {ea:#08x} in {fname}: {why}\n")
print(f"{stem}: {sites} sites, {len(bad)} unresolved -> {OUT}")
