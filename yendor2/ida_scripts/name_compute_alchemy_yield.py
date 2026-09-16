"""
Names sub_2AD32, called once from CastSpell -- computes a stat-tiered
yield/quotient from a BCD counter, capped at 100.

Checks IsBCDCounterAtLeast(si=word_32904, threshold=100); if the
counter is below 100, replaces the working value with
sub_19B3E(word_32904) (plausibly "read this BCD counter as a binary
word" -- not traced this round) instead of the flat 100. word_32904 is
a pointer to a 4-byte packed-BCD counter, reused elsewhere in the
binary as a generic "current record" pointer, so the specific
resource it names here (plausibly MAGIC ORE or NUORE, given the
CastSpell/alchemy context) isn't independently confirmed.

Then picks a divisor (2/4/5/10) tiered by the current party member's
[+0x70] stat -- the last of the 13-entry derived-stat column
documented elsewhere, and one of the 5 fields highlighted when a
character holds a party "role" -- higher [+0x70] gives a smaller
divisor, i.e. a better yield. Finally computes
floor(value/divisor) (via a div;mul;div sequence that is
mathematically redundant with a single division -- not explained,
possibly a leftover/quirk of the original source).

In short: a stat-gated conversion of a raw material counter into a
smaller "yield" value, plausibly an alchemy ore-refining calculation.
-> ComputeAlchemyRefinementYield

Run via:
    .\run_ida_script.ps1 name_compute_alchemy_yield.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2AD32
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ComputeAlchemyRefinementYield", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ComputeAlchemyRefinementYield': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Caps a BCD counter (word_32904, resource identity unconfirmed) "
    "at 100 (or reads its binary value via sub_19B3E if under 100), "
    "then divides by a divisor (2/4/5/10) tiered by party member "
    "[+0x70] -- higher stat, smaller divisor, better yield. Result in "
    "word_32940. Called from CastSpell.",
    False,
)
