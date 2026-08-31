"""Targeted follow-up to coerce_code.py for dun.idb.

A slice of DUN.EXE's seg000 back half is still raw `db` bytes --
compiled-BASIC procs in the daisy-chained `jmp -> mov cx,N ->
basProcEnter -> body` form that the generic sweep skipped (and the full
coerce_code.py re-run loops on this module). Every one of these `db`
runs sits *inside* an already-carved function boundary, so this just
undefines each affected function's body and re-decodes it linearly,
keeping the function.

    .\run_ida_script.ps1 -Idb dun -ScriptName fix_dun_coerce_gaps.py

Run order:  fix_dun_coerce_gaps -> resolve_thunks -> apply_renames_dun
            -> fix_dun_loaddungeondata -> resolve_thunks
"""
import idc
import idautils
import ida_bytes
import ida_funcs
import ida_auto
import ida_xref

# (start_ea, end_ea) of the functions that still carry undefined bytes,
# from diag_dun_gaps.py. loadDungeonData (0x12E9B) is already fixed.
RANGES = [
    (0x12814, 0x1299A),   # findJewel
    (0x1299A, 0x129E3),   # sub_1299A
    (0x12A13, 0x12ACA),   # sub_12A13
    (0x12B5A, 0x12C34),   # sub_12B5A
    (0x12C34, 0x12CE7),   # rollChestContents
    (0x12D93, 0x12E52),   # sub_12D93
    (0x12E52, 0x12E7D),   # sub_12E52
    (0x12E7D, 0x12E9B),   # sub_12E7D
    (0x12F9F, 0x1305C),   # sub_12F9F
    (0x1305C, 0x1320C),   # rebuildLevelView  (mostly done; mops up the tail)
    (0x13406, 0x1345D),   # showHitPoints  (+ a stuck daisy chunk)
    (0x1345D, 0x13571),   # monsterSpecialAttack (+ stuck chunks)
    (0x139FC, 0x13AC7),   # sub_139FC
]


def coerce(lo, hi):
    ida_bytes.del_items(lo, ida_bytes.DELIT_SIMPLE | ida_bytes.DELIT_NOTRUNC,
                        hi - lo)
    a, n = lo, 0
    while a < hi:
        ln = idc.create_insn(a)
        if ln <= 0:
            ida_bytes.del_items(a, ida_bytes.DELIT_SIMPLE, 1)
            ln = idc.create_insn(a)
        if ln <= 0:
            a += 1
            continue
        n += 1
        # add a fall-through cref past every `call far` so IDA's overlay
        # special-casing doesn't chop the block after each thunk call
        if (ida_bytes.get_byte(a) == 0x9A):
            ida_xref.add_cref(a, a + ln, ida_xref.fl_F | ida_xref.XREF_USER)
        a += ln
    return n


def main():
    total = 0
    for lo, hi in RANGES:
        fn = ida_funcs.get_func(lo)
        name = idc.get_name(lo)
        if fn:
            ida_funcs.del_func(fn.start_ea)
        n = coerce(lo, hi)
        ida_auto.auto_wait()
        ida_funcs.add_func(lo, hi)
        if name and not name.startswith(("loc_", "unk_", "byte_")):
            idc.set_name(lo, name, idc.SN_NOWARN | idc.SN_CHECK)
        total += n
        print(f"  {lo:#08x}-{hi:#08x}  {name:22} +{n} insns")
    ida_auto.auto_wait()
    print(f"total: +{total} insns")


main()
