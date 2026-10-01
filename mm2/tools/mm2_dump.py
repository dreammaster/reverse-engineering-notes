"""
Text dumps of ITEMS.DAT and MONSTERS.DAT using the field layouts in docs/file-formats.md.
    python mm2_dump.py items > ../data/items.txt
    python mm2_dump.py monsters > ../data/monsters.txt
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_data import Game   # noqa: E402

CLASSES = ["Knight", "Paladin", "Archer", "Cleric", "Sorcerer", "Robber", "Ninja", "Barbarian"]
ATTR = ["Might", "Intellect", "Personality", "Speed", "Accuracy", "Luck"]
DIE = [1, 10, 100, 1000]
PCT = [0, 10, 20, 35, 50, 75, 90, 100]
TOUCH = ["adds friends", "lost gold", "lost gems", "poison", "disease", "sleep", "curse", "silence", "paralysis", "collapse", "die",
         "stone", "eradicate", "lost item", "lost backpack", "lost food", "lost all food", "lost all gold", "lost all gems",
         "lost valuables", "age", "age x2", "lose stat", "lose stat x2", "lose stat x3", "lose level", "lose level x2",
         "lose experience", "scramble items", "lose spell points", "assassinate", "poison spray"]


def items(g):
    d = g.read("ITEMS.DAT")
    print("id name class-restriction bonus use price")
    for i in range(256):
        r = d[i * 20:(i + 1) * 20]
        name = r[:12].decode("latin1").strip()
        mask = r[0x0D]
        cant = ",".join(c for k, c in enumerate(CLASSES) if mask & (0x80 >> k)) or "-"
        m = r[0x0E]
        bonus = "not equippable" if m == 0xF0 else ("%s +%d" % (ATTR[m >> 4], m & 15) if m & 15 and (m >> 4) < 6 else "-")
        print("%3d %-12s cannot:%s bonus:%s use:%d value:%d price:%d" % (i, name, cant, bonus, r[0x0F], r[0x10] | (r[0x11] << 8),
                                                                     struct.unpack_from("<H", r, 0x12)[0]))


def monsters(g):
    d = g.monsters()
    print("id name hp exp ac speed blows dmgdie group picture magic-res% touch spell")
    for i in range(256):
        r = d[i * 26:(i + 1) * 26]
        name = bytes(b & 0x7F for b in r[:14]).decode("latin1").strip()
        hp = ((r[0x0E] & 0x3F) + 1) * DIE[r[0x0E] >> 6]
        e = r[0x0F]
        exp = ((e & 0x1F) + 1) * (1000 if e & 0x80 else DIE[(e >> 5) & 3])
        def sized(b):
            v = (b & 0x1F) + 1
            return min(250, v * 10) if b & 0x20 else v
        ac, dmg, spd = sized(r[0x10 + 6]), sized(r[0x17]), sized(r[0x18])
        blows = (r[0x14] & 15) + 1
        g_ = (r[0x13] & 15) + 1
        g_ = g_ * 10 if r[0x13] & 0x10 else g_
        pic = r[0x15] & 0x7F
        mr = PCT[r[0x19] >> 5]
        t = r[0x12] & 0x1F
        sp = r[0x11] & 0x1F
        print("%3d %-14s hp%-5d exp%-7d ac%-3d spd%-3d x%d d%-3d grp%-3d pic%-3d mr%-3d touch:%s spell:%d" %
              (i, name, hp, exp, ac, spd, blows, dmg, g_, pic, mr, TOUCH[t] if t else "-", sp))


if __name__ == "__main__":
    g = Game()
    {"items": items, "monsters": monsters}[sys.argv[1]](g)
