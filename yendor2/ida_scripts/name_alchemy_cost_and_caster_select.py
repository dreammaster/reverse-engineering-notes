"""
Names two more RunAlchemyScreen helpers.

sub_1E61B, called once from RunAlchemyScreen: caches the current
caster id (word_328D6 -> word_36CCD), subtracts an MP cost
(word_332D2) from the current party member's MP ([bx+0x54]), then
pays two BCD-counter costs via SubtractFromBCDCounter: NUORE
(word_332D4, counter 0x94BB) and MAGIC ORE (word_332D6, counter
0x94B7). In short: deducts the MP/NUORE/ORE costs for casting an
alchemy spell, matching the documented "MP:"/"NUORE:"/"ORE:" cost
fields. -> DeductAlchemySpellCosts

sub_1E473, called once from RunAlchemyScreen (at screen entry):
resets word_32924 (a party-record slot pointer, the same one
AccumulateLearnedAbilityFlags reads) to 0. If a previously-cached
caster id (word_36CCD) exists, resolves it via SelectPartyRecordById
and checks [bx+0x94] (the same "class-tier eligible" marker field
ApplySecondaryClassTierFlags gates on); if set, scans the confirmed
g_partySlotAssignment table for a slot matching the current caster id
(word_328D6) and, if found, sets word_32924 to that slot's address.
Falls back to unnamed sub_1E447 (a default-selection helper) in every
other case. In short: re-validates/restores the previously-selected
alchemy caster when re-entering the screen, falling back to a default
selector otherwise. -> RestoreOrSelectAlchemyCaster

Run via:
    .\run_ida_script.ps1 name_alchemy_cost_and_caster_select.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x1E61B: "DeductAlchemySpellCosts",
    0x1E473: "RestoreOrSelectAlchemyCaster",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1E61B,
    "Deducts alchemy spell costs: MP (word_332D2) from [bx+0x54], "
    "NUORE (word_332D4, counter 0x94BB) and MAGIC ORE (word_332D6, "
    "counter 0x94B7) via SubtractFromBCDCounter. Called from "
    "RunAlchemyScreen.",
    False,
)
ida_bytes.set_cmt(
    0x1E473,
    "Re-validates the cached caster (word_36CCD) via "
    "SelectPartyRecordById + [bx+0x94] check, finds their slot in "
    "g_partySlotAssignment matching word_328D6, and sets word_32924 "
    "to it; falls back to sub_1E447 otherwise. Called from "
    "RunAlchemyScreen (screen entry).",
    False,
)
