"""
Round 2 of high-confidence-tier spot-checking (remaining ~27 of the 68
functions). See docs23/engine-diffs.md for the full writeup.

- ds:0xCF81 -> g_partySlotAssignment: confirmed via several existing
  inline comments in yendor3.asm that already describe this exact
  address as "the matching g_partySlotAssignment entry
  (0x95EB/0x95ED/0x95EF/0x95F1)" -- same global as yendor2's, just not
  yet renamed. (Comments referencing the raw hex offsets there are
  yendor2 addresses baked into carried-over prose; the live yendor3
  address for the table base is 0xCF81.)
- sub_26D47 -> ClearCharacterFromPartySlots: loops the 4
  g_partySlotAssignment entries, zeroing any that equal the input (bx)
  -- removes a character's active-roster-slot reference wherever it
  appears.
- sub_26D2E -> ReassignPartySlotReference: wraps the above (always
  clears bx from the roster first) and, when called with an output
  pointer (si != 0), also writes bx into [si] -- a "move this
  character's slot reference" primitive. New in Chapter 3, called from
  10 different sites (UseAbilityCommand, RunConversation,
  HandleSearchCommand, RepairItemCommand, RunPartyMemberDetailScreen,
  and others) -- plausibly a fix for a stale-slot-reference bug when
  reassigning/incapacitating characters mid-action.
- sub_1B704 -> ApplyScriptedMapCellOverrides: checks the current map
  cell's position against 2 fixed coordinate pairs; if it matches one
  and a specific global flag (checked via TestGlobalFlag) is set, sets
  a new flag bit (0x200) on the cell record and overwrites its overlay
  tile id. Reads as a scripted/quest-flag-gated map decoration system,
  called from RefreshDungeonMapWindow. No yendor2 equivalent.
- sub_1D76B -> RefreshMultiStatEffectsAlt: the same remove-then-
  reapply multi-stat-effect pattern as round 1's RefreshMultiStatEffects
  (identical field offsets/strides), but with an added raw block-copy
  step (ds:0xF44-segment, offsets 0x72->0x32) in between -- likely a
  variant syncing an additional field range. Called from
  ApplyRestEffectsToCharacter. No yendor2 equivalent.

Run via:
    .\run_ida_script.ps1 apply_round2_findings.py
"""
import idc
import ida_name

RENAMES = [
    (0xCF81, "g_partySlotAssignment"),
    (0x26D47, "ClearCharacterFromPartySlots"),
    (0x26D2E, "ReassignPartySlotReference"),
    (0x1B704, "ApplyScriptedMapCellOverrides"),
    (0x1D76B, "RefreshMultiStatEffectsAlt"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
