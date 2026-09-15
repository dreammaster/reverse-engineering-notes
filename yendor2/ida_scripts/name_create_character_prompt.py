"""
Names three functions found while investigating sub_25544 (a ranked
naming candidate, called from ShowPartyMembers):

sub_243C3 (0x243C3): zeroes exactly 0xFA words (500 bytes = 0x1F4,
the confirmed g_partyRecords stride) at es:di where di=word_328D4 --
wipes one entire party record clean. -> ClearPartyRecord

sub_22387 (0x22387, far, 11 call sites incl. InitGame,
RunDungeonGameLoop): draws a full-screen picture (dir 0, id=word_2E530,
set by each caller) at (1,1), then saves the resulting screen to an
EMS page (0x55D8) via MapUnmapPages + rep movsw of 0x7D00 words
(64000 bytes = one full VGA screen). A generic "draw this full-screen
picture, then cache it to EMS" utility reused by many different
screens for different specific pictures.
-> DrawFullScreenPictureAndCacheToEMS

sub_25544 (0x25544): calls SelectDefaultPartyRecord (its documented
"first record with +0xE==0" fallback scan for an empty roster slot).
If none found (word_328D4==0), returns -- no create-character prompt
shown (roster full). If found, calls ClearPartyRecord (wipe the empty
slot), DrawFullScreenPictureAndCacheToEMS (picture 3), and writes the
message at 0x7962 = "CHARACTER CREATION" at a fixed position. Called
from ShowPartyMembers. -> ShowCreateCharacterPrompt

Run via:
    .\run_ida_script.ps1 name_create_character_prompt.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x243C3, "ClearPartyRecord"),
    (0x22387, "DrawFullScreenPictureAndCacheToEMS"),
    (0x25544, "ShowCreateCharacterPrompt"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x243C3,
    "Zeroes exactly 0xFA words (500 bytes = the confirmed g_partyRecords "
    "stride 0x1F4) at es:di, di=word_328D4 -- wipes one entire party "
    "record clean.",
    False,
)
ida_bytes.set_cmt(
    0x22387,
    "Draws a full-screen picture (dir 0, id=word_2E530, set by caller) "
    "at (1,1), then saves the resulting screen to EMS page 0x55D8 "
    "(0x7D00 words = one full VGA screen). Generic full-screen "
    "draw-then-cache utility; called from many different screens "
    "(InitGame, RunDungeonGameLoop, ShowCreateCharacterPrompt, etc.) "
    "each with their own picture id.",
    False,
)
ida_bytes.set_cmt(
    0x25544,
    "Uses SelectDefaultPartyRecord's empty-slot scan (first record with "
    "+0xE==0); if none found, returns (roster full, no prompt). "
    "Otherwise wipes the slot (ClearPartyRecord), draws picture 3 full-"
    "screen (DrawFullScreenPictureAndCacheToEMS), and writes "
    "'CHARACTER CREATION'. Called from ShowPartyMembers.",
    False,
)
