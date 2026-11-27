"""
Symbol renames for MARTIAN.EXE of Martian Dreams (md_martian.idb).  Idempotent.

    .\run_ida_script.ps1 -Idb md\md_martian -ScriptName apply_renames_md_martian.py

The routines are the ones tools/emulate_intro.py, tools/gen_intro_script.py and tools/gen_md_create.py run or read; they read the listing through
tools/idanames.py, which puts the automatic names back, so renaming does not change what they produce.
"""

import idc
import ida_name

# (ea, new_name, note)
FUNCTIONS = [
    (0x19266, "intro_sequence", "the introduction: loads SCENE*.LZC pictures, draws, captions, waits (md_intro_script.inc)"),
    (0x15C84, "ending_sequence", "the ending (avatar type word_29B98 = 0x157 plays the Grand Finale)"),
    (0x1675E, "credits_sequence", "the credits (word_29662 = 1)"),
    (0x15177, "load_picture", "load_picture(file name offset, item) -> handle"),
    (0x15610, "wait_ticks", "waits for some ticks"),
    (0x152B5, "draw_caption", "a line of SCENETXT.LZC under the picture"),
    (0x1F2AE, "wait_vsync", "the wait for the vertical retrace (70 a second)"),
    (0x1CB71, "fade_in_picture", "puts the picture on the screen with black colours and fades them in"),
    (0x21EB8, "start_song", "starts a song of MDD_MUS.LZC"),
    (0x218FC, "stop_song", "stops or fades the song playing"),
    (0x20951, "play_effect", "a sound effect of the table at ds:04CB (8 bytes each)"),
    (0x20D32, "stop_effects", "stops the effects named"),
    (0x18557, "creation_questions", "the character creation: Dr. Freud's questions"),
    (0x17E7A, "creation_scene", "a scene of the creation (table at ds:01E3)"),
    (0x21038, "music_sequencer", "the AdLib sequencer (STRAX.DRV data)"),
    (0x15409, "load_songs", "reads the songs of MDD_MUS.LZC into the handles the intro starts"),
]

# (ea, new_name, note): globals
GLOBALS = [
    (0x29B98, "avatar_type", "the avatar's type word of the ending (0x157)"),
    (0x2B4CA, "music_flag", "1: the runs call the songs"),
    (0x29662, "credits_flag", "1: sub_1675E plays the credits"),
]

for ea, name, note in FUNCTIONS + GLOBALS:
    cur = idc.get_name(ea)
    if cur != name:
        if not ida_name.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN):
            print("FAILED %05X %s -> %s" % (ea, cur, name))
print("renamed %d functions and %d globals" % (len(FUNCTIONS), len(GLOBALS)))
