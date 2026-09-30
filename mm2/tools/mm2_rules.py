"""Game-rule tables/formulas decoded from the code (see docs/classes.md).  Tables are read from the
EXE's DGROUP so nothing is hard-coded except the piecewise experience additions."""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_layout import Layout  # noqa: E402

CLASSES = ["Knight", "Paladin", "Archer", "Cleric", "Sorcerer", "Robber", "Ninja", "Barbarian"]
RACES = ["Human", "Elf", "Dwarf", "Gnome", "Half-Orc"]
STATS = ["Might", "Intellect", "Personality", "Endurance", "Speed", "Accuracy", "Luck"]

_lay = Layout()
_d, _ = _lay.read_segment(_lay.dgroup)


def _dw(off, i):
    return struct.unpack_from("<I", _d, off + 4 * i)[0]


def exp_for_level(cls, level):
    """Total experience needed to *reach* `level` (>= 2) -- 2CMDS training hall `1CC8C`."""
    group = 1 if cls in (1, 2, 4, 6) else 0
    e = _dw(0x2E5C + group * 0x24, min(level, 10))
    if level >= 11: e += 192000
    if level >= 12: e += 192000
    if level >= 13: e += 192000
    if level >= 14: e += 384000
    if level >= 15: e += 384000
    if level >= 16: e += min(level - 15, 5) * 768000
    if level >= 21: e += min(level - 20, 10) * 1536000
    if level >= 31: e += min(level - 30, 20) * 3072000
    if level >= 51: e += min(level - 50, 25) * 0x190000
    if level >= 76: e += (level - 75) * 6144000
    return e


def training_cost(town, level):
    """Gold to train to `level` in town 0-4: 50 x level x town multiplier."""
    mult = struct.unpack_from("<5H", _d, 0x2F04)[town]
    return mult * level * 50


if __name__ == "__main__":
    for c in (0, 1):
        print(CLASSES[c], [exp_for_level(c, l) for l in range(2, 18)])
