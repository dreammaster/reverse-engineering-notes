"""Helpers shared by the MM2 IDA scripts (import via exec/sys.path from __file__)."""
import os
import sys

import ida_bytes
import ida_segment
import idc

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(_HERE), "tools"))
import mm2_layout  # noqa: E402

BASE = mm2_layout.IDA_BASE_PARA * 16          # linear address of load-base:0
CODE_SEL = 0x1000                             # IDA selector of the resident code segment
DGROUP_SEL = 0x1D85                           # relative 0D85h + IDA base
OVL_A = (0x17E10, 0x1C130)                    # overlay window shared by 1MENU2/2COMBAT/2PLAY
OVL_B = (0x1C130, 0x1D850)                    # overlay window shared by the 11 others
DGROUP = (0x1D850, 0x28220)                   # initialised data + BSS (0A9D0h bytes)
STACK = (0x28220, 0x28A20)


def layout():
    return mm2_layout.Layout(exe_path=idc.get_input_file_path())


def load_bytes(ea, blob):
    ida_bytes.put_bytes(ea, blob)


def apply_relocs(rec_relocs, body_ea, load_para_base):
    """Apply Plink86 fixups: each (offset, seg) adds the load base to the word at seg:offset."""
    n = 0
    for off, seg in rec_relocs:
        ea = (seg + mm2_layout.IDA_BASE_PARA) * 16 + off
        idc.patch_word(ea, (idc.get_wide_word(ea) + mm2_layout.IDA_BASE_PARA) & 0xFFFF)
        n += 1
    return n


def resolve_string_offsets(start, end):
    """Turn `mov r16, imm` / `push imm` / `mov [mem], imm` whose immediate is the offset of a
    defined string in DGROUP into a proper offset operand (creates the xref, shows `offset aFoo`).
    Returns the number of operands converted."""
    import idautils, ida_offset
    n = 0
    for ea in idautils.Heads(start, end):
        if not ida_bytes.is_code(ida_bytes.get_flags(ea)):
            continue
        mnem = idc.print_insn_mnem(ea)
        if mnem not in ("mov", "push"):
            continue
        for op in (0, 1):
            if idc.get_operand_type(ea, op) != idc.o_imm:
                continue
            v = idc.get_operand_value(ea, op)
            if not 0x100 <= v < DGROUP[1] - DGROUP[0]:
                continue
            tgt = DGROUP[0] + v
            if ida_bytes.is_strlit(ida_bytes.get_flags(tgt)):
                if ida_offset.op_plain_offset(ea, op, DGROUP[0]):
                    n += 1
    return n


def fix_jump_tables(start, end):
    """IDA sometimes truncates `jmp cs:table[bx]` switch tables (it stopped at 8 of 50 cases in
    2PLAY).  Re-derive the case count from the bounds check (`cmp reg, N` + ja/jbe/jnb) that
    precedes the jump, define the table as offset words and make every target code."""
    import idautils, ida_offset, ida_ua, ida_funcs, ida_auto
    fixed = 0
    for ea in idautils.Heads(start, end):
        if idc.print_insn_mnem(ea) != "jmp" or "cs:" not in idc.generate_disasm_line(ea, 0):
            continue
        if idc.get_operand_type(ea, 0) not in (idc.o_mem, idc.o_displ, idc.o_phrase):
            continue
        table = idc.get_operand_value(ea, 0)
        if not table:
            continue
        # look back for `cmp r, N`
        n = None
        p = ea
        for _ in range(8):
            p = idc.prev_head(p)
            if p == idc.BADADDR:
                break
            if idc.print_insn_mnem(p) == "cmp" and idc.get_operand_type(p, 1) == idc.o_imm:
                n = idc.get_operand_value(p, 1) + 1
                break
        if not n or n > 256:
            continue
        base = 0x10000
        tea = base + table
        for i in range(n):
            a = tea + 2 * i
            idc.del_items(a, idc.DELIT_SIMPLE, 2)
            idc.create_word(a)
            ida_offset.op_plain_offset(a, 0, base)
            tgt = base + idc.get_wide_word(a)
            if start <= tgt < end:
                idc.create_insn(tgt)
        idc.set_name(tea, "jpt_%X" % tea, idc.SN_NOCHECK | idc.SN_NOWARN | idc.SN_AUTO) if not idc.get_name(tea) else None
        fixed += 1
    ida_auto.auto_wait()
    return fixed
