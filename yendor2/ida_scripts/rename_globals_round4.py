"""
Round 4 of global-variable renaming: attack-resolution staging globals
and the clue book's category/scroll/selection state.

- word_3292C -> g_viewportRowDepth: the first-person dungeon corridor
  renderer's per-frame row-depth counter, reset by RenderDungeonViewport
  before its 7 RenderDungeonViewRow calls and incremented/decremented
  per cell; also gates TryTriggerMonsterEncounterAtCell (only fires at
  max depth) and TryActivateMonsterByDistance's detection-range check.
- word_2E49C -> g_stagedAttackDamage, word_2E49A ->
  g_stagedAttackStatusFlags: the pending attack-resolution commit pair.
  ApplyTargetResistancesToAttack negates/halves g_stagedAttackDamage on
  resistance/immunity matches and accumulates filtered status-effect
  bits into g_stagedAttackStatusFlags; ApplyAttackToTarget's early
  returns all re-check both are 0 (a completed-commit sentinel), and
  ReapplyDamageWithCompoundedResistance runs a second compounding pass
  over the same pair.
- word_2E3F6 -> g_clueBookCategory: the clue book's current top-level
  category selector, confirmed ("word_2E3F6 is a clue category") and
  used throughout ShowClueBook's whole F-key/category dispatch switch.
- word_2E3F0 -> g_clueEntryScrollOffset: the clue entry list's scroll
  offset, paged by a fixed 0x38-entry page size.
- word_2E3EE -> g_clueEntrySelectedIndex: the current highlighted entry
  index within the clue entry list, adjusted alongside the scroll
  offset.

Run via:
    .\run_ida_script.ps1 rename_globals_round4.py
"""
import idc
import ida_name

RENAMES = [
    (0x3292C, "g_viewportRowDepth"),
    (0x2E49C, "g_stagedAttackDamage"),
    (0x2E49A, "g_stagedAttackStatusFlags"),
    (0x2E3F6, "g_clueBookCategory"),
    (0x2E3F0, "g_clueEntryScrollOffset"),
    (0x2E3EE, "g_clueEntrySelectedIndex"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
