"""
Updates RunTitleScreen's comment with what was learned tracing its
handlers this round: the letter-keys and mouse-click numeric codes
funnel into the SAME handler labels (ax==2 shares loc_1D411 with 'A',
ax==3 shares loc_1D423 with 'E', etc.) -- 5 menu options selectable by
keyboard or mouse. 'E' is the clearest: it sets a flag on each of up to
4 party-member records then returns from the function entirely (back to
`start`/ConfirmNewGame's caller) -- i.e. it's what actually leaves the
title screen and proceeds into the game. 'I' calls the newly-named
RunCharacterCreation wizard.

Run via:
    .\run_ida_script.ps1 update_title_screen_comment.py
"""
import ida_bytes

ida_bytes.set_cmt(
    0x1D2A6,
    "Main title screen: draws g_pictureDir entry 2 (combat scene) "
    "full-screen + mouse cursor, then dispatches 5 menu options -- "
    "selectable by keyboard (C/A/E/R/I) or mouse click (numeric "
    "codes 1-5 from sub_1D118, funneled into the same handler labels). "
    "C: sub_25862+sub_23BAE, redraw. A: sub_25862+sub_2BD1A, redraw. "
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
print("done")
