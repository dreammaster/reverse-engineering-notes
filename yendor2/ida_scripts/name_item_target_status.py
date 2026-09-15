"""
Traced word_2E40C's other writers (besides ApplyItemEffectFlags) --
a cluster of small functions that classify the CURRENTLY-TARGETED
party member's condition for display, used while picking a target to
use an item on (word_32924, "which party slot" -> FindPartySlotForRecord
earlier, here read the other way to get back the record).

- sub_27A56 -> TestRecordFlag_10C: the missing Test accessor for the
  +0x10C per-record flag bank (GetRecordFlagBitAndWord_10C/
  SetRecordFlag_10C, named 2 rounds ago). ZF = ([si+bank] & mask)==0.

- sub_1B74A -> ClassifyPartyMemberCondition: looks up the targeted
  party member and checks 3 conditions -- status bit 0x40 ("dead"?),
  a wider status mask 0xFF80, and current HP < max HP -- counting how
  many are true. Sets word_2E40C to a tier: 0x2000/0x4000/0x8000 for
  whichever specific condition(s) hit, then an overall summary bit:
  0x1000 if 2+ conditions are true, 0x200 if none are. Plausibly picks
  which status icon/message to show for this target.

- sub_1B717/sub_1B7A5 -> CheckPartyMemberItemFlag /
  CheckPartyMemberItemFlagAndClearPanel: near-identical (the latter
  additionally calls ClearStatusPanelIfDirty first). Look up the
  targeted party member, call TestRecordFlag_10C using the current
  item catalog record's own +0x1A field (the SAME index
  SetRecordFlag_10C uses when marking an item as used/read -- see 3
  rounds ago) -- if NOT set (this member hasn't triggered this item's
  personal flag yet), sets word_2E40C bit 0x8000. Reads as an "already
  used/read this on this character?" indicator for the target-status
  display.

- sub_16E18 -> ClearStatusPanelIfDirty: gated on word_328C4 bit 0x100
  (the widespread "redraw needed" dirty flag) -- if set, blits a fill
  pattern over fixed screen regions via the EMS page-frame trick (same
  technique as ShowResourceDepletedOverlay). Called extremely widely,
  including directly from `start` -- a general "erase the status panel
  before redrawing it" step.

Run via:
    .\run_ida_script.ps1 name_item_target_status.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x27A56: "TestRecordFlag_10C",
    0x1B74A: "ClassifyPartyMemberCondition",
    0x1B717: "CheckPartyMemberItemFlag",
    0x1B7A5: "CheckPartyMemberItemFlagAndClearPanel",
    0x16E18: "ClearStatusPanelIfDirty",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x27A56,
    "TestRecordFlag_10C(si=record, ax=flag index): ZF = "
    "([si+0x10C-bank] & mask)==0, via GetRecordFlagBitAndWord_10C.",
    False,
)
ida_bytes.set_cmt(
    0x1B74A,
    "Classifies the targeted party member's (word_32924) condition "
    "into word_2E40C: checks status bit 0x40, status mask 0xFF80, and "
    "HP<maxHP, setting 0x2000/0x4000/0x8000 for whichever hit, plus "
    "an overall tier (0x1000 if 2+, 0x200 if none) -- plausibly "
    "selects a status icon/message for a target-selection display.",
    False,
)
ida_bytes.set_cmt(
    0x1B717,
    "Looks up the targeted party member (word_32924) and tests "
    "whether they've already triggered the current item's personal "
    "flag (TestRecordFlag_10C, index from the item catalog's own "
    "+0x1A field -- the same index SetRecordFlag_10C uses to mark it "
    "used). Sets word_2E40C bit 0x8000 if not yet triggered.",
    False,
)
ida_bytes.set_cmt(
    0x1B7A5,
    "Same as CheckPartyMemberItemFlag, plus a leading "
    "ClearStatusPanelIfDirty call.",
    False,
)
ida_bytes.set_cmt(
    0x16E18,
    "Gated on word_328C4 bit 0x100 (the widespread 'redraw needed' "
    "dirty flag): if set, blits a fill pattern over fixed screen "
    "regions via the EMS page-frame trick (same technique as "
    "ShowResourceDepletedOverlay) -- erases the status panel before "
    "it gets redrawn. Called very widely, including directly from "
    "`start`.",
    False,
)
