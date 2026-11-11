#!/usr/bin/env python3
"""
The character status dialog (CAD.EXE 1000:2FD4 for class-7 objects): HEALTH / GOLD / XP / LEVEL / COMPLETE / rank.  docs/formats.md 6.7.

Box at (68, 10) with 16 fill columns and 4 middle rows (message.draw_box), labels = LNG string 13 (lines separated by CR, pitch 6) in the small message font (block 3: the text routine is entered at 1000:8776 without resetting the font set by the previous 8714 call)
at (78, 23), values in a column at x = 134 (pitch 6).  HEALTH = energy/max, GOLD = money, XP = score, LEVEL = level number, COMPLETE = visited rooms in percent,
last line = rank title: the first entry of the 20-entry table at the end of LEVn.MP1 (string id, score_hi, score_lo; big-endian words) whose threshold is >= the score.
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import message, status

BOX = (64, 10, 16, 4)        # x = 0x44 rounded down to a multiple of 8 (the tile blitter works on byte columns)


def rank_table(mp1):
    t = struct.unpack('>60H', mp1[-120:])
    return [(t[i], (t[i + 1] << 16) | t[i + 2]) for i in range(0, 60, 3)]


def rank_string(table, score):
    for sid, thr in table:
        if score <= thr:
            return sid
    return table[-1][0]


def complete_percent(rooms_visited, six_fa, tad_w5):
    """1000:3061: (0x6400 // (ds:6FA - TAD table word 5) * visited) >> 8"""
    d = six_fa - tad_w5
    return ((0x6400 // d) * rooms_visited) >> 8 if d > 0 else 0


def draw_dialog(canvas, tiles, font, names, energy, energy_max, money, score, level, percent, rank_id, label_pitch=6):
    x, y, fill, rows = BOX
    message.draw_box(canvas, x, y, fill, rows, tiles, rnd=lambda: 3)
    ty = y + 13
    for ln in names[13].split('\r'):
        message.draw_text(canvas, 78, ty, ln, font)
        ty += label_pitch
    vals = ['%d/%d' % (energy, energy_max), str(money), str(score), str(level), '%d%%' % percent, names[rank_id]]
    ty = y + 13
    for v in vals:
        message.draw_text(canvas, 134, ty, v, font)
        ty += label_pitch
