"""
Names sub_1A233, called once from TickPartyAilmentIconBar -- populates
an icon-bar slot (di) with a duration/severity value gated by the
current party member's perception stat, unless they're dead.

bx=word_328D4 (current party member). Returns immediately if
[bx+0x1C] bit 0x40 (the confirmed DEAD flag) is set. Otherwise tiers
[bx+0x58] (the confirmed "monster-detail-reveal"/perception derived
stat) against 6 ascending thresholds (0x37/0x4B/0x50/0x64/0x7D/0x96)
to pick a *decreasing* value (0xC/9/6/3/0/0) -- higher perception,
smaller result. If that result is 0, no effect. Otherwise fills the
icon-bar slot the same way other effect appliers in this cluster do:
[di+0x10]=the tiered value, [di+8]/[di+0xA]=the staged effect id/
magnitude pair (word_3293E/word_32940), [di+0xC]=the character
pointer, then sets word_328CA bit 0x100. In short: a perception-gated
ailment effect whose severity/duration shrinks as the character's
perception rises, plausibly a confusion/disorientation-style status
tied into the icon-bar system. -> TickPerceptionGatedAilmentSlot

Run via:
    .\run_ida_script.ps1 name_perception_gated_ailment_slot.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1A233
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TickPerceptionGatedAilmentSlot", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TickPerceptionGatedAilmentSlot': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "No-op if [bx+0x1C] bit 0x40 (DEAD). Tiers [bx+0x58] (perception "
    "stat) against 6 thresholds to a decreasing value (0xC..0); if "
    "nonzero, fills icon-bar slot di ([di+0x10]=value, "
    "[di+8]/[di+0xA]=word_3293E/word_32940, [di+0xC]=bx) and sets "
    "word_328CA bit 0x100. Called from TickPartyAilmentIconBar.",
    False,
)
