"""
Names sub_1EA6E: the full in-game options dialog handler. Draws the
picture-panel background (DrawPicture, picture id 1 -- the 210x105
GameDialog_draw* panel decoded earlier this session, now confirmed by
this exact match) and the mouse cursor (picture id 8), calls
GameDialog_drawButtons for the icon row, then loops on PollKeyboardInput
dispatching every one of the panel's 8 hotkeys to its own handler --an
exhaustive match against the decoded panel labels:

  A -> sub_1F163   (Animation)
  D -> sub_1F8C7   (Dos)
  F -> sub_1F93D   (Sound Fx)
  L -> (New Game/Load flow at loc_1EDAA, not a plain call)
  M -> sub_1F8F5   (Music)
  N -> sub_1F5A5   (New Game)
  R / ESC -> return/exit path (sub_28412 ax=1 + Fade?)
  S -> GameDialog_drawButtons(styleNum=1) + a nested sub-flow (Save)

Called directly from `start` and InitGame -- this is the actual
save/load/options dialog the manual's "D" hotkey ("disk icon") and the
whole GameDialog_draw* family exist to support.

-> RunGameDialog

Not renaming the individual per-key handlers yet (sub_1F163 etc.) --
their exact behavior isn't traced, only which button they're wired to.

Run via:
    .\run_ida_script.ps1 name_game_dialog.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1EA6E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunGameDialog", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunGameDialog': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "In-game options dialog: draws the panel background (DrawPicture id "
    "1) + mouse cursor (id 8) + GameDialog_drawButtons, then loops on "
    "PollKeyboardInput dispatching each of the panel's 8 hotkeys: "
    "A=Animation(sub_1F163) D=Dos(sub_1F8C7) F=SoundFx(sub_1F93D) "
    "L=Load M=Music(sub_1F8F5) N=NewGame(sub_1F5A5) R/ESC=Return "
    "S=Save. Called directly from `start` and InitGame.",
    False,
)
