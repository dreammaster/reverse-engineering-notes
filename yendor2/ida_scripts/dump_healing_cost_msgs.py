"""
Dump messages 0x805F, 0x806D, 0x8073 (the concatenated cost-display
template pieces in sub_1B96F, called from UseHealingItem/
UseItemType_400) plus the 4 bx message ids passed by UseHealingItem's
call sites (0x7F79, 0x7F62, 0x7F47, 0x7F26).

Run via:
    .\\run_ida_script.ps1 dump_healing_cost_msgs.py -NoExport
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

for off in (0x805F, 0x806D, 0x8073, 0x7F79, 0x7F62, 0x7F47, 0x7F26):
    ea = off + 0x2D860
    print(f"{off:#x} -> {ea:#x}: {read_str(ea)!r}")
