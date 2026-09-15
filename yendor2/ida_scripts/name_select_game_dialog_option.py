"""
Names sub_1F29D, called from RunGameDialog (the main pause/options
dialog with its 8 GameDialog_draw* icons: Animation, Dos, Return,
Load, Music, NewGame, Save, SoundFx). Polls keyboard/mouse input to
pick one of those options:

- Mouse click: hit-tests region table 0x5CD0; results 1-6 select
  directly, 7/8 map to ESC (byte_2E400=0x1B) unless word_32910=='.'
  (0x2C), in which case one of 7/8 falls through to a direct pick
  (loc_1F2EF) instead.
- Keyboard: digit keys '1'-'6' select 1-6 directly; when
  word_32910=='.', 'L'/'S' (Load/Save shortcuts) map to ESC instead of
  a direct pick; otherwise any other key loops back to poll again.

Stores the 1-based selection in word_3291E, then falls through to
further handling (region-redraw setup, not traced beyond this point).

-> SelectGameDialogOption

Run via:
    .\run_ida_script.ps1 name_select_game_dialog_option.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1F29D
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SelectGameDialogOption", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SelectGameDialogOption': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Input handler for RunGameDialog's 8 icon options (Animation/Dos/"
    "Return/Load/Music/NewGame/Save/SoundFx): polls keyboard ('1'-'6', "
    "or 'L'/'S' shortcuts when word_32910=='.') and mouse (region table "
    "0x5CD0) to pick one, storing the 1-based selection in word_3291E.",
    False,
)
