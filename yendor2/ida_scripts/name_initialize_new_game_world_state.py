"""
Names sub_2B436, called once from RunTitleScreen -- the "New Game"
initializer.

Clears the party roster (g_partySlotAssignment/word_36E4D/
word_36E4F/word_36E51 all 0) and a per-record flag bit ([+0x15C] bit
0x800) across all 9 party record slots (stride 0x1F4, matching the
confirmed g_partyRecords layout). Resets a large block of world-state
globals to their starting values: word_36CE7=5 (animation setting),
the party's starting world position (word_36CF7=0xA6, word_36CF9=0x24)
and facing (word_36CF5=0x2000), the in-game clock/calendar fields
(word_36CFB/CFD/CFF/D01), and several other tracked globals. Writes
zeroed/reset records back to WORLD.DAT across several loops (clearing
what are plausibly ground-item slots and other per-level persisted
state) via repeated FileEntry_Write calls. Reads as: reset the save
game's entire persistent world state back to a fresh starting
condition before a new game begins. -> InitializeNewGameWorldState

Run via:
    .\run_ida_script.ps1 name_initialize_new_game_world_state.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2B436
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "InitializeNewGameWorldState", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'InitializeNewGameWorldState': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "'New Game' initializer: clears the party roster and a per-"
    "record flag across all 9 party slots, resets world-state "
    "globals (starting position word_36CF7/word_36CF9=0xA6/0x24, "
    "facing, clock/calendar fields, etc.), and writes zeroed/reset "
    "records back to WORLD.DAT across several loops. Called once "
    "from RunTitleScreen.",
    False,
)
