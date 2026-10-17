"""
Differential fuzz of the C++ world model (containment tree, attributes, game flags) against the REAL retail machine code.

Random operation sequences are executed (1) by the original functions of SHANNARA.EXE under unicorn
(reversing/scripts/retail_emu.py) and (2) by port/build/shan_tool `world` (World/SaveState); every result and a hash of the whole
saved object state + the flag set must agree after every operation.

    python port/tests/world_fuzz.py [operations] [seed]
"""
import os
import random
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "reversing", "scripts"))
EXE = os.path.join(ROOT, "game", "SHANNARA.EXE")
TOOL = os.path.join(ROOT, "port", "build", "shan_tool.exe")

from retail_emu import RetailEmu   # noqa: E402
from unicorn.x86_const import UC_X86_REG_EAX   # noqa: E402

M = (1 << 64) - 1
SIZES = {1: 36, 2: 44, 3: 48, 4: 24, 5: 52}
PREFIX = {1: 33, 2: 41, 3: 48, 4: 24, 5: 49}
FLAGS = 0xCD8AC

# original entry points (register convention eax, edx, ebx)
LOC, SLOT, NEXT, HEAD = 0x1A200, 0x1A2A0, 0x1A49C, 0x1A348
MOVE, HAS, SET, CLR, PARENT_IS, WITHIN = 0x1B470, 0x1A184, 0x1A1B0, 0x1A1D8, 0x1B588, 0x1B5C8
BS_TEST, BS_SET, BS_CLR = 0x21F30, 0x21F80, 0x21FC4


def fnv(data, h=1469598103934665603):
    for b in data:
        h ^= b
        h = (h * 1099511628211) & M
    return h


class Model:
    def __init__(self):
        self.e = RetailEmu(EXE)
        e = self.e
        # sub_10F70 is a UI "current selection" helper that object_move? calls at its end: emulate it without the UI
        def sel(emu):
            a = emu.reg(UC_X86_REG_EAX) & 0xFFFFFFFF
            if a != 0xFFFFFFFF:
                emu.w16(0xA7E54, a)
            return emu.r16(0xA7E54)
        e.hook(0x10F70, sel)
        self.n = e.r16(0x217A0)
        self.types = [e.r8(0x20918 + 5 * i) for i in range(self.n)]
        self.ptrs = [e.r32(0x20919 + 5 * i) for i in range(self.n)]

    def state_hash(self):
        h = 1469598103934665603
        for i in range(self.n):
            h = fnv(self.e.rb(self.ptrs[i], PREFIX[self.types[i]]), h)
        return fnv(self.e.rb(FLAGS, 100), h)

    def loc(self, o): return self.e.call(LOC, eax=o) & 0xFFFF
    def within(self, o, a): return self.e.call(WITHIN, eax=o, edx=a) & 0xFF


def run(nops, seed):
    rng = random.Random(seed)
    m = Model()
    e, n = m.e, m.n
    ops = []
    expect = []
    for i in range(nops):
        r = rng.random()
        obj = rng.randrange(0, n + 3)                     # includes invalid ids
        if r < 0.10:
            ops.append(("loc", obj, 0, 0)); res = e.call(LOC, eax=obj) & 0xFFFF
        elif r < 0.16:
            ops.append(("slot", obj, 0, 0)); res = e.call(SLOT, eax=obj) & 0xFF
        elif r < 0.22:
            ops.append(("next", obj, 0, 0)); res = e.call(NEXT, eax=obj) & 0xFFFF
        elif r < 0.30:
            sl = rng.randrange(0, 4)
            ops.append(("head", obj, sl, 0)); res = e.call(HEAD, eax=obj, edx=sl) & 0xFFFF
        elif r < 0.52:                                      # move, avoiding cycles (the original has no cycle check)
            par = rng.choice([0, rng.randrange(1, n + 1), rng.randrange(1, n + 1), rng.randrange(1, n + 1)])
            if not (1 <= obj <= n):
                continue
            if par and (par == obj or m.within(par, obj)):
                continue
            sl = rng.randrange(0, 3)
            ops.append(("move", obj, par, sl)); e.call(MOVE, eax=obj, edx=par, ebx=sl); res = 0
        elif r < 0.62:
            at = rng.randrange(0, 25)   # 24 descriptors exist (0x208A0..0x20918); beyond that the original reads the object table as descriptors
            ops.append(("has", obj, at, 0)); res = e.call(HAS, eax=obj, edx=at) & 0xFF
        elif r < 0.70:
            at = rng.randrange(0, 25)   # 24 descriptors exist (0x208A0..0x20918); beyond that the original reads the object table as descriptors
            ops.append(("setattr", obj, at, 0)); e.call(SET, eax=obj, edx=at); res = 0
        elif r < 0.78:
            at = rng.randrange(0, 25)   # 24 descriptors exist (0x208A0..0x20918); beyond that the original reads the object table as descriptors
            ops.append(("clrattr", obj, at, 0)); e.call(CLR, eax=obj, edx=at); res = 0
        elif r < 0.83:
            par = rng.randrange(0, n + 2)
            ops.append(("parentis", obj, par, 0)); res = e.call(PARENT_IS, eax=obj, edx=par) & 0xFF
        elif r < 0.88:
            a = rng.randrange(0, n + 2)
            ops.append(("within", obj, a, 0)); res = e.call(WITHIN, eax=obj, edx=a) & 0xFF
        elif r < 0.93:
            f = rng.randrange(0, 801)
            ops.append(("flag", f, 0, 0)); res = e.call(BS_TEST, eax=FLAGS, edx=f) & 0xFF
        elif r < 0.97:
            f = rng.randrange(0, 801)
            ops.append(("setflag", f, 0, 0)); e.call(BS_SET, eax=FLAGS, edx=f); res = 0
        else:
            f = rng.randrange(0, 801)
            ops.append(("clrflag", f, 0, 0)); e.call(BS_CLR, eax=FLAGS, edx=f); res = 0
        expect.append("%d %s %d %016x" % (len(ops) - 1, ops[-1][0], res, m.state_hash()))
    return ops, expect


if __name__ == "__main__":
    nops = int(sys.argv[1]) if len(sys.argv) > 1 else 3000
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    ops, expect = run(nops, seed)
    path = os.path.join(ROOT, "port", "build", "world_ops.txt")
    with open(path, "w") as f:
        for op in ops:
            f.write("%s %d %d %d\n" % op)
    out = subprocess.run([TOOL, "world", EXE, path], capture_output=True, text=True, check=True).stdout.splitlines()
    bad = [(a, b) for a, b in zip(out, expect) if a != b]
    print("operations:", len(expect), "C++ lines:", len(out), "mismatches:", len(bad) + abs(len(out) - len(expect)))
    for a, b in bad[:5]:
        print("  C++     :", a, "\n  original:", b)
    sys.exit(1 if bad or len(out) != len(expect) else 0)
