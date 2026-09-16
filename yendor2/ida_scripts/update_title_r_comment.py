"""
Checked RunTitleScreen's 'R' option: it doesn't set word_2E530/532
before calling ShowIntroPicture, so it replays whatever picture is
already showing (entry 2, the combat scene set at the very top of
RunTitleScreen) with the standard fade+wait-for-key treatment, rather
than showing distinct content. Not enough signal to name confidently --
updating the comment to record this rather than leaving the earlier
under-informed "About"/"Register" guess standing unqualified.

Run via:
    .\run_ida_script.ps1 update_title_r_comment.py
"""
import ida_bytes

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
    "R: replays whatever picture is already showing (entry 2, since "
    "word_2E530/532 aren't reset here) via ShowIntroPicture's fade+"
    "wait-for-key -- doesn't show distinct content, so its actual "
    "purpose (About/credits/register nag?) isn't confirmed. "
    "I: RunCharacterCreation (plausibly 'Import', given this is "
    "Chapter 2 of a series). Called from `start` and from "
    "ConfirmNewGame after confirming a new game.",
    False,
)
print("done")
