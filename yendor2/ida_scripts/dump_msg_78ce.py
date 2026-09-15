"""
Dump the 4 tiered messages at DS:0x78CE (9-byte stride), selected by
sub_21530 (called from RunAlchemyScreen) based on word_36CF5 bits
0x8000/0x4000/0x1000/none.

Run via:
    .\\run_ida_script.ps1 dump_msg_78ce.py -NoExport
"""
import ida_bytes

def read_str(ea, maxlen=20):
    out = bytearray()
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(b)
    return bytes(out)

base = 0x78CE + 0x2D860
for i in range(4):
    ea = base + i * 9
    print(f"tier {i}: {ea:#x}: {read_str(ea)!r}")
