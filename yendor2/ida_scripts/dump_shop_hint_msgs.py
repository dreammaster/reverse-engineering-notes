"""
Dump the 3 context-sensitive hint messages drawn by sub_185A2 based on
which shop action bit is active: 0x7FD0 (cx=3, default/sell), 0x813B
(cx=2, enhance bit 8), 0x81CE (cx=2, repair bit 4).

Run via:
    .\\run_ida_script.ps1 dump_shop_hint_msgs.py -NoExport
"""
import ida_bytes

def read_str(ea, maxlen=40):
    out = bytearray()
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(b)
    return bytes(out)

for off in (0x7FD0, 0x813B, 0x81CE):
    ea = off + 0x2D860
    ctx = bytes(ida_bytes.get_byte(ea + i) for i in range(80))
    print(f"{off:#x} -> {ea:#x}: {ctx!r}")
