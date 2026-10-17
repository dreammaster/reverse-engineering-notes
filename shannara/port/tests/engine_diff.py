"""
Differential test of the TRANSPILED C++ engine (port/build/engine_tool.exe, built by port/build_engine.sh) against the unicorn oracle
(reversing/scripts/retail_game.py), which runs the original machine code.

Random command sequences (verb x object x object, state carries over) are executed by the oracle; commands that make the emulation fault
(UI paths that are not stubbed) are rolled back and skipped.  The C++ engine replays the same sequence; per command the return value, a
hash of the saved state (object records + flags) and the printed text must agree.

    python port/tests/engine_diff.py [commands] [seed]
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
    out = []
    for ch in s.encode("latin-1"):
        out.append("\\n" if ch == 10 else chr(ch) if 0x20 <= ch < 0x7F and ch != 0x5C else "\\x%02x" % ch)
    return "".join(out)


def main():
    ncmd = int(sys.argv[1]) if len(sys.argv) > 1 else 200
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    rng = random.Random(seed)
    g = Game(os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game", "SHANSTR.DAT"))
    n = g.r16(0x217A0)
    ptrs = [g.r32(0x20919 + 5 * i) for i in range(n)]
    types = [g.r8(0x20918 + 5 * i) for i in range(n)]
    verbs = [(int(r["verb"]), int(r["flags"] or 1)) for r in load_tsv("retail_verb_handlers.tsv") if r["name"]]
    pool = [3, 35, 48, 77, 4, 5, 2, 88, 46, 11, 95, 98, 6, 7]
    pool += [rng.randrange(1, n + 1) for _ in range(20)]
    ops, expect = [], []
    tries = 0
    rooms = [int(r["room(obj id)"]) for r in load_tsv("retail_rooms.tsv")]
    while len(ops) < ncmd and tries < ncmd * 4:
        tries += 1
        kind = rng.random()
        snap = g.snapshot()
        try:
            if kind < 0.70:
                vid, nobj = rng.choice(verbs)
                d = rng.choice(pool) if nobj >= 1 else 0
                i = rng.choice(pool) if nobj >= 2 else 0
                line = "cmd %d %d %d" % (vid, d, i)
                r = g.command(vid, d, i)
            elif kind < 0.80:
                fl, on = rng.randrange(1, 801), rng.randrange(2)
                line = "flag %d %d 0" % (fl, on)
                a = 0xCD8AC + ((fl - 1) >> 3)
                b = 1 << ((fl - 1) & 7)
                g.w8(a, (g.r8(a) | b) if on else (g.r8(a) & ~b & 0xFF))
                r = 0
            elif kind < 0.88:
                o, par, sl = rng.choice(pool), rng.choice([0, 88, 98, 3, rng.randrange(1, n + 1)]), rng.randrange(0, 3)
                if o == par or (par and g.call(0x1B5C8, eax=par, edx=o) & 0xFF):    # no cycles
                    continue
                line = "move %d %d %d" % (o, par, sl)
                g.call(0x1B470, eax=o, edx=par, ebx=sl)
                r = 0
            elif kind < 0.94:
                rm = rng.choice(rooms)
                line = "goto %d 0 0" % rm
                r = g.call(0x1EE10, eax=rm) & 0xFFFFFFFF
            else:
                o, at, on = rng.choice(pool), rng.randrange(1, 25), rng.randrange(2)
                line = "attr %d %d %d" % (o, at, on)
                g.call(0x1A1B0 if on else 0x1A1D8, eax=o, edx=at)
                r = 0
        except Exception:
            g.restore(snap)
            continue
        text = g.text()
        h = 1469598103934665603
        for k in range(n):
            h = fnv(g.rb(ptrs[k], PREFIX[types[k]]), h)
        h = fnv(g.rb(0xCD8AC, 100), h)
        ops.append(line)
        expect.append("%d %d %016x [%s]" % (len(ops) - 1, r & 0xFFFFFFFF, h, esc(text)))
    path = os.path.join(ROOT, "port", "build", "engine_ops.txt")
    with open(path, "w") as f:
        f.write("\n".join(ops) + "\n")
    p = subprocess.run([TOOL, os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game"), path], capture_output=True, text=True)
    out = p.stdout.splitlines()
    bad = [(a, b, ops[k]) for k, (a, b) in enumerate(zip(out, expect)) if a != b]
    print("commands:", len(expect), "(skipped %d unrunnable)" % (tries - len(ops) - 0), "C++ lines:", len(out), "mismatches:", len(bad) + abs(len(out) - len(expect)))
    print(p.stderr.strip().splitlines()[-1] if p.stderr.strip() else "")
    for a, b, o in bad[:8]:
        print("  op      :", o, "\n  C++     :", a[:200], "\n  original:", b[:200])
    sys.exit(1 if bad or len(out) != len(expect) else 0)


main()
