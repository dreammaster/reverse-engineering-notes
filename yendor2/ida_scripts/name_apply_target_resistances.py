"""
Names sub_2D1C2, called once from unnamed sub_2D4B6 right after
TryResolveAttackAgainstTarget -- the resistance/immunity-filtering
step of the same damage-application sequence. `di` is the target
record (type not confirmed -- likely a monster or combat-scratch
struct, not necessarily the party-record layout; see the caution note
added to file-formats.md for TryResolveAttackAgainstTarget).

Four distinct behaviors, each gated on the incoming attack's flag
words (word_33304/word_33306) against matching bits on the target
([di+0x96]/[di+0x98]):

1. For 6 "high" status-effect bits (0x8000-0x400 of word_33304):  if
   requested and the target does NOT already have the matching
   [di+0x96] bit, OR it into the accumulated to-apply flags
   (word_2E49A). (Skipped entirely if word_33304 & 0xFC00 == 0.)
2. For 5 "low" bits (0x8,0x4,0x2,0x1, and an inverted check on 0x10):
   if requested AND the target already has (or, for 0x10, lacks) the
   matching [di+0x96] bit, the attack is fully negated -- zeroes the
   staged damage (word_2E49C) and returns.
3. For 7 resistance-category bits (0x8000-0x200 of word_33306): if
   requested and the target has the matching [di+0x98] bit, halves the
   staged damage (word_2E49C).
4. For a 5-way elemental-type selector (bits 0x20-0x200 of
   word_33304, priority-encoded to a field offset 0x10/0x54/0x56/
   0x58/0x5A): drains word_332E0 from the target's field at that
   offset, floored at 0 -- a per-element resource/charge counter on
   the target, distinct from the similarly-offset party-record fields
   documented elsewhere (again, `di`'s record type here isn't
   confirmed to be a party record).

In short: applies the target's resistances/immunities to both the
pending status-effect flags and the pending damage amount, plus an
elemental resource drain. -> ApplyTargetResistancesToAttack

Run via:
    .\run_ida_script.ps1 name_apply_target_resistances.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D1C2
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyTargetResistancesToAttack", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyTargetResistancesToAttack': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Filters pending status-effect flags (word_33304 high bits) by "
    "target immunity ([di+0x96]) into word_2E49A; fully negates "
    "damage (word_2E49C=0) if a low-bit status/immunity match is "
    "found; halves damage for a resistance-category match "
    "(word_33306 vs [di+0x98]); and drains word_332E0 from an "
    "elemental resource field on the target (offset selected by "
    "word_33304 bits 0x20-0x200), floored at 0. Called from "
    "sub_2D4B6, right after TryResolveAttackAgainstTarget.",
    False,
)
