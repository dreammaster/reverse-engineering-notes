"""
Continued tracing ApplyEffectAndDrawIconBar's default branch (the one
that calls ApplyEffectCost) one step earlier -- it first calls
sub_183D5 then sub_18333, both before the cost is actually applied.
Together with two small callees, these turn out to be a complete
saving-throw / resistance-check pipeline for trap/status effects:

- sub_2746C -> RandomInRange(ax=exclusive upper bound): a DOS-time-
  seeded (INT 21h AH=0x2Ch on first call) linear-congruential PRNG
  (multiplier 0x805), returning a value in [0, bound). Widely called
  (sub_16A39, sub_16DAA, and others) -- the game's general-purpose RNG.

- sub_2781C -> FailsSavingThrow(si=party-member record,
  word_3293E=difficulty threshold, word_32940=resistance bonus):
  save chance = max(5, 5*([si+0x16] - threshold) + resistance bonus)
  (so [si+0x16] is plausibly a level/skill stat -- higher beats a
  higher threshold), then rolls RandomInRange(100) against it. Returns
  1 if the roll exceeds the chance (save FAILS, effect applies), 0 if
  it doesn't (resisted).

- sub_183D5 -> RollEffectMagnitude(si=g_partyEffectIconSlots entry,
  di=effect-definition record): if the effect isn't one of the simple
  low-bit-flag types (di+8 bits 0-2) and the slot doesn't already have
  a magnitude, computes one into [si+0x10] -- either a fixed amount
  ([di+4], if display-mode flag 0x4000 is set) or a random roll
  between [di+4] and [di+6] (via RandomInRange) scaled by the party
  member's [+0x16] stat (skipped if display-mode flag 0x2000 is set).

- sub_18333 -> RollEffectResistance(si=g_partyEffectIconSlots entry,
  di=effect-definition record): if the slot hasn't already been
  resolved this pass ([si+0xE]==0) and the effect's high cost-flag
  bits are set, sums the party member's equipment/bonus resistance
  values -- a newly-found 9-field block on the party record, +0x20
  through +0x30 (2 bytes each), one field per matching high bit of
  [di+8] (0x8000..0x80) -- then calls FailsSavingThrow (threshold =
  word_32DC0, a fixed value ApplyMapTriggerEffect sets to 0x5A for
  this whole pass). Records the outcome into [si+0xE]: 0 if resisted,
  or the raw high cost-flags if the save failed (so ApplyEffectCost,
  called right after by ApplyEffectAndDrawIconBar, can tell the effect
  actually applies).

Run via:
    .\run_ida_script.ps1 name_saving_throw_system.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2746C: "RandomInRange",
    0x2781C: "FailsSavingThrow",
    0x183D5: "RollEffectMagnitude",
    0x18333: "RollEffectResistance",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2746C,
    "RandomInRange(ax=exclusive upper bound): DOS-time-seeded "
    "(INT 21h AH=0x2Ch on first call) linear-congruential PRNG "
    "(multiplier 0x805). Returns a value in [0, bound). Widely used "
    "general-purpose RNG.",
    False,
)
ida_bytes.set_cmt(
    0x2781C,
    "FailsSavingThrow(si=party-member record, word_3293E=difficulty "
    "threshold, word_32940=resistance bonus): chance = "
    "max(5, 5*([si+0x16]-threshold) + bonus); rolls RandomInRange(100) "
    "against it. Returns 1 if the roll exceeds the chance (save "
    "fails, effect applies), 0 if resisted.",
    False,
)
ida_bytes.set_cmt(
    0x183D5,
    "RollEffectMagnitude: if the effect isn't a simple low-bit-flag "
    "type (di+8 bits 0-2) and [si+0x10] isn't already set, computes a "
    "magnitude there -- fixed ([di+4], if di+0xA bit 0x4000 set) or a "
    "RandomInRange([di+4],[di+6]) roll scaled by the party member's "
    "[+0x16] stat (skipped if di+0xA bit 0x2000 set).",
    False,
)
ida_bytes.set_cmt(
    0x18333,
    "RollEffectResistance: if [si+0xE]==0 (not yet resolved this "
    "pass) and the effect's high cost-flag bits (di+8 & 0xFF80) are "
    "set, sums the party member's equipment/bonus resistance fields "
    "(+0x20..+0x30, one per matching high bit of di+8) and calls "
    "FailsSavingThrow (threshold=word_32DC0). Records 0 into [si+0xE] "
    "if resisted, or the raw high cost-flags if the save failed.",
    False,
)
