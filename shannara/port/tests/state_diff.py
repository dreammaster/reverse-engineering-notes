"""
State-driven differential test.  The single-command matrix (matrix_diff.py) reaches nearly every handler but only ~30% of their instructions because most
branches depend on game state.  Every scenario here starts from the start state, randomises the 800 game flags (random density), goes to a random room, puts a few
random items into the room and the party's hands, then runs a burst of random commands on objects that are in scope (room contents, inventory) or random.
Setup steps are replayed on the native engine but not compared; every command is: return value, state hash and text must equal the oracle's.

    python port/tests/state_diff.py [scenarios] [seed] [commands-per-scenario] [coverage-file]
"""
import os
import random
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "reversing", "scripts"))
TOOL = os.path.join(ROOT, "port", "build", "engine_tool.exe")

from retail_game import Game, load_tsv   # noqa: E402

M = (1 << 64) - 1
PREFIX = {1: 33, 2: 41, 3: 48, 4: 24, 5: 49}


def fnv(data, h=1469598103934665603):
    for b in data:
        h ^= b
        h = (h * 1099511628211) & M
    return h


def esc(s):
    return "".join("\\n" if ch == 10 else chr(ch) if 0x20 <= ch < 0x7F and ch != 0x5C else "\\x%02x" % ch for ch in s.encode("latin-1"))


def main():
    nsc = int(sys.argv[1]) if len(sys.argv) > 1 else 50
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    per = int(sys.argv[3]) if len(sys.argv) > 3 else 40
    cov = sys.argv[4] if len(sys.argv) > 4 else None
    rng = random.Random(seed)
    g = Game(os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game", "SHANSTR.DAT"))
    g.max_insns = 400_000      # commands that spin on stubbed waits are dropped quickly (the native side never sees them)
    n = g.r16(0x217A0)
    ptrs = [g.r32(0x20919 + 5 * i) for i in range(n)]
    types = [g.r8(0x20918 + 5 * i) for i in range(n)]
    verbs = [(int(r["verb"]), int(r["flags"] or 1)) for r in load_tsv("retail_verb_handlers.tsv") if r["name"]]
    rooms = [int(r["room(obj id)"]) for r in load_tsv("retail_rooms.tsv")]
    items = [k + 1 for k in range(n) if types[k] == 2]
    base = g.snapshot()
    ops, expect = [], []
    conds = set()       # overlay condition callbacks (called with the region's object field in eax)
    for r in load_tsv("retail_overlays.tsv"):
        fn = int(r["conditionFn"], 16)
        if fn:
            conds.add((fn, int(r["obj"]) & 0xFFFFFFFF))
    conds = sorted(conds)

    def state_hash():
        h = 1469598103934665603
        for k in range(n):
            h = fnv(g.rb(ptrs[k], PREFIX[types[k]]), h)
        return fnv(g.rb(0xCD8AC, 100), h)

    def setup(line):
        ops.append(line)
        expect.append(None)

    def checked(line, r):
        ops.append(line)
        expect.append("%d %d %016x [%s]" % (len(ops) - 1, r & 0xFFFFFFFF, state_hash(), esc(g.text())))

    import time
    t0 = time.time()
    for s in range(nsc):
        print('scenario', s, 'ops', len(ops), 't=%.0f' % (time.time() - t0), file=sys.stderr, flush=True)
        g.restore(base)
        setup("reset 0 0 0")
        try:
            density = rng.choice([0.05, 0.2, 0.5, 0.8, 0.95])
            for fl in range(1, 801):
                if rng.random() < density:
                    a = 0xCD8AC + ((fl - 1) >> 3)
                    g.w8(a, g.r8(a) | (1 << ((fl - 1) & 7)))
                    setup("flag %d 1 0" % fl)
            rm = rng.choice(rooms)
            r0 = g.call(0x1EE10, eax=rm)
            checked("goto %d 0 0" % rm, r0)
            here = g.r16(0xADE94 + 2)
            for o in rng.sample(items, 3):
                par = here if rng.random() < 0.5 else 3
                g.call(0x1B470, eax=o, edx=par, ebx=0)
                checked("move %d %d 0" % (o, par), 0)
            scope = [k for k in range(1, n + 1) if k != here and (g.call(0x1B5C8, eax=here, edx=k) & 0xFF or g.call(0x1B5C8, eax=3, edx=k) & 0xFF)]
        except Exception:
            ops[:] = ops[:max(i for i, o in enumerate(ops) if o.startswith("reset"))]
            expect[:] = expect[:len(ops)]
            continue
        if not scope:
            scope = [rng.randrange(1, n + 1)]
        for fn, arg in conds:
            snap = g.snapshot()
            try:
                r = g.call(fn, eax=arg, edx=0)
            except Exception:
                g.restore(snap)
                continue
            checked("call %d %d 0" % (fn, arg), r)
        for _ in range(per):
            vid, nobj = rng.choice(verbs)
            pick = lambda: rng.choice(scope) if rng.random() < 0.8 else rng.randrange(1, n + 1)
            d = pick() if nobj >= 1 else 0
            i = pick() if nobj >= 2 else 0
            snap = g.snapshot()
            try:
                r = g.command(vid, d, i)
            except Exception:
                g.restore(snap)
                continue
            checked("cmd %d %d %d" % (vid, d, i), r)
    path = os.path.join(ROOT, "port", "build", "state_ops.txt")
    open(path, "w").write("\n".join(ops) + "\n")
    env = dict(os.environ)
    if cov:
        env["SHAN_COVERAGE"] = cov
    p = subprocess.run([TOOL, os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game"), path], capture_output=True, text=True, env=env)
    out = p.stdout.splitlines()
    pairs = [(a, e, ops[k]) for k, (a, e) in enumerate(zip(out, expect)) if e is not None]
    bad = [x for x in pairs if x[0] != x[1]]
    print("commands:", len(pairs), "lines:", len(out), "of", len(ops), "mismatches:", len(bad) + abs(len(out) - len(ops)))
    for a, b, o in bad[:8]:
        print("  op      :", o, "\n  C++     :", a[:200], "\n  original:", b[:200])
    sys.exit(1 if bad or len(out) != len(ops) else 0)


main()
