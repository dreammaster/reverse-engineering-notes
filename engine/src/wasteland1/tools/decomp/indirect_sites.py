import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from model import Program
here = os.path.dirname(os.path.abspath(__file__))
root = os.path.join(here, "..", "..", "..", "..", "..")
image = open(os.path.join(root, "ida_scripts", "wl_image.bin"), "rb").read()
entries = [int(l.split("#")[0].strip(), 16) for l in open(os.path.join(here, "..", "recomp", "entries.txt")) if l.split("#")[0].strip()]
prog = Program(image, entries)
for e, f in sorted(prog.funcs.items()):
    for a, ins in sorted(f.insns.items()):
        if ins.op in ("calli", "jmpi", "callfi", "jmpfi", "callf"):
            m = ins.a
            desc = ""
            if m is not None and m.kind == "mem":
                desc = "[%s+%X] seg=%s" % ("+".join(m.base), m.disp & 0xFFFF, m.seg)
            elif m is not None:
                desc = "reg"
            print("%04X in f_%04X: %s %s" % (a, e, ins.op, desc))
