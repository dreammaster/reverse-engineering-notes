"""
Names sub_112AE: called directly from `start`. Dispatches on byte_2E400
(the key, filled by PollKeyboardInput) against 'H'/'P'/'K'/'M'
(0x48/0x50/0x4B/0x4D) -- the classic IBM PC extended scan codes for the
cursor Up/Down/Left/Right keys, matching the manual's "Cursor UP-move
forward, DOWN-move backward, LEFT-turn left, RIGHT-turn right" exactly
-- plus 's' for a fifth case. Updates the player's position
(word_36CF7/36CF9) and facing (word_328D2) by direction-dependent
deltas (dx accumulates row-stride/cell-size values matching
GetMapCellPtr's 0x270/8), then calls RevealCellsAroundPlayer's chain
(sub_21D30) to update the automap. The movement/turning input handler.

-> HandleMovementInput

Run via:
    .\run_ida_script.ps1 name_movement_handler.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x112AE
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleMovementInput", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleMovementInput': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Movement/turning input handler, called from `start`. Dispatches "
    "on byte_2E400 == 'H'/'P'/'K'/'M' (BIOS extended scan codes for "
    "cursor Up/Down/Left/Right) -- matches the manual's move forward/"
    "backward/turn left/turn right exactly, plus an 's' case. Updates "
    "player position (word_36CF7/36CF9) and facing (word_328D2), then "
    "triggers the automap reveal (sub_21D30/RevealCellsAroundPlayer).",
    False,
)
