"""
Traced sub_1B428 and sub_25C61 -- together they confirm party field
+0x18 (already suspected as XP, from ShowLootAndAwardExperience) and
close the loop on +0x1E (already known to gate a portrait-redraw call
from ShowLootAndAwardExperience/sub_1B7DD).

sub_1B428(implicit si=word_328D4) -> CheckForLevelUp: skips invalid
(status 0x1C40) characters. Otherwise walks an XP-threshold table at
0x9277-0x93DB (65 x 4-byte packed-BCD entries, one per level),
starting at the character's current level ([+0x16]), comparing their
XP ([+0x18]) against each threshold via CompareBCD4 and advancing as
long as XP >= threshold. If the resulting level is higher than the
current one, stores it into [+0x1E] -- a "pending new level" flag/
value, not applied yet.

sub_25C61(si=character) -> ShowLevelUpMessage: draws the character's
current level ([+0x16]), and if [+0x1E] (pending new level) is
nonzero, also draws a second "new level" line showing that value --
the level-up notification screen. Confirms [+0x1E]'s "pending level-
up" role.

Run via:
    .\run_ida_script.ps1 name_level_up_system.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1B428: "CheckForLevelUp",
    0x25C61: "ShowLevelUpMessage",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1B428,
    "CheckForLevelUp (implicit si=word_328D4): walks the XP-threshold "
    "table at 0x9277 (65 x 4-byte packed-BCD entries, one per level) "
    "starting at the character's current level [+0x16], comparing "
    "their XP [+0x18] against each threshold and advancing while "
    ">=. If the result exceeds the current level, stores it into "
    "[+0x1E] (pending new level, not yet applied).",
    False,
)
ida_bytes.set_cmt(
    0x25C61,
    "ShowLevelUpMessage(si=character): shows current level [+0x16], "
    "and if [+0x1E] (pending new level, from CheckForLevelUp) is "
    "nonzero, also shows it as a second line -- the level-up "
    "notification screen.",
    False,
)
