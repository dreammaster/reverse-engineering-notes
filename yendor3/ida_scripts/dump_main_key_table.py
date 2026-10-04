"""
The main loop's keyboard jump tables (start, yendor3.asm:172 and :200): `mov ax, cs:[0777h + 2 * (key - 0x20)]` for ASCII keys 0x20-0x7F and
`cs:[0837h + 2 * (scan - 0x3B)]` for the function keys (extended codes 0x3B-0x76), jumping to the stored seg000 offset, or ignoring the key if it is
zero. Dumps each non-zero entry with the first calls at its target. Chapter 2 offsets are 0x0B07 / 0x0BC7.

    .\run_ida_script.ps1 dump_main_key_table.py -NoExport
"""
import ida_segment
import idc
import idautils

TABLE_ASCII = 0x0777
TABLE_FUNC = 0x0837
base = ida_segment.get_segm_by_name("seg000").start_ea
lines = []


def describe(target):
    ea = base + target
    calls = []
    text = []
    for i in range(14):
        mnem = idc.print_insn_mnem(ea)
        if mnem == "":
            break
        if mnem == "call":
            calls.append(idc.print_operand(ea, 0))
        if mnem in ("jmp", "retn", "retf"):
            text.append("%s %s" % (mnem, idc.print_operand(ea, 0)))
            break
        ea = idc.next_head(ea)
    return " ".join(calls) + (" | " + " ".join(text) if text else "")


for i in range(0x60):
    t = idc.get_wide_word(base + TABLE_ASCII + 2 * i)
    if t:
        lines.append("key %02X %r -> %04X: %s" % (0x20 + i, chr(0x20 + i), t, describe(t)))
for i in range(0x76 - 0x3B + 1):
    t = idc.get_wide_word(base + TABLE_FUNC + 2 * i)
    if t:
        lines.append("ext %02X -> %04X: %s" % (0x3B + i, t, describe(t)))
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor3\ida_scripts\main_key_table.txt", "w", encoding="utf-8").write(text)
print(text)
