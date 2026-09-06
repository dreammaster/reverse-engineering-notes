"""One-off: find where robCommand sets ds:1F04 (the rob "loudness" 0/1 that
picks heat 18 vs 1 at the ~0x10AAF armer).

robCommand (twndr.asm:10521) is a db-blob from ~0x10521 to ~0x10B00 that IDA
only partly decoded. Coerce the whole span -NoExport and dump, plus a byte
scan for `C7 06 04 1F` (mov word ds:1F04h, imm) / `A3 04 1F`.

Run:
    .\run_ida_script.ps1 -Idb twndr -ScriptName dump_twndr_rob1f04.py -NoExport
"""
import idc
import ida_bytes
import ida_auto
import ida_xref
import ida_lines

OUT = r"C:\dev\lota\ida_scripts\rob1f04_dump.txt"
WINDOWS = [(0x10521, 0x10B10)]


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


def scan_pattern(pat, lo=0x10000, hi=0x18000):
    raw = ida_bytes.get_bytes(lo, hi - lo) or b""
    hits, i = [], 0
    while True:
        j = raw.find(pat, i)
        if j < 0:
            break
        hits.append(lo + j)
        i = j + 1
    return hits


def main():
    chunks = []
    for name, pat in (("mov ds:1F04,imm  C7 06 04 1F", b"\xC7\x06\x04\x1F"),
                      ("mov ds:1F04,ax   A3 04 1F", b"\xA3\x04\x1F"),
                      ("cmp ds:1F04,imm  83 3E 04 1F", b"\x83\x3E\x04\x1F")):
        chunks.append(f"=== {name}: {[hex(h) for h in scan_pattern(pat)]} ===")
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
