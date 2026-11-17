"""
fuzz_compare.py N SEED [STEPS]: plays N random click scripts of STEPS clicks (default 8) in the oracle and in the standalone game (compare_play.py) and prints the first step of
each script where the screens differ outside the picture window. A difference after a long text is expected (see docs); anything else is a bug to look at.
"""
import random
import subprocess
import sys
import os

HERE = os.path.dirname(os.path.abspath(__file__))


def script(rng, steps):
    acts = []
    while len(acts) < steps:
        r = rng.random()
        if r < 0.5:
            acts.append("c 20 %d" % (4 + 10 * rng.randrange(7) + 5))      # a verb
            acts.append("c %d %d" % (rng.randrange(52, 314), rng.randrange(6, 124)))
        elif r < 0.7:
            acts.append("c %d %d" % (rng.randrange(8, 44), rng.randrange(132, 166)))        # compass
        elif r < 0.85:
            acts.append("c 14 175")                                                         # wait
        else:
            acts.append("c %d %d" % (rng.randrange(52, 314), rng.randrange(6, 124)))        # a thing, no verb
    return acts[:steps]


def main():
    n, seed = int(sys.argv[1]), int(sys.argv[2])
    steps = int(sys.argv[3]) if len(sys.argv) > 3 else 8
    rng = random.Random(seed)
    for i in range(n):
        acts = script(rng, steps)
        out = subprocess.run([sys.executable, os.path.join(HERE, "compare_play.py"), ";".join(acts), "--every"], capture_output=True, text=True).stdout
        bad = [l for l in out.splitlines() if "DIFFERENT" in l]
        print("script %d: %s" % (i, "first difference: " + bad[0].strip() if bad else "identical"))
        if bad:
            print("   ", ";".join(acts))


main()
