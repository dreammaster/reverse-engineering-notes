"""
Names sub_1A085, moderate confidence: called from RunDungeonGameLoop's
main tick and from ApplyMapTriggerEffect. Gated on word_328C4 bits
0x1800. Branches on a status flag (word_36C79 bit 2, a 4th flag not
among the 3 already confirmed via TickStatusEffects/ApplyStatusEffect)
into two different periodic cadences:

- Normal path: a counter (word_36CBD) reaching 40 resets it and does
  two passes over the 4-slot party roster (g_partySlotAssignment,
  0x95EB), calling PrepareTrapEffectSlots(ax=2) then sub_1A14D (checks
  party record +0x1C bits 0x2000/0x4000/0x8000 -- the same 3-bit group
  CastSpell's 0x18 "dispel" code clears -- accumulating a severity
  count), then PrepareTrapEffectSlots(ax=0xE) then sub_1A195 (same
  shape but gated on +0x54 != 0, i.e. only characters with MP, checking
  a different 3-bit group at +0x1C: 0x80/0x100/0x200).
- word_36C79-bit-2 path: a much slower counter (word_36CBF, a 16-bit
  signed wraparound -- roughly every 32768 calls) does one pass calling
  sub_1A233, which instead tiers off the derived stat at +0x58 (already
  confirmed via ComputeDerivedCharacterStats) via 6 descending
  thresholds.

Either path populates a per-member icon-bar slot (di+8/+0xA/+0xC = the
effect id/definition ptr/owner) whenever the computed severity is
nonzero, sets word_328CA bit 0x100, and then calls
ApplyEffectAndDrawIconBar to render it. Overall shape (periodically
recompute per-member ailment severity, refresh the status icon bar) is
clear; the specific ailments behind effect ids 2/0xE and the word_36C79
bit-2 condition aren't confirmed -- not asserting narrative specifics.
-> TickPartyAilmentIconBar

Run via:
    .\run_ida_script.ps1 name_tick_ailment_iconbar.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1A085
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TickPartyAilmentIconBar", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TickPartyAilmentIconBar': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Periodically (word_36CBD reaching 40, or a much slower word_36CBF "
    "wraparound path gated on word_36C79 bit 2) recomputes each party "
    "member's ailment severity via PrepareTrapEffectSlots(ax=2/0xE) + "
    "helper checks against +0x1C status bits (or +0x58 for the slow "
    "path), populates the icon-bar slot when nonzero, and calls "
    "ApplyEffectAndDrawIconBar. Called from RunDungeonGameLoop and "
    "ApplyMapTriggerEffect. Exact ailment identities behind effect ids "
    "2/0xE not confirmed.",
    False,
)
