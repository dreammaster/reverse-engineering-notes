"""
Dump messages 0x8A82 and 0x8A8E (labels for sub_13678's two stat
fields) to identify which clue-book detail screen this is.

Run via:
    .\\run_ida_script.ps1 dump_msg_8a82.py -NoExport
"""
import ida_bytes

def read_str(ea, maxlen=100):
    out = bytearray()
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(b)
    return bytes(out)

for off in (0x8A82, 0x8A8E):
    ea = off + 0x2D860
    ctx = bytes(ida_bytes.get_byte(ea + i) for i in range(60))
    print(f"{off:#x} -> {ea:#x}: {ctx!r}")
