"""Decode the inline ON..GOSUB jump tables that follow every `call far rt_FC`.

MS BASIC 6 compiles `ON <n> GOSUB a,b,c` (and the GOTO sibling) as

    mov  bx, <n>                 ; 1-based selector
    call far ptr rt_FC           ; leglib int-3Fh entry FC
    db   <count>                 ; number of arms
    dw   arm0, arm1, ... armN    ; near offsets, relative to the call site's CS
    <resume / past-table>        ; arms `retn` back here; also the
                                 ; out-of-range fall-through target

rt_FC (leglib seg003:0x1ca63) does `lds si,[bp+2] ; lodsb` to read the
count off its own return address, indexes the table by bx, and
`push CS ; push arm ; retf`s into the arm (pushing the past-table offset
first so the arm's `retn` lands after the table).

IDA follows the `call far` and then mis-decodes the `db count` + `dw[]`
as instructions, so every ON GOSUB body in the game currently exports as
garbage (`or ch,cl` / `add [bx],ch` ...).  This pass finds each call
site, lays the table out as data, points a code xref at every arm, makes
sure each arm is a function, and re-decodes the resume point.

Read-only w.r.t. logic; only fixes representation.  Generic -- run on
any module:

    .\run_ida_script.ps1 -Idb out -ScriptName fix_on_gosub_tables.py
"""
import idc
import idautils
import ida_bytes
import ida_funcs
import ida_auto
import ida_xref
import ida_name

RT_FC_NAMES = ("rt_FC", "rtm_FC", "j_rt_FC")
LOG = None


def rt_fc_targets():
    eas = set()
    for nm in RT_FC_NAMES:
        ea = idc.get_name_ea_simple(nm)
        if ea != idc.BADADDR:
            eas.add(ea)
    # also anything whose name contains FC thunk comment -- fall back to
    # scanning for `call far` whose operand name endswith _FC
    return eas


def iter_call_sites(targets):
    """Yield (call_ea, call_len) for every `call far ptr <rt_FC>`."""
    for seg in idautils.Segments():
        s, e = idc.get_segm_start(seg), idc.get_segm_end(seg)
        ea = s
        while ea < e:
            if idc.get_wide_byte(ea) == 0x9A:          # call far ptr seg:off
                # far operand encoded little-endian off,seg after the 9A
                # resolve via IDA's xref to be safe
                for xr in idautils.XrefsFrom(ea, 0):
                    if xr.to in targets:
                        yield ea, 5
                        break
            nxt = idc.next_head(ea, e)
            ea = nxt if nxt > ea else ea + 1


def lay_table(call_ea):
    seg_start = idc.get_segm_start(call_ea)
    tbl = call_ea + 5
    count = idc.get_wide_byte(tbl)
    if count == 0 or count > 64:
        LOG.append("  !! %#08x  implausible count=%d, skipped" % (call_ea, count))
        return None
    span = 1 + count * 2
    resume = tbl + span

    ida_bytes.del_items(tbl, ida_bytes.DELIT_SIMPLE | ida_bytes.DELIT_NOTRUNC, span)
    ida_bytes.create_data(tbl, ida_bytes.FF_BYTE, 1, idc.BADADDR)
    idc.set_cmt(tbl, "ON..GOSUB arm count", 0)

    arms = []
    for i in range(count):
        wea = tbl + 1 + i * 2
        ida_bytes.create_data(wea, ida_bytes.FF_WORD, 2, idc.BADADDR)
        off = idc.get_wide_word(wea)
        arm = seg_start + off
        arms.append(arm)
        # represent the word as an offset from the segment base so the
        # listing shows `dw offset armLabel`
        idc.op_plain_offset(wea, 0, seg_start)
        ida_xref.add_cref(call_ea, arm, ida_xref.fl_JN | ida_xref.XREF_USER)

    # resume point: arms return here; out-of-range lands here
    ida_xref.add_cref(call_ea, resume, ida_xref.fl_F | ida_xref.XREF_USER)
    if idc.create_insn(resume) <= 0:
        ida_bytes.del_items(resume, ida_bytes.DELIT_SIMPLE, 1)
        idc.create_insn(resume)

    return count, arms, resume


def ensure_arm_funcs(call_ea, arms):
    made = 0
    for arm in sorted(set(arms)):
        if idc.create_insn(arm) <= 0:
            ida_bytes.del_items(arm, ida_bytes.DELIT_SIMPLE, 2)
            idc.create_insn(arm)
        f = ida_funcs.get_func(arm)
        if f is None:
            if ida_funcs.add_func(arm):
                made += 1
        if not ida_name.get_name(arm):
            idc.set_name(arm, "onArm_%04X_%02X" % (call_ea & 0xFFFF, arm & 0xFFFF),
                         idc.SN_NOWARN | idc.SN_CHECK)
    return made


def main():
    global LOG
    LOG = []
    targets = rt_fc_targets()
    LOG.append("rt_FC targets: %s" % ", ".join("%#x" % t for t in targets))
    if not targets:
        LOG.append("no rt_FC symbol in this idb -- nothing to do")
        print("\n".join(LOG))
        return

    sites = list(iter_call_sites(targets))
    LOG.append("found %d call sites" % len(sites))
    total_arms = total_funcs = 0
    for call_ea, _ in sites:
        res = lay_table(call_ea)
        if res is None:
            continue
        count, arms, resume = res
        made = ensure_arm_funcs(call_ea, arms)
        total_arms += count
        total_funcs += made
        LOG.append("  %#08x  n=%d  resume=%#08x  arms=[%s]  (+%d funcs)" %
                   (call_ea, count, resume,
                    " ".join("%#06x" % (a & 0xFFFF) for a in arms), made))
    ida_auto.auto_wait()
    LOG.append("total: %d arms across %d sites, %d new funcs" %
               (total_arms, len(sites), total_funcs))
    txt = "\n".join(LOG)
    open(r"C:\dev\lota\ida_scripts\on_gosub_report.txt", "w").write(txt)
    print(txt)


main()
