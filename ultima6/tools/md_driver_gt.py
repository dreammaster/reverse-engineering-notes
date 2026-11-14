"""Ground truth for src/engine/cpu8086.cpp: runs entry 0x15E9 of the SUPERTM.DRV (VGA) block (md/game/drv/SUPERTM_0.bin from tools/extract_md_drivers.py), the driver's rotate/scale draw, on the
Unicorn emulator (pip install unicorn). GT().shapes(item bytes of a *.LZC), .clear(), .draw(x, y, frame, angle, size) returns the 320 x 200 pixels; tests/test_md_driver.cpp holds checksums of such calls."""
import struct, sys, subprocess
from unicorn import *
from unicorn.x86_const import *

import os
BLOCK = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'md', 'game', 'drv', 'SUPERTM_0.bin'), 'rb').read()
CS, DSs, BM, SH, STUB, SS = 0x2000, 0x1000, 0x3000, 0x5000, 0x4000, 0x1000
def lin(seg, off): return seg * 16 + off

class GT:
    def __init__(self):
        self.mu = Uc(UC_ARCH_X86, UC_MODE_16)
        self.mu.mem_map(0, 0x100000)
        self.mu.mem_write(lin(CS, 0), BLOCK)
        self.mu.mem_write(lin(CS, 0xC), struct.pack('<HH', 0, STUB))
        # stub resolver: mov ax,0 ; mov dx,SH ; retf
        self.mu.mem_write(lin(STUB, 0), bytes([0xB8, 0, 0, 0xBA]) + struct.pack('<H', SH) + b'\xCB')
        # row table at ds:0x100 (200 rows of 320)
        self.mu.mem_write(lin(DSs, 0x100), struct.pack('<201H', *[(y * 320) & 0xFFFF for y in range(201)]))
        self.sent = lin(0x7000, 0)
        self.mu.mem_write(self.sent, b'\xF4')  # hlt
    def shapes(self, item_bytes):
        self.mu.mem_write(lin(SH, 0), struct.pack('<I', len(item_bytes) + 4) + item_bytes)
    def clear(self, v=0):
        self.mu.mem_write(lin(BM, 0), bytes([v]) * 64000)
    def draw(self, x, y, frame, angle, size, flags=0, clip=(0, 0, 319, 199)):
        mu = self.mu
        mu.mem_write(lin(CS, 0), b'\0\0')  # the reentrancy flag
        # descriptor at ds:0x200: seg, rowtable, left, top, right, bottom
        mu.mem_write(lin(DSs, 0x800), struct.pack('<6H', BM, 0x100, clip[0], clip[1], clip[2], clip[3]))
        sp = 0xF000
        args = [flags & 0xFFFF, size & 0xFFFF, angle & 0xFFFF, frame & 0xFFFF, SH, 0, y & 0xFFFF, x & 0xFFFF, 0x800]
        mu.reg_write(UC_X86_REG_SS, SS); mu.reg_write(UC_X86_REG_DS, DSs); mu.reg_write(UC_X86_REG_ES, DSs)
        for a in args:
            sp -= 2; mu.mem_write(lin(SS, sp), struct.pack('<H', a))
        sp -= 4; mu.mem_write(lin(SS, sp), struct.pack('<HH', 0, 0x7000))  # far return
        mu.reg_write(UC_X86_REG_SP, sp)
        mu.reg_write(UC_X86_REG_CS, CS)
        try:
            mu.emu_start(lin(CS, 0x15E9) - lin(CS, 0), lin(0x7000, 0) - lin(CS, 0) + 0x10000000, count=20000000) if False else None
        except Exception: pass
        mu.reg_write(UC_X86_REG_IP, 0x15E9)
        try:
            mu.emu_start(lin(CS, 0x15E9), self.sent, count=50000000)
        except UcError as e:
            print('emu error', e, 'ip', hex(mu.reg_read(UC_X86_REG_IP)), 'cs', hex(mu.reg_read(UC_X86_REG_CS)))
        return bytes(mu.mem_read(lin(BM, 0), 64000))

def lzc_shape_decode(d, x0, y0):
    """The plain draw (no rotation) done in python from the format: returns {(x,y): value}"""
    import struct
    a, hx, hy, dd = struct.unpack_from('<4H', d, 0)
    p = 8; out = {}
    while True:
        cx, = struct.unpack_from('<H', d, p); p += 2
        if cx == 0: break
        x, y = struct.unpack_from('<hh', d, p); p += 4
        n = cx >> 1; px = []
        if cx & 1 == 0:
            px = list(d[p:p + n]); p += n
        else:
            bx = n
            while bx:
                al = d[p]; p += 1; ln = al >> 1
                if al & 1:
                    v = d[p]; p += 1; px += [v] * ln
                else:
                    px += list(d[p:p + ln]); p += ln
                bx -= ln
        for i, v in enumerate(px): out[(x0 + x + i, y0 + y)] = v
    return out
