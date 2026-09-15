"""
Names sub_283EA, called from InitGame: the top-level sound/music
driver initialization. Clears the low nibble of g_driverStateFlags,
calls sub_28619 (not traced), bails early if bits 0xC000 are set
(already initialized/error state). Otherwise conditionally calls
DetectSoundDriver (unless word_328C8 bit 1 -- plausibly a "sound
disabled" command-line/config flag) and sub_28564 (unless bit 0 --
plausibly a separate "music disabled" flag).

-> InitSoundSystem

Run via:
    .\run_ida_script.ps1 name_init_sound_system.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x283EA
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "InitSoundSystem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'InitSoundSystem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Top-level sound/music driver init, called from InitGame: bails "
    "early if already initialized (g_driverStateFlags bits 0xC000), "
    "else conditionally runs DetectSoundDriver and sub_28564 gated on "
    "word_328C8 bits 1/0 (plausibly sound/music disable flags).",
    False,
)
