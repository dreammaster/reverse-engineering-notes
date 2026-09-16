"""
Names sub_14A87 and its helper sub_23BA4, called from
ShowWeaponDetailRow.

sub_23BA4: a generic packed-string-table utility -- advances bx past
the current NUL-terminated string and past the NUL itself, landing on
the start of the next string. Also called from BuildClueEntryText.
-> AdvanceToNextPackedString

sub_14A87: draws the fixed label "SKILL:" (confirmed via string dump
at 0x8AEF), then selects one of 5 packed strings at 0x7E8A --
confirmed via string dump to be "PROJECTILE"/"SLASHING"/"BASHING"/
"POLEARM"/"CASTING" -- by testing word_2E548's [+2] flag word against
0x8000/0x4000/0x2000/0x1000 in descending order (walking bx forward
with AdvanceToNextPackedString for each bit that ISN'T set), falling
through to the 5th ("CASTING") string if none of the 4 bits match.
This identifies word_2E548+2's high nibble as the held item's weapon
skill-type classification (projectile/slashing/bashing/polearm, or
casting for non-weapon spell-casting items). -> DrawWeaponSkillTypeRow

Run via:
    .\run_ida_script.ps1 name_draw_weapon_skill_type_row.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x23BA4
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "AdvanceToNextPackedString", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'AdvanceToNextPackedString': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "Advances bx past the current NUL-terminated string and past the "
    "NUL, to the start of the next string in a packed string table. "
    "Called from DrawWeaponSkillTypeRow and BuildClueEntryText.",
    False,
)

ea = 0x14A87
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawWeaponSkillTypeRow", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawWeaponSkillTypeRow': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "Draws 'SKILL:' then one of 5 packed strings (PROJECTILE/"
    "SLASHING/BASHING/POLEARM/CASTING) selected by testing "
    "word_2E548+2's flag bits 0x8000/0x4000/0x2000/0x1000 in "
    "descending order via AdvanceToNextPackedString, defaulting to "
    "CASTING if none match. Identifies word_2E548+2 as the held "
    "item's weapon skill-type classification. Called from "
    "ShowWeaponDetailRow.",
    False,
)
