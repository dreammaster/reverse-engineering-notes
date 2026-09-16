"""
Names sub_119E0, called once from ShowIntroPicture (with a frame count
in cx) -- a frame-paced wait loop with ESC-abort, built on the same
word_328C4 bit 0x400 "tick ready" flag seen throughout
PlayCharacterCreationIntroAnimation's staged sub-animations.

Busy-waits (spins on itself) until word_328C4 bit 0x400 becomes set --
plausibly a flag raised by a timer/vsync interrupt handler elsewhere,
not traced this round. Once set, checks for ESC via the already-named
PollForEscapeKeyOnly; if pressed, returns immediately. Otherwise
clears the bit (consuming this tick) and loops for cx total ticks.
-> WaitFrameTicksOrEscape

Run via:
    .\run_ida_script.ps1 name_wait_frame_tick_escape.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x119E0
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "WaitFrameTicksOrEscape", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'WaitFrameTicksOrEscape': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Busy-waits for word_328C4 bit 0x400 ('tick ready', plausibly set "
    "by an untraced timer/vsync interrupt handler), checks ESC via "
    "PollForEscapeKeyOnly (returns immediately if pressed), else "
    "clears the bit and repeats for cx ticks. Called from "
    "ShowIntroPicture.",
    False,
)
