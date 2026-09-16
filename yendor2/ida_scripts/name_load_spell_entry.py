"""
Names sub_1D198, called from BuildClueEntryText and
RunClueBookSpellCategory (the F8 clue book's F3 SPELLS category):
maps in a dedicated EMS page (0x5610, distinct from the clue book's
full-screen page 0x5616) and copies one 80-byte spell record (index
word_3330A, 1-based) into scratch buffer 0x5A5A. The spell-data
equivalent of LoadClueBookMonsterEntry's WORLD.DAT read, but sourced
from its own EMS-cached table instead. -> LoadClueBookSpellEntry

Run via:
    .\run_ida_script.ps1 name_load_spell_entry.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1D198
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "LoadClueBookSpellEntry", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'LoadClueBookSpellEntry': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Maps in EMS page 0x5610 and copies one 80-byte spell record "
    "(1-based index in ax) into scratch buffer 0x5A5A. Called from "
    "BuildClueEntryText and RunClueBookSpellCategory (F8 clue book, "
    "F3 SPELLS).",
    False,
)
