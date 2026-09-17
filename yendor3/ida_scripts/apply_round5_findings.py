"""
Round 5: first batch of the low-confidence tier (<0.70 similarity).
See docs23/engine-diffs.md for the full writeup. Two matches from this
batch (CheckAndPaySpecialItemCost, DispatchItemAbilityCommand) are
deliberately NOT renamed -- both grew enormously (+41 and +70
instructions) with substantially different call sequences, needing a
dedicated read rather than a quick confirmation.

Straightforward same-function confirmations (several carry a real,
documented difference noted in engine-diffs.md):
    RedrawItemDescriptionAndMaterials, ExtendDungeonCeilingTexture,
    DrawResourceCounterPanel, ShowAbilityDescriptionColumn,
    ClearOffscreenBuffer, RestoreClueBookBackgroundFromEMS (extra
    MapUnmapPages call), ShowConsumableItemTypeLegend, ShowGameClockCommand,
    DrawItemTypeLegendAttributeRow, DrawItemTypeLegendSkillRow (both
    gained a new AccumulateTextColumnWidth call, named below),
    InitGlobals, DrawMessageBox (new StrLen call), DeductAlchemySpellCosts
    (lost one of its two cost-deduction calls -- see the "one fewer
    value" pattern note), TickTravelResourceAilments (its 3
    CheckAndTickAvailableAilment calls consolidated into a single new
    helper, sub_109C8, not yet examined), SaveClueBookBackgroundToEMS
    (extra MapUnmapPages call), TravelToDestination (new
    ApplyMapTriggerEffect call), FinalizeCharacterCreation (reworked to
    fit round 4's new character-creation opening sequence).

- sub_17132 -> AccumulateTextColumnWidth: computes a label's string
  length, scales it by 6 (pixel width per character) and accumulates
  it into a running column-position global (ds:0x53FA) -- lets the
  item-type-legend rows auto-advance their draw position based on
  label length instead of a fixed offset.

Run via:
    .\run_ida_script.ps1 apply_round5_findings.py
"""
import idc
import ida_name

RENAMES = [
    (0x2121F, "RedrawItemDescriptionAndMaterials"),
    (0x201B1, "ExtendDungeonCeilingTexture"),
    (0x27184, "DrawResourceCounterPanel"),
    (0x2971B, "ShowAbilityDescriptionColumn"),
    (0x2BC65, "ClearOffscreenBuffer"),
    (0x188B2, "RestoreClueBookBackgroundFromEMS"),
    (0x16F2F, "ShowConsumableItemTypeLegend"),
    (0x2825B, "ShowGameClockCommand"),
    (0x1709C, "DrawItemTypeLegendAttributeRow"),
    (0x170E7, "DrawItemTypeLegendSkillRow"),
    (0x1F040, "InitGlobals"),
    (0x18BE4, "DrawMessageBox"),
    (0x1D336, "DeductAlchemySpellCosts"),
    (0x10999, "TickTravelResourceAilments"),
    (0x18B92, "SaveClueBookBackgroundToEMS"),
    (0x15512, "TravelToDestination"),
    (0x2BBD3, "FinalizeCharacterCreation"),
    (0x17132, "AccumulateTextColumnWidth"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
