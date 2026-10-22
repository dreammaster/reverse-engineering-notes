"""junkcheck.py : lists registers/flags declared junk in ports.txt that some caller reads after the call (real bugs)."""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from model import Program, is_weak
from analysis import Analysis, build_dataref_set, load_hints

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.join(here, "..", "..", "..", "..", "..")
image = open(os.path.join(root, "ida_scripts", "wl_image.bin"), "rb").read()
entries = [int(l.split("#")[0].strip(), 16) for l in open(os.path.join(here, "..", "recomp", "entries.txt")) if l.split("#")[0].strip()]
prog = Program(image, entries)
an = Analysis(prog, set(), load_hints(os.path.join(here, "indirect.txt")))
for line in open(os.path.join(here, "..", "recomp", "ports.txt")):
    p = line.split("#")[0].split()
    if len(p) < 2:
        continue
    a = int(p[0], 16)
    junk = []
    for x in p[2:]:
        if x.startswith("junk="):
            junk = x[5:].split(",")
    f = prog.funcs.get(a)
    if not f:
        continue
    names = set()
    for j in junk:
        names.update({"ax": ["al", "ah"], "bx": ["bl", "bh"], "cx": ["cl", "ch"], "dx": ["dl", "dh"]}.get(j, [j]))
    bad = sorted(n for n in names if n in f.out and n not in ("ds", "df"))
    if bad:
        print("%04X %-28s junk but read by callers: %s" % (a, p[1], " ".join(bad)))
