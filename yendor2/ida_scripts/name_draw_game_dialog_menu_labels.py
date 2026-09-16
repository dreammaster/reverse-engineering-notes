"""
Names sub_1F217, called once from RunGameDialog (the in-game
pause/system menu handler) right after dialog setup -- draws the
dialog's menu labels, each one conditionally skipped based on a
word_328C4 bit (SAVE skipped if bit 0x80 set, LOAD if 0x40, NEW GAME
if 0x20, DOS if 0x10, ANIMATION if 0x8 -- drawing GameDialog_drawAnimation's
ON/OFF pair when clear, or calling sub_1F884 instead when set --
RETURN if 0x4), then unconditionally checks g_driverStateFlags for
MUSIC (bit 1) and SOUND FX (bit 4) labels, and draws checkbox
indicators for two more driver-state bits (0x8, 0x2) via
DrawCheckboxIndicator. Reads as the dialog's one-time initial label
draw, where each word_328C4 bit suppresses the label for a menu
option that isn't available/applicable in the current context (exact
per-bit meaning of "why unavailable" not confirmed).
-> DrawGameDialogMenuLabels

Run via:
    .\run_ida_script.ps1 name_draw_game_dialog_menu_labels.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1F217
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawGameDialogMenuLabels", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawGameDialogMenuLabels': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "One-time initial label draw for RunGameDialog's pause/system "
    "menu: draws each of SAVE/LOAD/NEW GAME/DOS/ANIMATION/RETURN, "
    "each skipped if its word_328C4 bit is set (0x80/0x40/0x20/0x10/"
    "0x8/0x4 respectively; ANIMATION calls sub_1F884 instead when its "
    "bit is set), then always draws MUSIC/SOUND FX labels gated on "
    "g_driverStateFlags bits 1/4, and two DrawCheckboxIndicator calls "
    "gated on g_driverStateFlags bits 8/2. Called once from "
    "RunGameDialog.",
    False,
)
