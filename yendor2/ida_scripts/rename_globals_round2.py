"""
Round 2 of global-variable renaming: continuing down the
reference-count-ranked candidate list from yendor2.asm, picking the
next batch of single-clear-meaning globals confirmed by existing
file-formats.md/overview.md documentation.

- word_36CF7 -> g_partyWorldX, word_36CF9 -> g_partyWorldY: the
  party's current world position, confirmed multiple times (e.g.
  overview.md: "sets word_36CF7/word_36CF9 (the confirmed party world
  X/Y position) directly to the ... starting world position
  word_36CF7/word_36CF9 = 0xA6/0x24"). Also read by ProcessLevelMonsters
  to step monsters toward the player.
- word_36CF5 -> g_partyFacing: the party's current facing direction
  (0-3 tier value), read throughout the compass/minimap-icon/dungeon
  side-feature cluster (ShowCompassDirection, DrawMinimapCompassIcon,
  DrawDungeonCellSideFeature, SpawnMonsterInFacingDirection) via a
  consistently-documented "tier-bit convention".
- word_2E546 -> g_currentItemRecord: the current item catalog record
  pointer, set by LoadItemCatalogRecord and read throughout the
  shop/trade item-description redraw path (confirmed:
  "word_2E546 as the current-item-record pointer").
- word_31946 -> g_heldItemType: the currently-held (dragged) item's
  type/id, checked against 0 (none) by ClampDragCursorPosition to
  decide whether to offset the drag cursor hotspot.
- word_36D01 -> g_gameClockMinutes: the master "minutes since
  midnight" counter (0-1439), advanced by AdvanceGameClock and
  RestPartyAndAdvanceClock, read by ShowGameClockCommand/
  ComputeGameClockTime and the day/night ambient-music switch.
  Confirmed in file-formats.md as "Full mechanism traced".
- word_36CFB -> g_gameDay, word_36CFD -> g_gameMonth,
  word_36CFF -> g_gameYear: the in-game calendar counters
  AdvanceGameClock rolls on a g_gameClockMinutes >= 1440 rollover
  (30-day months, 12-month years; new-game start day 4, month 11,
  year 0x222).
- word_32A1E -> g_activeCombatMonster: the currently-targeted monster
  in formal (row-based) combat, read directly by
  ResolveAttackOrAbilityAction once a target is already selected;
  confirmed explicitly in file-formats.md ("word_32A1E (the active
  combat monster)").

Run via:
    .\run_ida_script.ps1 rename_globals_round2.py
"""
import idc
import ida_name

RENAMES = [
    (0x36CF7, "g_partyWorldX"),
    (0x36CF9, "g_partyWorldY"),
    (0x36CF5, "g_partyFacing"),
    (0x2E546, "g_currentItemRecord"),
    (0x31946, "g_heldItemType"),
    (0x36D01, "g_gameClockMinutes"),
    (0x36CFB, "g_gameDay"),
    (0x36CFD, "g_gameMonth"),
    (0x36CFF, "g_gameYear"),
    (0x32A1E, "g_activeCombatMonster"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
