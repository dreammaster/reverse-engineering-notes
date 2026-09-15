"""
Dump message 0x7DA5, RestPartyAndAdvanceClock's "can't rest here"
rejection (shown for any nonzero errorCode from its eligibility check).

Run via:
    .\\run_ida_script.ps1 dump_rest_rejection_msg.py -NoExport
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

ea = 0x7DA5 + 0x2D860
ctx = bytes(ida_bytes.get_byte(ea + i) for i in range(60))
print(f"0x7da5 -> {ea:#x}: {ctx!r}")
