"""
Emulation of CONUNIT (unit 13), the console/graphics driver inside SYSTEM.INTERP.

Every routine here is a transcription of the x86 handler it names (addresses
are file offsets in wiz1_interp / lines in wiz1_interp.asm).  The driver is a
*window compositor*:

  window record (lives in p-System heap, built by WIZARDRY proc 19):
      +0  u16  next window (list head = global 14)
      +2  u8   x          +3  u8  y
      +4  u8   w          +5  u8  h        (both include the border)
      +6  u8   cursor x   +7  u8  cursor y (interior coordinates)
      +8  u8   priority (bit 7 = hidden; 0x3F = "emphasis" border)
      +A  u16[(w-2)*(h-2)]   interior cells  (lo = glyph, hi = attribute)

  driver state:  PRIO[1000]  per-screen-cell priority + dirty bits (0xE900)
                 CELL[1000]  per-screen-cell word last composed   (0xED00)
                 VRAM        CGA 320x200x2bpp (mode 5), cell = 8x8 px

UNITREAD/UNITWRITE(unit, buf, X, Y, Z, W) -- for unit 13: Y selects the
function, Z is its parameter (window record / char / block number), buf+X the
primary buffer.  (docs/pmachine.md called Y "blocknumber".)
"""
import struct, zlib

COLS, ROWS = 40, 25


class NeedInput(Exception):
    """the program is blocked waiting for a key and the script has none left"""


def write_png(path, w, h, rows):
    """rows: list of bytes (RGB) per scanline"""
    raw = b''.join(b'\0' + r for r in rows)

    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xFFFFFFFF)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


class Win:
    """a view onto a window record in VM memory"""

    def __init__(self, vm, a):
        self.vm, self.a = vm, a

    @property
    def nxt(self): return self.vm.r16(self.a)
    @property
    def x(self): return self.vm.mem[self.a + 2]
    @property
    def y(self): return self.vm.mem[self.a + 3]
    @property
    def w(self): return self.vm.mem[self.a + 4]
    @property
    def h(self): return self.vm.mem[self.a + 5]
    @property
    def prio(self): return self.vm.mem[self.a + 8]

    def cell(self, i): return self.vm.r16(self.a + 0xA + 2 * i)


class Conunit:
    s = [0x5BAB, 0xD02B, 0x7E15, 0x7351]
    idle = 0
    rnglog = None
    IDLE_LIMIT = 300

    def __init__(self, vm):
        self.vm = vm
        self.prio = bytearray(COLS * ROWS)
        self.cell = [0] * (COLS * ROWS)
        self.vram = bytearray(0x4000)
        self.charset = bytes(5120)
        self.mon_blk = 0
        self.mon_pic = [None] * 4
        self.mon_rec = [bytes(512)] * 4
        self.head = 0
        self.inited = False
        self.calls = []                 # (selector, z) log for analysis

    # -- dispatch -----------------------------------------------------------
    def call(self, write, buf, sel, z, w):
        vm = self.vm
        vm.ioresult = 0
        self.calls.append((write, sel, z))
        if vm.oplog is not None and write:
            self.log0(sel, buf, z)
        if not write:
            if sel == 10: self.random(buf)
            elif sel == 11:                                       # key buffered?
                vm.w16(buf, 1 if vm.keys else 0)
                self.idle = 0 if vm.keys else self.idle + 1
                if self.idle > self.IDLE_LIMIT:
                    if self.idle == self.IDLE_LIMIT + 1:
                        vm.shot(0)                       # the screen the player is looking at
                    raise NeedInput()
            elif sel == 12: vm.w16(buf, 1)
            elif sel == 21: vm.w16(buf, 0x278)                    # drive size in blocks (sub_1210)
            else: vm.ioresult = 3
            return
        f = {1: self.init, 2: self.layout, 3: self.present, 4: self.putchar,
             5: self.drawline, 13: self.blit13, 14: self.load_charset,
             15: self.shutdown, 17: self.clear_attr, 18: self.load_monsters,
             19: self.pause, 22: self.noop, 23: self.title, 24: self.sound,
             26: self.strcat, 29: self.strpos, 30: self.putstring}.get(sel)
        if f is None:
            raise RuntimeError(f'CONUNIT write selector {sel} not emulated '
                               f'(buf={buf:04x} z={z:04x})')
        f(buf, z, w)

    def log0(self, sel, buf, z):
        vm = self.vm
        L = vm.oplog
        t = 'L0e' if vm.logdepth > 0 else 'L0d'        # echo of a logged L1/L2 proc vs direct call
        if sel == 3: L.append(f'  {t} present W{buf:04x}')
        elif sel == 2: L.append(f'  {t} layout head=W{buf:04x}')
        elif sel == 30: L.append(f'  {t} puts W{buf:04x} {vm.pstr(z)!r}')
        elif sel == 4: L.append(f'  {t} putc W{buf:04x} {z:04x}')
        elif sel == 5:
            a = [vm.r16(buf + 2 * i) for i in range(8)]
            L.append(f'  {t} drawline W{z:04x} {a[0] & 255} {a[1] & 255} {a[2]} {a[3]} {a[4] & 255} {a[5] & 255} {a[6]} {a[7]}')
        elif sel == 13: L.append(f'  {t} blit13 W{z:04x} {" ".join(str(b) for b in vm.mem[buf:buf + 4])}')
        elif sel == 17: L.append(f'  {t} clearattr W{z:04x}')
        elif sel == 23: L.append(f'  {t} title {z}')
        else: L.append(f'  {t} sel{sel} buf={buf:04x} z={z:04x}')

    def visible(self):
        """the composed screen: one word per cell, 0 = blank"""
        out = []
        for si in range(COLS * ROWS):
            c = self.cell[si]
            if (self.prio[si] & 0x3F) == 0 or c in (0x2020, 0x0020):
                c = 0
            out.append(c)
        return out

    def dump_grid(self, path):
        v = self.visible()
        with open(path, 'w') as f:
            for r in range(ROWS):
                f.write(' '.join(f'{c:04x}' for c in v[r * COLS:(r + 1) * COLS]) + chr(10))
            f.write(f'vram {zlib.crc32(bytes(self.vram)) & 0xFFFFFFFF:08x}' + chr(10))
            f.write('rng ' + ','.join(f'{x:04x}' for x in self.s) + chr(10))

    def noop(self, buf, z, w): pass
    def sound(self, buf, z, w): pass
    def pause(self, buf, z, w): pass

    # -- RANDOM ----------------------------------------------------------------
    def random(self, buf):
        inc = [0x3619, 0xFF8B, 0x0183, 0x7FC9]
        mul = [0x6A2D, 0xFFF1, 0xFFAF, 0xFFD9]
        s = self.s
        for i in range(4):
            s[i] = (s[i] * mul[i] + inc[i]) & 0xFFFF
        sw = ((s[3] & 0xFF) << 8) | (s[3] >> 8)
        self.vm.w16(buf, sw & 0x700F)
        if self.rnglog is not None: self.rnglog.append(sw & 0x700F)

    # -- init / shutdown ------------------------------------------------------
    def init(self, buf, z, w):
        self.inited = True
        self.prio = bytearray(COLS * ROWS)
        self.cell = [0] * (COLS * ROWS)
        self.vram = bytearray(0x4000)
        # result: video type bits (mode != 6 -> 0x31)
        self.vm.w16(buf, 1 | 0x10 | 0x20)

    def shutdown(self, buf, z, w): pass

    def load_charset(self, buf, z, w):
        self.charset = self.vm.vol.read(z, 5120)

    def load_monsters(self, buf, z, w):
        self.mon_blk = z
        self.mon_pic = [None] * 4

    # -- cell/pixel output -----------------------------------------------------
    def vaddr(self, si):
        y, x = divmod(si, COLS)
        return y * 320 + x * 2

    def put8(self, si, rows16):
        """rows16: 8 x 2 bytes, top to bottom"""
        a = self.vaddr(si)
        v = self.vram
        for r in range(8):
            o = a + (r & 1) * 0x2000 + (r >> 1) * 80
            v[o:o + 2] = rows16[2 * r:2 * r + 2]

    def draw(self, si, ax):
        """sub_1882: dispatch on the cell word"""
        ah, al = ax >> 8, ax & 0xFF
        if ah == 0:
            if al == 0 or al == 0x20: return self.blank(si)
            self.put8(si, self.charset[al * 16:al * 16 + 16])
        elif ah == 0x20:
            self.blank(si)
        elif ah >= 0x80:
            self.put8(si, b'\xff' * 16)
        else:
            tile = (ah & 0x1F) - 1
            grp = (ah >> 4 & 6) >> 1
            rec = self.mon_rec[grp]
            self.put8(si, rec[tile * 16:tile * 16 + 16])

    def blank(self, si):
        self.cell[si] = 0
        self.put8(si, bytes(16))

    def b1853(self, si, ax):
        """sub_1853: store a cell word and redraw it if it visibly changed"""
        old = self.cell[si]
        self.cell[si] = ax
        a = 0 if old in (0x2020, 0x20) else old
        b = 0 if ax in (0x2020, 0x20) else ax
        if a != b:
            self.draw(si, ax)

    def b1844(self, win, si, ax):
        bl = win.prio
        p = self.prio[si]
        if bl == p:
            pass
        elif bl < p:
            return
        else:
            self.prio[si] = bl
        self.b1853(si, ax)

    # -- window geometry ------------------------------------------------------
    @staticmethod
    def border_cells(win, glyphs):
        """the (cell, glyph) sequence sub_159A / sub_1797 emit"""
        TL, T, TR, L, R, BL, B, BR = glyphs
        start = win.y * COLS + win.x
        w, h = win.w, win.h
        out = [(start, TL)]
        for i in range(w - 2): out.append((start + 1 + i, T))
        out.append((start + w - 1, TR))
        for i in range(h - 2):
            out.append((start + COLS * (1 + i), L))
            out.append((start + COLS * (1 + i) + w - 1, R))
        bl = start + COLS * (h - 1)
        out.append((bl, BL))
        for i in range(w - 2): out.append((bl + 1 + i, B))
        out.append((bl + w - 1, BR))
        return out

    STD = (1, 2, 3, 4, 5, 6, 7, 8)
    EMPH = (0x0D, 0x0A, 0x0E, 0x0B, 0x0C, 0x0E, 0x09, 0x0D)

    # -- blk 2: lay out all windows ------------------------------------------
    def layout(self, buf, z, w):
        vm = self.vm
        self.head = buf
        self.prio = bytearray(COLS * ROWS)
        a = self.head
        while a:
            win = Win(vm, a)
            if not (win.prio & 0x80):
                glyphs = self.EMPH if win.prio == 0x3F else self.STD
                for si, g in self.border_cells(win, glyphs):
                    self.put_border(win, si, g)
                self.interior(win)
            a = win.nxt
        self.flush()

    def put_border(self, win, si, g):                      # sub_1674 / sub_16A9
        dl = win.prio
        dh = self.prio[si] & 0x3F
        if dl < dh: return
        if dl > dh: self.prio[si] = dl
        old = self.cell[si]
        if g == old: return
        self.cell[si] = g
        cl = 0x80
        ch = old >> 8
        if ch != 0 and ch < 0x80: cl = 0xC0
        self.prio[si] |= cl

    def interior(self, win):                               # sub_16F2
        si0 = win.y * COLS + win.x + COLS + 1
        iw, ih = win.w - 2, win.h - 2
        i = 0
        for r in range(ih):
            for c in range(iw):
                si = si0 + r * COLS + c
                ax = win.cell(i); i += 1
                al = win.prio
                if al < (self.prio[si] & 0x3F): continue
                self.prio[si] = al
                dx = self.cell[si]
                if ax == dx: continue
                self.cell[si] = ax
                self.prio[si] |= 0x80
                dh = dx >> 8
                if dh != 0 and dh < 0x80 and dh != 0x20:
                    self.prio[si] |= 0x40

    def flush(self):                                       # sub_175D
        for si in range(COLS * ROWS):
            p = self.prio[si]
            if p == 0:
                self.blank(si)
            elif p & 0x80:
                self.prio[si] = p & 0x3F
                self.draw(si, self.cell[si])

    # -- blk 3: redraw one window immediately -----------------------------------
    def present(self, buf, z, w):
        win = Win(self.vm, buf)
        if win.prio & 0x80: return
        for si, g in self.border_cells(win, self.STD):
            self.b1844(win, si, g)
        si0 = win.y * COLS + win.x + COLS + 1
        iw, ih = win.w - 2, win.h - 2
        i = 0
        for r in range(ih):
            for c in range(iw):
                si = si0 + r * COLS + c
                ax = win.cell(i); i += 1
                if win.prio < self.prio[si]: continue
                self.prio[si] = win.prio
                self.b1853(si, ax)

    # -- blk 4 / 30: write into a window ---------------------------------------
    def putchar(self, buf, z, w):
        self._putc(buf, z)

    def putstring(self, buf, z, w):
        vm = self.vm
        n = vm.mem[z]
        for i in range(n):
            self._putc(buf, vm.mem[z + 1 + i])

    def _putc(self, a, ch):
        vm = self.vm
        win = Win(vm, a)
        iw, ih = win.w - 2, win.h - 2
        cx, cy = vm.mem[a + 6], vm.mem[a + 7]
        vm.w16(a + 0xA + 2 * (cy * iw + cx), ch)
        if not (win.prio & 0x80):
            si = (cy + win.y + 1) * COLS + (cx + win.x + 1)
            if win.prio >= self.prio[si]:
                self.prio[si] = win.prio
                self.b1853(si, ch)
        cx += 1
        if cx == iw:
            cx = 0
            cy += 1
            if cy == ih: cy = 0
        vm.mem[a + 6], vm.mem[a + 7] = cx, cy

    # -- blk 5: DRAWLINE ------------------------------------------------------------
    def drawline(self, buf, z, w):
        vm = self.vm
        bp = buf
        win = Win(vm, z)
        dl, dh = win.w - 2, win.h - 2
        r16 = vm.r16
        lo, hi = vm.mem[bp], vm.mem[bp + 2]
        col, row = vm.mem[bp + 4], vm.mem[bp + 6]
        dcol, drow = vm.mem[bp + 8], vm.mem[bp + 0xA]
        count = r16(bp + 0xC)
        glyph = r16(bp + 0xE)
        si = z + 0xA + 2 * ((row * dl) & 0xFFFF) + 2 * r16(bp + 4)
        step = 0
        if drow:
            step = dl
            if drow & 0x80: step = -step
        step = (step + dcol) & 0xFF
        step = step - 256 if step & 0x80 else step
        step *= 2
        al, ah = col, row
        while True:
            if lo <= al <= hi:
                vm.w16(si, glyph)
            si += step
            al = (al + dcol) & 0xFF
            if al & 0x80: break
            ah = (ah + drow) & 0xFF
            if ah & 0x80: break
            if al == dl or ah == dh: break
            count = (count - 1) & 0xFFFF
            if count == 0: break
        vm.w16(bp + 0xC, count)
        vm.w16(bp + 0xA, step & 0xFFFF)

    # -- blk 13 / 17: monster portraits ---------------------------------------------
    def clear_attr(self, buf, z, w):
        self._clr(z)

    def _clr(self, win):
        vm = self.vm
        for i in range(0x2D0):
            vm.mem[win + 0xB + 2 * i] = 0

    COLTAB = {1: [15], 2: [11, 19], 3: [7, 15, 23], 4: [3, 11, 19, 27]}

    def blit13(self, buf, z, w):
        vm = self.vm
        win = z
        self._clr(win)
        pics = [vm.mem[buf + k] for k in range(4)]
        valid = [p for p in pics if p != 0xFF]
        cols = self.COLTAB.get(len(valid), [])
        for k, c in enumerate(cols):
            base = win + 0xA + 2 * (324 + c)
            al = k << 5
            for row in range(5):
                for cc in range(6):
                    al += 1
                    vm.mem[base + 2 * (row * 36 + cc) + 1] = al & 0xFF
        # load the records (sub_1A4A)
        self.mon_pic = [None] * 4
        for k, p in enumerate(valid):
            self.mon_pic[k] = p
            self.mon_rec[k] = vm.vol.read(self.mon_blk + (p - 1), 512)
        for si in range(COLS * ROWS):
            hb = self.cell[si] >> 8
            if hb != 0 and hb < 0x80 and hb != 0x20:
                self.cell[si] = (self.cell[si] & 0xFF) | 0x1F00

    # -- blk 23: title bitmap ------------------------------------------------------
    def title(self, buf, z, w):
        vm = self.vm
        a0 = self.vaddr(2 * COLS + 2)
        if z == 0:
            for i in range(28):
                for bank in (0, 0x2000):
                    o = a0 + bank + i * 80
                    self.vram[o:o + 70] = bytes(70)
            return
        data = vm.vol.read(z, 56 * 80 + 512)[4:]
        pos = 0
        for i in range(28):
            for bank in (0, 0x2000):
                o = a0 + bank + i * 80
                self.vram[o:o + 70] = data[pos:pos + 70]
                pos += 80
        self.mon_pic = [None] * 4

    # -- string services (Pascal runtime helpers living in the same unit) -------
    def strcat(self, buf, z, w):
        vm = self.vm

        def app(dst, src):
            if not src: return
            n = vm.mem[dst]; m = vm.mem[src]
            vm.mem[dst + 1 + n:dst + 1 + n + m] = vm.mem[src + 1:src + 1 + m]
            vm.mem[dst] = (n + m) & 0xFF
        app(buf, z)
        if w: app(buf, w)

    def strpos(self, buf, z, w):
        vm = self.vm
        pat = bytes(vm.mem[z + 1:z + 1 + vm.mem[z]])
        s = bytes(vm.mem[w + 1:w + 1 + vm.mem[w]])
        vm.w16(buf, (s.find(pat) + 1) if pat and s else 0)

    # -- rendering ------------------------------------------------------------------------
    PAL = {0: (0, 0, 0), 1: (0, 170, 170), 2: (170, 0, 0), 3: (255, 255, 255)}

    def rows_rgb(self, scale=2):
        out = []
        for y in range(200):
            bank = (y & 1) * 0x2000
            base = bank + (y >> 1) * 80
            line = bytearray()
            for x in range(320):
                b = self.vram[base + (x >> 2)]
                v = (b >> (6 - 2 * (x & 3))) & 3
                line += bytes(self.PAL[v]) * scale
            for _ in range(scale):
                out.append(bytes(line))
        return out

    def save_png(self, path, scale=2):
        write_png(path, 320 * scale, 200 * scale, self.rows_rgb(scale))
