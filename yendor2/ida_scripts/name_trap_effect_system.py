"""
Traced sub_18257's caller chain (left unnamed last round pending more
context) back to HandleMovementInput and found a complete, coherent
"map trigger / trap effect" system:

- sub_19E56 (called directly from HandleMovementInput when a move
  lands on a flagged object): branches on the hit object's type flags
  ([di+2]). 0x4000 = teleport (sets word_36CF7/word_36CF9 from
  [di+4]/[di+6], full map redraw). 0x2000 = a different, separate
  effect (sub_1FC3F+sub_20C46, not traced). 0x1000/0x800/0x400/the
  0x300 pair = trap/status effects: look up an effect-definition
  record via PrepareTrapEffectSlots, then for each of the party's 4
  slots (via g_partySlotAssignment -> g_partyRecords, confirmed fixed
  array: record address = g_partyRecords + (slot-1)*0x1F4, 0x1F4=500
  bytes/record), skip members with status bits 0x1C40 set (the same
  "invalid target" mask CastSpell checks), then populate that member's
  g_partyEffectIconSlots entry (party-member ptr, effect-def ptr, a
  message value, extra position/amount fields) before calling
  ApplyEffectAndDrawIconBar. -> ApplyMapTriggerEffect

- sub_18068: computes bx = g_trapEffectDefs + id*0xC (12-byte-stride
  effect-definition table) for the given effect id, and clears the
  "populated" fields ([+8..+0x14)) of all 4 g_partyEffectIconSlots
  entries first -- a reset-before-repopulate step. Returns bx = the
  looked-up effect-definition record pointer (ax is left 0, not a
  useful return despite being captured by callers). -> PrepareTrapEffectSlots

- sub_180BA (the icon-bar loop from the previous round): iterates the
  4 g_partyEffectIconSlots entries; for each occupied one, draws the
  slot's effect icon (g_pictureDir offset from the effect-def's +2
  field, one of 3 draw variants selected by the effect-def's +0xA
  flags) and applies the effect's cost via sub_18257 (now confidently
  named ApplyEffectCost -- its [di+8] flag bits select HP/MP/one-of-3
  material-counter deduction, all now-named DeductHPClamped/
  DeductMPClamped/SpendMaterialCounterClamped from the previous
  round). -> ApplyEffectAndDrawIconBar

- sub_18257 -> ApplyEffectCost (confidently renamed now that its
  caller and the effect-definition record's field layout are
  understood).

Data locations named (all DS-relative, ds == word_2E4AA == seg seg129
throughout):
  0xC50  (linear 0x2E4B0) -> g_partyEffectIconSlots: 4 entries x 20
    bytes. +0/+2 and +4/+6: two (x,y) screen-position pairs (set once
    at init from a source table, ApplyMapTriggerEffect only ever
    touches +8 onward). +8: a message/data value. +0xA: pointer to the
    effect-definition record (g_trapEffectDefs entry) for this slot.
    +0xC: pointer to the affected party-member record. +0x10/+0x12:
    extra position/amount fields, meaning varies by trigger branch.
  0x905B (linear 0x368BB) -> g_trapEffectDefs: 12-byte-stride
    effect-definition records. Confirmed fields: +2 = g_pictureDir
    icon offset, +8 = cost-type flags (consumed by ApplyEffectCost),
    +0xA = display-mode flags (consumed by ApplyEffectAndDrawIconBar).
  0x95EB (linear 0x36E4B) -> g_partySlotAssignment: 4 entries x 2
    bytes, which 1-based g_partyRecords index occupies UI slot N
    (0 = empty).
  0x95F3 (linear 0x36E53) -> g_partyRecords: base of the party-member
    record array, confirmed fixed stride 0x1F4 (500) bytes/record via
    ApplyMapTriggerEffect's explicit index arithmetic. The previously-
    documented word_328D4 "linked list" of active party members is
    almost certainly iterating pointers into this same fixed array,
    not separately heap-allocated records.

Run via:
    .\run_ida_script.ps1 name_trap_effect_system.py
"""
import idc
import ida_name
import ida_bytes

FUNC_RENAMES = {
    0x19E56: "ApplyMapTriggerEffect",
    0x18068: "PrepareTrapEffectSlots",
    0x180BA: "ApplyEffectAndDrawIconBar",
    0x18257: "ApplyEffectCost",
}

DATA_RENAMES = {
    0x2E4B0: "g_partyEffectIconSlots",
    0x368BB: "g_trapEffectDefs",
    0x36E4B: "g_partySlotAssignment",
    0x36E53: "g_partyRecords",
}

for ea, name in FUNC_RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

for ea, name in DATA_RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x19E56,
    "Handles a movement-triggered map object (called from "
    "HandleMovementInput). Branches on [di+2] type flags: 0x4000 = "
    "teleport ([di+4]/[di+6] -> word_36CF7/word_36CF9, full redraw); "
    "0x2000 = separate effect (sub_1FC3F+sub_20C46, not traced); "
    "0x1000/0x800/0x400/0x300-pair = trap/status effects -- looks up "
    "an effect-definition record (PrepareTrapEffectSlots), then for "
    "each of the party's 4 slots (g_partySlotAssignment -> "
    "g_partyRecords, record = g_partyRecords+(slot-1)*0x1F4), skips "
    "members with status bits 0x1C40 set, otherwise fills their "
    "g_partyEffectIconSlots entry and calls ApplyEffectAndDrawIconBar.",
    False,
)
ida_bytes.set_cmt(
    0x18068,
    "PrepareTrapEffectSlots(ax=effect id): returns bx = "
    "g_trapEffectDefs + id*0xC (the effect-definition record). Also "
    "clears the [+8..+0x14) fields of all 4 g_partyEffectIconSlots "
    "entries first -- reset before repopulate. ax is left 0 on return.",
    False,
)
ida_bytes.set_cmt(
    0x180BA,
    "Iterates the 4 g_partyEffectIconSlots entries; for each occupied "
    "slot, draws its effect icon (via the effect-def's +2 "
    "g_pictureDir offset, one of 3 draw variants selected by the "
    "effect-def's +0xA flags) and applies the effect's cost via "
    "ApplyEffectCost.",
    False,
)
ida_bytes.set_cmt(
    0x18257,
    "Dispatches an effect-definition record's cost (di = "
    "g_trapEffectDefs entry, [di+8] flag bits) to one or more of "
    "DeductHPClamped/DeductMPClamped/SpendMaterialCounterClamped "
    "(HP/MP costs use [si+0xC]=party-member record + a plain amount; "
    "material costs use one of 3 BCD counters selected by a different "
    "flag bit, amount read from the slot's own +0x10 field).",
    False,
)
ida_bytes.set_cmt(
    0x2E4B0,
    "4 entries x 20 bytes. +0/+2, +4/+6: two (x,y) screen-position "
    "pairs (set once at init by sub_1251D). +8: a message/data value. "
    "+0xA: pointer to the effect-definition record (g_trapEffectDefs "
    "entry) for this slot. +0xC: pointer to the affected party-member "
    "record. +0x10/+0x12: extra position/amount fields, meaning varies "
    "by which ApplyMapTriggerEffect branch populated it.",
    False,
)
ida_bytes.set_cmt(
    0x368BB,
    "12-byte-stride trap/status-effect definition records, indexed by "
    "effect id (PrepareTrapEffectSlots computes id*0xC + this base). "
    "Confirmed fields: +2 = g_pictureDir icon offset, +8 = cost-type "
    "flags (ApplyEffectCost), +0xA = display-mode flags "
    "(ApplyEffectAndDrawIconBar).",
    False,
)
ida_bytes.set_cmt(
    0x36E4B,
    "4 entries x 2 bytes: which 1-based g_partyRecords index occupies "
    "UI/effect slot N (0 = empty).",
    False,
)
ida_bytes.set_cmt(
    0x36E53,
    "Base of the party-member record array. Confirmed fixed stride "
    "0x1F4 (500) bytes/record via ApplyMapTriggerEffect's explicit "
    "index arithmetic (record = this + (slot-1)*0x1F4).",
    False,
)
