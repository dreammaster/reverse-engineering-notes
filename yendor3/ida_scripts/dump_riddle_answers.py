"""
Chapter 3 sibling: UseRiddleAnswerItem's answer pointer table is at DS:0xA8C6
with 11 entries (the count word at DS:0x5458, set in InitGlobals).

    .\run_ida_script.ps1 dump_riddle_answers.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
lines = []
for i in range(11):
    ptr = ida_bytes.get_word(DS_BASE + 0xA8C6 + 2 * i)
    s = b""
    ea = DS_BASE + ptr
    while True:
        b = ida_bytes.get_byte(ea)
        if b == 0 or len(s) > 40:
            break
        s += bytes([b])
        ea += 1
    lines.append(f"{i + 1}: DS:{ptr:#06x} {s!r}")
out_path = r"C:\dev\yendor\yendor3\ida_scripts\riddle_answers.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))
