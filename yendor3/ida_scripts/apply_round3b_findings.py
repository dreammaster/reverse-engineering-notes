"""
Round 3 continued: second batch of mid-confidence-tier matches. See
docs23/engine-diffs.md for the full writeup. Two matches from this
batch (PollForEscapeKeyOnlyAlt, RunTitleScreen) are deliberately NOT
renamed here -- their called-target sequences differ too much to
confirm without a dedicated read.

Run via:
    .\run_ida_script.ps1 apply_round3b_findings.py
"""
import idc
import ida_name

RENAMES = [
    (0x14D10, "DrawTrainingScreenStatSheet"),
    (0x1D0F4, "DrawAlchemySpellList"),
    (0x2AD5A, "ApplyMultiStatEffect"),
    (0x1197C, "ShowClueBook"),
    (0x1B2AB, "FindItemInInventoryRange"),
    (0x2A98E, "DrawRleMaskedShadedRun"),
    (0x1CFCC, "CheckSpellCastability"),
    (0x23AF3, "ComputeDerivedCharacterStats"),
    (0x217B2, "DrawLocalMapCell"),
    (0x25482, "IsMonsterStepBlocked"),
    (0x22FD2, "DrawMonsterHealthBar"),
    (0x200CE, "RedrawDungeonScreen"),
    (0x2AA1A, "DrawRleScaledSpriteColumn"),
    (0x2B547, "DrawWeaponSelectIcon"),
    (0x2AE05, "RestCharacter"),
    (0x1ABD2, "ApplyMapTriggerEffect"),
    (0x20103, "RefreshDungeonScreen"),
    (0x12F93, "ClassifyObstacleAtViewportRow"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
