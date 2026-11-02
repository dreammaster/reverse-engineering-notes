"""
Names for the graphics and sound driver blobs of Ultima VI (u6_drv_* and u6_snd_*).

The drivers are entered through a table of 3-byte `jmp near` entries at offset 0; GAME.EXE
calls entry n by storing 3 * n in the offset of video_driver_call.  The graphics drivers
(MCGA, EGA, CGA, Tandy) have 27 entries with the same meaning; the sound drivers 4 (Roland: 5).
Run against any of the driver databases; it renames whichever entries it finds.

    .\\run_ida_script.ps1 -Idb u6_drv_mcga -ScriptName apply_renames_u6_drivers.py

Only entries whose behaviour was read from the MCGA driver are named; the others keep entry_NN.
"""

import idc
import ida_name

GRAPHICS = {
    0: ("video_init", "saves the BIOS mode, sets mode 13h, returns the driver's data block (cs:0052h)"),
    1: ("video_shutdown", "restores the saved video mode"),
    2: ("set_color", "colour used by the fill and text routines (stored as both bytes of a word)"),
    6: ("fill_rect", "fill_rect(x1, y1, x2, y2): the corners are sorted, filled with the current colour"),
    7: ("copy_rect", "copy_rect(...): screen to screen"),
    8: ("draw_bitmap", "draw_bitmap(y, x, far ptr): the bitmap starts with u16 width and u16 height, then rows of bytes"),
    10: ("draw_char", "draw_char(y, x, char): 8x8 glyph from the font table, foreground and background colours"),
    14: ("tile_cache_or_draw", "overloaded: with a first argument >= 3000h it sets the tile cache up (reads tile positions and kinds through host callbacks); used 37 times by GAME.EXE"),
    15: ("draw_tile", "draw_tile(y, x, tile): kind = kind_table[tile] & 3: 0 copy 16x16, 1 copy skipping colour 255, 2 pixel blocks"),
}
SOUND = {
    0: ("snd_init", "snd_init(port): programs the device"),
    1: ("snd_silence", "silences every voice"),
    2: ("snd_play", "snd_play(far ptr to music data): hooks the timer interrupt (INT 08h, PIT reprogrammed) and starts playing"),
    3: ("snd_stop", "restores the timer interrupt and silences"),
    4: ("snd_set_channel", "(Roland only) snd_set_channel(a, b, channel): per-channel instrument/channel set-up"),
}

nentries = 0
while idc.get_name_ea_simple("entry_%02d" % nentries) != idc.BADADDR or idc.get_name_ea_simple(
        GRAPHICS.get(nentries, SOUND.get(nentries, ("x",)))[0]) != idc.BADADDR:
    nentries += 1
    if nentries > 40:
        break

table = GRAPHICS if nentries > 8 else SOUND
done = 0
for n, (name, note) in table.items():
    ea = idc.get_name_ea_simple("entry_%02d" % n)
    if ea != idc.BADADDR:
        ida_name.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN)
        idc.set_cmt(ea, note, 0)
        done += 1
print("%d entries found, %d named" % (nentries, done))
