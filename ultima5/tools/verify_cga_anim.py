"""Check src/graphics/animate_cga.c against the original CGA.DRV by running the driver's own machine code (Unicorn, 16-bit x86).

python tools/verify_cga_anim.py [--runs N] [--seed S] [--length L]

Real and random 32 KB tilesets and random operation sequences (tileset animation, the moongate scratch tile, the modes of the
time/status helper) are applied by the driver in the emulator and by the C port; after every operation the whole tileset (hash), the
driver's random seed and, for the moongate, the scratch tile must be identical.  Needs CGA.DRV and TILES.4 of the original game
(U5_GAME_DIR, default C:/games/ultima5).
"""
import argparse
import os
import random
import subprocess
import sys
import tempfile

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_CS,
                               UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS, UC_X86_REG_SP, UC_X86_REG_IP, UC_X86_REG_FLAGS)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = os.environ.get("U5_GAME_DIR", "C:/games/ultima5")

CS = 0x2000
TS_SEG = 0x3000
SS = 0x5000
STUB = 0x6000


def fnv(data):
    h = 1469598103934665603
    for b in data:
        h = ((h ^ b) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h


class Cga:
    def __init__(self, tileset):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_16)
        self.uc.mem_map(CS * 16, 0x10000)
        self.uc.mem_map(TS_SEG * 16, 0x10000)
        self.uc.mem_map(SS * 16, 0x10000)
        self.uc.mem_map(0x40000, 0x10000)   # the caller's DS/ES
        self.uc.mem_map(STUB * 16, 0x1000)
        self.uc.mem_write(CS * 16, open(os.path.join(GAME, "CGA.DRV"), "rb").read())
        self.uc.mem_write(STUB * 16, b"\xf4")
        self.uc.mem_write(TS_SEG * 16, tileset)
        # the segment of the loaded tileset (written by driver entry 48 in the game)
        self.uc.mem_write(CS * 16 + 0x206, TS_SEG.to_bytes(2, "little"))
        self.scratch = None
        # the moongate routine ends with "push cs; call 0d99" (draw the scratch tile): record the tile and skip the drawing
        self.uc.hook_add(UC_HOOK_CODE, self.on_code, begin=CS * 16 + 0x18E8, end=CS * 16 + 0x18E8)

    def on_code(self, uc, address, size, user):
        self.scratch = bytes(uc.mem_read(TS_SEG * 16 + 0x4580, 0x40))
        uc.reg_write(UC_X86_REG_IP, 0x18EC)

    def call(self, entry, ax=0, bx=0, carry=False):
        uc = self.uc
        uc.reg_write(UC_X86_REG_SS, SS)
        sp = 0xFFF0 - 4
        uc.mem_write(SS * 16 + sp, (0).to_bytes(2, "little") + STUB.to_bytes(2, "little"))
        uc.reg_write(UC_X86_REG_SP, sp)
        uc.reg_write(UC_X86_REG_DS, 0x4000)
        uc.reg_write(UC_X86_REG_ES, 0x4000)
        uc.reg_write(UC_X86_REG_AX, ax)
        uc.reg_write(UC_X86_REG_BX, bx)
        uc.reg_write(UC_X86_REG_CX, 0x0102)
        uc.reg_write(UC_X86_REG_DX, 0x0304)
        uc.reg_write(UC_X86_REG_SI, 0x0506)
        uc.reg_write(UC_X86_REG_DI, 0x0708)
        uc.reg_write(UC_X86_REG_FLAGS, 0x203 if carry else 0x202)
        uc.reg_write(UC_X86_REG_CS, CS)
        uc.emu_start(CS * 16 + entry, STUB * 16, timeout=0, count=3000000)

    def tileset(self):
        return bytes(self.uc.mem_read(TS_SEG * 16, 0x8000))

    def seed(self):
        return int.from_bytes(self.uc.mem_read(CS * 16 + 0x1408, 2), "little")


def make_ops(rng, n):
    ops = []
    for _ in range(n):
        k = rng.choice(["T", "T", "T", "M", "M", "G"])
        if k == "M":
            ops.append(("M", rng.choice([0, 1, 2, 2, 3, 4, 5]), rng.randrange(24), rng.randrange(60)))
        elif k == "G":
            ops.append(("G", rng.randrange(0, 17), rng.choice([0xFF, 0, 5])))
        else:
            ops.append(("T",))
    return ops


def run_driver(tileset, ops):
    c = Cga(tileset)
    out = []
    for op in ops:
        c.scratch = None
        if op[0] == "T":
            c.call(0x60)
        elif op[0] == "M":
            c.call(0x6C, ax=op[1], bx=(op[3] << 8) | op[2])
        else:
            c.call(0x60, ax=op[1], bx=op[2], carry=True)
            out.append("G scratch %x" % fnv(c.scratch))
        out.append("%s %x %04x" % (op[0], fnv(c.tileset()), c.seed()))
    return out


def run_port(tileset, ops, exe):
    with tempfile.TemporaryDirectory() as d:
        tp, sp = os.path.join(d, "ts.bin"), os.path.join(d, "script.txt")
        open(tp, "wb").write(tileset)
        open(sp, "w").write("\n".join(" ".join(str(x) for x in op) for op in ops) + "\n")
        r = subprocess.run([exe, tp, sp], capture_output=True, text=True)
        if r.returncode:
            sys.exit("port failed: %s %s" % (r.returncode, r.stderr))
        return r.stdout.splitlines()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=40)
    ap.add_argument("--seed", type=int, default=3)
    ap.add_argument("--length", type=int, default=12)
    args = ap.parse_args()

    exe = os.path.join(ROOT, "build", "test_animate_cga.exe")
    subprocess.run(["gcc", "-std=gnu11", "-I" + os.path.join(ROOT, "src"), "-o", exe, os.path.join(ROOT, "tests", "test_animate_cga.c"),
                    os.path.join(ROOT, "src", "graphics", "animate_cga.c"), os.path.join(ROOT, "src", "common", "memory.c")], check=True)
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    from u5lzw import lzw_decompress
    real = lzw_decompress(open(os.path.join(GAME, "TILES.4"), "rb").read()).ljust(0x8000, b"\0")

    rng = random.Random(args.seed)
    for run in range(args.runs):
        ts = real if run % 2 == 0 else bytes(rng.randrange(256) for _ in range(0x8000))
        ops = make_ops(rng, args.length)
        want = run_driver(ts, ops)
        got = run_port(ts, ops, exe)
        if want != got:
            for i, (a, b) in enumerate(zip(want, got)):
                if a != b:
                    print("run %d (%s tileset): first difference at output line %d (ops so far %s): driver %r, port %r" %
                          (run, "real" if run % 2 == 0 else "random", i, ops, a, b))
                    break
            else:
                print("run %d: output length differs (%d vs %d)" % (run, len(want), len(got)))
            sys.exit(1)
    print("CGA.DRV and the port agree on %d runs" % args.runs)


main()
