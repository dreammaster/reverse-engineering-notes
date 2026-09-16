"""
Round 7 of global-variable renaming: more fresh-disassembly finds.

- word_2E776 -> g_mouseCursorX, word_31956 -> g_mouseCursorY: the mouse
  cursor's current on-screen draw position. Written by the cursor-draw
  routines (including a fixed default of (0xE6,0xB4) at one site) and
  read by RestoreCursorBackground to erase the cursor from its last
  drawn position before it moves -- distinct from the round-6
  g_mouseLeftDownX/Y-style click-event latches, which record where a
  button transition happened rather than where the cursor currently is.
- word_2E500 -> g_lastEmsMappingArrayPtr: read directly from
  MapUnmapPages (the central EMS page-mapping primitive): caches its
  bx parameter (a pointer to the mapping-array descriptor) and skips
  the actual `int 67h` EMS call entirely if called again with the same
  pointer -- a call-memoization cache avoiding redundant EMS remaps.
- word_2E4A6 -> g_currentMusicTrack: the currently-playing/forced music
  track id. RefreshDungeonMapWindow compares it against two
  region-specific track ids to decide whether to stop the current
  track before a region change; an existing inline comment on the
  driver-reset path already calls it the "forced-track tracker".

Run via:
    .\run_ida_script.ps1 rename_globals_round7.py
"""
import idc
import ida_name

RENAMES = [
    (0x2E776, "g_mouseCursorX"),
    (0x31956, "g_mouseCursorY"),
    (0x2E500, "g_lastEmsMappingArrayPtr"),
    (0x2E4A6, "g_currentMusicTrack"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
