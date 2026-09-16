"""
Names sub_2A9AD: HandleGameCommand's handler for word_32974 in
0x36-0x46 (17 codes -- plausibly one per party member/context, all
routed to this one function). Extracts a value from the party-member
record ([si+0xE], si=word_328D4) reduced mod 10 (two conditional -0xA
subtractions after cmp 9) -- reads as a time-of-day component; if under
4 (plausibly "too early/can't rest yet"), sets a flag bit
([si+0x1C] |= 0x8000) instead of proceeding. Otherwise regenerates a
stat: `new = min(max, max*percent/100 + current)`, applied to one of
two current/max pairs ([si+0x52]/[si+0x92] or [si+0x54]/[si+0x94])
depending on a flag on the target (word_2E548's [+2] bit 0x8000) --
classic HP/MP-style percentage regeneration. Matches the manual's "R
rest (1 food per person needed)". -> RestCharacter

This confirms/extends the party-member record field map: +0xE a time-
related value, +0x1C status flags (now also a "resting" bit),
+0x52/+0x92 one stat's current/max, +0x54/+0x94 another's current/max
(HP and MP, order not confirmed which is which).

Run via:
    .\run_ida_script.ps1 name_rest_character.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2A9AD
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RestCharacter", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RestCharacter': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Rest/regeneration: if a time-of-day check on [si+0xE] (si=party "
    "member) fails, sets a 'resting'-ish flag ([si+0x1C] |= 0x8000) "
    "instead. Otherwise regenerates one of two stats (current/max at "
    "+0x52/+0x92 or +0x54/+0x94, selected by a flag on the target) by "
    "a percentage of max, capped at max. Matches the manual's "
    "'R rest (1 food per person needed)'.",
    False,
)
