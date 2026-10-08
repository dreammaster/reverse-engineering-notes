#!/usr/bin/env python3
"""Compare the draw lists of the translated C view pipeline (src/tests/view_run) with the original code in the emulator.

usage: mm3_viewcheck.py MM3.EXE MM3.CUR VIEW_RUN [N] [SEED] [MM3VIEW DATADIR]
With the last two arguments the data segment is built by the C glue (mm3view --dump: maze page, monsters, objects, sprite handles)
instead of the Python setup, so monsters and objects are part of the comparison.
Random positions on the indoor maps (1-40), all four facings; both run prepareIndoorView + renderIndoorView from the same
DGROUP snapshot and the lists (sprite-set pointers and records) must be identical."""
import os
import random
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm3_vieworacle as vo  # noqa: E402
import mm3_cc  # noqa: E402


def parse_c(out):
    lists, cur = [], None
    for line in out.split("\n"):
        p = line.split()
        if not p:
            continue
        if p[0] == "L":
            cur = []
            lists.append(cur)
        elif p[0] == "S":
            cur.append(("set", int(p[2]), int(p[1])))
        elif p[0] == "R":
            cur.append(("rec",) + tuple(int(v) for v in p[1:]))
    return lists


def raw_lists(lists):
    """the oracle's lists use marker names; convert back to (set, seg, off) form for an exact comparison"""
    out = []
    for l in lists:
        out.append([("set", vo.name_to_seg(r[1]), 0) if r[0] == "set" else r for r in l])
    return out


def main():
    exe, cur, runner = sys.argv[1:4]
    n = int(sys.argv[4]) if len(sys.argv) > 4 else 100
    rnd = random.Random(int(sys.argv[5]) if len(sys.argv) > 5 else 7)
    glue = (sys.argv[6], sys.argv[7]) if len(sys.argv) > 7 else None
    o = vo.ViewOracle(exe, cur)
    e = o.emu
    maps = [m for m in range(1, 41) if mm3_cc.name_id("MAZE%02d.DAT" % m) in o.members]
    tmp = tempfile.mkdtemp()
    bad = 0
    nonempty = 0
    for idx in range(n):
        map_id, x, y, f = rnd.choice(maps), rnd.randrange(16), rnd.randrange(16), rnd.randrange(4)
        if glue:
            subprocess.run([glue[0], glue[1], str(map_id), str(x), str(y), str(f), "--dump", tmp + "/in.dg"], check=True)
            e.reset_dgroup()
            e.wbytes(0, open(tmp + "/in.dg", "rb").read())
        else:
            o.setup(map_id, x, y, f)
        # randomise the animation counters like a running game would have them
        e.wb(0x2884D - 0x286F0, rnd.randrange(3))
        e.wb(0x28875 - 0x286F0, rnd.randrange(2))
        snap = e.rbytes(0, 0x10000)
        open(tmp + "/in.dg", "wb").write(snap)
        want = o.render()
        r = subprocess.run([runner, tmp + "/in.dg"], capture_output=True, text=True)
        got = parse_c(r.stdout)
        want_n = [[("set", vo.segof(x[1]), 0) if x[0] == "set" else x for x in l] for l in want]
        got_n = [[("set", x[1], 0) if x[0] == "set" else x for x in l] for l in got]
        if sum(len(l) for l in want) > 8:
            nonempty += 1
        if want_n != got_n:
            bad += 1
            print("MISMATCH map %d (%d,%d) facing %d" % (map_id, x, y, f))
            for a, b in zip(want_n, got_n):
                for i, (ra, rb) in enumerate(zip(a, b)):
                    if ra != rb:
                        print("  first difference at entry %d: original %s, C %s" % (i, ra, rb)); break
            if r.returncode or r.stderr:
                print("  runner:", r.returncode, r.stderr[:200])
            if bad > 5:
                break
    print("%d positions, %d with scene content, %d mismatches" % (n, nonempty, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
