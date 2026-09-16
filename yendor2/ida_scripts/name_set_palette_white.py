"""
Names sub_16244, called twice from sub_1559A (character creation
step 3): fills the 768-byte palette scratch buffer (0x4D5C) with byte
0x3F (63, the maximum 6-bit VGA DAC value) repeated for every
R/G/B component, then applies all 256 entries via SetPaletteRange --
setting the entire palette to uniform maximum brightness (white).
Plausibly a screen-flash/transition effect. -> SetPaletteToWhite

Run via:
    .\run_ida_script.ps1 name_set_palette_white.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x16244
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SetPaletteToWhite", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SetPaletteToWhite': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Fills the 0x4D5C palette buffer with byte 0x3F (max 6-bit DAC "
    "value) and applies all 256 entries via SetPaletteRange -- sets "
    "the whole palette to white. Called from sub_1559A.",
    False,
)
