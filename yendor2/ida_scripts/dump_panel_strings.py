"""
DrawPartyMemberStatusPanel (yendor2.asm:32307) writes a NUL-terminated string at DS:0x7AAE (Chapter 3 DS:0x7DE0) into a dead
member's name bar. Dumps it, and the portrait-overlay constant (_val38: Chapter 2 mov _val38, 0E0h; Chapter 3 word DS:0x5454).

    .\run_ida_script.ps1 dump_panel_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
ADDR = 0x7AAE
data = bytes(ida_bytes.get_byte(DS_BASE + ADDR + i) for i in range(24))
text = repr(data.split(b"\0")[0])
open(r"C:\dev\yendor\yendor2\ida_scripts\panel_strings.txt", "w", encoding="utf-8").write(text)
print(text)
