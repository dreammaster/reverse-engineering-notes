"""
Round 9 of global-variable renaming: more structures found via fresh
disassembly tracing, continuing the focus on likely structures and
bitfields.

- word_32DCE -> g_lockStatusFlags: the currently-examined lock/door's
  status flags, loaded by LoadLockState ("feeds word_32DCE and
  friends") and decoded by ShowLockStatus's own confirmed switch: bit
  0x20 = "magically locked", bits 0x200-0x8000 = the 7-tier key
  hierarchy (BRASS/BRONZE/COPPER/IRON/STEEL/SILVER/GOLD, independently
  cross-confirmed against the Hex Hacking Item Guide's door-key item
  table). Bits 0x1/0x2 are also tested at a few UseAbilityCommand/
  RunShopScreen call sites whose exact relationship to lock state isn't
  confirmed -- named for its dominant, well-confirmed role.
- word_36C81 -> g_heldKeyFlags: the player's held-key flags, an OR'd
  accumulator built from carried items' flag bytes, compared against
  g_lockStatusFlags' key-tier bits by HandleGameCommand's unlock-door
  handler to resolve an unlock attempt.
- word_32DE8 -> g_facingTileCellPtr: computed by ProbeFacingTile from
  the party's current map-grid cell address, offset by one row
  (0x270 = 78*8, the confirmed dungeon-grid row stride) or one column
  (8, the confirmed per-cell size) depending on facing -- "the tile
  directly ahead of the party". Read (compared against 0 as a
  found-a-valid-tile sentinel, then loaded into si) by the unlock-door
  handler and other facing-tile interaction code.
- word_36E4D/36E4F/36E51 -> g_partyReserveSlot1/2/3: a confirmed
  3-slot "reserve roster" array, sibling to g_partySlotAssignment's 4
  active slots (same base/stride relationship, same
  cascade-down-to-fill-gaps cleanup logic on ShowWorldMap's exit path).

Run via:
    .\run_ida_script.ps1 rename_globals_round9.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = [
    (0x32DCE, "g_lockStatusFlags"),
    (0x36C81, "g_heldKeyFlags"),
    (0x32DE8, "g_facingTileCellPtr"),
    (0x36E4D, "g_partyReserveSlot1"),
    (0x36E4F, "g_partyReserveSlot2"),
    (0x36E51, "g_partyReserveSlot3"),
]

COMMENTS = {
    0x32DCE: (
        "Currently-examined lock/door status flags, loaded by "
        "LoadLockState. Confirmed via ShowLockStatus's own switch: bit "
        "0x20='magically locked', bits 0x200/0x400/0x800/0x1000/0x2000/"
        "0x4000/0x8000=the 7-tier key hierarchy (BRASS/BRONZE/COPPER/"
        "IRON/STEEL/SILVER/GOLD). Bits 0x1/0x2 are also tested at "
        "several UseAbilityCommand/RunShopScreen/RunMapEditorScreen call "
        "sites whose exact relationship to lock state isn't confirmed."
    ),
}

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

for ea, text in COMMENTS.items():
    ida_bytes.set_cmt(ea, text, False)
    print(f"{ea:#x}  comment set ({len(text)} chars)")
