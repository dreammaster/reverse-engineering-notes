"""
Names two related functions, resolving last round's open lead
(sub_22989).

sub_227F5, called once from sub_22989: a chance-scaled avoidance
roll. Resets word_2E49C=0; subtracts the character's stat (ax,
sub_22989 passes the still-mysterious party-record field [+0x50])
from a trap/effect threshold (bx); if the remainder is negative
(character's stat exceeds the threshold), no effect. Otherwise rolls
RandomInRange(100); if the roll exceeds the remaining margin, again
no effect. If it triggers, computes a magnitude scaled by that margin
(`cx * bx / 100`, `cx` a max-magnitude parameter, rounded via +50
before the divide) into word_2E49C -- the same staging global used
throughout the icon-bar/attack pipeline. In short: "the higher this
character's [+0x50] stat relative to the trap's threshold, the less
likely (and smaller) the effect" -- a save-vs-trap-style avoidance
roll. -> RollTrapAvoidanceMagnitude

sub_22989, called once from unnamed sub_2278C: picks a random active
party member (PickRandomActivePartyMember) and stores their pointer
into an icon-bar-style output record ([di+6]). Rolls
RollTrapAvoidanceMagnitude using that member's [+0x50] stat against a
trap/side-feature record's threshold ([si+0x64]) and magnitude cap
([si+0x66]); if it triggers, sets word_328C8 bit 0x10. Then computes a
facing-direction-dependent position delta (word_36CF5 tier bits, the
same convention as DrawDungeonCellSideFeature/ShowCompassDirection)
and tests one of 4 direction bits on the trap record's [si+0xE] flags
-- only if the party's current facing matches the trap's side does it
finish populating the icon-bar record (character-relative position,
pointer, a fixed type marker of 6, and a flag from [si+0x92]) and set
word_328C8 bit 8. In short: rolls whether a random party member
triggers a wall/door-embedded ("side") trap they're currently facing,
staging the result for the icon-bar effect system.
-> TriggerSideTrapForRandomPartyMember

Run via:
    .\run_ida_script.ps1 name_side_trap_trigger.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x227F5: "RollTrapAvoidanceMagnitude",
    0x22989: "TriggerSideTrapForRandomPartyMember",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x227F5,
    "Chance-scaled avoidance roll: bx-=ax (threshold minus stat); "
    "skip if negative or RandomInRange(100) exceeds the remaining "
    "margin; else word_2E49C = round(cx*bx/100). Higher ax (the "
    "caller's [+0x50] stat) means less likely and smaller effect. "
    "Called from TriggerSideTrapForRandomPartyMember.",
    False,
)
ida_bytes.set_cmt(
    0x22989,
    "Picks a random party member, rolls RollTrapAvoidanceMagnitude "
    "using their [+0x50] stat against a trap record's threshold/cap "
    "([si+0x64]/[si+0x66]), then -- only if the party's current "
    "facing (word_36CF5 tier bits) matches one of 4 direction bits on "
    "the trap's [si+0xE] flags -- finishes populating an icon-bar-"
    "style output record and sets word_328C8 bit 8. A wall/door-"
    "embedded 'side trap' trigger. Called from unnamed sub_2278C.",
    False,
)
