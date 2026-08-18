"""
IDA Pro script: names the 8-byte per-frame scratch array at DATA+0x4AF
that draw_dungeon_monster reads via `[di+4AFh]`.

Background: this array is filled by precompute_dungeon_corridor (asm
~11809) with the low 3 bits of the dungeon tile 8 steps ahead of the
player, one byte per look-ahead depth (di = 0-7) -- the same
"monster present here" bitfield already documented in
docs/file-formats.md's dungeon tile format section (used by
attack/map_get_monster_at? for combat). draw_dungeon_monster
(asm ~12856) reads it back to decide whether to draw a monster marker
at each corridor depth. Found while tracing the MONSTERS master file
(2026-08-19, see docs/file-formats.md's MONSTERS section) -- it has no
symbolic name yet (IDA has no item boundary there at all, unlike
byte_1697E/word_17899/byte_1788F which are already auto-named and get
handled as plain renames in apply_renames.py), so this needs a
structural script like split_map_monsters.py rather than a plain
rename.

The address is a raw segment-literal displacement (`[di+4AFh]`, same
idiom as the _mapMonsters case) -- computed here as DATA segment base
+ 0x4AF rather than hardcoded, so it stays correct if the DATA segment
ever moves.

USAGE: dry-run first (structural/graph-surgery script convention) --
back up the idb, run once with DRY_RUN=True, review, then flip to
False.
"""

import idaapi
import ida_bytes
import ida_funcs
import ida_segment
import ida_ua
import idautils
import idc

DRY_RUN = False

SEG_NAME = "DATA"
SEG_OFFSET = 0x4AF
ARRAY_LEN = 8
ARRAY_NAME = "_dungeonCorridorMonsterSlot"
REFERENCING_FUNCS = ["precompute_dungeon_corridor", "draw_dungeon_monster"]
NOTE = (
    "Per-frame scratch, 8 entries (one per dungeon corridor look-ahead "
    "depth, di=0-7). Filled by precompute_dungeon_corridor from the low "
    "3 bits of the dungeon tile at that depth -- the same monster-"
    "presence bitfield attack/map_get_monster_at? use for combat, just "
    "re-cached here for draw_dungeon_monster's rendering pass. Nonzero "
    "= draw a monster marker at this depth."
)


def get_base_ea():
    seg = ida_segment.get_segm_by_name(SEG_NAME)
    if seg is None:
        print(f"[!] segment {SEG_NAME!r} not found")
        return None
    return seg.start_ea


def fix_operand_display(base_ea):
    """Naming a data item doesn't retroactively reformat other
    instructions that reference it via a small segment-relative
    displacement (e.g. `[di+4AFh]`) -- idc.op_plain_offset(ea, n,
    base_ea) is what makes those render symbolically. See
    refresh_dungeon_monster_slot_operands.py's docstring for the full
    gotcha writeup."""
    for name in REFERENCING_FUNCS:
        func_ea = idc.get_name_ea_simple(name)
        if func_ea == idc.BADADDR:
            print(f"[!] {name} not found -- can't fix its operand display")
            continue
        pfn = ida_funcs.get_func(func_ea)
        for ea in idautils.FuncItems(pfn.start_ea):
            insn = ida_ua.insn_t()
            if ida_ua.decode_insn(insn, ea) == 0:
                continue
            for n in range(2):
                op = insn.ops[n]
                if op.type == ida_ua.o_displ and op.addr == SEG_OFFSET:
                    idc.op_plain_offset(ea, n, base_ea)


def main():
    base_ea = get_base_ea()
    if base_ea is None:
        return
    ea = base_ea + SEG_OFFSET
    print(f"{SEG_NAME} base = {base_ea:X}, target EA = {ea:X}")

    cur_name = idc.get_name(ea)
    cur_size = ida_bytes.get_item_size(ea) if cur_name else 0
    if cur_name == ARRAY_NAME and cur_size == ARRAY_LEN:
        print(f"{ea:X}: already {ARRAY_NAME!r}, {ARRAY_LEN} bytes -- skipping")
        return

    print(f"{ea:X}: {cur_name!r} ({cur_size} bytes) -> {ARRAY_NAME!r} ({ARRAY_LEN} bytes)")
    print(f"    {NOTE}")
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
        return

    if not ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, ARRAY_LEN):
        print(f"    [!] del_items reported failure at {ea:X} (may be harmless)")
    if not ida_bytes.create_data(ea, ida_bytes.FF_BYTE, ARRAY_LEN, idaapi.BADADDR):
        print(f"    [!] create_data FAILED at {ea:X}")
        return
    if idc.get_name(ea) != ARRAY_NAME:
        if not idc.set_name(ea, ARRAY_NAME, idc.SN_NOWARN):
            print(f"    [!] set_name FAILED at {ea:X}")
            return
    idc.set_cmt(ea, NOTE, 0)
    fix_operand_display(base_ea)

    print("\nDone. Re-export the .asm/.idc and check the new name took, "
          "then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
