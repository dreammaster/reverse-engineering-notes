"""
Verifies an operations file (engine_tool format: reset / flag / goto / move / cmd / attr / call lines; '#' lines are comments) against the unicorn oracle with every comparison
state_diff.py makes: return value, object/flag hash, text, the 92 savegame regions, a CRC of the whole data segment and the boundary-event stream.
Operations that make the oracle fault (UI waits that the headless stubs cannot satisfy) are rolled back and dropped on both sides.
Typical use: the corpus written by port/build/fuzz_tool.exe (coverage-guided explorer).

    python port/tests/replay_diff.py OPS [coverage-file]
"""
import os
import subprocess
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "reversing", "scripts"))
TOOL = os.path.join(ROOT, "port", "build", "engine_tool.exe")

from retail_game import Game, load_tsv   # noqa: E402

M = (1 << 64) - 1
PREFIX = {1: 33, 2: 41, 3: 48, 4: 24, 5: 49}


POLL = {"update_sound", "get_mouse_button", "key_hit", "video_sync", "get_event", "delay?", "delay_animate?", "wait_ticks?", "_hide_mouse", "gxVirtualDisplay"}


def collapse(lst):
    """polling loops spin a clock-dependent number of times: keep one entry per run of the same polling call (name and stack arguments)"""
    out = []
    for x in lst:
        t = x.split()
        key = (t[0], t[5:])
        if out and t[0] in POLL and out[-1][0] == key:
            continue
        out.append((key, x))
    return [x for _, x in out]


def fnv(data, h=1469598103934665603):
    for b in data:
        h ^= b
        h = (h * 1099511628211) & M
    return h


def esc(s):
    return "".join("\\n" if ch == 10 else chr(ch) if 0x20 <= ch < 0x7F and ch != 0x5C else "\\x%02x" % ch for ch in s.encode("latin-1"))


def main():
    src = [l.split() for l in open(sys.argv[1]).read().splitlines() if l.strip() and not l.startswith("#")]
    cov = sys.argv[2] if len(sys.argv) > 2 else None
    g = Game(os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game", "SHANSTR.DAT"))
    g.max_insns = 600_000
    n = g.r16(0x217A0)
    ptrs = [g.r32(0x20919 + 5 * i) for i in range(n)]
    types = [g.r8(0x20918 + 5 * i) for i in range(n)]
    regions = [(int(r["address"], 16), int(r["bytes"])) for r in load_tsv("retail_save_regions.tsv")]
    base = g.snapshot()
    ops, expect, evs, rhs, crcs = [], [], [], [], []

    def state_hash():
        h = 1469598103934665603
        for k in range(n):
            h = fnv(g.rb(ptrs[k], PREFIX[types[k]]), h)
        return fnv(g.rb(0xCD8AC, 100), h)

    def region_hash():
        h = 1469598103934665603
        for a, n_ in regions:
            h = fnv(g.rb(a, n_), h)
        return h

    for p in src:
        kind, a = p[0], [int(x) for x in p[1:4]]
        snap = g.snapshot()
        del g.events[:]
        try:
            if kind == "reset":
                g.restore(base)
                r = 0
            elif kind == "flag":
                ad = 0xCD8AC + ((a[0] - 1) >> 3)
                g.w8(ad, (g.r8(ad) | (1 << ((a[0] - 1) & 7))) if a[1] else (g.r8(ad) & ~(1 << ((a[0] - 1) & 7)) & 0xFF))
                r = 0
            elif kind == "goto":
                r = g.call(0x1EE10, eax=a[0]) & 0xFFFFFFFF
            elif kind == "move":
                g.call(0x1B470, eax=a[0], edx=a[1], ebx=a[2])
                r = 0
            elif kind == "cmd":
                r = g.command(a[0], a[1], a[2])
            elif kind == "attr":
                g.call(0x1A1B0 if a[2] else 0x1A1D8, eax=a[0], edx=a[1])
                r = 0
            elif kind == "call":
                r = g.call(a[0], eax=a[1], edx=a[2]) & 0xFFFFFFFF
            else:
                continue
        except Exception:
            g.restore(snap)
            del g.events[:]
            g.text()
            continue
        ops.append(" ".join(p))
        evs.append(list(g.events))
        del g.events[:]
        rhs.append(region_hash())
        crcs.append(zlib.crc32(bytes(g.rb(0xA0000, 0x50000))))
        expect.append("%d %d %016x [%s]" % (len(ops) - 1, r & 0xFFFFFFFF, state_hash(), esc(g.text())) if kind != "reset" else "%d 0 %016x []" % (len(ops) - 1, state_hash()))
    path = os.path.join(ROOT, "port", "build", "replay_ops.txt")
    open(path, "w").write("\n".join(ops) + "\n")
    evpath = os.path.join(ROOT, "port", "build", "replay_events.txt")
    env = dict(os.environ, SHAN_EVENTS=evpath)
    if cov:
        env["SHAN_COVERAGE"] = cov
    p = subprocess.run([TOOL, os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game"), path], capture_output=True, text=True, env=env)
    out = p.stdout.splitlines()
    bad = [(k, a, b) for k, (a, b) in enumerate(zip(out, expect)) if a != b and not (ops[k].startswith("flag") and False)]
    native_ev, native_rh, native_crc = {}, {}, {}
    for l in open(evpath).read().splitlines():
        k, rest = l.split(" ", 1)
        if rest.startswith("@region "):
            native_rh[int(k)] = int(rest.split()[1], 16)
            native_crc[int(k)] = int(rest.split()[2], 16)
        else:
            native_ev.setdefault(int(k), []).append(rest)
    rh_bad = [k for k in range(len(ops)) if native_rh.get(k) != rhs[k]]
    crc_bad = [k for k in range(len(ops)) if native_crc.get(k) != crcs[k]]
    names = {x.split()[0] for v in native_ev.values() for x in v}
    ev_bad = []
    for k in range(len(ops)):
        want = collapse([x for x in evs[k] if x.split()[0] in names])
        got = collapse(native_ev.get(k, []))
        strip = lambda lst: [(x.split()[0], x.split()[5:]) for x in lst]
        if strip(want) != strip(got):
            ev_bad.append(k)
    print("operations replayed: %d (of %d lines)  text/state mismatches: %d  region-hash: %d  data-CRC: %d  boundary-events: %d" %
          (len(ops), len(src), len(bad) + abs(len(out) - len(expect)), len(rh_bad), len(crc_bad), len(ev_bad)))
    first = sorted(set([b[0] for b in bad[:1]] + rh_bad[:1] + crc_bad[:1] + ev_bad[:1]))
    for k in first[:3]:
        print("  first problem near op %d: %s" % (k, ops[k]))
    for k in ev_bad[:3]:
        got = native_ev.get(k, [])
        want = collapse([x for x in evs[k] if x.split()[0] in names])
        first = next((j for j in range(max(len(got), len(want))) if j >= len(got) or j >= len(want) or got[j] != want[j]), 0)
        print("  events of op %d (%s): C++ %d vs original %d, first difference at %d\n     C++     : %s\n     original: %s" % (k, ops[k], len(got), len(want), first, got[first:first + 3], want[first:first + 3]))
    for k, a, b in bad[:3]:
        print("  op      :", ops[k], "\n  C++     :", a[:200], "\n  original:", b[:200])
    sys.exit(1 if bad or rh_bad or crc_bad or ev_bad or len(out) != len(expect) else 0)


main()
