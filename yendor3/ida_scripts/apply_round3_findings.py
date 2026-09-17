"""
Round 3: first batch of mid-confidence-tier (0.70-0.95) matches,
spot-checked the same way as the high-confidence tier. See
docs23/engine-diffs.md for the full writeup.

- ds:0xCF63 -> g_driverStateFlags: confirmed via
  ParseSoundBlasterEnvironmentVariable, which ANDs/ORs this address
  with the exact same masks (0x3FF0/0x8000/0xC000) yendor2's version
  uses on g_driverStateFlags. This also resolves round 1's open
  question about what sub_286D8 tests (bit 0x8 of this same flags
  word -- one of the two bits DrawCheckboxIndicator already gates the
  pause-menu MUSIC/SOUND FX checkboxes on).
- The following 20 are the same function as their yendor2 counterpart,
  confirmed via called-target diff; several carry a real, documented
  difference noted in engine-diffs.md rather than repeated here:
  ResolveAttackerActionOutcome, TryDrawDungeonCellSideFeature,
  ApplyIconBarStatDelta, DrawAlchemyStatusPanel (lost one BCD display),
  DrawMapEditorInteractionTypeOverlay, DrawDungeonCellSideFeature (new
  DrawViewportSprite call + tile-classification helper),
  HasDroppableItemInInventory, DrawPicture, ShowClueBookMonsterDetail,
  DrawCellIconPair, BuildItemUseMessage, IsItemDroppable,
  ShowArmorAttributeBonusList, HandleGameCommand (several branches use
  BinDiff matches that still need individual re-verification -- treat
  the dispatch structure as confirmed, not every branch),
  LoadItemData (new ReassignPartySlotReference + TestGlobalFlag calls),
  ParseSoundBlasterEnvironmentVariable (dropped a fallback env-var
  lookup), HasDroppableItemInContainer, IsDestinationUnlocked (new
  DrawStringColumn tail), DrawCharacterStatSheet (one fewer stat
  display), TryTravelToClickedMapCell (new ComputeMapCellIndex/
  ReadMapCellAttributeByte calls, part of a broader map-cell-lookup
  refactor also seen in RunAlchemyScreen).

Run via:
    .\run_ida_script.ps1 apply_round3_findings.py
"""
import idc
import ida_name

RENAMES = [
    (0xCF63, "g_driverStateFlags"),
    (0x11318, "ResolveAttackerActionOutcome"),
    (0x20657, "TryDrawDungeonCellSideFeature"),
    (0x137D9, "ApplyIconBarStatDelta"),
    (0x1D27B, "DrawAlchemyStatusPanel"),
    (0x1FC03, "DrawMapEditorInteractionTypeOverlay"),
    (0x2065F, "DrawDungeonCellSideFeature"),
    (0x2559F, "HasDroppableItemInInventory"),
    (0x29AFC, "DrawPicture"),
    (0x17C78, "ShowClueBookMonsterDetail"),
    (0x1FD35, "DrawCellIconPair"),
    (0x19312, "BuildItemUseMessage"),
    (0x25512, "IsItemDroppable"),
    (0x18398, "ShowArmorAttributeBonusList"),
    (0x29868, "HandleGameCommand"),
    (0x1A1B7, "LoadItemData"),
    (0x28868, "ParseSoundBlasterEnvironmentVariable"),
    (0x255FF, "HasDroppableItemInContainer"),
    (0x155A3, "IsDestinationUnlocked"),
    (0x2488F, "DrawCharacterStatSheet"),
    (0x2952B, "TryTravelToClickedMapCell"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
