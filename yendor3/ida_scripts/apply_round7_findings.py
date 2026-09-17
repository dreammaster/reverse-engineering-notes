"""
Round 7: continuing the low-confidence tier turned up a new failure
mode -- BinDiff swapping two related functions with each other, not
just matching one to something unrelated. See docs23/engine-diffs.md.

- sub_1D358 -> RestPartyAndAdvanceClock: BinDiff labeled this address
  "HandleRangedOrCombatAction" (0.26 similarity), but its full call
  sequence is yendor2's RestPartyAndAdvanceClock almost verbatim
  (ClearStatusPanelIfDirty/.../MaybeForceTickWorldAilments/
  ProcessLevelMonsters/ResetDailyAbilityCharges/.../
  ApplyRestEffectsToCharacter/.../TickTravelResourceAilments at the
  tail) -- not the combat/AnimateProjectileStep logic
  HandleRangedOrCombatAction should have at all.
- sub_1D7F2 -> IsRestingAllowedHere: found as RestPartyAndAdvanceClock's
  own first call (replacing what a bad low-confidence match had
  separately -- and wrongly -- labeled "IsRestingAllowedHere" at a
  different address entirely, see round 3). Confirmed by behavior:
  checks a global flag for a forbidden-rest condition, then
  IsPositionInTriggerList for a special-cell block, exactly matching
  the "YOU CAN NOT REST HERE" eligibility check.
- sub_237BA -> RunCharacterDetailOverlay: BinDiff labeled this address
  "DrawShadowedTextAlt" (0.04 similarity), but its call sequence
  matches yendor2's RunCharacterDetailOverlay call-for-call in order,
  plus one new ReassignPartySlotReference call. yendor3's real
  DrawShadowedTextAlt, and the real identity of the address BinDiff
  mislabeled "RunCharacterDetailOverlay", are both still unknown.
- sub_136F6 -> ClearPartySlotReferenceOnDamage: confirmed real --
  gained exactly the call its own name implies it always needed
  (ReassignPartySlotReference), even though yendor2's version had no
  sub-calls at all (fully inlined).

Run via:
    .\run_ida_script.ps1 apply_round7_findings.py
"""
import idc
import ida_name

RENAMES = [
    (0x1D358, "RestPartyAndAdvanceClock"),
    (0x1D7F2, "IsRestingAllowedHere"),
    (0x237BA, "RunCharacterDetailOverlay"),
    (0x136F6, "ClearPartySlotReferenceOnDamage"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
