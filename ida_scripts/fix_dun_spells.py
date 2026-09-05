"""Coerce + unfold the DUN spell-effect code so the spell table reads.

castSpell's `ON (selectedSpell-25) GOTO` table (rt_FD @ ~0x122fb) and the
code after it are still partly `db`; psychoStrengthSpell and
j_clearTurnFlag export as COLLAPSED FUNCTION.  Also sub_124FE.
This clears the fold flags and linearly re-decodes castSpell's tail +
the three arm targets.

    .\run_ida_script.ps1 -Idb dun -ScriptName fix_dun_spells.py
"""
import idc
import idautils
import ida_bytes
import ida_funcs
import ida_auto
import ida_xref

FUNC_HIDDEN = getattr(ida_funcs, "FUNC_HIDDEN", 0x10)


def coerce(lo, hi):
    ida_bytes.del_items(lo, ida_bytes.DELIT_SIMPLE | ida_bytes.DELIT_NOTRUNC, hi - lo)
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


def main():
    # 1. unfold every folded func
    for ea in idautils.Functions():
        f = ida_funcs.get_func(ea)
        if f and (f.flags & FUNC_HIDDEN):
            f.flags &= ~FUNC_HIDDEN
            ida_funcs.update_func(f)
            print("unfolded", idc.get_func_name(ea), hex(ea))

    # 2. castSpell's tail: from the rt_FD call site's resume to sub_124FE end
    cs = idc.get_name_ea_simple("castSpell")
    seg0 = idc.get_segm_start(cs)
    # rt_FD call: find `call far` whose next byte run is `db count`
    fd = idc.get_name_ea_simple("rt_FD")
    call_ea = None
    f = ida_funcs.get_func(cs)
    for ea in idautils.Heads(f.start_ea, f.end_ea):
        if ida_bytes.get_byte(ea) == 0x9A:
            for xr in idautils.XrefsFrom(ea, 0):
                if xr.to == fd:
                    call_ea = ea
    print("castSpell rt_FD call @", hex(call_ea) if call_ea else None)
    if call_ea:
        tbl = call_ea + 5
        cnt = idc.get_wide_byte(tbl)
        span = 1 + cnt * 2
        resume = tbl + span
        print("  count=%d resume=%#x" % (cnt, resume))
        # lay table as data
        ida_bytes.del_items(tbl, ida_bytes.DELIT_SIMPLE | ida_bytes.DELIT_NOTRUNC, span)
        ida_bytes.create_data(tbl, ida_bytes.FF_BYTE, 1, idc.BADADDR)
        arms = []
        for i in range(cnt):
            wea = tbl + 1 + i * 2
            ida_bytes.create_data(wea, ida_bytes.FF_WORD, 2, idc.BADADDR)
            off = idc.get_wide_word(wea)
            arms.append(seg0 + off)
            idc.op_plain_offset(wea, 0, seg0)
            ida_xref.add_cref(call_ea, seg0 + off, ida_xref.fl_JN | ida_xref.XREF_USER)
        ida_xref.add_cref(call_ea, resume, ida_xref.fl_F | ida_xref.XREF_USER)
        print("  arms:", [hex(a) for a in arms])
        # coerce resume .. sub_124FE end (or +0x400 fallback)
        s124 = idc.get_name_ea_simple("sub_124FE")
        end = idc.get_func_attr(s124, idc.FUNCATTR_END)
        if end == idc.BADADDR or end <= resume:
            end = resume + 0x420
        coerce(resume, end)
        ida_auto.auto_wait()
        for a in arms + [resume]:
            if idc.create_insn(a) <= 0:
                ida_bytes.del_items(a, ida_bytes.DELIT_SIMPLE, 2)
                idc.create_insn(a)
            if ida_funcs.get_func(a) is None:
                ida_funcs.add_func(a)
        ida_auto.auto_wait()

    # re-add castSpell / sub_124FE cleanly
    for nm in ("castSpell", "sub_124FE", "psychoStrengthSpell"):
        ea = idc.get_name_ea_simple(nm)
        if ea != idc.BADADDR and ida_funcs.get_func(ea) is None:
            ida_funcs.add_func(ea)
    ida_auto.auto_wait()
    print("done")


main()
