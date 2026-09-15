"""
Names sub_203F7, called from RunMapEditorScreen (2 sites): zeroes
0x500 words (2560 bytes) starting at VGA segment 0xA000:0000 -- a
partial video-memory clear (not the full 64000-byte mode-0x13 frame),
plausibly just the legend/preview area at the top of the screen.

-> ClearVideoMemoryRegion

Run via:
    .\run_ida_script.ps1 name_clear_video_memory_region.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x203F7
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ClearVideoMemoryRegion", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ClearVideoMemoryRegion': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Zeroes 0x500 words (2560 bytes) at VGA segment 0xA000:0000 -- a "
    "partial screen clear, not the full 64000-byte frame. Called from "
    "RunMapEditorScreen.",
    False,
)
