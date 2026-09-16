"""
Names 4 more small helpers.

sub_1251D (called once from InitGame): zeroes the icon-bar area
(0xC50, matching the confirmed "0xC50 + slot*0x14" icon-bar layout),
then for 4 party members computes a small hit-test region (8x8,
stride 0x14) from table 0x61C2's per-member coordinates. Boot-time
setup of the party status icon bar's clickable regions.
-> InitializeStatusIconBarHitTestRegions

sub_22CBC (called from RunDungeonGameLoop and
HandleRangedOrCombatAction): zeroes 8 scratch words
(word_32A16-1C, word_32BF6-FC) -- resets per-turn combat scratch
counters/accumulators at the start of processing.
-> ResetCombatRoundScratchState

sub_23305 (called once from ProcessLevelMonsters): clears the old
map cell's flag bit 0x400 and reference field, advances a position
record by (word_2E402, word_2E406) and a cell-pointer by
word_2E404, then sets the new cell's flag bit 0x400 and reference --
moves a monster's map-cell position marker to an adjacent cell.
-> RelocateMonsterCellMarker

sub_2D547 (called twice from the still-untraced combat dispatcher
sub_2C0FE): if a mode flag (word_332E2 bit 0x40) and the current
party member's affliction bit 0x40 ([+0x1C]) are both set, zeroes 3
fields at [di+0xE]/[0x10]/[0x12]; otherwise copies word_332DC/
word_332DE into [di+0xE]/[0x10]. Exact narrative role within
sub_2C0FE not established. -> ResetOrCopyTargetPositionFields

Run via:
    .\run_ida_script.ps1 name_more_small_helpers.py
"""
import idc
import ida_name
import ida_bytes

entries = [
    (0x1251D, "InitializeStatusIconBarHitTestRegions",
     "Zeroes the icon-bar area (0xC50) and computes each of the 4 "
     "party members' small status-icon hit-test regions from table "
     "0x61C2. Called once from InitGame."),
    (0x22CBC, "ResetCombatRoundScratchState",
     "Zeroes 8 combat scratch words (word_32A16-1C, word_32BF6-FC). "
     "Called from RunDungeonGameLoop and "
     "HandleRangedOrCombatAction."),
    (0x23305, "RelocateMonsterCellMarker",
     "Clears the old map cell's flag bit 0x400/reference, advances "
     "the position by (word_2E402, word_2E406), sets the new cell's "
     "flag bit 0x400/reference. Called once from ProcessLevelMonsters."),
    (0x2D547, "ResetOrCopyTargetPositionFields",
     "If word_332E2 bit 0x40 and the current party member's "
     "affliction bit 0x40 are both set, zeroes [di+0xE]/[0x10]/"
     "[0x12]; else copies word_332DC/word_332DE into [di+0xE]/[0x10]. "
     "Called from the combat dispatcher sub_2C0FE.",
     ),
]

for ea, name, desc in entries:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
