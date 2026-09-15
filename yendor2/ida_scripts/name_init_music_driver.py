"""
Names sub_28564, called from InitSoundSystem: initializes the sound
driver's hardware configuration and allocates its music-data buffer.

Reads a WORLD.DAT-ish config block (FileEntry bx=0x9043, errorCode=8)
for hardware settings (word_32916/word_32914 -- plausibly IRQ/port),
then calls into the driver via the already-named g_soundDriverFarPtr
using a function-selector convention (bx=1/2/3/4/5, classic DOS
sound-driver TSR interface): 1/2 pass the hardware settings, 3 is
init (bails with an error flag if it returns nonzero), 4/5 are
further setup calls. Finally computes a buffer size from a table
(0xD039, _val45 entries) and allocates it (allocMem) into
word_3292E, setting g_driverStateFlags bits to reflect success.

-> InitMusicDriver

Run via:
    .\run_ida_script.ps1 name_init_music_driver.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x28564
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "InitMusicDriver", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'InitMusicDriver': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Initializes the sound driver's hardware config (word_32916/"
    "word_32914) and calls into it via g_soundDriverFarPtr (function "
    "selectors 1-5: settings, init, further setup), then allocates its "
    "music-data buffer (word_3292E). Called from InitSoundSystem.",
    False,
)
