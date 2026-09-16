"""
Round 6 of global-variable renaming: the first round built on fresh
disassembly tracing rather than harvesting existing docs prose -- the
frequency-count/doc-mention approach is largely exhausted, so this
round read actual code (yendor2.asm) to confirm new candidates.

**Major find: the core mouse-event position-latch cluster.** Read the
INT 33h mouse-callback handler directly (yendor2.asm around 0x23717):
it's a standard MS Mouse user callback -- AX on entry is the condition
mask (bit 1=left-down, bit 2=left-up, bit 3=right-down, bit 4=right-up,
matching the classic `test ax,2/4/8/0x10` sequence seen here), and
after converting the raw motion counters through ClampDragCursorPosition
(which loads CX/DX from the clamped drag-cursor position before
returning), the handler latches CX/DX into a different global pair per
event type:
- word_2E76E/word_2E770 -> g_mouseLeftDownX/g_mouseLeftDownY (bit 2):
  the left-click position read throughout the game's click/hit-test
  handlers (catalog-slot clicks, portrait drag-and-drop, map editor
  cell painting, etc.) -- explains its very high reference count.
- word_3194E/word_31950 -> g_mouseLeftUpX/g_mouseLeftUpY (bit 4).
- word_2E772/word_2E774 -> g_mouseRightDownX/g_mouseRightDownY (bit 8):
  used by RunMapEditorScreen's right-click overlay/wall-tile paint
  handler.
- word_31952/word_31954 -> g_mouseRightUpX/g_mouseRightUpY (bit 0x10).

**The drag-cursor clamp bounds**, read directly from
ClampDragCursorPosition's own comparisons: word_2E77A/word_2E778 are
the min/max X bounds for g_dragCursorX (first check clamps UP to
word_2E77A if below it = min; second clamps DOWN to word_2E778 if
above it = max), and word_3195A/word_31958 are the equivalent min/max
Y bounds for g_dragCursorY. RunMapEditorScreen temporarily overrides
and restores all 4 on entry/exit.
- word_2E77A -> g_dragCursorMinX, word_2E778 -> g_dragCursorMaxX
- word_3195A -> g_dragCursorMinY, word_31958 -> g_dragCursorMaxY

**Map editor legend scroll indices**: word_2E496/word_2E4A2 are
initialized to 0 alongside g_mapEditorWallType/g_mapEditorFloorType and
"nudge toward" them (per an existing comment on the wall side), driving
the animated scroll position of the two 17-icon legend strips
(DrawWallTypeLegendRow's table 0xE551 / DrawFloorTypeLegendRow's table
0xE175) -- distinct from the actual selected type value.
- word_2E496 -> g_mapEditorWallScrollIndex
- word_2E4A2 -> g_mapEditorFloorScrollIndex

**word_2E49E -> g_stagedAttackTypeFlags**: the third member of the
attack-resolution staging trio alongside g_stagedAttackDamage/
g_stagedAttackStatusFlags -- confirmed by an existing inline comment:
"attack type flags vs [si+0x98] resistance flags -- each match halves
the damage".

Run via:
    .\run_ida_script.ps1 rename_globals_round6.py
"""
import idc
import ida_name

RENAMES = [
    (0x2E76E, "g_mouseLeftDownX"),
    (0x2E770, "g_mouseLeftDownY"),
    (0x3194E, "g_mouseLeftUpX"),
    (0x31950, "g_mouseLeftUpY"),
    (0x2E772, "g_mouseRightDownX"),
    (0x2E774, "g_mouseRightDownY"),
    (0x31952, "g_mouseRightUpX"),
    (0x31954, "g_mouseRightUpY"),
    (0x2E77A, "g_dragCursorMinX"),
    (0x2E778, "g_dragCursorMaxX"),
    (0x3195A, "g_dragCursorMinY"),
    (0x31958, "g_dragCursorMaxY"),
    (0x2E496, "g_mapEditorWallScrollIndex"),
    (0x2E4A2, "g_mapEditorFloorScrollIndex"),
    (0x2E49E, "g_stagedAttackTypeFlags"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
