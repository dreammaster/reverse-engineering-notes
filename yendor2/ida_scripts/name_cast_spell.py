"""
Names sub_2AA58: HandleGameCommand's handler for word_32974 in
0x12-0x1D (spans the range RestCharacter's 0x36-0x46 doesn't cover --
these look like the actual spell/ability id space). Gates on target
validity flags (word_2E548's +0 bits 1/2 -- e.g. alive/valid-target),
then sub-dispatches on the specific code: 0x12 and 0x13 both modify
[si+0x52] (si=word_328D4, confirmed HP-like current stat from
RestCharacter, capped at [si+0x92] max) by adding a fraction of the
missing amount -- (max-current)/4 for 0x12, /2 for 0x13 -- classic
minor/major heal tiers. Other codes (0x14, 0x17, 0x18, 0x1D) do
something else (not traced). Matches the manual's "C cast spell".

-> CastSpell

Run via:
    .\run_ida_script.ps1 name_cast_spell.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2AA58
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CastSpell", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CastSpell': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Spell dispatch on word_32974 (0x12-0x1D range). Gates on target "
    "validity flags (word_2E548's +0 bits 1/2). 0x12/0x13: heal "
    "[si+0x52] (HP-like stat) by 25%/50% of the missing amount, capped "
    "at [si+0x92] max -- minor/major heal. Other codes (0x14/0x17/"
    "0x18/0x1D) not yet traced. Matches the manual's 'C cast spell'.",
    False,
)
