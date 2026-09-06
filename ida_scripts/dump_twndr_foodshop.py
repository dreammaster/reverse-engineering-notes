"""One-off: coerce TWNDR's foodShop `db` gaps and dump its disassembly.

foodShop carries the mail-delivery PAYOUT ("THANKS FOR THE LETTER
DELIVERY") in a `db` blob IDA chopped after the far calls.  This coerces
just that function and writes a linear disasm to
`ida_scripts/foodshop_dump.txt` so the payout math can be read.

Run with -NoExport so twndr.asm / twndr.idc are NOT re-written:
    .\run_ida_script.ps1 -Idb twndr -ScriptName dump_twndr_foodshop.py -NoExport
"""
import idc
import ida_bytes
import ida_funcs
import ida_auto
import ida_xref
import ida_lines

TARGET = "foodShop"
OUT = r"C:\dev\lota\ida_scripts\foodshop_dump.txt"


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


def main():
    ea = idc.get_name_ea_simple(TARGET)
    if ea == idc.BADADDR:
        print(f"{TARGET}: not found")
        return
    f = ida_funcs.get_func(ea)
    lo, hi = f.start_ea, f.end_ea
    ida_funcs.del_func(lo)
    coerce(lo, hi)
    ida_auto.auto_wait()
    ida_funcs.add_func(lo, hi)
    idc.set_name(lo, TARGET, idc.SN_NOWARN | idc.SN_CHECK)
    ida_auto.auto_wait()

    lines = []
    a = lo
    while a < hi:
        flags = ida_bytes.get_full_flags(a)
        dis = idc.generate_disasm_line(a, 0) or ""
        dis = ida_lines.tag_remove(dis)
        lines.append(f"{a:#07x}  {dis}")
        nxt = idc.next_head(a, hi)
        a = nxt if nxt > a else a + 1
    with open(OUT, "w") as fh:
        fh.write("\n".join(lines))
    print(f"wrote {len(lines)} lines to {OUT}")


main()
