"""
Round 3 of global-variable renaming: continuing down the
reference-count-ranked candidate list, picking the next batch of
single-clear-meaning globals confirmed by existing documentation.

- word_2E40A -> g_currentCommandCode: the "current key/command code"
  convention documented across several dispatch switches (ShowClueBook's
  F-key switch, IsItemEligibleForCommand's eligibility gate,
  ClearDepletedResourceCounterForCommand) -- distinct from
  g_lastKeyChar (the raw PollKeyboardInput char) and g_currentActionId
  (the ability/item id HandleGameCommand dispatches on).
- word_36863 -> g_groundItemSlotRecord: "the confirmed ground/
  world-object item slot record cache", written by
  LoadGroundItemSlotRecord and read by PlaceItemOnGround.
- word_2E562 -> g_dungeonMapGridSegment, word_2E564 ->
  g_dungeonMapGridOriginRow, word_2E55C -> g_dungeonMapGridOriginCol:
  the in-memory dungeon map grid RefreshDungeonMapWindow (re)builds
  from WORLD.DAT -- 78x78 cells, 8 bytes/cell, found via GetMapCellPtr:
  "segment word_2E562, with the grid's own origin held in word_2E564
  (row/y)/word_2E55C (column/x)".
- word_32926 -> g_shadeShiftDelta: confirmed via a multi-round trace
  (file-formats.md) to be the distance-based lighting shade-shift delta
  DrawPicture applies to its palette lookup, fed by
  ApplyDistanceShadingToFloorOrCeiling's 7-entry gradient table.
- word_2E54A -> g_itemStatEffectTable: the item's multi-stat-effect
  table, walked by both ApplyMultiStatEffectForItem (add) and
  RemoveMultiStatEffect (subtract) to apply/reverse an equipped item's
  stat bonuses.
- word_36CE7 -> g_animationSpeed: the confirmed animation-speed setting,
  cycled through by CycleAnimationSetting.
- word_328FA -> g_wipeEffectX, word_32900 -> g_wipeEffectY: the current
  pixel position SetWipeEffectPixel/RestoreWipeEffectPixel stash/
  restore around, driving the character-creation intro's scan-line wipe
  effect.

Run via:
    .\run_ida_script.ps1 rename_globals_round3.py
"""
import idc
import ida_name

RENAMES = [
    (0x2E40A, "g_currentCommandCode"),
    (0x36863, "g_groundItemSlotRecord"),
    (0x2E562, "g_dungeonMapGridSegment"),
    (0x2E564, "g_dungeonMapGridOriginRow"),
    (0x2E55C, "g_dungeonMapGridOriginCol"),
    (0x32926, "g_shadeShiftDelta"),
    (0x2E54A, "g_itemStatEffectTable"),
    (0x36CE7, "g_animationSpeed"),
    (0x328FA, "g_wipeEffectX"),
    (0x32900, "g_wipeEffectY"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
