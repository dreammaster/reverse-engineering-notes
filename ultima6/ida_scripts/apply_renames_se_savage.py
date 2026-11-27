"""
Symbol renames for SAVAGE.EXE of Savage Empire (se_savage.idb).  Idempotent.

    .\run_ida_script.ps1 -Idb se\se_savage -ScriptName apply_renames_se_savage.py

The routines of the front end that src/ultima6/se_front_end.* and tools/gen_se_front.py port (menu, introduction, creation, opening, ending).
"""

import idc
import ida_name

# (ea, new_name, note)
FUNCTIONS = [
    # the program
    (0x13EB7, "main", "the program: reads the arguments, then the title and opening and the menu, or the ending (D, B, Y, L arguments)"),
    (0x13D25, "parse_arguments", "one command line argument: V E T video, A R N P sound, M K music, X no opening, D B Y the days / months / years played, L[A|T] the ending's variant"),
    (0x1372C, "read_config", "CONFIG.SE: video mode and sound card"),
    (0x13BC4, "fatal_error", "closes down with the error in word_1DFFC"),
    (0x13A00, "launch_game", "Journey Onward: closes the front end and runs GAME.EXE"),
    (0x139C8, "exit_program", "quits to DOS"),
    (0x13A52, "music_load", "loads the songs 0x12, 0 and 0x13 of MUSIC.LZC when there is sound"),
    # the loading of pictures
    (0x1389B, "load_lzc_item", "(name, item): reads an item of an *.LZC into memory; returns its handle"),
    (0x13874, "free_handle", "frees the memory of a loaded item and clears the variable that held it"),
    (0x1353D, "check_disk", "asks for the floppy disk with the files of this part (a hard disk install never asks)"),
    (0x13B79, "set_palette", "sets palette n of SAVAGE.PAL (0, 1 or 2) in the DAC"),
    (0x18E2A, "set_dac_palette", "sets the 256 colours of the DAC from a table of 8-bit values"),
    # the screen
    (0x13CB8, "show_screen", "the back screen to the front one: faded in on the VGA"),
    (0x13C84, "fade_out_clear", "fades the screen out, clears it and sets the palette again"),
    (0x18CA6, "fade_in", "(front, back): the back screen is put on the front one with the colours rising from black, one step a retrace"),
    (0x18964, "fade_out", "the colours fall to black, one step a retrace"),
    (0x18A9B, "used_colours", "which colours a screen uses"),
    (0x18AFA, "palette_step", "one step of a fade: each component moves by the accumulating rule of a line"),
    (0x17D78, "wait_retrace", "waits for the vertical retrace"),
    (0x13CE2, "text_reset", "clears the text window with the background colour and puts the cursor at its corner"),
    # the keys and the clock
    (0x11BFD, "key_available", "is a key waiting"),
    (0x118C3, "get_key", "reads a key"),
    (0x13AE7, "get_key_extended", "a key; an extended one is 0x100 more"),
    (0x13B0E, "flush_keys", "reads the keys that are waiting"),
    (0x13B22, "wait_ms", "waits about 1.5 times the milliseconds given (a key stops it and goes to the menu)"),
    (0x13BFF, "abort_to_menu", "a key during a scene: the picture fades and the menu begins again"),
    (0x1CFD6, "delay_counts", "waits so many counts of the timer chip (1193 a millisecond)"),
    (0x1D257, "random_range", "(lo, hi): a number from the multiplicative generator x = 65539 x"),
    (0x1D23A, "random_next", "the next number of the generator"),
    (0x1D27B, "random_seed", "starts the generator"),
    # the text window
    (0x17B32, "text_printf", "a printf into the text window: %s, %P prints the paragraph with the word wrapping"),
    (0x17B10, "text_print_direct", "a printf whose characters are put at once"),
    (0x178FD, "text_format", "the format interpreter of both"),
    (0x17575, "text_paragraph", "puts a paragraph line by line, breaking at the last space that fits"),
    (0x17856, "text_put_char", "a character; a line feed starts a new line"),
    (0x178B9, "text_newline", "the next line, or a scroll of the window"),
    (0x17B9A, "text_scroll", "scrolls the window up by a line"),
    (0x177EC, "text_set_cursor", "(x, y)"),
    (0x17804, "text_char_width", "the width of a character of the font"),
    (0x14FB6, "wait_key_prompt", "the blinking arrow after a page of text until a key is pressed"),
    (0x14E44, "get_key_cursor", "a key with the blinking underscore; mode 1 echoes it as the name is typed, 2 as an answer"),
    # the menu
    (0x14A37, "menu_loop", "the menu: arrows, Enter and the letters S T C A J pick a choice; argument 1 straight from the opening"),
    (0x14978, "menu_draw", "the menu's picture with the choices (tables at ds:05DC and ds:05E4)"),
    (0x14A04, "menu_choose", "the chosen picture flashes"),
    (0x14CE9, "acknowledgements", "the credits of CREATE.LZC item 6 scrolling up in a window"),
    (0x15D3B, "intro_pages", "the pages of the Introduction (ds:0623), one under each picture of CREATE.LZC item 5"),
    (0x15BE0, "check_savegame", "reads the saved game: is a character made, is a game in progress"),
    (0x15C3D, "game_in_progress", "bit 0 of objlist 0x1912"),
    (0x15CBC, "character_made", "objlist 0x1D72 is 1"),
    # the creation of a character
    (0x150AC, "creation_hut", "the hut of the character creation: Intanya's pages, the name, three questions"),
    (0x154BD, "creation_pages", "Intanya's three pages and the name"),
    (0x1539C, "creation_name", "types the name: up to 13 letters, digits, space, . - and the apostrophe"),
    (0x15359, "name_char_refused", "1 for a character a name may not hold"),
    (0x155A0, "creation_question", "asks a question of the table at ds:0564 (12 bytes a row) and returns the next question"),
    (0x157B5, "creation_raise_stat", "the stat an answer raises grows by 3 to 5; returns the card"),
    (0x15818, "creation_card", "the animation of the stat's card"),
    (0x15A7C, "creation_write_savegame", "writes the character into savegame\objlist"),
    # the opening and the title
    (0x141DE, "opening_sequence", "Lord British presents, the banner, the savannah with its panel and the scroll (INTRO.LZC)"),
    (0x14724, "opening_panel", "the panel of INTRO.LZC item 2 rises over the savannah"),
    (0x16FCD, "title_sequence", "the title sequence (Origin FX)"),
    (0x16FBA, "title_load", "loads an item of TITLE.LZC"),
    (0x16379, "title_backdrop", "the planet and the crowd"),
    (0x163B1, "title_scene_1", "the first scene of the title: the logo's pieces put in their places"),
    (0x1640C, "title_scene_2", "the logo's pieces move"),
    (0x165B5, "title_scene_3", "the logo's animation"),
    (0x1695A, "title_scene_4", "the logo grows (scaled drawing)"),
    (0x16AC4, "title_scene_5", "the logo and the sparks"),
    (0x16C80, "title_scene_6", "the last scene of the title"),
    (0x168C4, "title_sparks", "the sparks round the logo"),
    # the ending
    (0x15E82, "ending_sequence", "the ending: ENDGAME.LZC pictures and the 14 pages (pointer table at ds:0637)"),
    (0x15E52, "ending_count_text", "'one year' or '3 months'"),
]

GLOBALS = []

for ea, name, note in FUNCTIONS + GLOBALS:
    cur = idc.get_name(ea)
    if cur != name:
        if not ida_name.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN):
            print("FAILED %05X %s -> %s" % (ea, cur, name))
print("renamed %d functions and %d globals" % (len(FUNCTIONS), len(GLOBALS)))
