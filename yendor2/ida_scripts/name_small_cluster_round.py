"""
Names 4 more small, previously-untraced helpers.

sub_2AE1A (called once from ShowPartyWipeScreen, the game-over/party-
death screen): clears several word_328Cx/word_328CA UI flag bits and
zeroes the confirmed g_combatTurnOrder table (0x539E, 7 entries x
8 bytes) -- resets combat UI/turn-order state after the party wipes.
-> ResetCombatStateOnPartyWipe

sub_1303C (called once from ShowClueCategoryEntries): calls
sub_12FB0 (untraced), then loops over a hit-test-style table at
0x68D2 (stride 0xA), assigning sequential 1-based ids into each
entry's [+8] field -- builds the id numbering for the clue book
category entry list. -> AssignClueCategoryEntryIds

sub_18095 (called from DeductHPClamped and ApplyEffectCost): converts
a party-record pointer (ax) back into its 1-based slot number (using
the confirmed g_partyRecords base 0x95F3 / stride 0x1F4), then scans
a 5-entry table at 0x94A3 and clears any entry matching that slot
number to 0 -- removes a party member's reference from some 5-slot
tracking list when they take HP damage (plausibly pending-effect or
spell-target slots; not independently confirmed which).
-> ClearPartySlotReferenceOnDamage

sub_1CC98 (called from UseItemType_400 and UseTrainingItem): copies
0x10 words from the current party record's [+0x72] to [+0x32], then
(skipping 4 bytes on both sides) another 0xE words from [+0x76] to
[+0x36] -- syncs a staged/computed stat region back into the record's
live fields after a training/service item use. -> SyncPartyRecordStagedStats

Run via:
    .\run_ida_script.ps1 name_small_cluster_round.py
"""
import idc
import ida_name
import ida_bytes

entries = [
    (0x2AE1A, "ResetCombatStateOnPartyWipe",
     "Clears UI flag bits and zeroes g_combatTurnOrder (0x539E). "
     "Called once from ShowPartyWipeScreen."),
    (0x1303C, "AssignClueCategoryEntryIds",
     "Assigns sequential 1-based ids into a hit-test table's [+8] "
     "field (stride 0xA, base 0x68D2), after calling untraced "
     "sub_12FB0. Called once from ShowClueCategoryEntries."),
    (0x18095, "ClearPartySlotReferenceOnDamage",
     "Converts a party-record pointer to its 1-based slot number "
     "and clears any matching entry in a 5-entry table at 0x94A3. "
     "Called from DeductHPClamped and ApplyEffectCost."),
    (0x1CC98, "SyncPartyRecordStagedStats",
     "Copies two staged stat regions ([+0x72]->[+0x32], "
     "[+0x76]->[+0x36]) within the current party record back into "
     "its live fields. Called from UseItemType_400 and "
     "UseTrainingItem."),
]

for ea, name, desc in entries:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
