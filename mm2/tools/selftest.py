"""
Smoke test of every reader against the installed game: python selftest.py
Checks sizes/invariants that were verified while reverse engineering (see docs/).
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm2_gfx as g          # noqa: E402
import mm2_view as v         # noqa: E402
from mm2_data import Game, split_scripts   # noqa: E402

G = Game()
ok = 0


def check(cond, msg):
    global ok
    assert cond, msg
    ok += 1


check(len(G.monsters()) == 6656, "MONSTERS.DAT size")
check(len(G.attrib()) == 3840, "ATTRIB.DAT size")
for m in range(60):
    check(len(G.map(m)) == 512, "map %d size" % m)
for m in range(60):
    kind = "O" if 5 <= m <= 16 else "I"
    c = G.events(kind, m)
    if c:
        trig, body, msgs = G.parse_events(c)
        scripts = split_scripts(body)       # raises if the area does not end on a command boundary
        check(len(scripts) > 0, "events map %d" % m)
D = r"D:\GOG Games\Might and Magic 2" + "\\"
for f in sorted(os.listdir(D)):
    if f.endswith(".16") and not f.startswith("MONSTERS"):
        b = g.bank(open(D + f, "rb").read())
        for io, _ in g.entries(b):
            g.image(b, io, 4)
        check(True, f)
    if f.endswith(".4") and not f.startswith("MONSTERS"):
        b = g.bank(open(D + f, "rb").read())
        for io, _ in g.entries(b):
            g.image(b, io, 2)
        check(True, f)
v.render(v.GAME, 0, 8, 8, "N", "TOWN")
v.render_outdoors(5, 8, 8, "N")
print("selftest: %d checks passed" % ok)
