"""
Dump title message 0x8A7A for sub_1318D (F5 item subtype 8's category
loop).

Run via:
    .\\run_ida_script.ps1 dump_msg_8a7a.py -NoExport
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

ea = 0x8A7A + 0x2D860
print(f"0x8a7a -> {ea:#x}: {read_str(ea)!r}")
