"""
Names sub_1F5FF, called once from RunGameDialog (the pause/system
menu's SAVE option handler) -- the actual save-game file-copy
operation.

Creates (first use, tracked by a bit in word_32906+1) or opens (later
uses) a second FileEntry ("this" = 0x902C, distinct from the live
game's usual 0x8FFB), then writes matching master-header records
(record type 0xA via sub_27DA8) to both files, and copies the
CURGAME-style data record-by-record from the live file (0x8FFB) into
the save-slot file (0x902C) across several loops of differently-typed
records (types 0xA/0xB, incrementing word_36863/word_36894 cursors as
it goes). Implements the `CURGAME` -> `SAVGAMEn` copy documented in
file-formats.md's `CURGAME`/`SAVGAME1` section. -> SaveCurrentGameToSlot

Run via:
    .\run_ida_script.ps1 name_save_current_game_to_slot.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1F5FF
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SaveCurrentGameToSlot", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SaveCurrentGameToSlot': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Save-game file-copy: creates/opens the save-slot file (0x902C), "
    "writes matching header records to both it and the live game "
    "file (0x8FFB), then copies the game data record-by-record from "
    "live into the save slot across several typed-record loops. "
    "Called once from RunGameDialog's SAVE option.",
    False,
)
