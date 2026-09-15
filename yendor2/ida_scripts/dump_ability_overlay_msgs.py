"""
Dump labels 0x8B45/0x8B4F for sub_1381C (the ability-info overlay in
RunClueBookItemDetailWithAbilityInfo).

Run via:
    .\\run_ida_script.ps1 dump_ability_overlay_msgs.py -NoExport
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

for off in (0x8B45, 0x8B4F):
    ea = off + 0x2D860
    print(f"{off:#x} -> {ea:#x}: {read_str(ea)!r}")
