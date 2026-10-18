"""
Debugging aid: replay the scenario that contains operation INDEX of an ops file (state_diff.py writes port/build/state_ops.txt) on the oracle and on the native
engine and list the savegame-region bytes that differ afterwards (region index, address, offsets).

    python port/tests/region_diff.py port/build/state_ops.txt INDEX
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "reversing", "scripts"))
TOOL = os.path.join(ROOT, "port", "build", "engine_tool.exe")

from retail_game import Game, load_tsv   # noqa: E402


def main():
    lines = [l for l in open(sys.argv[1]).read().splitlines() if l.strip()]
    idx = int(sys.argv[2])
    start = max(i for i, l in enumerate(lines[:idx + 1]) if l.startswith("reset"))
    seg = lines[start:idx + 1]
    g = Game(os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game", "SHANSTR.DAT"))
    for l in seg:
        p = l.split()
        a = [int(x) for x in p[1:4]]
        if p[0] == "flag":
            ad = 0xCD8AC + ((a[0] - 1) >> 3)
            g.w8(ad, g.r8(ad) | (1 << ((a[0] - 1) & 7)))
        elif p[0] == "goto":
            g.call(0x1EE10, eax=a[0])
        elif p[0] == "move":
            g.call(0x1B470, eax=a[0], edx=a[1], ebx=a[2])
        elif p[0] == "call":
            g.call(a[0], eax=a[1], edx=a[2])
        elif p[0] == "cmd":
            g.command(*a)
        g.text()
    ops = os.path.join(ROOT, "port", "build", "region_ops.txt")
    open(ops, "w").write("\n".join(seg) + "\n")
    dump = os.path.join(ROOT, "port", "build", "region_dump.bin")
    dumpall = os.path.join(ROOT, "port", "build", "data_dump.bin")
    env = dict(os.environ, SHAN_DUMP=dump, SHAN_DUMPALL=dumpall)
    subprocess.run([TOOL, os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game"), ops], capture_output=True, text=True, env=env)
    native = open(dump, "rb").read()
    regions = [(int(r["address"], 16), int(r["bytes"]), r["current name"]) for r in load_tsv("retail_save_regions.tsv")]
    full = open(dumpall, "rb").read()
    want_full = bytes(g.rb(0xA0000, 0x50000))
    d = [i for i in range(len(full)) if full[i] != want_full[i]]
    runs = []
    for i in d:
        if runs and i - runs[-1][1] <= 1:
            runs[-1][1] = i
        else:
            runs.append([i, i])
    print("data segment: %d bytes differ in %d runs:" % (len(d), len(runs)), ["0x%X-0x%X" % (0xA0000 + a, 0xA0000 + b) for a, b in runs[:20]])
    for a, b in runs[:6]:
        print("   0x%X: oracle %s native %s" % (0xA0000 + a, want_full[a:b + 1][:12].hex(), full[a:b + 1][:12].hex()))
    off = 0
    for k, (ad, n, name) in enumerate(regions):
        want = bytes(g.rb(ad, n))
        got = native[off:off + n]
        off += n
        if want != got:
            d = [i for i in range(n) if want[i] != got[i]]
            print("region %d @0x%X (%s, %d bytes): %d bytes differ, first offsets %s; oracle %s native %s" %
                  (k, ad, name, n, len(d), d[:6], [want[i] for i in d[:6]], [got[i] for i in d[:6]]))


main()
