"""
Reference (Python) decoder for the Q-video block stream, written from the demo's sub_1652E0 and verified against
the real machine code with q_fuzz.py (differential fuzzing under unicorn).  See analysis/q_codec_spec.md.

A frame is a grid of W x H blocks of 4x4 pixels (8 bpp, palette indexed) drawn into a buffer with a fixed pitch of
640 bytes.  The stream is a byte sequence of block operations; the decoder keeps a *state* equal to the kind of the
previous operation, which changes the meaning of the run escapes 0xF9 (repeat) and 0xFB (run with inline data).
"""
import struct

PITCH = 640
ROW = PITCH * 4  # bytes per block row (0xA00)

# states = which operation produced the previous block (the engine has one jump table per state)
T0, SKIP, BACK, BLK4, SOLID, FWD, MOTION, ABS, BLK3, COMP_COPY, COMP_OVL = range(11)


def load_tables(dump_dir):
    """mask tables and motion offsets live in the exe's data segment"""
    import json, os
    meta = json.load(open(os.path.join(dump_dir, "segs.json")))
    data = open(os.path.join(dump_dir, "seg_187000.bin"), "rb").read()

    def rd(a): return struct.unpack_from("<I", data, a - 0x187000)[0]
    n = meta["names"]
    hi = [rd(n["dword_18B6BC"] + 4 * i) for i in range(256)]
    lo = [rd(n["dword_18B2BC"] + 4 * i) for i in range(256)]
    mot = [struct.unpack("<i", struct.pack("<I", rd(n["dword_18BABC"] + 4 * i)))[0] for i in range(256)]
    return hi, lo, mot


class QDecoder:
    def __init__(self, tables, W, H, qmap, video, composite=None):
        self.hi, self.lo, self.mot = tables      # hi[b] = 4 mask bytes for the high nibble row, lo[b] for the low
        self.W, self.H = W, H
        self.qmap = bytes(qmap)
        self.v = video                            # bytearray, pitch 640
        self.comp = composite
        self.raw_mode = False

    # -- helpers ---------------------------------------------------------
    def baddr(self, idx):
        return (idx // self.W) * ROW + (idx % self.W) * 4

    def get_block(self, buf, off):
        return [bytes(buf[off + y * PITCH: off + y * PITCH + 4]) for y in range(4)]

    def put_block(self, off, rows):
        for y in range(4):
            self.v[off + y * PITCH: off + y * PITCH + 4] = rows[y]

    def mask_rows(self, b_hi_rows, b_lo_rows, c0, c1):
        """2-colour block: byte b_hi_rows gives rows 0,1; b_lo_rows rows 2,3 (high nibble first)"""
        def row(mask4, ):
            return bytes(c1 if mask4[i] else c0 for i in range(4))
        def m(tab, b): return struct.pack("<I", tab[b])
        return [row(m(self.hi, b_hi_rows)), row(m(self.lo, b_hi_rows)),
                row(m(self.hi, b_lo_rows)), row(m(self.lo, b_lo_rows))]

    # -- main loop -------------------------------------------------------
    def decode(self, s, min_col=0, min_row=0, max_col=None, max_row=None):
        W = self.W
        max_col = W - 1 if max_col is None else max_col
        max_row = self.H if max_row is None else max_row
        self.s = s
        self.p = 0
        self.idx = min_row * W + min_col
        self.cols = max_col                    # byte 3 of the engine's counter: columns left - 1
        self.rows = max_row                    # byte 2: rows left
        self.start = self.baddr(self.idx)
        self.dst = self.start
        self.rowstart = self.start
        self.row_i = 0
        self.max_col = max_col
        self.min_col = min_col
        self.state = T0
        self.c0 = self.c1 = 0
        self.done = False
        self.prev_src = 0
        while not self.done:
            self.step()
        return self.p

    def next_block(self, nblocks=1):
        """advance after writing nblocks blocks; handles row end"""
        self.idx += nblocks
        self.dst += 4 * nblocks
        self.cols -= nblocks
        if self.cols == -1:      # row finished
            self.cols = self.max_col
            self.rows -= 1
            self.idx += self.W - self.max_col - 1
            if self.rows == 0:
                self.done = True
                return
            self.row_i += 1
            self.rowstart += ROW
            self.dst = self.rowstart
            self.state = T0
        # (a run that ends mid-row keeps the state-reset below)

    def b(self, k=0):
        return self.s[self.p + k]

    def step(self):
        s, p = self.s, self.p
        op = s[p]
        if op < 0xF8:
            self.plain(op)
        else:
            k = op & 7
            [self.esc0, self.esc1, self.esc2, self.esc3, self.esc4, self.esc5, self.esc6, self.esc7][k]()

    # -- plain blocks ----------------------------------------------------
    def plain(self, c0):
        s, p = self.s, self.p
        c1 = s[p + 1]
        if c1 == c0:                                    # solid block
            self.put_block(self.dst, [bytes([c0]) * 4] * 4)
            self.p += 2
            nxt = SOLID
        elif s[p + 2] & 0x80:                           # explicit 16-bit pattern
            b2, b3 = s[p + 2], s[p + 3]
            self.c0, self.c1 = c0, c1
            self.put_block(self.dst, self.mask_rows(b3, b2, c0, c1))
            self.p += 4
            nxt = BLK4
        else:                                           # pattern from the qmap table
            q = s[p + 2]
            self.c0, self.c1 = c0, c1
            self.put_block(self.dst, self.mask_rows(self.qmap[q * 2 + 1], self.qmap[q * 2], c0, c1))
            self.p += 3
            nxt = BLK3
        self.finish(1, nxt)

    def finish(self, n, nxt):
        """n blocks written: advance, set the state for the next operation (T0 after a run or a row end)"""
        row_before = self.row_i
        self.next_block(n)
        if not self.done and self.row_i == row_before:
            self.state = nxt

    # -- escapes ---------------------------------------------------------
    def esc7(self):                                     # 0xFF skip one block
        self.p += 1
        self.finish(1, SKIP)

    def copy_from(self, src_off, dst_off=None):
        self.put_block(self.dst if dst_off is None else dst_off, self.get_block(self.v, src_off))

    def esc6(self):                                     # 0xFE copy block (idx - b - 1)
        b = self.b(1)
        self.copy_from(self.baddr((self.idx - b - 1) & 0xFFFF))
        self.p += 2
        self.finish(1, BACK)

    def esc5(self):                                     # 0xFD copy block (idx + b + 1)
        b = self.b(1)
        self.copy_from(self.baddr(self.idx + b + 1))
        self.p += 2
        self.finish(1, FWD)

    def esc4(self):                                     # 0xFC copy from pixel offset table
        b = self.b(1)
        self.copy_from(self.dst + self.mot[b])
        self.p += 2
        self.finish(1, MOTION)

    def esc2(self):                                     # 0xFA copy absolute block index
        i = self.b(1) | (self.b(2) << 8)
        self.copy_from(self.baddr(i))
        self.last_abs = self.baddr(i)
        self.p += 3
        self.finish(1, ABS)

    def esc0(self):                                     # 0xF8 composite (type-6 snapshot) operations
        sub = self.b(1)
        comp_off = self.baddr(self.idx)
        if sub == 0xF8:                                 # co-located composite block
            self.put_block(self.dst, self.get_block(self.comp, comp_off))
            self.p += 2
            self.finish(1, COMP_COPY)
        elif sub == 0xFB:                               # 16 literal bytes, 0xF8 = keep composite pixel
            cb = self.get_block(self.comp, comp_off)
            rows = []
            for y in range(4):
                rows.append(bytes(cb[y][x] if self.s[self.p + 2 + y * 4 + x] == 0xF8 else self.s[self.p + 2 + y * 4 + x]
                                  for x in range(4)))
            self.put_block(self.dst, rows)
            self.p += 18
            self.finish(1, COMP_OVL)
        else:                                           # colour over composite: mask 1 = composite, 0 = colour
            c = sub
            b2, b3 = self.b(2), self.b(3)
            cb = self.get_block(self.comp, comp_off)
            def m(tab, b): return struct.pack("<I", tab[b])
            masks = [m(self.hi, b3), m(self.lo, b3), m(self.hi, b2), m(self.lo, b2)]
            rows = [bytes(cb[y][x] if masks[y][x] else c for x in range(4)) for y in range(4)]
            self.put_block(self.dst, rows)
            self.p += 4
            self.finish(1, COMP_OVL)

    # -- runs ------------------------------------------------------------
    def esc1(self):                                     # 0xF9 repeat previous op n times
        n = self.b(1)
        st = self.state
        if st == T10_SKIP1():
            pass
        d = self.dst
        if st in (T0, BLK4, SOLID, BLK3):               # replicate the previous block (index - 1)
            src = self.get_block(self.v, self.baddr(self.idx - 1))
            for k in range(n):
                self.put_block(d + 4 * k, src)
        elif st == SKIP:
            pass
        elif st == BACK:
            src_off = self.baddr((self.idx - self.s[self.p - 1] - 1) & 0xFFFF)
            for k in range(n):
                self.put_block(d + 4 * k, self.get_block(self.v, src_off + 4 * k))
        elif st == FWD:
            src_off = self.baddr(self.idx + self.s[self.p - 1] + 1)
            for k in range(n):
                self.put_block(d + 4 * k, self.get_block(self.v, src_off + 4 * k))
        elif st == MOTION:
            src_off = d + self.mot[self.s[self.p - 1]]
            for k in range(n):
                self.put_block(d + 4 * k, self.get_block(self.v, src_off + 4 * k))
        elif st == ABS:
            src = self.get_block(self.v, self.last_abs)
            for k in range(n):
                self.put_block(d + 4 * k, src)
        elif st == COMP_COPY:
            src_off = self.baddr(self.idx)
            for k in range(n):
                self.put_block(d + 4 * k, self.get_block(self.comp, src_off + 4 * k))
        elif st == COMP_OVL:                            # quirk: consumes 2 bytes, skips ONE block
            self.p += 2
            self.finish(1, T0)
            self.state = T0
            return
        self.p += 2
        self.run_done(n)

    def run_done(self, n):
        self.next_block(n)
        if not self.done:
            self.state = T0

    def esc3(self):                                     # 0xFB run with inline data
        n = self.b(1)
        st = self.state
        d = self.dst
        q = self.p + 2
        s = self.s
        if self.raw_mode or st in (T0, SKIP, COMP_COPY, COMP_OVL):
            for k in range(n):
                rows = [bytes(s[q + 16 * k + y * 4: q + 16 * k + y * 4 + 4]) for y in range(4)]
                self.put_block(d + 4 * k, rows)
            self.p = q + 16 * n
        elif st == SOLID:
            for k in range(n):
                self.put_block(d + 4 * k, [bytes([s[q + k]]) * 4] * 4)
            self.p = q + n
        elif st == BACK:
            for k in range(n):
                self.copy_from(self.baddr((self.idx + k - s[q + k] - 1) & 0xFFFF), d + 4 * k)
            self.p = q + n
        elif st == FWD:
            for k in range(n):
                self.copy_from(self.baddr(self.idx + k + s[q + k] + 1), d + 4 * k)
            self.p = q + n
        elif st == MOTION:
            for k in range(n):
                self.copy_from(d + 4 * k + self.mot[s[q + k]], d + 4 * k)
            self.p = q + n
        elif st == ABS:
            for k in range(n):
                i = s[q + 2 * k] | (s[q + 2 * k + 1] << 8)
                self.copy_from(self.baddr(i), d + 4 * k)
            self.p = q + 2 * n
        elif st == BLK4:                                # same colours, 16-bit patterns
            for k in range(n):
                b2, b3 = s[q + 2 * k], s[q + 2 * k + 1]
                self.put_block(d + 4 * k, self.mask_rows(b3, b2, self.c0, self.c1))
            self.p = q + 2 * n
        elif st == BLK3:                                # same colours, qmap index (<0x80) or 16-bit pattern
            pos = q
            for k in range(n):
                if s[pos] & 0x80:
                    b2, b3 = s[pos], s[pos + 1]
                    pos += 2
                    rows = self.mask_rows(b3, b2, self.c0, self.c1)
                else:
                    qi = s[pos]
                    pos += 1
                    rows = self.mask_rows(self.qmap[qi * 2 + 1], self.qmap[qi * 2], self.c0, self.c1)
                self.put_block(d + 4 * k, rows)
            self.p = pos
        self.run_done(n)


def T10_SKIP1():
    return -1
