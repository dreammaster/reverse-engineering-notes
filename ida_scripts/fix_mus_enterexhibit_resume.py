"""Coerce the orphan 'resume' code after enterExhibit's exhibitId+1 GOSUB.

The `ON (exhibitId+1) GOSUB` table (16 arms) that follows `enterExhibit`'s
body is followed by ~16 bytes of un-decoded `db` -- the code that runs
after an arm `retn`s (commits S3(exhibitId) = exhibitProgress, etc.).
We need it decoded to see whether the caretaker path (arm 15 =
checkFlag_2000) falls through into anything that sets questFlagWord bit
0x2000 (i.e. calls the full sub_11D02 entry with ds:20FE == 14).

Locate it by the `dw offset sub_11B14` table terminator, then coerce
forward to `sub_10B59`.

    .\run_ida_script.ps1 -Idb mus -ScriptName fix_mus_enterexhibit_resume.py
"""
import idc
import idautils
import ida_bytes
import ida_funcs
import ida_auto
import ida_xref

LOG = r"C:\dev\lota\ida_scripts\enterexhibit_resume.txt"
out = []


def w(s=""):
    out.append(str(s))


def main():
    fc = idc.get_name_ea_simple("rt_FC")
    tables = []
    for seg in idautils.Segments():
        s, e = idc.get_segm_start(seg), idc.get_segm_end(seg)
        ea = s
        while ea < e:
            if idc.get_wide_byte(ea) == 0x9A:
                for xr in idautils.XrefsFrom(ea, 0):
                    if xr.to == fc:
                        tables.append(ea)
            nxt = idc.next_head(ea, e)
            ea = nxt if nxt > ea else ea + 1

    # the enterExhibit exhibitId+1 GOSUB has 16 arms (count byte 0x10)
    target = None
    for call_ea in tables:
        tbl = call_ea + 5
        cnt = idc.get_wide_byte(tbl)
        if cnt == 0x10:
            resume = tbl + 1 + cnt * 2
            w("call rt_FC @ %#x  count=0x10  resume=%#x" % (call_ea, resume))
            # is this the enterExhibit one? check a nearby arm is checkFlag_2000
            names = [idc.get_name(idc.get_segm_start(call_ea) +
                                   idc.get_wide_word(tbl + 1 + k * 2))
                     for k in range(cnt)]
            if any("checkFlag_2000" in (n or "") for n in names):
                target = resume
                w("  -> this is enterExhibit's (has checkFlag_2000 arm)")
                w("  arms: " + ", ".join(n or "?" for n in names))

    if target is None:
        w("!! enterExhibit GOSUB table not found")
        open(LOG, "w").write("\n".join(out))
        return

    # coerce from `target` up to sub_10B59
    end = idc.get_name_ea_simple("sub_10B59")
    w("\ncoercing %#x .. %#x" % (target, end))
    ida_bytes.del_items(target, ida_bytes.DELIT_SIMPLE | ida_bytes.DELIT_NOTRUNC,
                        end - target)
    a = target
    while a < end:
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
    ida_auto.auto_wait()

    w("\ndisasm of the resume block:")
    a = target
    for _ in range(40):
        if a >= end:
            break
        w("  %#08x  %s" % (a, idc.generate_disasm_line(a, 0)))
        a = idc.next_head(a)

    open(LOG, "w").write("\n".join(out))
    print("wrote", LOG)


main()
