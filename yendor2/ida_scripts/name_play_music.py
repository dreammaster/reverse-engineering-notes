"""
Names sub_28296: gated on g_driverStateFlags bit1 (music active), takes
a track id in ax, sets up a driver call (sub_27BAD with the loaded
driver segment word_3195E), then reads the track's data from WORLD.DAT
(the fixed FileEntry at bx=0x9043) with errorCode=7. Called from
ShowIntroPicture, sub_11A10, and the item-icon dispatcher's 0x26D
handler. -> PlayMusicTrack

Run via:
    .\run_ida_script.ps1 name_play_music.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x28296
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PlayMusicTrack", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PlayMusicTrack': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Plays music track ax (no-op if g_driverStateFlags bit1/music-"
    "active isn't set). Sets up a driver call param (sub_27BAD) then "
    "reads the track's data from WORLD.DAT (fixed FileEntry bx=0x9043).",
    False,
)
