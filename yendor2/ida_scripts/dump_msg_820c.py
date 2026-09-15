"""
Dump labels for sub_2909C (called from ShowItemUsagePreview): 0x820C,
0x8212, 0x8247 -- checking whether this is a mount/transport use
preview (di starts at 0x77C6 = "PEGASUS", the transport table found
earlier).

Run via:
    .\\run_ida_script.ps1 dump_msg_820c.py -NoExport
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

for off in (0x820C, 0x8212, 0x8247):
    ea = off + 0x2D860
    print(f"{off:#x} -> {ea:#x}: {read_str(ea)!r}")
