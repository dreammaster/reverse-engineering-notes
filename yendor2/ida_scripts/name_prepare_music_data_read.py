"""
Names sub_27C5A, called from InitMusicDriver: sets up a WORLD.DAT-
style read context directly (word_368A7/A9/AB/AD/AF, the same fields
seen elsewhere as a nested-read guard) for a fixed data block/offset
(table at 0xCE23, fixed size 0x9BD) -- preparing to read the sound
driver's music/instrument data.

-> PrepareMusicDataRead

Run via:
    .\run_ida_script.ps1 name_prepare_music_data_read.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x27C5A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PrepareMusicDataRead", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PrepareMusicDataRead': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Sets up a WORLD.DAT-style read context for a fixed data block "
    "(table 0xCE23, size 0x9BD) -- preparing to read the sound "
    "driver's music/instrument data. Called from InitMusicDriver.",
    False,
)
