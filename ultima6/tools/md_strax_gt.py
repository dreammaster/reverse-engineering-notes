"""Ground truth for src/ultima6/md_adlib.cpp: runs entries of the AdLib block of STRAX.DRV (md/game/drv/STRAX_1.bin, tools/extract_md_drivers.py) with MD.TIM's AdLib timbres on Unicorn and prints the
writes to the chip. Usage: md_strax_gt.py; edit the scenario at the bottom."""
import os, struct
from unicorn import *
from unicorn.x86_const import *

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..', 'md')
BLOCK = open(os.path.join(ROOT, 'game', 'drv', 'STRAX_1.bin'), 'rb').read()
TIM = open(os.path.join(ROOT, 'game', 'MARTIAN', 'MD.TIM'), 'rb').read()
ents = [struct.unpack_from('<I', TIM, 4 + 4 * k)[0] for k in range(3)]
TIMBRES = TIM[ents[1] & 0xFFFFFF:ents[2] & 0xFFFFFF]
CS, TS, SS, RET = 0x2000, 0x4000, 0x1000, 0x9000
lin = lambda s, o: s * 16 + o


class Drv:
    def __init__(self):
        self.mu = Uc(UC_ARCH_X86, UC_MODE_16)
        self.mu.mem_map(0, 0x100000)
        self.mu.mem_write(lin(CS, 0), BLOCK)
        self.mu.mem_write(lin(TS, 0), TIMBRES)
        self.mu.mem_write(lin(RET, 0), b'\xf4')
        self.entry = [struct.unpack_from('<H', BLOCK, 2 + 2 * i)[0] for i in range(10)]
        self.log = []
        self.reg = 0
        self.pit = 0x1000
        self.high = False

        def hin(mu, port, size, ud):
            if port != 0x40:
                return 0
            v = (self.pit >> 8) if self.high else self.pit & 0xFF
            if self.high:
                self.pit -= 40
                if self.pit < 0:
                    self.pit += 0x4DAE
            self.high = not self.high
            return v & 0xFF

        def hout(mu, port, size, value, ud):
            if port == 0x388:
                self.reg = value & 0xFF
            elif port == 0x389:
                self.log.append((self.reg, value & 0xFF))
        self.mu.hook_add(UC_HOOK_INSN, hin, None, 1, 0, UC_X86_INS_IN)
        self.mu.hook_add(UC_HOOK_INSN, hout, None, 1, 0, UC_X86_INS_OUT)
        self.call(0, [])
        self.call(1, [TS, 0])
        self.call(8, [0x388])
        self.call(2, [])
        self.log = []

    def call(self, n, args, isr=False):
        mu = self.mu
        mu.reg_write(UC_X86_REG_SS, SS); mu.reg_write(UC_X86_REG_DS, CS); mu.reg_write(UC_X86_REG_ES, CS); mu.reg_write(UC_X86_REG_CS, CS)
        sp = 0xF000
        words = ([] if not isr else [0x0202]) + list(args)
        for a in words:
            sp -= 2; mu.mem_write(lin(SS, sp), struct.pack('<H', a & 0xFFFF))
        sp -= 4; mu.mem_write(lin(SS, sp), struct.pack('<HH', 0, RET))
        mu.reg_write(UC_X86_REG_SP, sp)
        mu.emu_start(lin(CS, self.entry[n]), lin(RET, 0), count=5000000)

    def take(self):
        l, self.log = self.log, []
        return l


if __name__ == '__main__':
    d = Drv()
    d.call(5, [2, 7, 100]); d.call(4, [2, 60, 100]); print('on', ' '.join('%02x=%02x' % w for w in d.take()))
    d.call(4, [2, 60, 0]); print('off', ' '.join('%02x=%02x' % w for w in d.take()))


def trace_note(path):
    d = Drv()
    d.call(5, [2, 7, 100])
    out = []
    mu = d.mu
    def hook(mu, addr, size, ud):
        r = lambda x: mu.reg_read(x)
        out.append('%04x %04x %04x %04x %04x %04x %04x %04x %04x' % (addr - CS * 16, r(UC_X86_REG_AX), r(UC_X86_REG_BX), r(UC_X86_REG_CX), r(UC_X86_REG_DX), r(UC_X86_REG_SI), r(UC_X86_REG_DI), r(UC_X86_REG_BP), r(UC_X86_REG_SP)))
    mu.hook_add(UC_HOOK_CODE, hook)
    d.call(4, [2, 60, 100])
    open(path, 'w').write('\n'.join(out) + '\n')
