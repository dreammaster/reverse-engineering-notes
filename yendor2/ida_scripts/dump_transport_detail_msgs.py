"""
Dump labels for sub_13EDF (called 3x from ShowClueBookTransportDetail,
one per mount): 0x7FBD, 0x8C21, 0x8C27, 0x8C2D, 0x8C3F, 0x8C45,
0x8C4B.

Run via:
    .\\run_ida_script.ps1 dump_transport_detail_msgs.py -NoExport
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

for off in (0x7FBD, 0x8C21, 0x8C27, 0x8C2D, 0x8C3F, 0x8C45, 0x8C4B):
    ea = off + 0x2D860
    print(f"{off:#x} -> {ea:#x}: {read_str(ea)!r}")
