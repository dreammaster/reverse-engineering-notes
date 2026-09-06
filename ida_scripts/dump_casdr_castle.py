"""One-off: coerce + dump CASDR's chest / door / self-destruct handlers.

openDoor / useKey (and the mislabeled blob after it) / fortressSelfDestruct
carry `db` gaps.  resolveOpenDoor / resolveUseKey are already clean but are
dumped too for context.  Writes a linear disasm to
ida_scripts/casdr_castle_dump.txt.

Run with -NoExport so casdr.asm / casdr.idc are NOT re-written:
    .\run_ida_script.ps1 -Idb casdr -ScriptName dump_casdr_castle.py -NoExport
"""
import idc
import ida_bytes
import ida_funcs
import ida_auto
import ida_xref
import ida_lines

TARGETS = ["openDoor", "resolveOpenDoor", "useKey", "resolveUseKey",
           "fortressSelfDestruct", "sub_11CBA"]
# the big mislabeled run that starts around weakenSpell/loc_14072 handles
# the chest (ds:1F02 == 0xDF); dump a raw window over it too.
RAW_WINDOWS = [(0x13f00, 0x14300)]
OUT = r"C:\dev\lota\ida_scripts\casdr_castle_dump.txt"


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


def dump_range(lo, hi, chunks):
    a = lo
    while a < hi:
        dis = ida_lines.tag_remove(idc.generate_disasm_line(a, 0) or "")
        chunks.append(f"{a:#07x}  {dis}")
        nxt = idc.next_head(a, hi)
        a = nxt if nxt > a else a + 1


def main():
    chunks = []
    for name in TARGETS:
        ea = idc.get_name_ea_simple(name)
        if ea == idc.BADADDR:
            chunks.append(f"=== {name}: NOT FOUND ===")
            continue
        f = ida_funcs.get_func(ea)
        lo, hi = f.start_ea, f.end_ea
        ida_funcs.del_func(lo)
        coerce(lo, hi)
        ida_auto.auto_wait()
        ida_funcs.add_func(lo, hi)
        idc.set_name(lo, name, idc.SN_NOWARN | idc.SN_CHECK)
        ida_auto.auto_wait()
        chunks.append(f"=== {name}  {lo:#x}-{hi:#x} ===")
        dump_range(lo, hi, chunks)
    for lo, hi in RAW_WINDOWS:
        coerce(lo, hi)
        ida_auto.auto_wait()
        chunks.append(f"=== raw {lo:#x}-{hi:#x} ===")
        dump_range(lo, hi, chunks)
    with open(OUT, "w") as fh:
        fh.write("\n".join(chunks))
    print(f"wrote {len(chunks)} lines to {OUT}")


main()
