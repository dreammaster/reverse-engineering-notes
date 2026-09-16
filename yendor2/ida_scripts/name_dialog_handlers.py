"""
Names the remaining RunGameDialog per-key handlers and their shared
helpers, all read directly this round:

- sub_1F8C7 (Dos, 'D'): draws GameDialog_drawButtons(styleNum=3), calls
  sub_1A76E(ax=1) for a yes/no confirmation; if confirmed (ax==5), sets
  byte_2E400=0xFF (the "confirmed exit" sentinel RunGameDialog/
  ShowIntroPicture check for). -> ConfirmQuitToDos
- sub_1F5A5 (New Game, 'N'): same shape, GameDialog_drawButtons(style=4)
  + sub_1A76E(ax=0x1F) confirmation; if confirmed, resets several state
  flags and calls sub_1D2A6 (a ShowIntroPicture caller) -- restarting
  the game. -> ConfirmNewGame
- sub_1A76E: the shared yes/no confirmation prompt both call, taking a
  message id in ax and returning the chosen button (5 = yes, per both
  call sites' handling). -> ShowConfirmPrompt
- sub_1F8F5 (Music, 'M'): toggles g_driverStateFlags bit 1, starts/stops
  the driver accordingly (sub_2849C / sub_28320), redraws a checkbox
  glyph. -> ToggleMusicSetting
- sub_1F93D (Sound Fx, 'F'): same shape, toggles bit 3.
  -> ToggleSoundFxSetting
- sub_1F1F4: draws g_pictureDir entry 9 (the small 8x8 icon also used by
  UpdateScrollArrows) at (ax, bx) with cache tag cx -- the checkbox/
  small-indicator draw both toggle handlers call.
  -> DrawCheckboxIndicator
- sub_1F163 (Animation, 'A'): cycles word_36CE7 through {1, 9, 5} (not a
  simple on/off) and redraws its indicator via sub_1F884.
  -> CycleAnimationSetting

Run via:
    .\run_ida_script.ps1 name_dialog_handlers.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1F8C7: "ConfirmQuitToDos",
    0x1F5A5: "ConfirmNewGame",
    0x1A76E: "ShowConfirmPrompt",
    0x1F8F5: "ToggleMusicSetting",
    0x1F93D: "ToggleSoundFxSetting",
    0x1F1F4: "DrawCheckboxIndicator",
    0x1F163: "CycleAnimationSetting",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1A76E,
    "Shows a yes/no confirmation prompt for message id ax; returns 5 "
    "when the user confirms (per both call sites -- ConfirmQuitToDos, "
    "ConfirmNewGame).",
    False,
)
ida_bytes.set_cmt(
    0x1F1F4,
    "Draws g_pictureDir entry 9 (8x8, the small icon UpdateScrollArrows "
    "also uses) at (ax, bx) with cache tag cx. Called by "
    "ToggleMusicSetting/ToggleSoundFxSetting as their checkbox "
    "indicator.",
    False,
)
