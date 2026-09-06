"""One-off: coerce + dump TWNDR's robCommand body (the "ROB" command).

robCommand's declared proc is a stub; the real body is a `db` blob that
runs on to stealGold.  Coerce a raw window over it and dump.

Run with -NoExport:
    .\run_ida_script.ps1 -Idb twndr -ScriptName dump_twndr_rob.py -NoExport
"""
import idc
import ida_bytes
import ida_funcs
import ida_auto
import ida_xref
import ida_lines

OUT = r"C:\dev\lota\ida_scripts\rob_dump.txt"
LO, HI = 0x15b77, 0x15d00


def coerce(lo, hi):
    for name in ("robCommand", "stealGold"):
        ea = idc.get_name_ea_simple(name)
        if ea != idc.BADADDR:
            f = ida_funcs.get_func(ea)
            if f:
                ida_funcs.del_func(f.start_ea)
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


def main():
    coerce(LO, HI)
    ida_auto.auto_wait()
    lines = []
    a = LO
    while a < HI:
        dis = ida_lines.tag_remove(idc.generate_disasm_line(a, 0) or "")
        lines.append(f"{a:#07x}  {dis}")
        nxt = idc.next_head(a, HI)
        a = nxt if nxt > a else a + 1
    with open(OUT, "w") as fh:
        fh.write("\n".join(lines))
    print(f"wrote {len(lines)} lines to {OUT}")


main()
