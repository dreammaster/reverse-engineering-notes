"""
Master list of symbol renames for Ultima VI's U.EXE (u6_u.idb): title/main menu,
intro and character creation.  Idempotent; see docs/u6_game.md for the evidence.

    .\\run_ida_script.ps1 -Idb u6_u -ScriptName apply_renames_u6_u.py
"""

import idc
import ida_name

# (ea, new_name, note) -- the DOS file layer is the same Origin run-time code as in GAME.EXE
# (matched by instruction signature against u6_game's file_* functions).
FUNCTIONS = [
    (0x109F3, "file_open", "= GAME.EXE file_open"),
    (0x10A32, "file_close", "= GAME.EXE file_close"),
    (0x10AE4, "file_read", "= GAME.EXE file_read(dest far, count dword, offset dword, handle)"),
    (0x10BCA, "file_write", "= GAME.EXE file_write"),
    # -- main menu and the options behind it (the jump table in _main: 0 Introduction,
    #    1 Create character, 2 Transfer character, 3 Acknowledgements, 4 Journey onward) --
    (0x113A5, "main_menu", "returns the chosen entry 0-4; uses MAINMENU.* and TITLES.*"),
    (0x17A36, "title_sequence", "INTRO1-3 screens shown before the menu"),
    (0x1C678, "intro_story", "the introduction: INTRO.* and STARPOS.DAT; _main then sets word_24B24"),
    (0x19A0B, "show_text_page", "show_text_page(id): the story pages and, for ids 105-132, the gypsy's questions"),
    (0x16766, "acknowledgements", "credits screens ('Ultima VI / A Lord British Production')"),
    (0x16DF9, "start_game", "execl GAME.EXE"),
    (0x13A06, "transfer_character_menu", "the Transfer character option"),
    (0x13470, "transfer_from_disk", "'TRANSFER A CHARACTER': reads party.sav / saved.gam of Ultima V or IV"),
    (0x1BFE2, "load_intro_palette", "load_intro_palette(n): PALETTES.INT entry n (576 bytes at n * 576), unpacked 3 bytes -> 4 six-bit components, set through the driver"),
    # -- character creation --
    (0x152C9, "create_character", "name, sex, portrait, then the gypsy's seven questions; returns 0 when a character was created"),
    (0x1418A, "enter_name_and_sex", "'By what name shalt thou be called?', 'And art thou Male, or Female?'"),
    (0x14FF9, "gypsy_choose_answer", "gypsy_choose_answer(question): waits for A or B and returns the virtue (0-7) it picks"),
    (0x150A0, "gypsy_apply_answer", "gypsy_apply_answer(virtue): adds the vial's strength/dexterity/intelligence; Humility (7) adds 3 karma; plays the pouring"),
    (0x14DD3, "shuffle_words", "shuffle_words(table, n): n*n+n-ish random swaps (count = rand % (n*n) + n)"),
    (0x14066, "write_new_game_objlist", "patches the Avatar into the savegame\objlist template (0x1C73 bytes) and writes it"),
]

# (ea, new_name, note) -- DS_BASE is 0x24990
DS_BASE = 0x24990
DATA = [
    (DS_BASE + 0x06B7, "gypsy_questions", "i16[8][8]: question number for a pair of virtues (-1 on the diagonal)"),
    (DS_BASE + 0x0737, "gypsy_answer_a", "i16[28]: virtue picked by answer A of question 105+n"),
    (DS_BASE + 0x076F, "gypsy_answer_b", "i16[28]: virtue picked by answer B"),
    (DS_BASE + 0x07A7, "gypsy_bonus_str", "i16[8], per virtue"),
    (DS_BASE + 0x07B7, "gypsy_bonus_dex", "i16[8]"),
    (DS_BASE + 0x07C7, "gypsy_bonus_int", "i16[8]"),
    (DS_BASE + 0x5A30, "new_char_name", "14 bytes"),
    (0x2A3D0, "new_char_str", "starts at 15"),
    (0x2A3D2, "new_char_dex", "starts at 15"),
    (0x2A3D4, "new_char_int", "starts at 15"),
    (0x2A3D6, "new_char_sex", "'M' or 'F'"),
    (0x2A3FA, "new_char_portrait", "0-5"),
    (0x25022, "new_char_karma", "starts at 75 (4Bh)"),
    (0x2A3F0, "new_char_exp", "new Avatar: 172h"),
    (0x2A3F2, "new_char_level", "new Avatar: 3"),
    (0x2A3F4, "new_char_hp", "new Avatar: 5Ah"),
    (DS_BASE + 0x5B2A, "gypsy_table", "u16[8]: the shuffled virtues / round winners"),
    (0x24B24, "intro_seen", "set after the introduction has been shown"),
]

for ea, name, note in FUNCTIONS + DATA:
    if idc.get_name(ea) != name:
        if not ida_name.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN):
            print("FAILED %05X %s" % (ea, name))
print("renamed %d functions and %d data items" % (len(FUNCTIONS), len(DATA)))
