"""
Names sub_2BD1A with moderate confidence -- RunTitleScreen's 'A' option
(only caller). Draws g_pictureDir entry 4 (224x74, a light gradient
panel) full-screen as a background, then iterates up to 9 locations
from a table (500-byte stride), skipping any not flagged discovered/
available (+0x16), and for each draws a small marker (g_pictureDir
entry 9, the same tiny icon used elsewhere as a checkbox/scroll-arrow
mark) at a per-location (x, y) position, with one of two cache tags
depending on another per-location flag (+0x15C bit 0x800) -- plausibly
"visited" vs "known but unvisited". Then polls for input.

Overall shape (background + up-to-9 flagged location pins + input loop)
strongly suggests a world/location overview map -- fits the docs'
string survey finding ~7 named towns (Port Hope, Thief's Den, Ancient
Ruin, Torchlight, Numagik, Stony Peak, Registration). -> ShowWorldMap
(moderate confidence: the overall "map with markers" shape is clear,
the specific meaning of each per-location flag isn't traced).

Run via:
    .\run_ida_script.ps1 name_world_map.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2BD1A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowWorldMap", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowWorldMap': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Moderate confidence: RunTitleScreen's 'A' option. Draws "
    "g_pictureDir entry 4 full-screen, then places up to 9 small "
    "markers (entry 9) at per-location positions from a table, "
    "skipping locations not flagged discovered (+0x16). Shape (map "
    "background + flagged location pins) fits the docs' ~7 named "
    "towns from the string survey. Not confirmed which letter/word "
    "this is short for.",
    False,
)

ida_bytes.set_cmt(
    0x1D2A6,
    "Main title screen: draws g_pictureDir entry 2 (combat scene) "
    "full-screen + mouse cursor, then dispatches 5 menu options -- "
    "selectable by keyboard (C/A/E/R/I) or mouse click (numeric "
    "codes 1-5 from sub_1D118, funneled into the same handler labels). "
    "C: ShowPartyMembers (view party characters). "
    "A: ShowWorldMap (moderate confidence -- a map with up to 9 "
    "flagged/discovered location markers). "
    "E: sets a flag on up to 4 party-member records then RETURNS from "
    "the function entirely -- this is what actually leaves the title "
    "screen and proceeds into the game (plausibly 'Enter'). "
    "R: sub_25862+ShowIntroPicture, redraw (plausibly 'About'/replay "
    "intro, or a registration-info screen given this shareware build's "
    "nag string). I: RunCharacterCreation (plausibly 'Import', given "
    "this is Chapter 2 of a series). Called from `start` and from "
    "ConfirmNewGame after confirming a new game.",
    False,
)
