"""Create a test save by patching the location (and optionally the party's strength) of an existing fixture.

    python tools/make_fixture.py BASE_FIXTURE NEW_FIXTURE --map 0 --x 81 --y 106 [--level 0] [--strong]

SAVED.GAM is the DGROUP image starting at DS:55a6 (see src/vars.h): map id at +02ed (0 = overworld, 1..20h towns/castles,
21h.. dungeons), level +02ef, x +02f0, y +02f1.  On the overworld the party standing on a town/shrine/dungeon tile can use
the Enter command.  --strong sets str/dex/int 40 and 999 HP for the whole party.
"""
import argparse
import os
import shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FIX = os.path.join(ROOT, "tests", "fixtures")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("base")
    ap.add_argument("new")
    ap.add_argument("--map", type=int, default=0)
    ap.add_argument("--x", type=int, required=True)
    ap.add_argument("--y", type=int, required=True)
    ap.add_argument("--level", type=int, default=0)
    ap.add_argument("--strong", action="store_true")
    ap.add_argument("--fill", action="append", default=[], help="START-END=VALUE hex offsets (inclusive start, exclusive end)")
    ap.add_argument("--poke", action="append", default=[], help="OFFSET=VALUE (hex offset into SAVED.GAM, decimal/0x value)")
    a = ap.parse_args()
    dst = os.path.join(FIX, a.new)
    shutil.rmtree(dst, ignore_errors=True)
    shutil.copytree(os.path.join(FIX, a.base), dst)
    p = os.path.join(dst, "SAVED.GAM")
    d = bytearray(open(p, "rb").read())
    d[0x2ED], d[0x2EF], d[0x2F0], d[0x2F1] = a.map, a.level, a.x, a.y
    if a.strong:
        for i in range(3):
            o = 2 + i * 32
            d[o + 0xC] = d[o + 0xD] = d[o + 0xE] = d[o + 0xF] = 40
            d[o + 0x10:o + 0x12] = (999).to_bytes(2, "little")
            d[o + 0x12:o + 0x14] = (999).to_bytes(2, "little")
    for fl in a.fill:
        rng, val = fl.split("=")
        lo, hi = (int(x, 16) for x in rng.split("-"))
        for i in range(lo, hi):
            d[i] = int(val, 0)
    for pk in a.poke:
        off, val = pk.split("=")
        d[int(off, 16)] = int(val, 0)
    open(p, "wb").write(d)


if __name__ == "__main__":
    main()
