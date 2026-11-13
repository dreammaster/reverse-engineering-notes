"""
compare_play.py "<script>" [--every]: plays the same clicks in the original (the oracle, engine/host/play.sh) and in the standalone game (xanth_play) and compares the
screens. Script: "c X Y" clicks separated by ';' ("c 20 28;c 160 100"); each prefix of the script is run when --every is given, else only the whole script.
Screens go to the scratch folder (env SCRATCH, default %TEMP%/cmp); regions: verbs, picture window, text window, compass, buttons, inventory.
The picture window may differ in the rain's animation phase (the oracle's clock is a step count): its count is reported but a run is only 'ok' if everything else
is identical.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
XANTH = os.path.abspath(os.path.join(HERE, "..", ".."))
HOST = os.path.join(XANTH, "engine", "host")
SCRATCH = os.environ.get("SCRATCH", os.path.join(tempfile.gettempdir(), "cmp"))
PLAY = os.environ.get("XANTH_PLAY", os.path.join(tempfile.gettempdir(), "xb", "xanth_play.exe"))
BASE, GAP = 41000000, 2500000


def oracle(actions, out):
    tl = []
    t = BASE
    for a in actions:
        op, x, y = a.split()[0], int(a.split()[1]), int(a.split()[2])
        if op != "c":
            continue
        tl.append("%d:%d,%d,0" % (t, x, y))
        tl.append("%d:%d,%d,1" % (t + 400000, x, y))
        tl.append("%d:%d,%d,0" % (t + 800000, x, y))
        t += GAP
    subprocess.run(["bash", "play.sh", ",".join(tl), str(t + 3000000), out], cwd=HOST, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=900)


def ppm(path):
    d = open(path, "rb").read().split(b"\n", 3)
    return d[3]


REGIONS = [("verbs", 0, 0, 49, 127), ("picture window", 50, 3, 315, 126), ("text window", 50, 127, 315, 166), ("compass", 0, 127, 49, 166), ("buttons", 0, 167, 49, 199),
           ("inventory", 50, 167, 319, 199)]


def compare(a, b):
    out = {}
    for name, x1, y1, x2, y2 in REGIONS:
        n = 0
        for y in range(y1, y2 + 1):
            row = y * 320 * 3
            n += sum(1 for x in range(x1, x2 + 1) if a[row + x * 3:row + x * 3 + 3] != b[row + x * 3:row + x * 3 + 3])
        out[name] = n
    return out


def main():
    os.makedirs(SCRATCH, exist_ok=True)
    script = sys.argv[1]
    actions = [s.strip() for s in script.split(";") if s.strip()]
    prefixes = range(1, len(actions) + 1) if "--every" in sys.argv else [len(actions)]
    for k in prefixes:
        part = actions[:k]
        o, m = os.path.join(SCRATCH, "oracle_%d.ppm" % k), os.path.join(SCRATCH, "ours_%d.ppm" % k)
        oracle(part, o)
        subprocess.run([PLAY, os.path.join(XANTH, "game"), m, ";".join(part)], check=True)
        diff = compare(ppm(o), ppm(m))
        others = sum(v for n, v in diff.items() if n != "picture window")
        print("%2d  %-12s %s  -> %s" % (k, part[-1], " ".join("%s=%d" % kv for kv in diff.items()), "ok" if others == 0 else "DIFFERENT"))


main()
