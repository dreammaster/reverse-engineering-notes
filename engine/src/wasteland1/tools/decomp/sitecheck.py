"""sitecheck.py [name] : for every direct call of a ported function, the junk registers that are live after the call."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from model import Program, is_weak
from analysis import Analysis, load_hints
here = os.path.dirname(os.path.abspath(__file__))
root = os.path.join(here, "..", "..", "..", "..", "..")
image = open(os.path.join(root, "ida_scripts", "wl_image.bin"), "rb").read()
entries = [int(l.split("#")[0].strip(), 16) for l in open(os.path.join(here, "..", "recomp", "entries.txt")) if l.split("#")[0].strip()]
prog = Program(image, entries)
an = Analysis(prog, set(), load_hints(os.path.join(here, "indirect.txt")))
ports = {}
for line in open(os.path.join(here, "..", "recomp", "ports.txt")):
    p = line.split("#")[0].split()
    if len(p) >= 2:
        junk = set()
        for x in p[2:]:
            if x.startswith("junk="):
                for j in x[5:].split(","):
                    junk.update({"ax": ["al", "ah"], "bx": ["bl", "bh"], "cx": ["cl", "ch"], "dx": ["dl", "dh"]}.get(j, [j]))
        ports[int(p[0], 16)] = (p[1], junk - {"ds", "df"})
only = sys.argv[1] if len(sys.argv) > 1 else None
res = {}
for e, f in sorted(prog.funcs.items()):
    rec = an.live_after_calls(f)
    for (callee, site), live in rec.items():
        if callee in ports:
            name, junk = ports[callee]
            if only and name != only:
                continue
            bad = sorted(r for r in (live & junk) if not is_weak(r))
            if bad:
                res.setdefault(name, []).append("f_%04X@%s:%s" % (e, site if site == "tail" else "%04X" % site, "".join(b + " " for b in bad).strip().replace(" ", "/")))
for n, v in sorted(res.items()):
    print(n, len(v), " ".join(v[:6]))
