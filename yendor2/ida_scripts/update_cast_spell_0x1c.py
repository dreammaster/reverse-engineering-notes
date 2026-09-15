"""
Updates CastSpell's comment with 0x1C's ending, now that FormatNumber
is named: after target selection and confirmation (ax==5 or ax==7
branch, each choosing between two resource-availability checks via
sub_19A7C), draws a small icon (sub_23B76, cx=2) and a FormatNumber'd
amount with a message, plus a sound cue -- reads as an offensive/damage
spell showing its effect amount, not just a self-heal. Still not fully
traced (sub_19A7C's exact check, sub_17FB8-adjacent damage source) but
enough to describe accurately.

Run via:
    .\run_ida_script.ps1 update_cast_spell_0x1c.py
"""
import ida_bytes

ida_bytes.set_cmt(
    0x2AA58,
    "Spell dispatch on word_32974. Gates on target validity flags "
    "(word_2E548's +0 bits 1/2), or for 0x1C, a separate target-"
    "picking loop with its own confirmation. Self-target effects "
    "(si=word_328D4): 0x12/0x13 heal HP ([si+0x52]/[si+0x92]) by 25%/"
    "50% of missing; 0x14 fully heals HP; 0x17 fully restores MP "
    "([si+0x54]/[si+0x94]); 0x1D heals MP by 50% of missing; 0x18 "
    "dispels/cures (clears status bits 13-15 in [si+0x1C]). 0x1C is a "
    "separate, more complex spell: picks a target, checks resource "
    "availability (sub_19A7C, two variants selected by the "
    "confirmation answer), then on success draws a small icon and a "
    "FormatNumber'd amount with a message and sound cue -- reads as an "
    "offensive/damage spell rather than a self-heal, not fully traced. "
    "Matches the manual's 'C cast spell'.",
    False,
)
print("done")
