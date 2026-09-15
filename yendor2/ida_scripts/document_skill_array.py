"""
Refines ShowCharacterSkills' existing comment now that the 16-word
array it clears (+0xCA-+0xE9) is understood to be the character's skill
values (see docs/file-formats.md's new "Skill values found" note),
and that this sharpens the earlier "unconfirmed record type" hedge on
GetRecordFlagBitAndWord_CA -- that per-object flag bank at the same
relative offset must be on a different record, since here +0xCA holds
plain word values, not a bitmask.

Run via:
    .\run_ida_script.ps1 document_skill_array.py
"""
import idc
import ida_bytes

ea = idc.get_name_ea_simple("ShowCharacterSkills")
print(f"ShowCharacterSkills @ {ea:#x}")
old_cmt = idc.get_cmt(ea, False)
print(f"old comment: {old_cmt!r}")

new_cmt = (
    "ShowPartyMembers' first pipeline step: clears status bits 0-5 of "
    "[+0x1C] and a 16-word skill-value array at [+0xCA]-[+0xE9], then "
    "draws 3 category headers each followed by a group of skill-name "
    "lines (3+4+8=15 total, via sub_23AF2) -- matches the manual's "
    "skill list grouped into categories. Individual skill "
    "names/offsets within the array aren't mapped yet. The character "
    "skills display."
)
ida_bytes.set_cmt(ea, new_cmt, False)
print("comment updated")
