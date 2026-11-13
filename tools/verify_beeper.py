#!/usr/bin/env python3
"""Check tools/beeper.py (BEEPER.BIN on tools/x86.py) against Unicorn running the same driver: for effects 1..0x45 (plus a stop) the
(divisor, speaker bits) after every tick must match.   python3 tools/verify_beeper.py [ticks]"""
import sys, os, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from unicorn import *
from unicorn import x86_const as X
import beeper

TICKS = int(sys.argv[1]) if len(sys.argv) > 1 else 150
BIN = open(beeper.ROOT + '/unpacked/cadaver1/BEEPER.BIN', 'rb').read()


class UcDrv:
    def __init__(self):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_16)
        self.uc.mem_map(0, 0x100000)
        self.uc.mem_write(0x30000, BIN)
        self.p61 = 0
        self.pit = []
        self.div = 0
        self.uc.hook_add(UC_HOOK_INSN, self.h_in, None, 1, 0, X.UC_X86_INS_IN)
        self.uc.hook_add(UC_HOOK_INSN, self.h_out, None, 1, 0, X.UC_X86_INS_OUT)
        self.far(4)

    def h_in(self, uc, port, size, user):
        return self.p61 if port == 0x61 else 0

    def h_out(self, uc, port, size, value, user):
        if port == 0x61:
            self.p61 = value & 0xFF
        elif port == 0x43:
            self.pit = []
        elif port == 0x42:
            self.pit.append(value & 0xFF)
            if len(self.pit) == 2:
                self.div = self.pit[0] | self.pit[1] << 8

    def far(self, off):
        uc = self.uc
        uc.reg_write(X.UC_X86_REG_CS, 0x3000)
        uc.reg_write(X.UC_X86_REG_SS, 0x200)
        uc.reg_write(X.UC_X86_REG_DS, 0x1F17)
        uc.reg_write(X.UC_X86_REG_SP, 0xF000 - 4)
        uc.mem_write(0x2000 + 0xF000 - 4, struct.pack('<HH', 0, 0x100))
        uc.emu_start(0x30000 + off, 0x1000, count=200000)

    def tick(self):
        self.far(0)
        return self.div if (self.p61 & 3) == 3 else 0

    def poke(self, off, v):
        self.uc.mem_write(0x30000 + off, bytes([v]))


bad = 0
for eff in range(1, 0x46):
    a = beeper.Beeper()
    u = UcDrv()
    a.poke(0xA, eff)
    u.poke(0xA, eff)
    for t in range(TICKS):
        if t == 40:                      # stop request for this effect
            a.poke(0xC, eff)
            u.poke(0xC, eff)
        x = a.tick()
        y = u.tick()
        xd = round(1193182 / x) if x else 0
        if xd != y:
            print('effect %#x tick %d: x86.py %d unicorn %d' % (eff, t, xd, y))
            bad += 1
            break
print('mismatches', bad)
sys.exit(1 if bad else 0)
