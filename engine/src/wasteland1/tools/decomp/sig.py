"""sig.py ADDR... : prints the inferred register signature of functions (inputs, outputs, other writes)."""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from model import Program
from analysis import Analysis, build_dataref_set, sig_in, sig_out, sig_junk, load_hints, load_hints

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.join(here, "..", "..", "..", "..", "..")
image = open(os.path.join(root, "ida_scripts", "wl_image.bin"), "rb").read()
entries = []
for line in open(os.path.join(here, "..", "recomp", "entries.txt")):
    line = line.split("#")[0].strip()
    if line:
        entries.append(int(line, 16))
prog = Program(image, entries)
esc = build_dataref_set(image, prog.entries)
an = Analysis(prog, esc, load_hints(os.path.join(here, "indirect.txt")))
order = lambda s: sorted(s)
for a in sys.argv[1:]:
    e = int(a, 16) % 0x10000
    f = prog.funcs[e]
    print("%04X in=%s out=%s junk=%s %s" % (e, " ".join(order(sig_in(f))), " ".join(order(sig_out(f))), " ".join(order(sig_junk(f))), "(escaping)" if e in esc else ""))
print("rounds", an.rounds)
