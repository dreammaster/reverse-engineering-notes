"""
Replaces CastSpell's 0x1C comment (which said "reads as an offensive/
damage spell rather than a self-heal, not fully traced") now that it's
actually traced: it's a materials-transmutation ability, not damage.
See name_bcd_arithmetic.py's docstring for the full trace (message
strings dumped directly from the data segment, and the BCD counter
arithmetic this branch drives).

Run via:
    .\run_ida_script.ps1 update_castspell_0x1c_transmute.py
"""
import idc
import ida_bytes

ea = 0x2AA58  # CastSpell

ida_bytes.set_cmt(
    ea,
    "Spell dispatch on word_32974. Gates on target validity flags "
    "(word_2E548's +0 bits 1/2), or for 0x1C, a separate target-picking "
    "loop with its own confirmation. Self-target effects (si=word_328D4): "
    "0x12/0x13 heal HP ([si+0x52]/[si+0x92]) by 25%/50% of missing; "
    "0x14 fully heals HP; 0x17 fully restores MP ([si+0x54]/[si+0x94]); "
    "0x1D heals MP by 50% of missing; 0x18 dispels/cures (clears status "
    "bits 13-15 in [si+0x1C]). 0x1C is a materials-transmutation ability, "
    "not damage (message strings confirm: 'YOUR SKILL IS NOT HIGH "
    "ENOUGH!' gates on a target skill byte at [si+0x70]; "
    "'YOU MUST HAVE AT LEAST 10 UNITS.' gates on IsBCDCounterAtLeast "
    "against one of two fixed BCD counters, 0x94BB/0x94B7; on success, "
    "SubtractFromBCDCounter(source)+AddToBCDCounter(dest) converts 10 "
    "units of one into the other, producing either 'NUORE CREATED' or "
    "'MAGIC ORE CREATED.' depending on which of the two confirm-prompt "
    "answers (5 vs 7) was picked). Matches the manual's 'C cast spell', "
    "though this specific effect may be better described as an alchemy/"
    "crafting ability than a combat spell.",
    False,
)
print("comment updated")
