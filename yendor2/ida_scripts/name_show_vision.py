"""
Traced sub_2B2CF, item-icon-dispatch handler (word_32974==0x253, the
first branch sub_2AE3C checks) -- a scrying/vision effect, part of the
same themed item cluster as UseLocationBoundPotion/
CheckQuestItemsCompleted (0x253 is just before the 0x254-0x258 run).

Saves the player's current position/view state (word_36CF7/word_36CF9/
word_36CF5/word_36C79), sets word_36CF5=0x8000 (a special view-mode
flag) and jumps the view to a FIXED coordinate (word_36CF7=0x154/340,
word_36CF9=0x63/99), running the exact same full-redraw sequence
ApplyMapTriggerEffect uses for teleports (sub_223D4/sub_209D2/
sub_20C1E/BuildMinimapTileData/DrawMinimap). Shows that location
briefly, then restores the saved state and redraws again -- the player
doesn't actually move, only the view does. Reads as a vision/scrying
effect showing a fixed, presumably story-significant location.

-> ShowVisionAtLocation

Run via:
    .\run_ida_script.ps1 name_show_vision.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2B2CF
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowVisionAtLocation", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowVisionAtLocation': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Item-icon-dispatch handler (word_32974==0x253, part of the same "
    "themed cluster as UseLocationBoundPotion/"
    "CheckQuestItemsCompleted). Saves the current view state, jumps "
    "to a fixed coordinate (340,99) using the same redraw sequence "
    "ApplyMapTriggerEffect uses for teleports, shows it briefly, then "
    "restores the original view -- the player doesn't actually move. "
    "A vision/scrying effect revealing a fixed, presumably story-"
    "significant location.",
    False,
)
