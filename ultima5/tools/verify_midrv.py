"""Check src/audio/u5music.c against the original MID.DRV of the Ultima V Upgrade by running the driver's own machine code.

python tools/verify_midrv.py [--runs N] [--length L] [--seed S]

MID.DRV (u5upgrade/mid.drv, git-ignored) is loaded into a Unicorn 16-bit x86 emulator.  Its ten far-call entry points are invoked with
random game states / scene numbers / Midpak status values; INT 66h (Midpak) is hooked to record "load NAME", "start" and "stop" and to
answer the status query.  The same random script is fed to tests/test_u5music.c (the C port) and the event streams and the two state
bytes after every call must agree.
"""
import argparse
import os
import random
import subprocess
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES,
                               UC_X86_REG_SS, UC_X86_REG_SP, UC_X86_REG_IP, UC_X86_REG_DX, UC_X86_REG_FLAGS)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DRV = os.path.join(ROOT, "u5upgrade", "mid.drv")

CS_DRV = 0x2000   # driver code segment; the data segment is 0x10 paragraphs below it (the driver does "mov ax,cs; sub ax,10h; mov ds,ax")
DS_GAME = 0x3000
SS = 0x5000
STUB = 0x6000     # holds a HLT: the far return address of every call

ENTRY_OFFSET = {"E0": 0x00, "E3": 0x03, "E6": 0x06, "E9": 0x09, "EC": 0x0C, "EF": 0x0F, "E12": 0x12, "E15": 0x15, "E18": 0x18, "E1B": 0x1B}

# game DS offsets the driver reads
MAP, TILE, LEVEL, D58A3 = 0x5893, 0x587C, 0x5895, 0x58A3


class Driver:
    def __init__(self):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_16)
        self.uc.mem_map(0x20000, 0x10000)
        self.uc.mem_map(0x30000, 0x10000)
        self.uc.mem_map(0x50000, 0x10000)
        self.uc.mem_map(0x60000, 0x1000)
        self.uc.mem_write(0x20000, open(DRV, "rb").read())
        self.uc.mem_write(0x60000, b"\xf4")  # hlt
        self.events = []
        self.status = 1
        self.uc.hook_add(UC_HOOK_INTR, self.on_int)
        # the driver's variables: current track ff, auto 0 (the file holds zeros at 1e/1f)
        self.uc.mem_write(0x2001E, bytes([0xFF, 0x00]))

    def on_int(self, uc, intno, user):
        if intno != 0x66:
            return
        ax = uc.reg_read(UC_X86_REG_AX)
        if ax == 0x070D:
            bx = uc.reg_read(UC_X86_REG_BX)
            cx = uc.reg_read(UC_X86_REG_CX)
            lin = (cx << 4) + bx
            name = bytearray()
            while True:
                c = uc.mem_read(lin + len(name), 1)[0]
                if c == 0:
                    break
                name.append(c)
            self.events.append("load " + name.decode())
        elif ax == 0x0702:
            self.events.append("start")
        elif ax == 0x0705:
            self.events.append("stop")
        elif ax == 0x070C:
            uc.reg_write(UC_X86_REG_AX, self.status)
        else:
            raise RuntimeError("unexpected INT 66h AX=%04x" % ax)

    def state(self):
        cur, auto = self.uc.mem_read(0x2001E, 2)
        return cur, auto

    def call(self, entry, bx=0):
        uc = self.uc
        uc.reg_write(UC_X86_REG_SS, SS)
        uc.reg_write(UC_X86_REG_SP, 0xFFF0)
        uc.reg_write(UC_X86_REG_DS, DS_GAME)   # the caller's DS; the driver switches to its own
        uc.reg_write(UC_X86_REG_ES, DS_GAME)
        # AL = ff: for a map above 28h the driver's auto-selection jumps past its own "mov al,0ff" and returns the caller's AL (a bug of the
        # upgrade); the port treats that case as "no music", so make the caller's AL agree
        uc.reg_write(UC_X86_REG_AX, 0x12FF)
        uc.reg_write(UC_X86_REG_BX, bx)
        uc.reg_write(UC_X86_REG_FLAGS, 0x202)
        # far call: push return CS:IP, jump
        sp = 0xFFF0 - 4
        uc.mem_write(SS * 16 + sp, (0).to_bytes(2, "little") + STUB.to_bytes(2, "little"))
        uc.reg_write(UC_X86_REG_SP, sp)
        uc.reg_write(UC_X86_REG_CS, CS_DRV)
        uc.emu_start(CS_DRV * 16 + ENTRY_OFFSET[entry], STUB * 16, timeout=0, count=100000)

    def set_game(self, mapid, tile, level, d58a3):
        for off, v in ((MAP, mapid), (TILE, tile), (LEVEL, level), (D58A3, d58a3)):
            self.uc.mem_write(DS_GAME * 16 + off, bytes([v & 0xFF]))


def jump_table_entries(drv):
    """Sanity check of the layout assumed above: ten 3-byte jmp instructions at the start of MID.DRV."""
    for i in range(10):
        assert drv[i * 3] == 0xE9, "entry %d is not a jmp" % i


def make_script(rng, length):
    ops = []
    maps = [0, 1, 2, 5, 8, 9, 0xC, 0xD, 0x10, 0x11, 0x12, 0x13, 0x18, 0x19, 0x1D, 0x1E, 0x20, 0x21, 0x28, 0x29, 0x40, 0x41, 0x7F, 0xFF]
    tiles = [0x1C, 0x20, 0x24, 0x27, 0x28, 0x10, 0x14, 0x00, 0xF8, 0x21]
    for _ in range(length):
        status = rng.choice([1, 1, 1, 0, 2])
        kind = rng.choice(["E0"] * 6 + ["E3", "E6", "E6", "E9", "EC", "EF", "EF", "E12", "E15", "E18", "E18", "E1B"])
        arg = None
        if kind == "E0":
            arg = (rng.choice(maps), rng.choice(tiles), rng.choice([0, 0, 1, 2, 0xFF]), rng.choice([0, 0, 1, 0x80]))
        elif kind in ("E6", "E18"):
            arg = (rng.choice([0, 1, 3, 4, 7, 8, 0xE, 0xF, 0x15, 0x16, 0x20, 0xFF, rng.randrange(256)]),)
        ops.append((status, kind, arg))
    return ops


def run_driver(ops):
    d = Driver()
    out = []
    for status, kind, arg in ops:
        d.status = status
        d.events = []
        if kind == "E0":
            d.set_game(*arg)
            d.call("E0", bx=DS_GAME)   # the game's wrapper passes its DS in BX
        elif kind in ("E6", "E18"):
            d.call(kind, bx=0x7700 | arg[0])  # only BL matters
        else:
            d.call(kind)
        out.extend(d.events)
        out.append("state %d %d" % d.state())
    return out


def run_port(ops, exe):
    lines = []
    for status, kind, arg in ops:
        lines.append("P %d" % status)
        lines.append(kind + ("" if arg is None else " " + " ".join(str(a) for a in arg)))
    r = subprocess.run([exe], input="\n".join(lines) + "\n", capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit("port harness failed: " + r.stderr)
    return r.stdout.splitlines()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=200)
    ap.add_argument("--length", type=int, default=60)
    ap.add_argument("--seed", type=int, default=1)
    args = ap.parse_args()

    if not os.path.exists(DRV):
        sys.exit("u5upgrade/mid.drv not found (the upgrade package is not part of the repository)")
    jump_table_entries(open(DRV, "rb").read())

    exe = os.path.join(ROOT, "build", "test_u5music.exe")
    subprocess.run(["gcc", "-std=gnu11", "-DU5MUSIC_TEST", "-I" + os.path.join(ROOT, "src"), "-o", exe, os.path.join(ROOT, "tests", "test_u5music.c"),
                    os.path.join(ROOT, "src", "audio", "u5music.c")], check=True)

    rng = random.Random(args.seed)
    calls = 0
    for run in range(args.runs):
        ops = make_script(rng, args.length)
        want = run_driver(ops)
        got = run_port(ops, exe)
        calls += len(ops)
        if want != got:
            for i, (a, b) in enumerate(zip(want, got)):
                if a != b:
                    print("run %d: first difference at output line %d: driver %r, port %r" % (run, i, a, b))
                    break
            else:
                print("run %d: output lengths differ (%d vs %d)" % (run, len(want), len(got)))
            sys.exit(1)
    print("MID.DRV and the port agree on %d runs (%d entry calls)" % (args.runs, calls))


main()
