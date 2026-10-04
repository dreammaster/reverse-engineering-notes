"""
Full instruction text of every distinct handler the main loop's key tables (dump_main_key_table.py) jump to, up to the closing jmp / ret.

    .\run_ida_script.ps1 dump_main_key_handlers.py -NoExport
"""
import ida_segment
import idc

TABLE_ASCII = 0x0777
TABLE_FUNC = 0x0837
base = ida_segment.get_segm_by_name("seg000").start_ea
targets = {}
for i in range(0x60):
    t = idc.get_wide_word(base + TABLE_ASCII + 2 * i)
    if t:
        targets.setdefault(t, []).append(chr(0x20 + i))
for i in range(0x76 - 0x3B + 1):
    t = idc.get_wide_word(base + TABLE_FUNC + 2 * i)
    if t:
        targets.setdefault(t, []).append("ext%02X" % (0x3B + i))
lines = []
for t in sorted(targets):
    lines.append("== %04X keys %s" % (t, " ".join(targets[t])))
    ea = base + t
    for n in range(40):
        text = idc.generate_disasm_line(ea, 0)
        lines.append("  " + text.split(";")[0].strip())
        if idc.print_insn_mnem(ea) in ("jmp", "retn", "retf"):
            break
        ea = idc.next_head(ea)
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor3\ida_scripts\main_key_handlers.txt", "w", encoding="utf-8").write(text)
print("done")
