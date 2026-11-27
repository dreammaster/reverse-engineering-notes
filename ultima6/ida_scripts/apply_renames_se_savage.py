"""
Symbol renames for SAVAGE.EXE of Savage Empire (se_savage.idb).  Idempotent.

    .\run_ida_script.ps1 -Idb se\se_savage -ScriptName apply_renames_se_savage.py

The routines of the front end that src/ultima6/se_front_end.* and tools/gen_se_front.py port (menu, introduction, creation, opening, ending).
"""

import idc
import ida_name

# (ea, new_name, note)
FUNCTIONS = [
    (0x15E82, "ending_sequence", "the ending: ENDGAME.LZC pictures and the 14 pages (pointer table at ds:0637)"),
    (0x15D3B, "intro_pages", "the pages of the Introduction (ds:0623), one under each picture of CREATE.LZC item 5"),
    (0x150AC, "creation_hut", "the hut of the character creation"),
    (0x154BD, "creation_questions", "the questions of the creation (table at ds:0564, 12 bytes a row)"),
    (0x14978, "menu", "the menu (tables at ds:05DC and ds:05E4)"),
    (0x16FCD, "title_sequence", "the title sequence (Origin FX)"),
    (0x16FBA, "title_load", "loads TITLE.LZC"),
    (0x141DE, "intro_load", "loads INTRO.LZC"),
    (0x13EB7, "creation_start", "loads CREATE.LZC and starts the creation"),
]

GLOBALS = []

for ea, name, note in FUNCTIONS + GLOBALS:
    cur = idc.get_name(ea)
    if cur != name:
        if not ida_name.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN):
            print("FAILED %05X %s -> %s" % (ea, cur, name))
print("renamed %d functions and %d globals" % (len(FUNCTIONS), len(GLOBALS)))
