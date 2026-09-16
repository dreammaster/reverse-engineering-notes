"""
Names sub_243D3, ShowPartyMembers' first pipeline step: clears bits in
the party-member record's flags field ([si+0x1C], si=word_328D4) and a
16-word scratch area, then draws 3 category headers each followed by a
group of skill lines (1+1+1+2+1+1+3+1+4 = 15 lines total via
sub_23AF2) -- consistent with the manual's skill list (Projectile/
Battle Accuracy+Damage, Absorption, Repair, Survival, Linguistics,
Thievry, Chemistry, Navigation, Casting, Mapping, Bartering) grouped
into categories. The character skills display (and the first per-
character reset step). -> ShowCharacterSkills

Run via:
    .\run_ida_script.ps1 name_char_skills.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x243D3
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowCharacterSkills", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowCharacterSkills': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "ShowPartyMembers' first pipeline step: resets some per-member "
    "state ([si+0x1C] flag bits, a 16-word scratch area) then draws 3 "
    "category headers each followed by a group of skill lines (15 "
    "total) -- consistent with the manual's skill list grouped into "
    "categories. The character skills display.",
    False,
)
