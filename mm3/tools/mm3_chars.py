#!/usr/bin/env python3
"""List the characters of a MAZE.CHR roster (30 x 303 bytes, layout in docs/character.h).  usage: mm3_chars.py MAZE.CHR"""
import struct
import sys

CLASSES = ["Knight", "Paladin", "Archer", "Cleric", "Sorcerer", "Robber", "Ninja", "Barbarian", "Druid", "Ranger"]
RACES = ["Human", "Elf", "Gnome", "Dwarf", "Half-orc"]
SEX = ["Male", "Female"]
ALIGN = ["Good", "Neutral", "Evil"]
d = open(sys.argv[1], "rb").read()
for i in range(len(d) // 303):
    r = d[i * 303:(i + 1) * 303]
    name = r[:16].split(b"\0")[0].decode("latin1")
    if not name:
        continue
    stats = [(r[0x14 + 2 * k], r[0x15 + 2 * k]) for k in range(7)]
    hp, sp = struct.unpack_from("<hh", r, 0x125)
    exp = struct.unpack_from("<I", r, 0x12B)[0]
    print("%2d %-10s %-8s %-8s %-6s %-7s lvl %2d hp %3d sp %3d exp %6d  M%d I%d P%d E%d S%d A%d L%d  skills %s" % (
        i, name, CLASSES[r[0x13]] if r[0x13] < 10 else r[0x13], RACES[r[0x11]] if r[0x11] < 5 else r[0x11], SEX[r[0x10]] if r[0x10] < 2 else r[0x10],
        ALIGN[r[0x12]] if r[0x12] < 3 else r[0x12], r[0x23], hp, sp, exp, *[s[0] for s in stats],
        [k for k in range(18) if r[0x27 + k]]))
