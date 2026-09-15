"""
Dump messages for sub_1E546: 0x7955 (title), 0x7B15 (stat label),
0x7C61/0x7C6D (the two ore-counter labels). Checking whether this is
the alchemy screen (pairs with CastSpell's 0x1C NUORE/MAGIC ORE
conversion ability).

Run via:
    .\\run_ida_script.ps1 dump_alchemy_screen_msgs.py -NoExport
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

for off in (0x7955, 0x7B15, 0x7C61, 0x7C6D):
    ea = off + 0x2D860
    print(f"{off:#x} -> {ea:#x}: {read_str(ea)!r}")
