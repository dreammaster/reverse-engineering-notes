"""
UseRiddleAnswerItem (yendor2.asm:18436) looks the expected answer up in a table
of near pointers at DS:0xBF48, _val40 (6, set in InitGlobals) entries. Dumps
the strings; the riddle id is the topic's DialogTopicArg.

    .\run_ida_script.ps1 dump_riddle_answers.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
lines = []
for i in range(6):
    ptr = ida_bytes.get_word(DS_BASE + 0xBF48 + 2 * i)
    s = b""
    ea = DS_BASE + ptr
    while True:
        b = ida_bytes.get_byte(ea)
        if b == 0 or len(s) > 40:
            break
        s += bytes([b])
        ea += 1
    lines.append(f"{i + 1}: DS:{ptr:#06x} {s!r}")
out_path = r"C:\dev\yendor\yendor2\ida_scripts\riddle_answers.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))
