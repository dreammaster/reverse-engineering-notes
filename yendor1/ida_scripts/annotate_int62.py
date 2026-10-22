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

from vid_api import VID_FUNCS

stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"int62_report_{stem}.txt")

from ida_util import find_ah


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
