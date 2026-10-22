"""
Find the function(s) in this database that perform DOS EXEC (INT 21h AH=4Bh),
name them execProgram (if still unnamed), and comment every call site with the
program file it launches. The call convention (seen in SW.EXE) is
`mov dx, <offset of ASCIZ name in the data segment>; call execProgram`; the
string is found by trying every segment paragraph as the base.

    .\run_ida_script.ps1 <name> annotate_exec.py
"""
import os
import re

import idc
import idautils
import ida_funcs
import ida_bytes
import ida_segment
import ida_xref

from ida_util import find_ah

stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"exec_{stem}.txt")

# 1. functions containing int 21h / AH=4Bh
execs = set()
for seg in idautils.Segments():
    for h in idautils.Heads(seg, idc.get_segm_end(seg)):
        if idc.print_insn_mnem(h).lower() == "int" and idc.get_operand_value(h, 0) == 0x21:
            ah, _, _ = find_ah(h)
            if ah == 0x4B:
                f = ida_funcs.get_func(h)
                if f:
                    execs.add(f.start_ea)

bases = sorted({(ida_segment.getnseg(i).start_ea >> 4) << 4 for i in range(ida_segment.get_segm_qty())})
pat = re.compile(r"^[A-Za-z0-9_\.:]{1,40}\.(EXE|COM|DRV|BAT)\b", re.I)


def string_at(off):
    for b in bases:
        s = ida_bytes.get_strlit_contents(b + off, -1, idc.STRTYPE_C)
        if s and pat.match(s.decode("latin1")):
            return s.decode("latin1")
    return None


lines = []
for f in sorted(execs):
    if idc.get_name(f).startswith("sub_"):
        idc.set_name(f, "execProgram", idc.SN_NOWARN)
    idc.set_func_cmt(f, "DOS EXEC (INT 21h AH=4Bh): runs the program whose ASCIZ name is at DS:DX", 1)
    lines.append(f"exec helper: {idc.get_name(f)} at {f:#x}")
    for xr in idautils.XrefsTo(f, ida_xref.XREF_ALL):
        if xr.type not in (ida_xref.fl_CN, ida_xref.fl_CF):
            continue
        site = xr.frm
        # nearest preceding `mov dx, imm` in the same function
        cur, name = site, None
        for _ in range(12):
            cur = idc.prev_head(cur)
            if cur == idc.BADADDR:
                break
            if idc.print_insn_mnem(cur).lower() in ("mov", "lea") and idc.print_operand(cur, 0).lower() == "dx" \
                    and idc.get_operand_type(cur, 1) in (idc.o_imm, idc.o_mem, idc.o_displ):
                name = string_at(idc.get_operand_value(cur, 1) & 0xFFFF)
                if name is None:
                    name = "?" + idc.print_operand(cur, 1)
                break
        idc.set_cmt(site, f"EXEC {name}" if name else "EXEC (target not resolved)", 0)
        fn = idc.get_func_name(site)
        lines.append(f"  {site:#08x} in {fn}: {name}")

with open(OUT, "w") as fh:
    fh.write("\n".join(lines) + "\n" if lines else "no EXEC helper\n")
print(stem, "exec helpers:", len(execs))
