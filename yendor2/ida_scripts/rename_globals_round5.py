"""
Round 5 of global-variable renaming: pushing past the pure
frequency-count list into globals with partial documentation that
needed a bit more cross-referencing/verification to confirm.

- word_36CA7 -> g_mapRevealAreaTier, word_36CA9 -> g_monsterDetailRevealTier:
  two of the three "+0x58/+0x64/+0x66 party-average derived stat"
  tiered systems documented in file-formats.md. +0x66's average
  (word_36CA7) gates RevealMapRegion's reveal-area size (a Locate/
  Scout/Magic-Mapping-style ability); +0x58's average (word_36CA9)
  gates progressively-revealed monster-detail icons in
  DrawMonsterInfoPanel (a perception/identify-style derived stat).
  Both explicitly called "now on solid ground" in file-formats.md.
  Left the third of the trio, word_36CA5 (+0x64's average, feeding the
  minimap/lighting bitfield word_36C7F), unrenamed -- file-formats.md
  still flags its underlying real-world identity (torch fuel vs.
  mapping) as an unconfirmed guess.
- word_3297E -> g_forcedMusicTrack: the "forced track" override
  UpdateAmbientMusic checks (0 = let the ambient day/night system
  choose); RunTitleScreen sets it to 1 on entry and clears it to 0 on
  exit, and it's briefly forced to 0 (silence) during character
  creation.
- word_36D03/05/07/09/0B -> g_partyRoleAssignment1..5: a confirmed
  5-slot array, each holding the roster-slot number of whichever party
  member currently holds one of 5 assignable practical roles (navigator/
  mapper/barterer-style skills per the attribute survey);
  DrawCharacterStatSheet highlights a character's derived-stat field
  when their own slot number matches. Individual role-to-name mapping
  isn't confirmed, only the "5 assignable roles" array shape.
- word_3330A -> g_clueBookClassId: the current class id
  ShowClueBookSpellDetail's helpers (DrawSpellLevelForCurrentClass,
  DrawClassEligibilityMarker) compare against for the spell-detail
  screen's per-class level/eligibility display.
- word_3293A -> g_pagedEntryIndex: the current page index for the
  generic ShowPagedEntryScreen paginated single-entry viewer (up to 31
  entries), read by UpdateScrollArrows to show/hide the scroll arrows.
- word_2E386 -> g_mapEditorFloorType, word_2E384 -> g_mapEditorWallType:
  the map legend editor's currently-selected floor/wall tile type
  numbers, set by EditFloorLegendTypeNumber/EditWallLegendTypeNumber
  and read by DrawFloorTypeLegendRow/DrawWallTypeLegendRow/
  DrawMapEditorFloorTypeReadout.
- word_2E544 -> g_attacksRemaining: the "attempts left" multi-shot
  attack counter, decremented per hit that doesn't kill the target,
  continuing the ranged/spell-cast attack loop to the next depth row.
- word_3685F -> g_exploredMapBitmapBase: the base offset
  PersistExploredCell adds to (x/8) to compute the CURGAME byte
  offset for the bit-packed "explored" automap bitmap (bit = x%8) --
  why the automap survives save/load.
- word_2E782 -> g_dragCursorX, word_2E784 -> g_dragCursorY: the
  accumulated drag-cursor position ClampDragCursorPosition clamps
  within bounds and offsets by (8,8) for the held/dragged item's draw
  position; confirmed cx/dx (x/y) register usage in the disassembly.
- word_328FE -> g_clueBookIconSelectionMask: a bit-per-icon toggle
  mask for the clue book's clickable sub-icon selector strip
  (DrawSubIconSelectorRow's region table), shared by
  RunClueBookItemCategory/RunClueBookWeaponCategory.

Run via:
    .\run_ida_script.ps1 rename_globals_round5.py
"""
import idc
import ida_name

RENAMES = [
    (0x36CA7, "g_mapRevealAreaTier"),
    (0x36CA9, "g_monsterDetailRevealTier"),
    (0x3297E, "g_forcedMusicTrack"),
    (0x36D03, "g_partyRoleAssignment1"),
    (0x36D05, "g_partyRoleAssignment2"),
    (0x36D07, "g_partyRoleAssignment3"),
    (0x36D09, "g_partyRoleAssignment4"),
    (0x36D0B, "g_partyRoleAssignment5"),
    (0x3330A, "g_clueBookClassId"),
    (0x3293A, "g_pagedEntryIndex"),
    (0x2E386, "g_mapEditorFloorType"),
    (0x2E384, "g_mapEditorWallType"),
    (0x2E544, "g_attacksRemaining"),
    (0x3685F, "g_exploredMapBitmapBase"),
    (0x2E782, "g_dragCursorX"),
    (0x2E784, "g_dragCursorY"),
    (0x328FE, "g_clueBookIconSelectionMask"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
