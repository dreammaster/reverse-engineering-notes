"""
CORRECTION: "+0xE: a time-of-day-like value" has stood since early in
the session (from RestCharacter's docstring). Last round's
UseTrainingItem flagged it as a possible class/race id instead, since
it reduces the SAME field the same way to pick a class-specific stat-
growth formula. This round settled it by reading UseTrainingItem's MP-
growth branch in full and rechecking RestCharacter's OWN two branches
side by side:

- RestCharacter's HP-regen branch (target flag [bx+2] bit 0x8000
  CLEAR) does not touch [si+0xE] at all -- straight percentage-of-max
  regen, no gate.
- RestCharacter's MP-regen branch (bit 0x8000 SET) reduces [si+0xE]
  the exact same way as UseTrainingItem (cmp 9 / sub 0xA / cmp 9 /
  sub 0xA), and if the result is < 4, sets a flag INSTEAD of
  regenerating MP.
- UseTrainingItem's MP-growth formula, gated on the identical reduced
  value, blends two class-specific stat tables ([si+0x82]/[si+0x84])
  in different proportions per branch (0/4/5/6/8/9, else default) --
  reads as different classes drawing MP from different stat
  combinations (e.g. a pure INT-caster vs a pure WIS-caster vs hybrids
  at 75/25 or 50/50).

Put together: a per-character CLASS ID that gates "does this class
have an MP pool to regenerate/grow at all" (ids 0-3, i.e. reduced
value <4 -- presumably non-casters) fits both call sites far better
than "time of day", which has no reason to determine class-specific MP
formulas or to correlate with a fixed per-character value the same way
in two unrelated functions. Correcting file-formats.md and
RestCharacter's stale comment rather than leaving the old guess
standing next to contradicting evidence.

Run via:
    .\run_ida_script.ps1 fix_field_0e_is_class_not_time.py
"""
import idc
import ida_bytes

ea = idc.get_name_ea_simple("RestCharacter")
print(f"RestCharacter @ {ea:#x}")
old_cmt = idc.get_cmt(ea, False)
print(f"old RestCharacter comment: {old_cmt!r}")

new_cmt = (
    "Rest/regeneration: the HP-regen branch (target flag [bx+2] bit "
    "0x8000 clear) always regenerates a percentage of max HP, no "
    "gate. The MP-regen branch (bit 0x8000 set) reduces [si+0xE] "
    "(plausibly a class id -- see UseTrainingItem, which reduces the "
    "same field the same way to pick a class-specific MP-growth "
    "formula) and, if <4 (presumably a non-caster class with no MP "
    "pool), sets a 'resting'-ish flag ([si+1Ch] |= 0x8000) instead of "
    "regenerating MP; otherwise regenerates a percentage of max MP. "
    "Matches the manual's 'R rest (1 food per person needed)'.",
)[0]

ida_bytes.set_cmt(ea, new_cmt, False)
print("RestCharacter comment corrected")
