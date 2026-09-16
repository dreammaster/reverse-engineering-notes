"""
Names sub_23C18, called once from ShowWorldMap (the party roster
screen) -- shows a full character detail overlay with navigation.

Draws the same content as the already-named DrawCharacterSheetPanel
(full-screen background, portrait, class/status picture,
DrawCharacterStatSheet + DrawThreeThresholdStats +
DrawCharacterClassAndLevel + name), plus two extra WriteTwoToneString
hint lines (msg 0x7A7D/0x7A84, plausibly keyboard-shortcut hints for
navigating between party members) and a label (0x7A11). Then loops via
WaitForClickOrEscape against a hit-test table (0x5F7E), redrawing via
RestoreWorldMapAreaFromEMS + the same hint lines on each navigation
step (loc_23D17). Reads as the roster screen's character-detail
popup, letting the player click/tab through party members without
leaving the overlay. -> RunCharacterDetailOverlay

Run via:
    .\run_ida_script.ps1 name_run_character_detail_overlay.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x23C18
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunCharacterDetailOverlay", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunCharacterDetailOverlay': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Roster screen's character-detail popup: draws the same content "
    "as DrawCharacterSheetPanel plus navigation hint lines, then "
    "loops via WaitForClickOrEscape (hit-test table 0x5F7E) letting "
    "the player click/tab through party members without leaving the "
    "overlay. Called once from ShowWorldMap.",
    False,
)
