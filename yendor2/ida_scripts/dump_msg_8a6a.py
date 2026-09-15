"""
Dump title message 0x8A6A for sub_13E98 (F5 item subtype 7's detail
screen, called from sub_1334E). Also peek 0x77C6/0x77E0/0x7814 (the 3
section pointers passed to sub_13EDF).

Run via:
    .\\run_ida_script.ps1 dump_msg_8a6a.py -NoExport
"""
import ida_bytes

def read_str(ea, maxlen=60):
    out = bytearray()
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(b)
    return bytes(out)

for off in (0x8A6A, 0x77C6, 0x77E0, 0x7814):
    ea = off + 0x2D860
    print(f"{off:#x} -> {ea:#x}: {read_str(ea)!r}")
