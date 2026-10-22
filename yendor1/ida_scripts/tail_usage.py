"""
Map the 127-byte parameter block that SW.EXE passes to every child program in
the DOS command tail (see docs1/ipc.md). In SW.EXE the block is DS:0006; in a
child, start() copies PSP:0080h..00FFh (7Fh bytes) to <data seg>:<di>. This
finds that copy, names each referenced slot parm_XX (XX = offset in the block)
and writes tail_<db>.txt with per-slot read/write counts.

    .\run_ida_script.ps1 <name> tail_usage.py        (renames, re-exports)
"""
import os
import collections

import idc
import idautils
import ida_segment
import ida_xref
import ida_name

stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"tail_{stem}.txt")


def find_base():
    if stem == "sw":
        seg = ida_segment.get_segm_by_name("seg019")
        return ((seg.start_ea >> 4) << 4) + 6, "SW.EXE DS:0006"
    # child: mov ax,seg X / mov es,ax / mov di,imm / mov cx,7Fh / rep movsb
    for seg in idautils.Segments():
        for h in idautils.Heads(seg, idc.get_segm_end(seg)):
            if idc.print_insn_mnem(h).lower() != "mov" or idc.print_operand(h, 0).lower() != "cx":
                continue
            if idc.get_operand_type(h, 1) != idc.o_imm or idc.get_operand_value(h, 1) != 0x7F:
                continue
            nxt = idc.next_head(h)
            if "movsb" not in idc.GetDisasm(nxt).lower():
                continue
            di = sel = None
            cur = h
            for _ in range(10):
                cur = idc.prev_head(cur)
                m = idc.print_insn_mnem(cur).lower()
                if m == "mov" and idc.print_operand(cur, 0).lower() == "di" and di is None:
                    di = idc.get_operand_value(cur, 1)
                if m == "mov" and idc.print_operand(cur, 0).lower() == "ax" and "seg " in idc.GetDisasm(cur):
                    sel = idc.get_operand_value(cur, 1)
            if di is not None and sel is not None:
                return (sel << 4) + di, f"copied to seg {sel:#x}:{di:#x}"
    return None, "tail copy not found"


base, how = find_base()
rows = []
if base is not None:
    for k in range(0x80):
        ea = base + k
        refs = [x for x in idautils.XrefsTo(ea, ida_xref.XREF_DATA)]
        if not refs:
            continue
        r = sum(1 for x in refs if x.type == ida_xref.dr_R)
        w = sum(1 for x in refs if x.type == ida_xref.dr_W)
        o = len(refs) - r - w
        old = idc.get_name(ea)
        if not old or old.startswith(("byte_", "word_", "unk_", "dword_", "off_")):
            ida_name.set_name(ea, f"parm_{k:02X}", ida_name.SN_NOWARN | ida_name.SN_NOCHECK)
        rows.append((k, idc.get_name(ea), r, w, o, len({idc.get_func_name(x.frm) for x in refs})))
with open(OUT, "w") as fh:
    fh.write(f"{stem}: block base {base if base is None else hex(base)} ({how})\n")
    for k, n, r, w, o, nf in rows:
        fh.write(f"  +{k:02X} {n:14s} r={r:3d} w={w:3d} other={o:3d} funcs={nf}\n")
print(stem, how, len(rows), "slots")
