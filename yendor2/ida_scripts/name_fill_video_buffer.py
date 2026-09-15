"""
Names sub_14B10: es = _videoBufferSeg; di = 0; cx = 0x7D00 (32000 words =
64000 bytes = exactly a 320x200 byte-per-pixel VGA buffer); rep stosw
with al/ah both set to the same fill byte. This is a straightforward
full-screen fill/clear of the video buffer. -> FillVideoBuffer

Run via:
    .\run_ida_script.ps1 name_fill_video_buffer.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x14B10
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "FillVideoBuffer", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'FillVideoBuffer': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Fills the entire 320x200 video buffer (_videoBufferSeg) with the "
    "byte passed in AL (replicated to AH before the word-store loop). "
    "cx=0x7D00 = 32000 words = 64000 bytes = one full VGA Mode 13h-style "
    "frame.",
    False,
)
