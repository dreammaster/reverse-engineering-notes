"""Targeted coerce for dun.idb: monsterAttack's normal-hit loop body.

`monsterAttack` (the monsters' turn) carries a raw `db` run from just
after `loc_1154B: call far monsterSpecialAttack` down to `loc_11698` --
the compiled-BASIC normal-hit to-hit + damage rolls that the generic
coerce_code.py sweep skipped (it sits inside an already-carved function
and the full re-run loops on dun).

Same approach as fix_dun_coerce_gaps.py: del the function, undefine its
whole body, re-decode linearly with a fall-through cref past every
`call far`, re-add the function.  Also mops up monsterSpecialAttack.

    .\run_ida_script.ps1 -Idb dun -ScriptName fix_dun_monsterattack.py

Run order:  fix_dun_monsterattack -> resolve_thunks -> apply_renames_dun
"""
import idc
import idautils
import ida_bytes
import ida_funcs
import ida_auto
import ida_xref

TARGETS = ["monsterAttack", "monsterSpecialAttack"]


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
        if ida_bytes.get_byte(a) == 0x9A:          # call far ptr
            ida_xref.add_cref(a, a + ln, ida_xref.fl_F | ida_xref.XREF_USER)
        a += ln
    return n


def body_stats(lo, hi):
    ins = und = dat = 0
    for a in range(lo, hi):
        fl = ida_bytes.get_full_flags(a)
        if ida_bytes.is_code(fl):
            ins += 1
        elif ida_bytes.is_unknown(fl):
            und += 1
        else:
            dat += 1
    return ins, und, dat


def main():
    for name in TARGETS:
        ea = idc.get_name_ea_simple(name)
        if ea == idc.BADADDR:
            print(f"  {name}: not found, skipping")
            continue
        f = ida_funcs.get_func(ea)
        lo, hi = f.start_ea, f.end_ea
        i0, u0, d0 = body_stats(lo, hi)
        ida_funcs.del_func(lo)
        n = coerce(lo, hi)
        ida_auto.auto_wait()
        ida_funcs.add_func(lo, hi)
        idc.set_name(lo, name, idc.SN_NOWARN | idc.SN_CHECK)
        i1, u1, d1 = body_stats(lo, hi)
        print(f"  {name:22} {lo:#08x}-{hi:#08x}  "
              f"insn {i0}->{i1}  undef {u0}->{u1}  (+{n} decoded)")
    ida_auto.auto_wait()


main()
