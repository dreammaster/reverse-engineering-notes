"""
Names two more members of the debug-hotkey family reachable only
from unresolved raw addresses early in the binary (same family as
EnforceDemoBoundary, DrawDebugPositionOverlay,
DebugSetFloorTileByNumber, DebugSetOverlayTileByNumber).

sub_26D54 (seg000:09C3): prompts for a 5-digit X value via
ReadTypedInteger, range-checks it against word_32A00/word_32A02,
then a 5-digit Y value range-checked against word_32A08/word_32A0A,
then sets word_36CF7/word_36CF9 (the confirmed party world X/Y
position) directly to the entered values -- a "type in X,Y and
teleport there" cheat. -> DebugTeleportToCoordinates

sub_26FC3 (seg000:09AB): prompts for a 4-digit index (written
directly into word_3292C by ReadTypedInteger), then toggles bit 0x1
of the [+6] flag word at 0x6D60 + index*8 -- the confirmed local
dungeon-viewport scratch cell buffer (file-formats.md: rebuilt each
render pass by BuildDungeonViewportCells, whose [+6] bit 0 is the
"hidden" flag every render-pass function checks). Loops, re-prompting
for another index, until cancelled. -> DebugToggleViewportCellHidden

Run via:
    .\run_ida_script.ps1 name_debug_teleport_and_toggle_hidden.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x26D54
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DebugTeleportToCoordinates", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DebugTeleportToCoordinates': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "Debug cheat: prompts for X (range-checked against "
    "word_32A00/word_32A02) then Y (word_32A08/word_32A0A) via "
    "ReadTypedInteger, then sets word_36CF7/word_36CF9 (party world "
    "X/Y) directly -- teleport by typed coordinates. Called from an "
    "unresolved raw address, part of the debug hotkey family "
    "(EnforceDemoBoundary, DrawDebugPositionOverlay, "
    "DebugSetFloorTileByNumber).",
    False,
)

ea = 0x26FC3
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DebugToggleViewportCellHidden", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DebugToggleViewportCellHidden': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "Debug cheat: prompts for an index via ReadTypedInteger (written "
    "into word_3292C), toggles bit 0x1 of the [+6] flag word at "
    "0x6D60+index*8 -- the confirmed dungeon-viewport scratch cell "
    "buffer's 'hidden' flag (file-formats.md, BuildDungeonViewportCells). "
    "Loops until cancelled. Called from an unresolved raw address, "
    "part of the debug hotkey family.",
    False,
)
