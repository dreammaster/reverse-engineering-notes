"""scan_regions.py OBJECT PHASE: which object records / saved regions differ between the standalone game and the oracle for one object's scenarios of one phase.

Runs test_scan (XANTH_BUILD = build folder, default %TEMP%/xb) and engine/host/host_test.exe scanlog with XANTH_SCANDETAIL, from xanth/ ; prints "O<object id>" or "R<region index>" and
the verbs where they differ (the region numbers are the lines of reversing/analysis/xanth_save_regions.tsv, counted from 0)."""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
XANTH = os.path.abspath(os.path.join(HERE, "..", ".."))
BUILD = os.environ.get("XANTH_BUILD", os.path.join(tempfile.gettempdir(), "xb"))


def hashes(cmd, env, cwd):
    e = dict(os.environ, XANTH_SCANDETAIL="1", **env)
    r = subprocess.run(cmd, env=e, cwd=cwd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    d = {}
    for line in r.stderr.decode("latin1").split(chr(10)):
        f = line.split()
        if len(f) == 5 and f[3][0] in "OR":
            d[(f[2], f[3])] = f[4]
    return d


def main():
    obj, phase = sys.argv[1], sys.argv[2]
    env = dict(XANTH_SCANONLY=obj, XANTH_PHASES=phase)
    mine = hashes([os.path.join(BUILD, "test_scan.exe")], dict(env, XANTH_DATA=os.path.join(XANTH, "game"), XANTH_REGIONS=os.path.join(XANTH, "reversing", "analysis", "xanth_save_regions.tsv")), os.path.join(XANTH, "src"))
    orc = hashes([os.path.join(XANTH, "engine", "host", "host_test.exe"), "game", "game_out/image.bin", "reversing/analysis/xanth_objects.tsv", "scanlog", os.path.join(tempfile.gettempdir(), "scan_regions.log")], env, XANTH)
    diff = {}
    for k in mine:
        if k in orc and mine[k] != orc[k]:
            diff.setdefault(k[1], []).append(k[0])
    for k, v in sorted(diff.items()):
        print(k, "verbs", ",".join(v))
    print(len(diff), "records / regions differ")


main()
