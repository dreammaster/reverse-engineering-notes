"""Coerce the raw db runs in TWNDR's guard / bribe / steal handlers.

offerGuardBribe / stealGold / arrestedByGuards / mailDeliveryJob carry
`db` gaps (IDA chops the block after every far call).  Same fix as
fix_dun_spells / fix_mus_caretaker.

NOTE: including `townServiceDispatch` (a ~5.7 KB dispatcher) in TARGETS
makes IDA re-flow half the module -> a ~16k-line twndr.asm diff for
~400 decoded bytes.  Leave it out unless you actually need that export;
the shop/sell math was read directly from the coerced idb without
re-exporting.

    .\run_ida_script.ps1 -Idb twndr -ScriptName fix_twndr_guard.py
"""
import idc
import idautils
import ida_bytes
import ida_funcs
import ida_auto
import ida_xref

TARGETS = ["offerGuardBribe", "stealGold", "arrestedByGuards",
           "mailDeliveryJob"]
# ("foodShop" and "townServiceDispatch" also carry gaps but re-flow the
#  whole module on export -- add them back only if you need that.)


def coerce(lo, hi):
    ida_bytes.del_items(lo, ida_bytes.DELIT_SIMPLE | ida_bytes.DELIT_NOTRUNC,
                        hi - lo)
    a = lo
    while a < hi:
        ln = idc.create_insn(a)
        if ln <= 0:
            ida_bytes.del_items(a, ida_bytes.DELIT_SIMPLE, 1)
            ln = idc.create_insn(a)
        if ln <= 0:
            a += 1
            continue
        if ida_bytes.get_byte(a) == 0x9A:
            ida_xref.add_cref(a, a + ln, ida_xref.fl_F | ida_xref.XREF_USER)
        a += ln


def body_stats(lo, hi):
    ins = und = 0
    for a in range(lo, hi):
        fl = ida_bytes.get_full_flags(a)
        if ida_bytes.is_code(fl):
            ins += 1
        elif ida_bytes.is_unknown(fl):
            und += 1
    return ins, und


def main():
    for name in TARGETS:
        ea = idc.get_name_ea_simple(name)
        if ea == idc.BADADDR:
            print(f"  {name}: not found")
            continue
        f = ida_funcs.get_func(ea)
        lo, hi = f.start_ea, f.end_ea
        i0, u0 = body_stats(lo, hi)
        if u0 == 0:
            print(f"  {name}: already clean")
            continue
        ida_funcs.del_func(lo)
        coerce(lo, hi)
        ida_auto.auto_wait()
        ida_funcs.add_func(lo, hi)
        idc.set_name(lo, name, idc.SN_NOWARN | idc.SN_CHECK)
        i1, u1 = body_stats(lo, hi)
        print(f"  {name:22} {lo:#x}-{hi:#x}  insn {i0}->{i1}  undef {u0}->{u1}")
    ida_auto.auto_wait()


main()
