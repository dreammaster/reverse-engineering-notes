#!/usr/bin/env python3
"""
A small 8086 interpreter (Python reference) for the native routines in ROUTINEn.TAD (docs/formats.md 6.7).  Only the instructions that occur in the
five TAD files (and the stub) are implemented; anything else raises X86Error.  Verified against Unicorn running the same code (tools/verify_x86.py).

Memory is a flat 1 MB byte array (real-mode segment:offset addressing, 20-bit wrap).  Far calls to given addresses can be hooked: `hook(cpu)`
is called instead of executing the target (like the game's service dispatcher 1000:6019) and must leave the registers/stack as the callee would.
"""
import struct


class X86Error(Exception):
    pass


M16 = 0xFFFF
CF, PF, AF, ZF, SF, TF, IF, DF, OF = 1, 4, 16, 64, 128, 256, 512, 1024, 2048
R16 = ('ax', 'cx', 'dx', 'bx', 'sp', 'bp', 'si', 'di')
SEGS = ('es', 'cs', 'ss', 'ds')


def parity(v):
    return bin(v & 0xFF).count('1') % 2 == 0


class CPU:
    def __init__(self, mem=None):
        self.mem = mem if mem is not None else bytearray(0x100000)
        self.r = dict.fromkeys(R16, 0)
        self.s = dict.fromkeys(SEGS, 0)
        self.ip = 0
        self.fl = 2
        self.hooks = {}                 # (cs, ip) -> callable(cpu) executed instead of the code there
        self.halted = False
        self.io_out = None              # callable(port, value, width)
        self.io_in = None               # callable(port, width) -> value
        self.steps = 0

    # -- memory -------------------------------------------------------------------------
    def rb(self, seg, off):
        return self.mem[((seg << 4) + (off & M16)) & 0xFFFFF]

    def wb(self, seg, off, v):
        self.mem[((seg << 4) + (off & M16)) & 0xFFFFF] = v & 0xFF

    def rw(self, seg, off):
        return self.rb(seg, off) | (self.rb(seg, off + 1) << 8)

    def ww(self, seg, off, v):
        self.wb(seg, off, v)
        self.wb(seg, off + 1, v >> 8)

    def fetch8(self):
        v = self.rb(self.s['cs'], self.ip)
        self.ip = (self.ip + 1) & M16
        return v

    def fetch16(self):
        v = self.fetch8()
        return v | (self.fetch8() << 8)

    def push(self, v):
        self.r['sp'] = (self.r['sp'] - 2) & M16
        self.ww(self.s['ss'], self.r['sp'], v)

    def pop(self):
        v = self.rw(self.s['ss'], self.r['sp'])
        self.r['sp'] = (self.r['sp'] + 2) & M16
        return v

    # -- registers ------------------------------------------------------------------------
    def reg(self, n, w):
        if w:
            return self.r[R16[n]]
        r = self.r[R16[n & 3]]
        return (r >> 8) & 0xFF if n & 4 else r & 0xFF

    def setreg(self, n, w, v):
        if w:
            self.r[R16[n]] = v & M16
        else:
            k = R16[n & 3]
            if n & 4:
                self.r[k] = (self.r[k] & 0x00FF) | ((v & 0xFF) << 8)
            else:
                self.r[k] = (self.r[k] & 0xFF00) | (v & 0xFF)

    # -- modrm -----------------------------------------------------------------------------
    def modrm(self, seg_override):
        m = self.fetch8()
        mod, reg, rm = m >> 6, (m >> 3) & 7, m & 7
        if mod == 3:
            return reg, ('r', rm)
        if mod == 0 and rm == 6:
            off = self.fetch16()
            base_seg = 'ds'
        else:
            r = self.r
            off = (r['bx'] + r['si'], r['bx'] + r['di'], r['bp'] + r['si'], r['bp'] + r['di'], r['si'], r['di'], r['bp'], r['bx'])[rm]
            base_seg = 'ss' if rm in (2, 3, 6) else 'ds'
            if mod == 1:
                d = self.fetch8()
                off += d - 256 if d >= 128 else d
            elif mod == 2:
                off += self.fetch16()
        off &= M16
        return reg, ('m', seg_override or base_seg, off)

    def rd(self, loc, w):
        if loc[0] == 'r':
            return self.reg(loc[1], w)
        seg = self.s[loc[1]]
        return self.rw(seg, loc[2]) if w else self.rb(seg, loc[2])

    def wr(self, loc, w, v):
        if loc[0] == 'r':
            self.setreg(loc[1], w, v)
        else:
            seg = self.s[loc[1]]
            if w:
                self.ww(seg, loc[2], v)
            else:
                self.wb(seg, loc[2], v)

    # -- flags ------------------------------------------------------------------------------
    def setf(self, mask, on):
        self.fl = (self.fl | mask) if on else (self.fl & ~mask)

    def szp(self, v, w):
        bits = 16 if w else 8
        v &= (1 << bits) - 1
        self.setf(ZF, v == 0)
        self.setf(SF, v >> (bits - 1))
        self.setf(PF, parity(v))

    def alu(self, op, a, b, w):
        bits = 16 if w else 8
        mask = (1 << bits) - 1
        top = 1 << (bits - 1)
        c = 1 if self.fl & CF else 0
        if op in (0, 2):                                  # add, adc
            cin = c if op == 2 else 0
            r = a + b + cin
            self.setf(CF, r > mask)
            self.setf(OF, ((a ^ r) & (b ^ r) & top) != 0)
            self.setf(AF, ((a ^ b ^ r) & 0x10) != 0)
        elif op in (5, 7, 3):                             # sub, cmp, sbb
            cin = c if op == 3 else 0
            r = a - b - cin
            self.setf(CF, r < 0)
            self.setf(OF, ((a ^ b) & (a ^ r) & top) != 0)
            self.setf(AF, ((a ^ b ^ r) & 0x10) != 0)
        else:                                             # or, and, xor
            r = a | b if op == 1 else a & b if op == 4 else a ^ b
            self.setf(CF, False)
            self.setf(OF, False)
        r &= mask
        self.szp(r, w)
        return r

    def cond(self, cc):
        f = self.fl
        zf, sf, cf, of, pf = bool(f & ZF), bool(f & SF), bool(f & CF), bool(f & OF), bool(f & PF)
        t = (of, cf, zf, cf or zf, sf, pf, sf != of, zf or (sf != of))[cc >> 1]
        return (not t) if cc & 1 else t

    # -- execution ---------------------------------------------------------------------------
    def step(self):
        key = (self.s['cs'], self.ip)
        if key in self.hooks:
            self.hooks[key](self)
            return
        self.steps += 1
        seg = None
        rep = None
        op = self.fetch8()
        while True:
            if op in (0x26, 0x2E, 0x36, 0x3E):
                seg = {0x26: 'es', 0x2E: 'cs', 0x36: 'ss', 0x3E: 'ds'}[op]
            elif op in (0xF2, 0xF3):
                rep = op
            else:
                break
            op = self.fetch8()
        if op < 0x40 and (op & 7) < 6:                    # ALU group
            kind, form = op >> 3, op & 7
            if form < 4:
                w = form & 1
                reg, loc = self.modrm(seg)
                if form < 2:
                    a, b = self.rd(loc, w), self.reg(reg, w)
                    r = self.alu(kind, a, b, w)
                    if kind != 7:
                        self.wr(loc, w, r)
                else:
                    a, b = self.reg(reg, w), self.rd(loc, w)
                    r = self.alu(kind, a, b, w)
                    if kind != 7:
                        self.setreg(reg, w, r)
            else:
                w = form & 1
                imm = self.fetch16() if w else self.fetch8()
                r = self.alu(kind, self.reg(0, w), imm, w)
                if kind != 7:
                    self.setreg(0, w, r)
        elif 0x40 <= op < 0x50:                           # inc / dec r16
            n = op & 7
            a = self.r[R16[n]]
            c = self.fl & CF
            r = self.alu(0 if op < 0x48 else 5, a, 1, 1)
            self.fl = (self.fl & ~CF) | c
            self.r[R16[n]] = r
        elif op in (0x06, 0x0E, 0x16, 0x1E):              # push es / cs / ss / ds
            self.push(self.s[SEGS[(op >> 3) & 3]])
        elif op in (0x07, 0x17, 0x1F):                    # pop es / ss / ds
            self.s[SEGS[(op >> 3) & 3]] = self.pop()
        elif op in (0xE4, 0xE5, 0xEC, 0xED):              # in al/ax, port
            w = op & 1
            port = self.fetch8() if op < 0xE8 else self.r['dx']
            self.setreg(0, w, self.io_in(port, w) if self.io_in else 0)
        elif op in (0xE6, 0xE7, 0xEE, 0xEF):              # out port, al/ax
            w = op & 1
            port = self.fetch8() if op < 0xE8 else self.r['dx']
            if self.io_out:
                self.io_out(port, self.reg(0, w), w)
        elif 0x50 <= op < 0x58:
            self.push(self.r[R16[op & 7]] if op & 7 != 4 else (self.r['sp'] - 2) & M16)
        elif 0x58 <= op < 0x60:
            self.r[R16[op & 7]] = self.pop()
        elif 0x70 <= op < 0x80:
            d = self.fetch8()
            if self.cond(op & 15):
                self.ip = (self.ip + (d - 256 if d >= 128 else d)) & M16
        elif op in (0x80, 0x81, 0x83):
            w = op & 1
            reg, loc = self.modrm(seg)
            if op == 0x81:
                imm = self.fetch16()
            else:
                imm = self.fetch8()
                if op == 0x83 and imm >= 128:
                    imm |= 0xFF00
            a = self.rd(loc, w)
            r = self.alu(reg, a, imm & (M16 if w else 0xFF), w)
            if reg != 7:
                self.wr(loc, w, r)
        elif op in (0x84, 0x85):
            w = op & 1
            reg, loc = self.modrm(seg)
            self.alu(4, self.rd(loc, w), self.reg(reg, w), w)
        elif op in (0x86, 0x87):
            w = op & 1
            reg, loc = self.modrm(seg)
            a, b = self.rd(loc, w), self.reg(reg, w)
            self.wr(loc, w, b)
            self.setreg(reg, w, a)
        elif op in (0x88, 0x89):
            w = op & 1
            reg, loc = self.modrm(seg)
            self.wr(loc, w, self.reg(reg, w))
        elif op in (0x8A, 0x8B):
            w = op & 1
            reg, loc = self.modrm(seg)
            self.setreg(reg, w, self.rd(loc, w))
        elif op == 0x8C:
            reg, loc = self.modrm(seg)
            self.wr(loc, 1, self.s[SEGS[reg & 3]])
        elif op == 0x8E:
            reg, loc = self.modrm(seg)
            self.s[SEGS[reg & 3]] = self.rd(loc, 1)
        elif op == 0x8D:
            reg, loc = self.modrm(seg)
            if loc[0] != 'm':
                raise X86Error('lea reg')
            self.setreg(reg, 1, loc[2])
        elif op == 0x90:
            pass
        elif 0x91 <= op < 0x98:
            n = op & 7
            self.r['ax'], self.r[R16[n]] = self.r[R16[n]], self.r['ax']
        elif op == 0x98:
            self.r['ax'] = (self.r['ax'] & 0xFF) | (0xFF00 if self.r['ax'] & 0x80 else 0)
        elif op == 0x99:
            self.r['dx'] = 0xFFFF if self.r['ax'] & 0x8000 else 0
        elif op == 0x9C:
            self.push(self.fl | 2)
        elif op == 0x9D:
            self.fl = (self.pop() & 0x0FD5) | 2
        elif op == 0x9A:
            o = self.fetch16()
            s = self.fetch16()
            self.far_call(s, o)
        elif op in (0xA0, 0xA1, 0xA2, 0xA3):
            w = op & 1
            off = self.fetch16()
            sg = self.s[seg or 'ds']
            if op < 0xA2:
                self.setreg(0, w, self.rw(sg, off) if w else self.rb(sg, off))
            elif w:
                self.ww(sg, off, self.r['ax'])
            else:
                self.wb(sg, off, self.r['ax'])
        elif op in (0xA4, 0xA5, 0xAA, 0xAB, 0xAC, 0xAD):
            self.string(op, seg, rep)
        elif op in (0xA8, 0xA9):
            w = op & 1
            imm = self.fetch16() if w else self.fetch8()
            self.alu(4, self.reg(0, w), imm, w)
        elif 0xB0 <= op < 0xB8:
            self.setreg(op & 7, 0, self.fetch8())
        elif 0xB8 <= op < 0xC0:
            self.setreg(op & 7, 1, self.fetch16())
        elif op in (0xC0, 0xC1, 0xD0, 0xD1, 0xD2, 0xD3):
            w = op & 1
            reg, loc = self.modrm(seg)
            cnt = self.fetch8() if op < 0xC2 else (1 if op < 0xD2 else self.r['cx'] & 0xFF)
            self.shift(reg, loc, w, cnt & 31)
        elif op == 0xC3:
            self.ip = self.pop()
        elif op == 0xC2:
            n = self.fetch16()
            self.ip = self.pop()
            self.r['sp'] = (self.r['sp'] + n) & M16
        elif op == 0xCB:
            self.ip = self.pop()
            self.s['cs'] = self.pop()
        elif op in (0xC6, 0xC7):
            w = op & 1
            reg, loc = self.modrm(seg)
            self.wr(loc, w, self.fetch16() if w else self.fetch8())
        elif op in (0xF6, 0xF7):
            w = op & 1
            reg, loc = self.modrm(seg)
            self.group3(reg, loc, w)
        elif op in (0xFE, 0xFF):
            w = op & 1
            reg, loc = self.modrm(seg)
            self.group5(reg, loc, w)
        elif op == 0xE2:
            d = self.fetch8()
            self.r['cx'] = (self.r['cx'] - 1) & M16
            if self.r['cx']:
                self.ip = (self.ip + (d - 256 if d >= 128 else d)) & M16
        elif op == 0xE3:
            d = self.fetch8()
            if not self.r['cx']:
                self.ip = (self.ip + (d - 256 if d >= 128 else d)) & M16
        elif op == 0xE8:
            d = self.fetch16()
            self.push(self.ip)
            self.ip = (self.ip + d) & M16
        elif op == 0xE9:
            d = self.fetch16()
            self.ip = (self.ip + d) & M16
        elif op == 0xEB:
            d = self.fetch8()
            self.ip = (self.ip + (d - 256 if d >= 128 else d)) & M16
        elif op == 0xF8:
            self.setf(CF, False)
        elif op == 0xF9:
            self.setf(CF, True)
        elif op == 0xFA:
            self.setf(IF, False)
        elif op == 0xFB:
            self.setf(IF, True)
        elif op == 0xFC:
            self.setf(DF, False)
        elif op == 0xFD:
            self.setf(DF, True)
        elif op == 0xD7:
            self.setreg(0, 0, self.rb(self.s[seg or 'ds'], self.r['bx'] + (self.r['ax'] & 0xFF)))
        else:
            raise X86Error('opcode %02x at %04x:%04x' % (op, self.s['cs'], (self.ip - 1) & M16))

    def far_call(self, s, o):
        self.push(self.s['cs'])
        self.push(self.ip)
        self.s['cs'], self.ip = s, o

    def string(self, op, seg, rep):
        w = op & 1
        step = (-2 if w else -1) if self.fl & DF else (2 if w else 1)
        n = self.r['cx'] if rep else 1
        while n:
            if op in (0xA4, 0xA5):
                v = self.rw(self.s[seg or 'ds'], self.r['si']) if w else self.rb(self.s[seg or 'ds'], self.r['si'])
                (self.ww if w else self.wb)(self.s['es'], self.r['di'], v)
                self.r['si'] = (self.r['si'] + step) & M16
                self.r['di'] = (self.r['di'] + step) & M16
            elif op in (0xAA, 0xAB):
                (self.ww if w else self.wb)(self.s['es'], self.r['di'], self.r['ax'] if w else self.r['ax'] & 0xFF)
                self.r['di'] = (self.r['di'] + step) & M16
            else:
                v = self.rw(self.s[seg or 'ds'], self.r['si']) if w else self.rb(self.s[seg or 'ds'], self.r['si'])
                self.setreg(0, w, v)
                self.r['si'] = (self.r['si'] + step) & M16
            n -= 1
            if rep:
                self.r['cx'] = (self.r['cx'] - 1) & M16
        return

    def shift(self, kind, loc, w, cnt):
        bits = 16 if w else 8
        mask = (1 << bits) - 1
        v = self.rd(loc, w)
        for _ in range(cnt):
            if kind == 4:                                 # shl
                self.setf(CF, v >> (bits - 1))
                v = (v << 1) & mask
            elif kind == 5:                               # shr
                self.setf(CF, v & 1)
                v >>= 1
            elif kind == 7:                               # sar
                self.setf(CF, v & 1)
                v = (v >> 1) | (v & (1 << (bits - 1)))
            elif kind == 0:                               # rol
                c = v >> (bits - 1)
                v = ((v << 1) | c) & mask
                self.setf(CF, c)
            elif kind == 1:                               # ror
                c = v & 1
                v = (v >> 1) | (c << (bits - 1))
                self.setf(CF, c)
            else:
                raise X86Error('shift %d' % kind)
        if cnt and kind in (4, 5, 7):
            self.szp(v, w)
            self.setf(OF, False)
        self.wr(loc, w, v)

    def group3(self, reg, loc, w):
        bits = 16 if w else 8
        mask = (1 << bits) - 1
        if reg in (0, 1):
            self.alu(4, self.rd(loc, w), self.fetch16() if w else self.fetch8(), w)
        elif reg == 2:
            self.wr(loc, w, ~self.rd(loc, w) & mask)
        elif reg == 3:
            v = self.rd(loc, w)
            r = self.alu(5, 0, v, w)
            self.setf(CF, v != 0)
            self.wr(loc, w, r)
        elif reg == 4:                                    # mul
            v = self.rd(loc, w)
            if w:
                p = self.r['ax'] * v
                self.r['ax'], self.r['dx'] = p & M16, (p >> 16) & M16
                self.setf(CF | OF, (p >> 16) != 0)
            else:
                p = (self.r['ax'] & 0xFF) * v
                self.r['ax'] = p & M16
                self.setf(CF | OF, (p >> 8) != 0)
        elif reg == 6:                                    # div
            v = self.rd(loc, w)
            if v == 0:
                raise X86Error('divide by zero')
            if w:
                n = (self.r['dx'] << 16) | self.r['ax']
                q, r = divmod(n, v)
                if q > M16:
                    raise X86Error('div overflow')
                self.r['ax'], self.r['dx'] = q, r
            else:
                q, r = divmod(self.r['ax'], v)
                if q > 0xFF:
                    raise X86Error('div overflow')
                self.r['ax'] = q | (r << 8)
        else:
            raise X86Error('group3 /%d' % reg)

    def group5(self, reg, loc, w):
        if reg in (0, 1):
            c = self.fl & CF
            v = self.alu(0 if reg == 0 else 5, self.rd(loc, w), 1, w)
            self.fl = (self.fl & ~CF) | c
            self.wr(loc, w, v)
        elif reg == 2 and w:
            t = self.rd(loc, 1)
            self.push(self.ip)
            self.ip = t
        elif reg == 3 and w and loc[0] == 'm':
            sg = self.s[loc[1]]
            o, s = self.rw(sg, loc[2]), self.rw(sg, loc[2] + 2)
            self.far_call(s, o)
        elif reg == 4 and w:
            self.ip = self.rd(loc, 1)
        elif reg == 6 and w:
            self.push(self.rd(loc, 1))
        else:
            raise X86Error('group5 /%d' % reg)

    def run(self, max_steps=200000, stop=None):
        """Run until the code returns to `stop` = (cs, ip) or max_steps."""
        n = 0
        while n < max_steps:
            if stop is not None and (self.s['cs'], self.ip) == stop:
                return n
            self.step()
            n += 1
        raise X86Error('step limit')
