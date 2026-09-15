"""
Traced sub_1C589, UseItem's handler for word_2E410 bits 0x1000|0x2000
(0x3000) -- and it directly confirms the special-ability system found
many rounds ago via RevealMapRegion.

Bit 2 of the current item-use record: checks whether the current party
member (word_328D4) already has the ability bit from es:[si+0x12] set
in their own [+0xB4] (the "abilities learned" bitmask -- same field
RevealMapRegion's gating check reads). If already known, shows an
"already known" message (0x8299). Otherwise pays a BCD material cost
(0x94B3 vs threshold 0x512A, the usual pattern), and on success sets
that bit in [+0xB4] (LEARNS the ability) and zeroes the matching
charge field -- [+0xB6]/[+0xB8]/[+0xBA] for bits 0x8000/0x4000/0x2000
respectively (a 4th, [+0xBC] for some other bit, wasn't reached in the
part read) -- resetting that ability's charge/cooldown to 0 on first
learning it. This is a scroll/tome/training item that teaches the
character one of the party's (at least 4) special abilities --
RevealMapRegion being one of them, from several rounds ago.

-> UseAbilityScroll

Run via:
    .\run_ida_script.ps1 name_use_ability_scroll.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1C589
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UseAbilityScroll", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UseAbilityScroll': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "UseItem's handler for word_2E410 bits 0x1000|0x2000 (0x3000). "
    "Bit-2 branch: if the current party member already has this "
    "item's ability bit (es:[si+0x12]) set in their own [+0xB4] "
    "('abilities learned' bitmask -- see RevealMapRegion), shows an "
    "'already known' message. Otherwise pays a BCD material cost "
    "(0x94B3 vs threshold 0x512A) then learns it: sets the bit in "
    "[+0xB4] and zeroes the matching charge field ([+0xB6]/[+0xB8]/"
    "[+0xBA] for bits 0x8000/0x4000/0x2000). A scroll/tome that "
    "teaches a new special ability.",
    False,
)
