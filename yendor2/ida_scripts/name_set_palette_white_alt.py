"""
Names sub_11EBE, called once from sub_11A10 -- byte-for-byte
identical to the just-named SetPaletteToWhite (was sub_16244),
another instance of this session's recurring overlay-segment
duplicate-function pattern. -> SetPaletteToWhiteAlt

Run via:
    .\run_ida_script.ps1 name_set_palette_white_alt.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x11EBE
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SetPaletteToWhiteAlt", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SetPaletteToWhiteAlt': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Byte-for-byte duplicate of SetPaletteToWhite (sub_16244), in a "
    "different overlay segment. Called from sub_11A10.",
    False,
)
