#!/usr/bin/env python3
"""
A UCSD p-machine for the DOS Wizardry SYSTEM.PASCAL, with an emulation of the
CONUNIT (unit 13) console/graphics driver that lives in SYSTEM.INTERP.

Purpose: an *oracle*.  The engine's UI is a port of the game's own drawing
code; this runs the real p-code against a faithful port of the native driver
so every screen can be captured exactly as the original draws it (no
screenshots, no guessing), then compared with the C++ port.

Semantics are copied from the x86 handlers in SYSTEM.INTERP (wiz1_interp.asm),
see docs/pmachine.md + docs/drawing.md.  Only the opcodes / CSPs the game
actually uses are implemented (checked against extracted/pcode/*.pd).

    python tools/wizvm.py run  [--keys "ABC\\r"] [--shot out.png] [--steps N]
"""
import os, sys, struct, zlib

sys.path.insert(0, os.path.dirname(__file__))
import pcode_dis as pd
from conunit import Conunit, NeedInput, write_png

ROOT = os.path.join(os.path.dirname(__file__), '..')
PASCAL = os.path.join(ROOT, 'extracted', 'wiz1', 'SYSTEM.PASCAL')
DISK = r'C:\games\wizard15\WIZ1.DSK'

M16 = 0xFFFF


def s16(v):
    v &= M16
    return v - 0x10000 if v & 0x8000 else v


class Trap(Exception):
    pass


# --------------------------------------------------------------------------
class Proc:
    __slots__ = ('seg', 'num', 'anchor', 'entry', 'dsize', 'psize', 'level',
                 'exit_ip')


class Frame:
    __slots__ = ('proc', 'base', 'static', 'dyn', 'ret_ip', 'ret_seg',
                 'sp', 'np_save')


class Volume:
    """a UCSD volume image, read/write in memory"""

    def __init__(self, path):
        self.data = bytearray(open(path, 'rb').read())

    def read(self, blk, n):
        a = blk * 512
        return bytes(self.data[a:a + n]).ljust(n, b'\0')

    def write(self, blk, buf):
        a = blk * 512
        if a + len(buf) > len(self.data):
            self.data.extend(b'\0' * (a + len(buf) - len(self.data)))
        self.data[a:a + len(buf)] = buf

    def hide(self, name):
        """make a directory entry invisible to FINDFILE (kind := 0) -- in the
        in-memory copy only"""
        for i in range(77):
            o = 2 * 512 + 26 * (1 + i)
            ln = self.data[o + 6]
            if self.data[o + 7:o + 7 + ln].decode('latin1') == name:
                self.data[o + 4:o + 6] = bytes(2)

    def find(self, name):
        for i in range(77):
            o = 2 * 512 + 26 * (1 + i)
            e = self.data[o:o + 26]
            if len(e) < 26:
                break
            first, last = struct.unpack_from('<HH', e, 0)
            ln = e[6]
            if e[7:7 + ln].decode('latin1') == name:
                return first, last
        return None


# --------------------------------------------------------------------------
class VM:
    NP_TOP = 0xF000          # frames grow down from here
    HEAP0 = 0x6000
    LIT0, LIT1 = 0x0800, 0x6000      # literal (LSA) pool

    def __init__(self, pascal=PASCAL, disk=DISK, keys=b''):
        self.mem = bytearray(0x10000)
        self.segs = {}
        for s in pd.load_segments(pascal):
            self.segs[s.segnum] = s
        self.procs = {}
        for s in self.segs.values():
            for p in s.procs:
                q = Proc()
                q.seg, q.num, q.anchor = s.segnum, p.num, p.anchor
                q.entry, q.dsize, q.psize = p.entry, p.dsize, p.psize
                q.level = s.data[p.anchor + 1] if p.anchor + 1 < len(s.data) else 0
                q.exit_ip = (p.anchor - 4) - pd.rd16(s.data, p.anchor - 4)
                self.procs[(s.segnum, p.num)] = q
        self.vol = Volume(disk)
        self.vol.hide('HAS.CACHE')      # no RAM page-cache emulation (pure speed-up in the original)
        self.keys = bytearray(keys)
        self.shotdir = None
        self.shotn = 0
        self.hp = self.HEAP0
        self.np = self.NP_TOP
        self.lit_next = self.LIT0
        self.lit = {}
        self.ioresult = 0
        self.frame = None
        self.gframe = None
        self.st = []
        self.seg = None
        self.ip = 0
        self.steps = 0
        self.con = Conunit(self)
        self.trace = False
        import collections
        self.hist = collections.deque(maxlen=60)

    # -- memory -----------------------------------------------------------
    def r16(self, a):
        a &= M16
        return self.mem[a] | (self.mem[a + 1] << 8)

    def w16(self, a, v):
        a &= M16
        self.mem[a] = v & 0xFF
        self.mem[(a + 1) & M16] = (v >> 8) & 0xFF

    def pstr(self, a):
        n = self.mem[a]
        return bytes(self.mem[a + 1:a + 1 + n]).decode('latin1')

    # -- frames -----------------------------------------------------------
    def enter(self, key, static_mode):
        """build the activation record for proc `key`; params come off the
        eval stack (top = word 1).  static_mode: 'local' | 'search' | 'global'
        | 'root'"""
        p = self.procs[key]
        nwords = (p.psize + p.dsize) // 2
        self.np -= nwords * 2
        if self.np < self.hp + 64:
            raise Trap('Stack overflow!!')
        f = Frame()
        f.proc, f.base = p, self.np
        f.dyn = self.frame
        f.ret_ip, f.ret_seg = self.ip, self.seg
        for i in range(p.psize // 2):
            self.w16(f.base + 2 * i, self.st.pop())
        for i in range(p.psize // 2, nwords):
            self.w16(f.base + 2 * i, 0)
        f.sp = len(self.st)
        f.np_save = self.np + nwords * 2
        if static_mode == 'local':
            f.static = self.frame
        elif static_mode == 'global':
            f.static = self.gframe
        elif static_mode == 'root':
            f.static = None
        else:                            # search the dynamic chain for level-1
            want = p.level - 1
            c = self.frame
            while c is not None and c.proc.level != want:
                c = c.dyn
            f.static = c
        self.frame = f
        self.seg = self.segs[p.seg]
        self.ip = p.entry

    def leave(self, nret):
        f = self.frame
        vals = [self.r16(f.base + 2 * i) for i in range(nret)]
        del self.st[f.sp:]
        for v in reversed(vals):
            self.st.append(v)
        self.np = f.np_save
        self.frame = f.dyn
        self.ip = f.ret_ip
        self.seg = f.ret_seg

    def walk(self, n):
        f = self.frame
        for _ in range(n):
            f = f.static
        return f

    # -- literals -----------------------------------------------------------
    def literal(self, kind, data_off, ln):
        key = (self.seg.segnum, data_off)
        a = self.lit.get(key)
        if a is None:
            a = self.lit_next
            blob = bytes(self.seg.data[data_off:data_off + ln])
            self.mem[a:a + len(blob)] = blob
            self.lit_next += (len(blob) + 2) & ~1
            if self.lit_next > self.LIT1:
                raise Trap('literal pool full')
            self.lit[key] = a
        return a

    # -- the interpreter ----------------------------------------------------
    def start(self):
        self.st = [0, 0]
        p = self.procs[(1, 1)]
        self.ip = 0
        self.seg = self.segs[1]
        self.enter((1, 1), 'root')
        self.gframe = self.frame
        self.w16(self.frame.base, 0x0700)        # &syscom-ish (ioresult word)
        self.w16(self.frame.base + 2, 0)

    def big(self):
        d = self.seg.data
        b = d[self.ip]
        if b < 0x80:
            self.ip += 1
            return b
        v = ((b & 0x7F) << 8) | d[self.ip + 1]
        self.ip += 2
        return v

    def run(self, max_steps=50_000_000):
        st = self.st
        mem = self.mem
        r16, w16 = self.r16, self.w16
        n = 0
        while n < max_steps:
            n += 1
            d = self.seg.data
            ip = self.ip
            op = d[ip]
            self.ip = ip + 1
            self.hist.append((self.seg.name, self.frame.proc.num, ip, op, tuple(st[-3:])))
            if self.trace:
                print(f'{self.seg.name}:{self.frame.proc.num} {ip:04x} {op:02x} {st[-4:]}')
            if op < 0x80:
                st.append(op)
            elif 0xD8 <= op <= 0xE7:
                st.append(r16(self.frame.base + 2 * (op - 0xD8)))
            elif 0xE8 <= op <= 0xF7:
                st.append(r16(self.gframe.base + 2 * (op - 0xE8)))
            elif op >= 0xF8:
                st.append(r16(st.pop() + 2 * (op - 0xF8)))
            else:
                self.steps += n
                if self.op(op):
                    return
                n = 0 if False else n
                st = self.st
        raise Trap('step limit')

    def op(self, op):
        st = self.st
        r16, w16 = self.r16, self.w16
        if op == 0x82:   b = st.pop(); st.append((st.pop() + b) & M16)
        elif op == 0x95: b = st.pop(); st.append((st.pop() - b) & M16)
        elif op == 0x8F: b = st.pop(); st.append((s16(st.pop()) * s16(b)) & M16)
        elif op == 0x86:
            b = s16(st.pop()); a = s16(st.pop())
            if b == 0: raise Trap('divide by zero')
            q = abs(a) // abs(b)
            st.append((-q if (a < 0) != (b < 0) else q) & M16)
        elif op == 0x8E:
            b = s16(st.pop()); a = s16(st.pop())
            if b == 0: raise Trap('divide by zero')
            q = abs(a) // abs(b)
            q = -q if (a < 0) != (b < 0) else q
            st.append((a - q * b) & M16)
        elif op == 0x91: st.append((-s16(st.pop())) & M16)
        elif op == 0x80: st.append(abs(s16(st.pop())) & M16)
        elif op == 0x84: b = st.pop(); st.append(st.pop() & b)
        elif op == 0x8D: b = st.pop(); st.append(st.pop() | b)
        elif op == 0x93: st.append((~st.pop()) & M16)
        elif op == 0xC3: b = st.pop(); st.append(1 if st.pop() == b else 0)
        elif op == 0xCB: b = st.pop(); st.append(1 if st.pop() != b else 0)
        elif op == 0xC8: b = s16(st.pop()); st.append(1 if s16(st.pop()) <= b else 0)
        elif op == 0xC9: b = s16(st.pop()); st.append(1 if s16(st.pop()) < b else 0)
        elif op == 0xC4: b = s16(st.pop()); st.append(1 if s16(st.pop()) >= b else 0)
        elif op == 0xC5: b = s16(st.pop()); st.append(1 if s16(st.pop()) > b else 0)
        elif op == 0x9F: st.append(0)                                     # LDCN
        elif op == 0xC7:                                                  # LDCI
            st.append(pd.rd16(self.seg.data, self.ip)); self.ip += 2
        elif op == 0xCA: st.append(r16(self.frame.base + 2 * (self.big() - 1)))
        elif op == 0xC6: st.append(self.frame.base + 2 * (self.big() - 1))
        elif op == 0xCC: w16(self.frame.base + 2 * (self.big() - 1), st.pop())
        elif op == 0xA9: st.append(r16(self.gframe.base + 2 * (self.big() - 1)))
        elif op == 0xA5: st.append(self.gframe.base + 2 * (self.big() - 1))
        elif op == 0xAB: w16(self.gframe.base + 2 * (self.big() - 1), st.pop())
        elif op in (0xB6, 0xB2, 0xB8):
            ll = self.seg.data[self.ip]; self.ip += 1
            f = self.walk(ll)
            a = f.base + 2 * (self.big() - 1)
            if op == 0xB6: st.append(r16(a))
            elif op == 0xB2: st.append(a)
            else: w16(a, st.pop())
        elif op == 0xA2: k = self.big(); st.append((st.pop() + 2 * k) & M16)   # INC
        elif op == 0xA3: k = self.big(); st.append(r16(st.pop() + 2 * k))      # IND
        elif op == 0x9A: v = st.pop(); w16(st.pop(), v)                        # STO
        elif op == 0xA4:                                                       # IXA
            k = self.big(); i = s16(st.pop()); b = st.pop()
            st.append((b + 2 * k * i) & M16)
        elif op == 0xA8:                                                       # MOV
            k = self.big(); src = st.pop(); dst = st.pop()
            self.mem[dst:dst + 2 * k] = self.mem[src:src + 2 * k]
        elif op == 0xBE: i = st.pop(); b = st.pop(); st.append(self.mem[(b + i) & M16])
        elif op == 0xBF: v = st.pop(); i = st.pop(); b = st.pop(); self.mem[(b + i) & M16] = v & 0xFF
        elif op == 0xBA:                                                       # LDP
            sh = st.pop(); w = st.pop(); a = st.pop()
            st.append((r16(a) >> sh) & ((1 << w) - 1))
        elif op == 0xBB:                                                       # STP
            v = st.pop(); sh = st.pop(); w = st.pop(); a = st.pop()
            m = ((1 << w) - 1) << sh
            w16(a, (r16(a) & ~m) | ((v << sh) & m))
        elif op == 0xC0:                                                       # IXP
            n = self.seg.data[self.ip]; w = self.seg.data[self.ip + 1]; self.ip += 2
            i = st.pop(); a = st.pop()
            st.append((a + 2 * (i // n)) & M16); st.append(w); st.append((i % n) * w)
        elif op == 0xA6:                                                       # LSA
            ln = self.seg.data[self.ip]
            a = self.literal('s', self.ip, ln + 1)
            self.ip += 1 + ln
            st.append(a)
        elif op == 0xAA:                                                       # SAS
            mx = self.seg.data[self.ip]; self.ip += 1
            src = st.pop(); dst = st.pop()
            if src < 256:
                if mx < 1: raise Trap('SAS range')
                self.mem[dst] = 1; self.mem[dst + 1] = src & 0xFF
            else:
                ln = self.mem[src]
                if mx < ln: raise Trap('SAS range')
                self.mem[dst:dst + ln + 1] = self.mem[src:src + ln + 1]
        elif op == 0x9B:                                                       # IXS
            i = st.pop(); a = st.pop()
            if i == 0 or i > 255 or i > self.mem[a]: raise Trap('Range Error (IXS)')
            st.append(a); st.append(i)
        elif op in (0xAF, 0xB7, 0xB4, 0xB5, 0xB0, 0xB1):
            self.compare(op)
        elif op == 0xA1:                                                       # FJP
            c = st.pop()
            self.jump(c & 1 == 0)
        elif op == 0xB9: self.jump(True)                                       # UJP
        elif op == 0xAC: self.xjp()
        elif op == 0xCD:                                                       # CXP
            sn = self.seg.data[self.ip]; pn = self.seg.data[self.ip + 1]; self.ip += 2
            if sn == 0: sn = self.seg.segnum
            self.enter((sn, pn), 'search')
        elif op == 0xCE:
            pn = self.seg.data[self.ip]; self.ip += 1
            self.enter((self.seg.segnum, pn), 'local')
        elif op == 0xAE:
            pn = self.seg.data[self.ip]; self.ip += 1
            self.enter((self.seg.segnum, pn), 'search')
        elif op == 0xCF:
            pn = self.seg.data[self.ip]; self.ip += 1
            self.enter((self.seg.segnum, pn), 'global')
        elif op == 0xAD:                                                       # RNP
            n = self.seg.data[self.ip]; self.ip += 1
            self.leave(n)
        elif op == 0xC1: raise Trap('RBP')
        elif op == 0x9E: return self.csp()
        elif op == 0xD2 or op == 0xD7: pass
        elif op == 0xD6: raise Trap('HLT')
        else:
            raise Trap(f'unimplemented opcode {op:02x} at {self.seg.name}:{self.frame.proc.num} {self.ip-1:04x}')
        return False

    def jump(self, take):
        d = self.seg.data
        db = d[self.ip]; self.ip += 1
        if not take:
            return
        if db < 0x80:
            self.ip += db
        else:
            slot = self.frame.proc.anchor + (db - 256)
            self.ip = slot - pd.rd16(d, slot)

    def xjp(self):
        d = self.seg.data
        q = self.ip
        if q & 1: q += 1
        lo = pd.rd16s(d, q); hi = pd.rd16s(d, q + 2)
        x = s16(self.st.pop())
        tbl = q + 6                      # q+4: out-of-range `UJP` (2 bytes)
        if x < lo or x > hi:
            self.ip = q + 4
        else:
            slot = tbl + 2 * (x - lo)
            self.ip = slot - pd.rd16(d, slot)

    def compare(self, op):
        st = self.st
        want = {0xAF: 1, 0xB7: 6, 0xB4: 3, 0xB5: 2, 0xB0: 5, 0xB1: 4}[op]
        ty = self.seg.data[self.ip]; self.ip += 1
        if ty == 4:                                  # string
            b = st.pop(); a = st.pop()
            def sv(v):
                return bytes([v & 0xFF]) if v < 256 else bytes(self.mem[v + 1:v + 1 + self.mem[v]])
            x, y = sv(a), sv(b)
            c = (x > y) - (x < y)
        elif ty == 6:                                # boolean
            b = st.pop() & 1; a = st.pop() & 1
            c = (a > b) - (a < b)
        elif ty in (10, 12):                         # byte / word array
            n = self.big()
            b = st.pop(); a = st.pop()
            ln = n if ty == 12 else n * 2
            ln = n * 2 if ty == 12 else n
            x = bytes(self.mem[a:a + ln]); y = bytes(self.mem[b:b + ln])
            c = (x > y) - (x < y)
        else:
            raise Trap(f'compare type {ty}')
        res = {1: c == 0, 6: c != 0, 3: c <= 0, 2: c < 0, 5: c >= 0, 4: c > 0}[want]
        st.append(1 if res else 0)

    # -- standard procedures -------------------------------------------------
    def csp(self):
        st = self.st
        n = self.seg.data[self.ip]; self.ip += 1
        if n == 1:                                               # NEW
            size = st.pop(); ptr = st.pop()
            self.w16(ptr, self.hp)
            self.hp += size * 2
            if self.hp > self.np - 64: raise Trap('Stack overflow!! (NEW)')
        elif n in (2, 3):                                        # MOVELEFT / RIGHT
            cnt = st.pop(); do = st.pop(); dp = st.pop(); so = st.pop(); sp = st.pop()
            s = sp + so; dd = dp + do
            blob = bytes(self.mem[s:s + cnt]) if n == 2 else bytes(self.mem[s:s + cnt])
            self.mem[dd:dd + cnt] = blob
        elif n == 10:                                            # FILLCHAR
            ch = st.pop(); cnt = s16(st.pop()); off = st.pop(); p = st.pop()
            if cnt > 0:
                self.mem[p + off:p + off + cnt] = bytes([ch & 0xFF]) * cnt
        elif n == 32: self.w16(st.pop(), self.hp)                # MARK
        elif n == 33: self.hp = self.r16(st.pop())               # RELEASE
        elif n == 34: st.append(self.ioresult)                   # IORESULT
        elif n == 38: st.pop()                                   # flush keybuf (n=2)
        elif n == 40: st.append(((self.np - self.hp) // 2) & M16)  # MEMAVAIL
        elif n == 4:                                             # EXIT(seg, proc)
            pn = st.pop(); sn = st.pop()
            f = self.frame
            while f is not None and not (f.proc.num == pn and f.proc.seg == sn):
                f = f.dyn
            if f is None: raise Trap(f'EXIT: no frame for {sn}:{pn}')
            while self.frame is not f:                           # unwind to it
                g = self.frame
                self.np = g.np_save
                self.frame = g.dyn
                self.seg = g.ret_seg
            self.ip = f.proc.exit_ip
        elif n == 5:  self.unit_io(False)
        elif n == 6:  self.unit_io(True)
        else:
            raise Trap(f'CSP {n}')
        return False

    def unit_io(self, write):
        st = self.st
        w = st.pop(); z = st.pop(); y = st.pop(); x = st.pop(); buf = st.pop(); unit = st.pop()
        buf = (buf + x) & M16
        if unit == 13:
            self.con.call(write, buf, y, z, w)
        elif unit in (4, 5, 9, 10, 11, 12):
            nblk = 632                              # the drive geometry URD 21 reports (0x278)
            if z >= nblk or z < 0:                  # off the end of the disk
                self.ioresult = 10
            elif write:
                self.vol.write(z, bytes(self.mem[buf:buf + y]))
                self.ioresult = 0
            else:
                self.mem[buf:buf + y] = self.vol.read(z, y)
                self.ioresult = 0
        elif unit in (1, 2):
            if write:
                self.ioresult = 0
            else:
                for i in range(y):
                    if self.shotdir:
                        self.shotn += 1
                        k = self.keys[0] if self.keys else 0
                        self.con.save_png(os.path.join(self.shotdir, f'{self.shotn:03d}_{k:02x}.png'), 2)
                    if not self.keys: raise NeedInput()
                    self.mem[buf + i] = self.keys.pop(0)
                self.ioresult = 0
        else:
            self.ioresult = 2

    def key_ready(self):
        return len(self.keys) > 0


# --------------------------------------------------------------------------
def main(argv):
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('cmd', choices=['run'])
    ap.add_argument('--keys', default='')
    ap.add_argument('--shot', default='')
    ap.add_argument('--steps', type=int, default=20_000_000)
    ap.add_argument('--trace', action='store_true')
    ap.add_argument('--shotdir', default='')
    a = ap.parse_args(argv)
    vm = VM(keys=a.keys.encode('latin1').decode('unicode_escape').encode('latin1'))
    vm.trace = a.trace
    if a.shotdir:
        os.makedirs(a.shotdir, exist_ok=True)
        vm.shotdir = a.shotdir
    vm.start()
    try:
        vm.run(a.steps)
    except NeedInput:
        print('blocked waiting for a key')
    except (Trap, IndexError, RuntimeError) as e:
        print('TRAP:', repr(e), 'at', vm.seg.name, vm.frame.proc.num, hex(vm.ip))
        for h in list(vm.hist)[-30:]:
            print('  ', h)
    if a.shot:
        vm.con.save_png(a.shot)
        print('wrote', a.shot)


if __name__ == '__main__':
    main(sys.argv[1:])
