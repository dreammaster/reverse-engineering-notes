"""
IDA Pro script: makes the 2 instructions that reference [di+4AFh]
render as `_dungeonCorridorMonsterSlot[di]` instead of the raw
literal, now that name_dungeon_monster_slot_array.py has named that
address.

Gotcha found here (2026-08-19): naming a data item at a previously-
undefined address does NOT retroactively reformat other instructions
that reference it via a small segment-relative displacement (like
`[di+4AFh]`, where 0x4AF is an offset *within* the segment, not the
item's linear address) -- confirmed IDA just keeps showing the raw
number, and `ida_bytes.del_items`+`idc.create_insn` (a full redecode)
doesn't fix it either. The actual fix is `idc.op_plain_offset(ea, n,
base_ea)` with `base_ea` = the segment's start address -- this tells
IDA "this operand's value is a displacement from base_ea," so it can
recompute base_ea+displacement, match it against the named item, and
render symbolically. (Note the API lives in `idc`, not `ida_bytes`,
despite `ida_bytes` having the sibling `op_offset`-style helpers for
other operand-marking cases.) Add this call to whatever script
originally creates a similarly-addressed array if this comes up again
-- would have saved a second pipeline run here.

Scope: cosmetic only (display formatting), not a functional change --
safe to run repeatedly, no dry-run needed.
"""

import ida_bytes
import ida_funcs
import ida_segment
import ida_ua
import idautils
import idc

TARGET_DISP = 0x4AF
FUNCS = ["precompute_dungeon_corridor", "draw_dungeon_monster"]


def refresh_matching_insns(func_ea, seg_base):
    pfn = ida_funcs.get_func(func_ea)
    if pfn is None:
        print(f"[!] function at {func_ea:X} not found")
        return
    for ea in idautils.FuncItems(pfn.start_ea):
        insn = ida_ua.insn_t()
        if ida_ua.decode_insn(insn, ea) == 0:
            continue
        text = idc.GetDisasm(ea).upper()
        if f"{TARGET_DISP:X}H" not in text:
            continue
        print(f"{ea:X}: before: {text}")
        for n in range(2):
            op = insn.ops[n]
            if op.type != ida_ua.o_displ:
                continue
            if op.addr != TARGET_DISP:
                continue
            ok = idc.op_plain_offset(ea, n, seg_base)
            print(f"    op_plain_offset(n={n}, base={seg_base:X}) -> {ok}")
        print(f"{ea:X}: after:  {idc.GetDisasm(ea)}")


def main():
    seg = ida_segment.get_segm_by_name("DATA")
    if seg is None:
        print("[!] DATA segment not found")
        return
    for name in FUNCS:
        ea = idc.get_name_ea_simple(name)
        if ea == idc.BADADDR:
            print(f"[!] {name} not found")
            continue
        refresh_matching_insns(ea, seg.start_ea)


if __name__ == "__main__":
    main()
