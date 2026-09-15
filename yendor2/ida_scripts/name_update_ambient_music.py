"""
Traced sub_28320 (called from PollKeyboardInput and sub_162B6) -- the
remaining ISR sub-task (word_32958, word_3295A bit 0x200, ~1-second
period at 20 ticks) turned out to drive day/night ambient music,
another consumer of the game clock.

Gated on the sound driver being active and not mid-transition. If the
~1-second timer has fired and no track is being force-played
(word_3297E==0), picks a music track based on the current game time:
word_36D01 (minutes since midnight) in [0x1A4, 0x474] (7:00 AM-7:00 PM)
selects the day track (word_36CB1), otherwise the night track
(word_36CB3) -- and only actually switches if the selected track is
nonzero and word_328C4 bit 0x2000 is set. Plays it via the already-named
PlayMusicTrack.

-> UpdateAmbientMusic

Run via:
    .\run_ida_script.ps1 name_update_ambient_music.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x28320
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UpdateAmbientMusic", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UpdateAmbientMusic': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Timer-ISR-gated (~1 second, word_32958/word_3295A bit 0x200) "
    "day/night ambient music switch. If no track is forced "
    "(word_3297E==0), picks word_36CB1 (day) or word_36CB3 (night) "
    "based on whether word_36D01 (clock minutes-since-midnight) "
    "falls in [0x1A4,0x474] (7:00 AM-7:00 PM), then plays it via "
    "PlayMusicTrack if word_328C4 bit 0x2000 allows.",
    False,
)
