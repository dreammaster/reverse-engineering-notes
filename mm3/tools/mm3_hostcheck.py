#!/usr/bin/env python3
"""Static check of the translator's host calls: for every RHOST(c, name, nargs, ret) in src/gen/game_gen.c, the return-address size
(ret: 2 near, 4 far) must agree with the first ret/retf of the original routine in the program image.  A mismatch means the call was
classified wrongly (stack depth bookkeeping, tail jumps).    usage: python3 tools/mm3_hostcheck.py"""
import os, re, sys
import capstone
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import asm2c, mm3_lockstep as L

tr = asm2c.Translator(os.path.join(HERE, "..", "mm3.asm"), os.path.join(HERE, "..", "mm3.idc"))
img = open(os.path.join(HERE, "..", "data", "IMAGE.BIN"), "rb").read()
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
seen = {}
for n, na, r in re.findall(r"RHOST\(c, (\w+), (\d+), (\d)\)", open(os.path.join(HERE, "..", "src", "gen", "game_gen.c")).read()):
    seen.setdefault(n, set()).add(int(r))
bad = 0
for n, rs in sorted(seen.items()):
    lin = tr.func_linear(L.canon(n[2:] if n.startswith("j_") else n))
    if lin is None:
        continue
    k = None
    for ins in md.disasm(img[lin - 0x10000:lin - 0x10000 + 0x600], lin):
        if ins.mnemonic in ("ret", "retn"):
            k = 2
            break
        if ins.mnemonic in ("retf", "lret"):
            k = 4
            break
    if k and rs != {k}:
        bad += 1
        print("MISMATCH %s: translator %s, binary %d" % (n, sorted(rs), k))
print("%d host routines checked, %d mismatches" % (len(seen), bad))
sys.exit(1 if bad else 0)
