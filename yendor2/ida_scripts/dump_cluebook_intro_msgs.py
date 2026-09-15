"""
Dump messages 0x8719 (title) and 0x8748 (13-line body) drawn by
sub_14BD5, called directly from ShowClueBook -- likely the clue book's
main/index page. Read-only.

Run via:
    .\\run_ida_script.ps1 dump_cluebook_intro_msgs.py -NoExport
"""
import ida_bytes

def read_lines(ea, count, stride=None):
    lines = []
    pos = ea
    for _ in range(count):
        out = bytearray()
        while True:
            b = ida_bytes.get_byte(pos)
            pos += 1
            if b == 0:
                break
            out.append(b)
        lines.append(bytes(out))
    return lines, pos

ea = 0x8719 + 0x2D860
lines, nxt = read_lines(ea, 3)
print(f"0x8719 -> {ea:#x}: {lines}")

ea2 = 0x8748 + 0x2D860
lines2, nxt2 = read_lines(ea2, 13)
print(f"0x8748 -> {ea2:#x}: {lines2}")
