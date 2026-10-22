"""Differential test: plays the same key script on the 8086 oracle (wlemu) and on the recompiled game (wlrc) and
compares a screenshot after every key.

usage: difftest.py <build dir> <image.bin> <gamedir> <key> [<key> ...]
  key: 4 hex digits (BIOS scan code << 8 | ascii), or text:<chars> to type characters
"""
import os
import subprocess
import sys


def main():
    if len(sys.argv) < 5:
        print(__doc__)
        return 2
    build, image, game = sys.argv[1:4]
    keys = sys.argv[4:]
    out = os.path.join(build, "..", "..", "tmp_out")
    os.makedirs(out, exist_ok=True)

    oracle = [os.path.join(build, "src", "wasteland1", "wlemu.exe"), image, game, "idle", "on", "run", "60"]
    recomp = [os.path.join(build, "src", "wasteland1", "wlrc.exe"), image, game, "wait", "300"]
    for i, k in enumerate(keys):
        if k.startswith("text:"):
            oracle += ["type", k[5:]]
            recomp += ["keys", k[5:]]
        else:
            oracle += ["key", k]
            recomp += ["key", k]
        oracle += ["run", "60", "shot", os.path.join(out, "d_o%02d.png" % i)]
        recomp += ["wait", "300", "shot", os.path.join(out, "d_r%02d.png" % i)]
    env = dict(os.environ, WL_MAXEDGES="2000000000")
    subprocess.run(oracle, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
    r = subprocess.run(recomp, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, check=False)
    if r.returncode:
        print("recomp run failed:", r.stderr.strip()[-300:])
    bad = 0
    for i, k in enumerate(keys):
        a = os.path.join(out, "d_o%02d.png" % i)
        b = os.path.join(out, "d_r%02d.png" % i)
        same = os.path.exists(a) and os.path.exists(b) and open(a, "rb").read() == open(b, "rb").read()
        print("%2d %-12s %s" % (i, k, "same" if same else "DIFFERENT"))
        bad += not same
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
