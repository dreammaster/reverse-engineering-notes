"""
Names sub_218DC, called once from InitGame (right before the
already-named ShowIntroPicture, both part of the boot sequence).

Polls for a keypress 3 times in sequence (each with the same
"ESC aborts immediately, any other key clears byte_2E400 and
continues" pattern used throughout the boot flow): first as a plain
gate, then after LoadMasterPalette + reading file record 1
(errorCode=3) via FileEntry_Read, then after DrawPicture(id 0, the
first picture-directory entry) + DrawMouseCursor. After the third
gate, plays music track 3 (PlayMusicTrack), then times a ~12-second
display window via BIOS INT 1Ah clock ticks, polling the keyboard
each loop and breaking early (StopMusicAndResetTimer) on ESC or once
the timer expires -- but skips the timed wait entirely if a driver
flag (g_driverStateFlags bit 2) and word_2E492==0 both hold. Reads as
the game's title screen: draw the title picture, start its theme
music, and hold it on screen for a fixed duration or until the
player presses a key. -> PlayTitleScreenSequence

Run via:
    .\run_ida_script.ps1 name_play_title_screen_sequence.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x218DC
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PlayTitleScreenSequence", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PlayTitleScreenSequence': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Boot title screen: 3 skippable keypress gates around "
    "LoadMasterPalette + FileEntry_Read(record 1) and "
    "DrawPicture(id 0), then PlayMusicTrack(3) and a ~12-second "
    "BIOS-clock-timed display window (skippable via ESC, or via "
    "StopMusicAndResetTimer once the clock target is reached). "
    "Called once from InitGame, right before ShowIntroPicture.",
    False,
)
