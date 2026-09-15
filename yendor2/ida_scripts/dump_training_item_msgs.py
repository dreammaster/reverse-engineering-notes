"""
Dump labels for sub_137C3 (RunClueBookItemDetailWithAbilityInfo's
overlay for items in RestCharacter's dispatch range): 0x8B57, 0x8B5F,
0x8B66.

Run via:
    .\\run_ida_script.ps1 dump_training_item_msgs.py -NoExport
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

for off in (0x8B57, 0x8B5F, 0x8B66):
    ea = off + 0x2D860
    print(f"{off:#x} -> {ea:#x}: {read_str(ea)!r}")
