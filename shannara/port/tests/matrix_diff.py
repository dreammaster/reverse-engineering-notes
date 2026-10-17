"""
Exhaustive single-command differential test: every verb against EVERY object (1-744, plus none) - and for two-object verbs against a set of
indirect objects - each from the start state (the state is reset between commands on both sides).  The native engine's return value, state hash
and text must equal the unicorn oracle's for all of them.  With SHAN_COVERAGE the native engine also records which transpiled instructions ran,
so the verified fraction of the handler code can be measured (see coverage_report.py).

    python port/tests/matrix_diff.py [stride] [coverage-file]      stride 1 = all objects (about 60,000 commands), 7 = every 7th object
"""
import os
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
    stride = int(sys.argv[1]) if len(sys.argv) > 1 else 7
    cov = sys.argv[2] if len(sys.argv) > 2 else None
    g = Game(os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game", "SHANSTR.DAT"))
    n = g.r16(0x217A0)
    ptrs = [g.r32(0x20919 + 5 * i) for i in range(n)]
    types = [g.r8(0x20918 + 5 * i) for i in range(n)]
    verbs = [(int(r["verb"]), int(r["flags"] or 1)) for r in load_tsv("retail_verb_handlers.tsv") if r["name"]]
    objs = [0] + list(range(1, n + 1, stride))
    iobjs = [3, 35, 77, 4, 88, 46]
    base = g.snapshot()
    ops, expect = [], []

    def state_hash():
        h = 1469598103934665603
        for k in range(n):
            h = fnv(g.rb(ptrs[k], PREFIX[types[k]]), h)
        return fnv(g.rb(0xCD8AC, 100), h)

    for vid, nobj in verbs:
        for d in (objs if nobj >= 1 else [0]):
            for i in (iobjs if nobj >= 2 else [0]):
                g.restore(base)
                ops.append("reset 0 0 0")
                expect.append("%d 0 %016x []" % (len(ops) - 1, state_hash()))
                try:
                    r = g.command(vid, d, i)
                    text = g.text()
                except Exception:
                    g.restore(base)
                    ops.pop()
                    expect.pop()
                    continue
                ops.append("cmd %d %d %d" % (vid, d, i))
                expect.append("%d %d %016x [%s]" % (len(ops) - 1, r & 0xFFFFFFFF, state_hash(), esc(text)))
    path = os.path.join(ROOT, "port", "build", "matrix_ops.txt")
    open(path, "w").write("\n".join(ops) + "\n")
    env = dict(os.environ)
    if cov:
        env["SHAN_COVERAGE"] = cov
    p = subprocess.run([TOOL, os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game"), path], capture_output=True, text=True, env=env)
    out = p.stdout.splitlines()
    bad = [(a, b, ops[k]) for k, (a, b) in enumerate(zip(out, expect)) if a != b]
    print("commands:", sum(1 for o in ops if o.startswith("cmd")), "lines:", len(out), "mismatches:", len(bad) + abs(len(out) - len(expect)))
    for a, b, o in bad[:8]:
        print("  op      :", o, "\n  C++     :", a[:200], "\n  original:", b[:200])
    sys.exit(1 if bad or len(out) != len(expect) else 0)


main()
