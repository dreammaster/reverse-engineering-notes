"""
Differential fuzz of the C++ print_obj port (Text) against the REAL print_obj of SHANNARA.EXE run under unicorn (retail_game.Game).

Random calls (all forms 0-7, the capital / period / verb / print-now flags, valid and invalid objects, regular and irregular verbs,
long strings that hit the 126-character limit) are interleaved with random attribute changes; the returned string, the text printed in
the print-now mode and a hash of the whole object state (print_obj marks objects as mentioned) must agree after every call.

    python port/tests/text_fuzz.py [operations] [seed]
"""
import os
import random
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "reversing", "scripts"))
TOOL = os.path.join(ROOT, "port", "build", "shan_tool.exe")

from retail_game import Game   # noqa: E402

M = (1 << 64) - 1
PREFIX = {1: 33, 2: 41, 3: 48, 4: 24, 5: 49}
VERBS = ["", "take", "are", "is", "were", "have", "do", "go", "put on", "carry", "try", "fly", "play", "watch", "wish", "pass", "buzz",
         "marry", "hurry", "see", "say", "x", "ry", "ly", "ss", "zz", "sh", "ch", "go to", "do not", "are not", "who has", "want to go",
         "a" * 130, "carry " * 30, "wash", "reach", "obey", "deny", "cry", "stay"]
SCRATCH = 0xF80000


def fnv(data, h=1469598103934665603):
    for b in data:
        h ^= b
        h = (h * 1099511628211) & M
    return h


def esc(b):
    return "".join(chr(c) if 0x20 <= c < 0x7F and c != 0x5C else "\\x%02x" % c for c in b)


def main():
    nops = int(sys.argv[1]) if len(sys.argv) > 1 else 3000
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    rng = random.Random(seed)
    g = Game(os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game", "SHANSTR.DAT"))
    n = g.r16(0x217A0)
    ptrs = [g.r32(0x20919 + 5 * i) for i in range(n)]
    types = [g.r8(0x20918 + 5 * i) for i in range(n)]
    po = g.byname["print_obj"]
    SET, CLR = 0x1A1B0, 0x1A1D8
    ops, expect = [], []
    for i in range(nops):
        r = rng.random()
        if r < 0.25:
            obj = rng.randrange(1, n + 1)
            at = rng.choice([1, 2, 3, 4, 6, 0x16, rng.randrange(1, 25)])
            if rng.random() < 0.5:
                ops.append("setattr %d %d" % (obj, at)); g.call(SET, eax=obj, edx=at); res, pn = "-", "-"
            else:
                ops.append("clrattr %d %d" % (obj, at)); g.call(CLR, eax=obj, edx=at); res, pn = "-", "-"
            opname = ops[-1].split()[0]
        else:
            flags = rng.randrange(0, 8) | rng.choice([0, 0x10]) | rng.choice([0, 0x20]) | rng.choice([0, 0x40, 0x40]) | rng.choice([0, 0, 0, 0x80])
            obj = rng.choice([rng.randrange(1, n + 1)] * 6 + [0, n + 1, 3, 4, 5, 46, 2])
            verb = rng.choice(VERBS)
            vtxt = "-" if verb is None else ("_" if verb == "" else verb)
            ops.append("po %d %d %s" % (flags, obj, vtxt))
            opname = "po"
            vptr = 0
            if verb is not None:
                g.uc.mem_write(SCRATCH, verb.encode() + b"\0")
                vptr = SCRATCH
            g.text()
            try:
                r = g.call(po, stack=(flags, obj, vptr) if flags & 0x40 else (flags, obj)) & 0xFFFFFFFF
            except Exception as e:
                print("original crashed on", ops[-1], e, g.where()); raise
            printed = g.text(clear=True) if flags & 0x80 else ""
            # print-now text is captured raw (latin-1 text, \v -> newline in text()); compare the original bytes instead
            res = "NULL" if r == 0 else esc(g._cstr(r))
            pn = "-"
            if r == 0 and (flags & 0x80) and 1 <= obj <= n:
                pn = esc(g.wlog[-1][2]) if g.wlog else "-"
        h = 1469598103934665603
        for k in range(n):
            h = fnv(g.rb(ptrs[k], PREFIX[types[k]]), h)
        expect.append("%d %s [%s] [%s] %016x" % (i, opname, res, pn, h))
    path = os.path.join(ROOT, "port", "build", "text_ops.txt")
    with open(path, "w", encoding="latin-1") as f:
        f.write("\n".join(ops) + "\n")
    out = subprocess.run([TOOL, "textops", os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game", "OBJECT.DAT"), path],
                         capture_output=True, text=True, check=True).stdout.splitlines()
    bad = [(a, b, ops[k]) for k, (a, b) in enumerate(zip(out, expect)) if a != b]
    print("operations:", len(expect), "C++ lines:", len(out), "mismatches:", len(bad) + abs(len(out) - len(expect)))
    for a, b, o in bad[:6]:
        print("  op     :", o, "\n  C++    :", a, "\n  original:", b)
    sys.exit(1 if bad or len(out) != len(expect) else 0)


main()
