"""
Traced sub_2849C, called directly from `start` (multiple sites) and
before ShowVisionAtLocation's sibling handler (word_32974==0x26D)
plays a forced music track. Stops the currently-playing music (via
byte_28616(bx=7), a low-level driver call) and clears word_2E4A6 (a
"currently forced track" tracker), then re-arms UpdateAmbientMusic's
~1-second timer (word_32958=0x14, word_3295A|=0x200) and clears its
"already triggered" latch (word_328CA bit 0x10) if not already
running -- resets music state so the ambient day/night system (or a
following forced PlayMusicTrack call) can take over cleanly.

-> StopMusicAndResetTimer

Run via:
    .\run_ida_script.ps1 name_stop_music.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2849C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "StopMusicAndResetTimer", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'StopMusicAndResetTimer': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Stops the currently-playing music (byte_28616(bx=7)) and clears "
    "word_2E4A6 (forced-track tracker), then re-arms "
    "UpdateAmbientMusic's ~1-second timer and clears its "
    "'already triggered' latch if not already running. Resets music "
    "state before a following track change.",
    False,
)
