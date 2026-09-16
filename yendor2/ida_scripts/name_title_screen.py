"""
Names sub_1D2A6: called directly from `start` and from ConfirmNewGame
(after the player confirms starting a new game). Draws g_pictureDir
entry 2 (140x155, the two-figure combat/fighting scene identified
earlier this session) full-screen plus the mouse cursor, then loops on
PollKeyboardInput dispatching top-level commands: C, A, E, R, I as
single-key options (not traced individually -- plausibly Continue/
About/Exit/Register/Info, given this shareware build's registration nag
string and the picture's title-screen-shaped role), plus direct music/
sound-fx toggles on Enter/Ctrl-S without going through RunGameDialog.

This is the game's main title screen. -> RunTitleScreen

Run via:
    .\run_ida_script.ps1 name_title_screen.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1D2A6
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunTitleScreen", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunTitleScreen': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Main title screen: draws g_pictureDir entry 2 (combat scene) "
    "full-screen + mouse cursor, then dispatches top-level single-key "
    "commands (C/A/E/R/I -- not individually traced, plausibly Continue/"
    "About/Exit/Register/Info) plus direct music/soundfx toggles. "
    "Called from `start` and from ConfirmNewGame after confirming a new "
    "game.",
    False,
)
