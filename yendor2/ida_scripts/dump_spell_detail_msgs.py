"""
Dump labels for sub_13B3F (the spell detail screen, called from
sub_13216 / to-be-named RunClueBookSpellCategory): 0x8C9E (title-ish),
0x8CAF (cx=3 lines), 0x8CC4 (cx=5 lines).

Run via:
    .\\run_ida_script.ps1 dump_spell_detail_msgs.py -NoExport
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

for off in (0x8C9E, 0x8CAF, 0x8CC4):
    ea = off + 0x2D860
    ctx = bytes(ida_bytes.get_byte(ea + i) for i in range(120))
    print(f"{off:#x} -> {ea:#x}: {ctx!r}")
