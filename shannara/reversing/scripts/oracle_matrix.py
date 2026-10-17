"""
Runs every verb against a set of objects in the START state of the real game logic (retail_game.py) and records, for each
command, the return value, the text printed, and the state it changed (object fields / game flags).  The result is a behavioural
specification of the verb layer taken from the original code - the thing a re-implementation has to reproduce.

    python oracle_matrix.py OUT.tsv [objects...]        (default: the start room's objects, the party and a few others)

The output contains the game's own text, so keep it local (game_out/), not in git.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from retail_game import Game, ROOT, load_tsv   # noqa: E402

PREFIX = {1: 33, 2: 41, 3: 48, 4: 24, 5: 49}
FLAGS = 0xCD8AC


def state(g, ptrs, types):
    """object prefixes + flag bytes as one comparable structure"""
    recs = [g.rb(p, PREFIX[t]) for p, t in zip(ptrs, types)]
    return recs, g.rb(FLAGS, 100)


def diff(a, b):
    out = []
    for i, (x, y) in enumerate(zip(a[0], b[0])):
        if x != y:
            ch = [j for j in range(len(x)) if x[j] != y[j]]
            out.append("obj%d:%s" % (i + 1, ",".join("+%x" % j for j in ch[:6])))
    fl = [8 * k + bit + 1 for k in range(100) for bit in range(8) if (a[1][k] ^ b[1][k]) >> bit & 1]
    if fl:
        out.append("flags:" + ",".join(str(f) for f in fl))
    return ";".join(out)


def main():
    out_path = sys.argv[1]
    g = Game(os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game", "SHANSTR.DAT"))
    n = g.r16(0x217A0)
    ptrs = [g.r32(0x20919 + 5 * i) for i in range(n)]
    types = [g.r8(0x20918 + 5 * i) for i in range(n)]
    objs = [int(a) for a in sys.argv[2:]] or [0, 3, 35, 48, 77, 4, 5, 2, 88]
    verbs = [(int(r["verb"]), r["name"], r["preposition"], r["flags"]) for r in load_tsv("retail_verb_handlers.tsv") if r["name"]]
    base = g.snapshot()
    rows = 0
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("verb\tname\tdobj\tiobj\tresult\tchanges\ttext\n")
        for vid, name, prep, flags in verbs:
            nobj = int(flags or 1)
            for d in (objs if nobj >= 1 else [0]):
                for i in (objs if nobj >= 2 else [0]):
                    if nobj >= 2 and i == 0:
                        continue
                    g.restore(base)
                    before = state(g, ptrs, types)
                    try:
                        r = g.command(vid, d, i)
                        err = ""
                    except Exception as e:
                        r, err = -1, "ERR " + str(e)[:40]
                    after = state(g, ptrs, types)
                    text = (g.text() + err).replace("\n", "\\n").replace("\t", " ")
                    f.write("%d\t%s%s\t%d\t%d\t%d\t%s\t%s\n" % (vid, name, " " + prep if prep else "", d, i, r & 0xFFFFFFFF, diff(before, after), text))
                    rows += 1
    print("wrote", rows, "rows to", out_path)


if __name__ == "__main__":
    main()
