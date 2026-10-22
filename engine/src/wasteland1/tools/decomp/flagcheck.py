"""flagcheck.py : calls of ported functions followed (directly) by a conditional jump on a flag the port declares as junk."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from model import Program, CONDFLAGS
here = os.path.dirname(os.path.abspath(__file__))
root = os.path.join(here, "..", "..", "..", "..", "..")
image = open(os.path.join(root, "ida_scripts", "wl_image.bin"), "rb").read()
entries = [int(l.split("#")[0].strip(), 16) for l in open(os.path.join(here, "..", "recomp", "entries.txt")) if l.split("#")[0].strip()]
prog = Program(image, entries)
ports = {}
for line in open(os.path.join(here, "..", "recomp", "ports.txt")):
    p = line.split("#")[0].split()
    if len(p) >= 2:
        junk = []
        for x in p[2:]:
            if x.startswith("junk="):
                junk = x[5:].split(",")
        ports[int(p[0], 16)] = (p[1], set(junk))
for e, f in sorted(prog.funcs.items()):
    for a, ins in sorted(f.insns.items()):
        if ins.op in ("call", "jmp") and ins.target in ports:
            name, junk = ports[ins.target]
            nxt = f.insns.get(a + ins.size)
            if ins.op == "call" and nxt is not None and nxt.op in CONDFLAGS:
                need = CONDFLAGS[nxt.op] & junk
                if need:
                    print("f_%04X at %04X calls %s then %s needs %s" % (e, a, name, nxt.op, ",".join(sorted(need))))
