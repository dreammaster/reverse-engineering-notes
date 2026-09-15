"""
Updates CastSpell's comment with the remaining codes traced this round
-- a coherent picture of the spell-effect table now:

  0x12: heal [si+0x52] (HP) by 25% of missing, capped at max
  0x13: heal [si+0x52] (HP) by 50% of missing, capped at max
  0x14: full heal -- [si+0x52] = [si+0x92] (max HP) directly
  0x17: full MP restore -- [si+0x54] = [si+0x94] (max MP), if not already full
  0x18: dispel/cure -- clears 3 status bits ([si+0x1C] &= 0x1FFF, i.e.
        bits 13-15) if any were set
  0x1D: heal [si+0x54] (MP) by 50% of missing, capped at max
  0x1C: handled specially before the main dispatch -- a targeted spell:
        loops picking a target (ShowConfirmPrompt msg=8, sub_25B14
        lookup, checks the target isn't already flagged 0x1C40),
        rejects targets with [target+0x70] < 'A' (some category
        gate), then a second confirmation (ShowConfirmPrompt msg=0x23,
        branching on the specific answer 5 vs 7) -- more complex than
        the self-target heals, plausibly an attack/offensive spell or
        one needing an ally target. Not fully traced past the
        confirmation branch.

Run via:
    .\run_ida_script.ps1 update_cast_spell_comment.py
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
    "separate, more complex targeted spell (target selection + "
    "confirmation), not fully traced. Matches the manual's "
    "'C cast spell'.",
    False,
)
print("done")
