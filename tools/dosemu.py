#!/usr/bin/env python3
"""
dosemu.py - a tiny real-mode DOS + VGA harness on top of Unicorn.

Purpose: run the *original* (unpacked) Cadaver executables so we can
  * trace which code touches which buffers (find the decoders without
    reading 30k lines of unlabeled asm),
  * dump memory after the game's own loader has decoded a file,
  * grab VGA frames (palette via DAC ports, pixels from A000h) to compare
    against the C reimplementation.

It is NOT a general DOS emulator: just the INT 21h/10h/16h/1Ah subset the
games use, a fake VGA retrace bit, a PIT-driven INT 8 tick and an optional
keyboard queue.  Unknown interrupts are logged when verbose.

    from dosemu import Emu
    e = Emu('unpacked/cadaver1/CAD.EXE', 'cadaver1')   # game dir = data root
    e.run(max_instr=50_000_000)
"""
import os, struct, sys
from unicorn import *
from unicorn import x86_const as X

LOAD_SEG = 0x1000
PSP_SEG = LOAD_SEG - 0x10


class Emu:
    def __init__(self, exe, gamedir, verbose=False):
        self.gamedir = os.path.abspath(gamedir)
        self.verbose = verbose
        self.uc = Uc(UC_ARCH_X86, UC_MODE_16)
        self.uc.mem_map(0, 0xA0000)            # conventional memory
        self.uc.mem_map(0xB0000, 0x50000)      # text/ROM area: plain RAM
        self.vga = Vga()
        self.uc.mmio_map(0xA0000, 0x10000, self.vga.mem_read, None, self.vga.mem_write, None)
        self.files = {}          # handle -> python file
        self.next_handle = 5
        self.dta = 0
        self.keys = []           # queued (scan<<8|ascii) words
        self.pending_keys = []   # released one at a time once the game is idle-polling
        self.scan_queue = []     # raw scancodes (make: sc, break: sc|0x80) delivered via INT 9 / port 60h
        self.scan_port = 0
        self.idle_polls = 0
        self.palette = [(0, 0, 0)] * 256
        self.dac_idx = 0
        self.dac_tmp = []
        self.retrace = 0
        self.ticks = 0
        self.mem_top = 0xA000
        self.exit_code = None
        self.int_log = {}
        self._load_exe(exe)
        self.uc.hook_add(UC_HOOK_INTR, self._on_intr)
        self.uc.hook_add(UC_HOOK_INSN, self._on_in, None, 1, 0, X.UC_X86_INS_IN)
        self.uc.hook_add(UC_HOOK_INSN, self._on_out, None, 1, 0, X.UC_X86_INS_OUT)

    # ------------------------------------------------------------ loading
    def _load_exe(self, path):
        d = open(path, 'rb').read()
        (_, lastpg, pages, nrel, hdrpar, minalloc, maxalloc, ss, sp, _cks, ip, cs,
         reloff, _ovl) = struct.unpack('<14H', d[:28])
        size = (pages - 1) * 512 + (lastpg or 512)
        img = bytearray(d[hdrpar * 16:size])
        for i in range(nrel):
            o, s = struct.unpack('<HH', d[reloff + i * 4:reloff + i * 4 + 4])
            p = s * 16 + o
            v = struct.unpack('<H', img[p:p + 2])[0]
            img[p:p + 2] = struct.pack('<H', (v + LOAD_SEG) & 0xFFFF)
        self.uc.mem_write(LOAD_SEG * 16, bytes(img))
        self.image_end_seg = LOAD_SEG + (len(img) + minalloc * 16 + 15) // 16
        self.alloc_next = (self.image_end_seg + 0xF) & ~0xF
        psp = bytearray(256)
        psp[0:2] = b'\xCD\x20'
        psp[2:4] = struct.pack('<H', 0xA000)
        psp[0x80] = 0
        psp[0x81] = 0x0D
        self.uc.mem_write(PSP_SEG * 16, bytes(psp))
        self.w('cs', (cs + LOAD_SEG) & 0xFFFF)
        self.w('ip', ip)
        self.w('ss', (ss + LOAD_SEG) & 0xFFFF)
        self.w('sp', sp)
        self.w('ds', PSP_SEG)
        self.w('es', PSP_SEG)
        self.uc.mem_write(0xF0000, b'\xCF')  # IRET stub
        for v in range(256):
            self.uc.mem_write(v * 4, struct.pack('<HH', 0, 0xF000))

    # ---------------------------------------------------------- reg helpers
    def r(self, name):
        return self.uc.reg_read(getattr(X, 'UC_X86_REG_' + name.upper()))

    def w(self, name, v):
        self.uc.reg_write(getattr(X, 'UC_X86_REG_' + name.upper()), v)

    @staticmethod
    def lin(seg, off):
        return ((seg << 4) + off) & 0xFFFFF

    def rd(self, seg, off, n):
        return bytes(self.uc.mem_read(self.lin(seg, off), n))

    def wr(self, seg, off, data):
        self.uc.mem_write(self.lin(seg, off), bytes(data))

    def cstr(self, seg, off):
        out = bytearray()
        while True:
            b = self.rd(seg, off + len(out), 1)[0]
            if b == 0:
                return bytes(out)
            out.append(b)

    def set_cf(self, c):
        # INT hook returns straight to the instruction after INT, so flags
        # are the live ones.
        fl = self.r('eflags')
        self.w('eflags', (fl | 1) if c else (fl & ~1))

    # --------------------------------------------------------------- ports
    def _on_in(self, uc, port, size, user):
        if port == 0x3DA:
            self.vga.ac_flip = 0
            self.retrace ^= 9
            return self.retrace
        if port == 0x60:
            return self.scan_port
        if port == 0x40:
            self.retrace += 1
            return self.retrace & 0xFF
        if port == 0x201:
            return 0xFF
        return 0

    def _on_out(self, uc, port, size, value, user):
        if size == 2 and port in (0x3C4, 0x3CE, 0x3D4, 0x3C0):
            self.vga.port_out(port, value & 0xFF)
            self.vga.port_out(port + 1, value >> 8)
            return
        if 0x3C0 <= port <= 0x3D5 and port not in (0x3C8, 0x3C9, 0x3DA):
            self.vga.port_out(port, value & 0xFF)
            return
        if port == 0x3C8:
            self.dac_idx = value & 0xFF
            self.dac_tmp = []
        elif port == 0x3C9:
            self.dac_tmp.append(value & 0x3F)
            if len(self.dac_tmp) == 3:
                self.palette[self.dac_idx] = tuple((c << 2) | (c >> 4) for c in self.dac_tmp)
                self.dac_idx = (self.dac_idx + 1) & 0xFF
                self.dac_tmp = []

    # ----------------------------------------------------------- interrupts
    def _dos_path(self, raw):
        p = raw.decode('latin1').replace('\\', '/')
        if len(p) > 1 and p[1] == ':':
            p = p[2:]
        parts = [x for x in p.split('/') if x and x != '.']
        cur = self.gamedir
        for part in parts:
            try:
                names = os.listdir(cur)
            except OSError:
                return None
            hit = [n for n in names if n.lower() == part.lower()]
            cur = os.path.join(cur, hit[0] if hit else part)
        return cur

    def _on_intr(self, uc, intno, user):
        self.int_log[intno] = self.int_log.get(intno, 0) + 1
        ax = self.r('ax')
        ah = ax >> 8
        if intno == 0x21:
            self._int21(ah, ax)
        elif intno == 0x10:
            if ah == 0x10 and (ax & 0xFF) == 0x12:  # set block of DAC regs
                es, dx, cx, first = self.r('es'), self.r('dx'), self.r('cx'), self.r('bx')
                data = self.rd(es, dx, cx * 3)
                for i in range(cx):
                    self.palette[(first + i) & 0xFF] = tuple(((c & 0x3F) << 2) | ((c & 0x3F) >> 4) for c in data[i * 3:i * 3 + 3])
            elif ah == 0x10 and (ax & 0xFF) == 0x00:      # set one palette register
                self.vga.ac_pal[self.r('bx') & 0xF] = (self.r('bx') >> 8) & 0xFF
            elif ah == 0x10 and (ax & 0xFF) == 0x02:      # set all palette registers
                data = self.rd(self.r('es'), self.r('dx'), 17)
                for i in range(16):
                    self.vga.ac_pal[i] = data[i]
            elif ah == 0x10 and (ax & 0xFF) == 0x10:      # set one DAC register
                self.palette[self.r('bx') & 0xFF] = tuple(((c & 0x3F) << 2) | ((c & 0x3F) >> 4) for c in
                                                         (self.r('dx') >> 8, self.r('cx') >> 8, self.r('cx') & 0xFF))
            elif ah == 0x00:
                self.vga.mode = ax & 0xFF
            elif ah == 0x0F:
                self.w('ax', 0x1300 | self.vga.mode)
        elif intno == 0x16:
            fl = self.r('eflags')
            if not self.keys and self.pending_keys and ah in (0, 0x10):
                self.idle_polls += 1
                if self.idle_polls > 3000:
                    self.keys.append(self.pending_keys.pop(0))
                    self.idle_polls = 0
            if ah in (0, 0x10):
                self.w('ax', self.keys.pop(0) if self.keys else 0)
            elif ah in (1, 0x11):
                if self.keys:
                    self.w('ax', self.keys[0])
                    self.w('eflags', fl & ~0x40)
                else:
                    self.w('eflags', fl | 0x40)
        elif intno == 0x1A:
            if ah == 0:
                self.w('cx', (self.ticks >> 16) & 0xFFFF)
                self.w('dx', self.ticks & 0xFFFF)
                self.w('ax', 0)
        elif intno == 0x11:
            self.w('ax', 0x0021)
        elif intno == 0x12:
            self.w('ax', 640)
        elif intno == 0x15:
            self.set_cf(True)
        elif self.verbose:
            print('unhandled int %02x ax=%04x' % (intno, ax), file=sys.stderr)

    def _int21(self, ah, ax):
        ds, dx, bx, cx = self.r('ds'), self.r('dx'), self.r('bx'), self.r('cx')
        self.set_cf(False)
        if ah == 0x09:
            s = bytearray()
            o = dx
            while True:
                b = self.rd(ds, o, 1)[0]
                if b == 0x24:
                    break
                s.append(b)
                o += 1
            print('DOS PRINT:', s.decode('latin1'), file=sys.stderr)
        elif ah == 0x25:
            self.wr(0, (ax & 0xFF) * 4, struct.pack('<HH', dx, ds))
        elif ah == 0x35:
            off, seg = struct.unpack('<HH', self.rd(0, (ax & 0xFF) * 4, 4))
            self.w('bx', off)
            self.w('es', seg)
        elif ah in (0x3D, 0x3C):
            raw = self.cstr(ds, dx)
            p = self._dos_path(raw)
            try:
                f = open(p, 'rb' if ah == 0x3D else 'w+b')
                h = self.next_handle
                self.next_handle += 1
                self.files[h] = f
                self.w('ax', h)
                self.on_open(h, raw.decode('latin1'), p)
            except (OSError, TypeError):
                if self.verbose:
                    print('open FAIL', raw, file=sys.stderr)
                self.w('ax', 2)
                self.set_cf(True)
        elif ah == 0x3E:
            f = self.files.pop(bx, None)
            if f:
                f.close()
        elif ah == 0x3F:
            f = self.files.get(bx)
            if f is None:
                self.w('ax', 6)
                self.set_cf(True)
            else:
                data = f.read(cx)
                self.wr(ds, dx, data)
                self.w('ax', len(data))
                self.on_read(bx, ds, dx, data)
        elif ah == 0x40:
            data = self.rd(ds, dx, cx)
            f = self.files.get(bx)
            if f:
                f.write(data)
            self.w('ax', cx)
        elif ah == 0x42:
            f = self.files.get(bx)
            off = (cx << 16) | dx
            if off & 0x80000000:
                off -= 1 << 32
            f.seek(off, ax & 0xFF)
            pos = f.tell()
            self.w('dx', pos >> 16)
            self.w('ax', pos & 0xFFFF)
        elif ah == 0x48:
            seg = self.alloc_next
            if seg + bx > self.mem_top:
                self.w('bx', max(0, self.mem_top - seg))
                self.w('ax', 8)
                self.set_cf(True)
            else:
                self.alloc_next += bx
                self.w('ax', seg)
        elif ah in (0x49, 0x4A, 0x47, 0x0B, 0x0C, 0x0D):
            pass
        elif ah == 0x4C:
            self.exit_code = ax & 0xFF
            self.uc.emu_stop()
        elif ah == 0x1A:
            self.dta = dx
        elif ah == 0x19:
            self.w('ax', (ax & 0xFF00) | 2)
        elif ah == 0x0E:
            self.w('ax', (ax & 0xFF00) | 3)
        elif ah == 0x30:
            self.w('ax', 0x0005)
        elif ah == 0x2A:
            self.w('cx', 1991)
            self.w('dx', 0x0101)
        elif ah == 0x2C:
            self.w('cx', 0)
            self.w('dx', 0)
        elif ah in (0x4E, 0x4F):
            self.w('ax', 18)
            self.set_cf(True)
        elif self.verbose:
            print('unhandled int21 ah=%02x' % ah, file=sys.stderr)

    # hooks for scripts
    def on_open(self, handle, dosname, hostpath):
        if self.verbose:
            print('open', dosname, '->', hostpath, handle, file=sys.stderr)

    def on_read(self, handle, seg, off, data):
        pass

    # ----------------------------------------------------------------- run
    def run(self, max_instr=10_000_000, slice_instr=20000):
        done = 0
        while done < max_instr and self.exit_code is None:
            cs, ip = self.r('cs'), self.r('ip')
            try:
                self.uc.emu_start(self.lin(cs, ip), 0xFFFFF, count=slice_instr)
            except UcError as e:
                print('emulation error:', e, 'at %04x:%04x' % (self.r('cs'), self.r('ip')), file=sys.stderr)
                return False
            done += slice_instr
            self._tick()
        return True

    def _tick(self):
        """Deliver an INT 8 if the game installed a handler and IF is set."""
        self.ticks += 1
        fl = self.r('eflags')
        if not fl & 0x200:
            return
        off9, seg9 = struct.unpack('<HH', self.rd(0, 9 * 4, 4))
        if self.scan_queue and seg9 != 0xF000:
            self.scan_port = self.scan_queue.pop(0)
            self._deliver(seg9, off9, fl)
            return
        off, seg = struct.unpack('<HH', self.rd(0, 8 * 4, 4))
        if seg == 0xF000:
            return
        self._deliver(seg, off, fl)

    def _deliver(self, seg, off, fl):
        ss, sp = self.r('ss'), (self.r('sp') - 6) & 0xFFFF
        self.wr(ss, sp, struct.pack('<HHH', self.r('ip'), self.r('cs'), fl & 0xFFFF))
        self.w('sp', sp)
        self.w('eflags', fl & ~0x300)
        self.w('cs', seg)
        self.w('ip', off)

    # ---------------------------------------------------------------- vga
    def frame(self):
        """Render the visible screen as 320x200 indexed pixels (EGA mode 0Dh
        planar, or mode 13h linear) plus an RGB palette list."""
        v = self.vga
        start = v.crtc[0x0C] << 8 | v.crtc[0x0D]
        out = bytearray(320 * 200)
        for y in range(200):
            for xb in range(40):
                a = (start + y * 40 + xb) & 0xFFFF
                p0, p1, p2, p3 = (v.planes[i][a] for i in range(4))
                for bit in range(8):
                    m = 7 - bit
                    c = ((p0 >> m) & 1) | (((p1 >> m) & 1) << 1) | (((p2 >> m) & 1) << 2) | (((p3 >> m) & 1) << 3)
                    out[y * 320 + xb * 8 + bit] = v.ac_pal[c] & 0x3F
        return bytes(out), self.palette

    def save_frame(self, path):
        from PIL import Image
        px, pal = self.frame()
        im = Image.frombytes('P', (320, 200), px)
        flat = [c for rgb in pal for c in rgb]
        im.putpalette(flat + [0] * (768 - len(flat)))
        im.save(path)


class Vga:
    """Planar VGA/EGA model: 4 x 64K planes, latches, write modes 0-3 (func,
    rotate, bit mask, set/reset), read modes 0/1, map mask, CRTC start."""

    def __init__(self):
        self.planes = [bytearray(0x10000) for _ in range(4)]
        self.latch = [0, 0, 0, 0]
        self.seq = [0] * 8
        self.seq[2] = 0xF
        self.gc = [0] * 9
        self.gc[8] = 0xFF
        self.crtc = [0] * 0x20
        self.ac_pal = list(range(16))
        self.ac_flip = 0
        self.ac_idx = 0
        self.seq_idx = self.gc_idx = self.crtc_idx = 0
        self.mode = 3
        self.read_trace = None   # optional callable(offset, size) for reads of the A000 window
        self.write_trace = None

    def port_out(self, port, val):
        if port == 0x3C4:
            self.seq_idx = val & 7
        elif port == 0x3C5:
            self.seq[self.seq_idx] = val
        elif port == 0x3CE:
            self.gc_idx = val & 15
        elif port == 0x3CF:
            if self.gc_idx < 9:
                self.gc[self.gc_idx] = val
        elif port == 0x3D4:
            self.crtc_idx = val & 0x1F
        elif port == 0x3D5:
            self.crtc[self.crtc_idx] = val
        elif port == 0x3C0:
            if self.ac_flip == 0:
                self.ac_idx = val & 0x1F
            elif self.ac_idx < 16:
                self.ac_pal[self.ac_idx] = val & 0x3F
            self.ac_flip ^= 1

    def mem_read(self, uc, off, size, user):
        if self.read_trace:
            self.read_trace(off, size)
        r = 0
        for i in range(size):
            a = (off + i) & 0xFFFF
            self.latch = [self.planes[p][a] for p in range(4)]
            if self.gc[5] & 8:      # read mode 1: colour compare
                cmp, care = self.gc[2] & 15, self.gc[7] & 15
                b = 0xFF
                for p in range(4):
                    if care >> p & 1:
                        b &= self.latch[p] if cmp >> p & 1 else ~self.latch[p] & 0xFF
                r |= (b & 0xFF) << (8 * i)
            else:
                r |= self.latch[self.gc[4] & 3] << (8 * i)
        return r

    def mem_write(self, uc, off, size, value, user):
        if self.write_trace:
            self.write_trace(off, size, value)
        for i in range(size):
            self._wb((off + i) & 0xFFFF, (value >> (8 * i)) & 0xFF)

    def _wb(self, a, val):
        wm = self.gc[5] & 3
        mapmask = self.seq[2] & 15
        sr, esr = self.gc[0] & 15, self.gc[1] & 15
        func = (self.gc[3] >> 3) & 3
        rot = self.gc[3] & 7
        for p in range(4):
            if not (mapmask >> p) & 1:
                continue
            if wm == 1:
                self.planes[p][a] = self.latch[p]
                continue
            if wm == 0:
                d = ((val >> rot) | (val << (8 - rot))) & 0xFF
                if esr >> p & 1:
                    d = 0xFF if sr >> p & 1 else 0
            elif wm == 2:
                d = 0xFF if val >> p & 1 else 0
            else:
                d = 0xFF if sr >> p & 1 else 0
                m = ((val >> rot) | (val << (8 - rot))) & 0xFF & self.gc[8]
                lt = self.latch[p]
                self.planes[p][a] = (lt & ~m & 0xFF) | (d & m)
                continue
            l = self.latch[p]
            if func == 1:
                d &= l
            elif func == 2:
                d |= l
            elif func == 3:
                d ^= l
            bm = self.gc[8]
            self.planes[p][a] = (l & ~bm & 0xFF) | (d & bm)
