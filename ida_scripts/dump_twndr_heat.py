"""One-off: hunt the ds:20B0 (rob "heat") setter + coerce/dump the rob paths.

Run with -NoExport:
    .\run_ida_script.ps1 -Idb twndr -ScriptName dump_twndr_heat.py -NoExport
"""
import idc
import ida_bytes
import ida_funcs
import ida_auto
import ida_xref
import ida_lines
import ida_search

OUT = r"C:\dev\lota\ida_scripts\heat_dump.txt"
# the ds:20B0 setter sites found by the pattern scan
WINDOWS = [(0x10a40, 0x10b00)]


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


def scan_pattern(pat):
    """crude byte-pattern scan across seg000"""
    hits = []
    ea = 0x10000
    end = 0x17000
    raw = ida_bytes.get_bytes(ea, end - ea) or b""
    i = 0
    while True:
        j = raw.find(pat, i)
        if j < 0:
            break
        hits.append(ea + j)
        i = j + 1
    return hits


def main():
    chunks = []
    # ds:20B0 = C7 06 B0 20  (mov word ptr ds:20B0h, imm) ; FF 06 B0 20 (inc)
    for name, pat in (("mov ds:20B0,imm", b"\xC7\x06\xB0\x20"),
                      ("inc ds:20B0", b"\xFF\x06\xB0\x20"),
                      ("A3 B0 20 (mov ds:20B0,ax)", b"\xA3\xB0\x20")):
        hits = scan_pattern(pat)
        chunks.append(f"=== {name}: {[hex(h) for h in hits]} ===")
    for lo, hi in WINDOWS:
        coerce(lo, hi)
        ida_auto.auto_wait()
        chunks.append(f"=== window {lo:#x}-{hi:#x} ===")
        a = lo
        while a < hi:
            dis = ida_lines.tag_remove(idc.generate_disasm_line(a, 0) or "")
            chunks.append(f"{a:#07x}  {dis}")
            nxt = idc.next_head(a, hi)
            a = nxt if nxt > a else a + 1
    with open(OUT, "w") as fh:
        fh.write("\n".join(chunks))
    print(f"wrote {len(chunks)} lines to {OUT}")


main()
