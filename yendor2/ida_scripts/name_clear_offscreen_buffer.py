"""
Names another byte-for-byte-identical overlay-segment duplicate pair:
clears the entire offscreen buffer (0x7D00 words = 64000 bytes, a
full mode-13h screen) to 0.

sub_152E1, called from FinalizeCharacterCreation and sub_1559A.
sub_11D58, called from sub_11A10 (the duplicate, different segment).

-> ClearOffscreenBuffer / ClearOffscreenBufferAlt

Run via:
    .\run_ida_script.ps1 name_clear_offscreen_buffer.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x152E1: "ClearOffscreenBuffer",
    0x11D58: "ClearOffscreenBufferAlt",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x152E1,
    "Clears the entire offscreen buffer (0x7D00 words, a full "
    "mode-13h screen) to 0. Called from FinalizeCharacterCreation and "
    "sub_1559A. Byte-for-byte identical to ClearOffscreenBufferAlt "
    "(sub_11D58) in a different overlay segment.",
    False,
)
ida_bytes.set_cmt(
    0x11D58,
    "Byte-for-byte duplicate of ClearOffscreenBuffer (sub_152E1). "
    "Called from sub_11A10.",
    False,
)
