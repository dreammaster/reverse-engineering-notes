"""
Names two more small helpers.

sub_1E447, called once from RestoreOrSelectAlchemyCaster's fallback
path: scans all 4 g_partySlotAssignment slots for the first occupied
one whose resolved record has [+0x94] set (the same "class-tier
eligible" marker ApplySecondaryClassTierFlags gates on), and sets
word_32924 to it -- picks a default alchemy caster when no valid
cached selection exists. -> SelectDefaultAlchemyCaster

sub_1CC70, called from UseItemType_400 and UseTrainingItem: copies 30
words (60 bytes) from the current party member's record
(word_328D4+0x32 onward) into the *same relative offsets*, but in a
different segment (word_2E4AA, a separate EMS-mapped bank also used
elsewhere this session for icon-bar/scratch data) starting at
+0x72 -- exactly the confirmed base-to-derived stat offset (+0x40).
Then calls UpdatePartyAverageStatTiers. Plausibly snapshots the
attribute/derived-stat block into an EMS-mapped preview/comparison
buffer for a stat-changing item's before/after display, though the
destination buffer's exact purpose isn't confirmed.
-> CopyPartyStatBlockToEmsCache

Run via:
    .\run_ida_script.ps1 name_default_caster_and_stat_snapshot.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x1E447: "SelectDefaultAlchemyCaster",
    0x1CC70: "CopyPartyStatBlockToEmsCache",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1E447,
    "Scans g_partySlotAssignment for the first occupied slot whose "
    "record has [+0x94] set, sets word_32924 to it -- default "
    "alchemy caster fallback. Called from "
    "RestoreOrSelectAlchemyCaster.",
    False,
)
ida_bytes.set_cmt(
    0x1CC70,
    "Copies 30 words from word_328D4+0x32 to the same relative "
    "offset (+0x72, the confirmed base->derived stat delta) in "
    "segment word_2E4AA, then calls UpdatePartyAverageStatTiers -- "
    "plausibly a before/after stat snapshot for a stat-changing item. "
    "Called from UseItemType_400 and UseTrainingItem.",
    False,
)
