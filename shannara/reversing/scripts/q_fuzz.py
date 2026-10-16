"""
Differential fuzzer: random valid Q block streams are decoded by q_decode_ref.QDecoder and by the demo's real
sub_1652E0 (q_emulate.QEmu); video buffers and consumed byte counts must match.

    python q_fuzz.py <qdump dir> [iterations] [seed]
"""
import random
import sys

from q_decode_ref import *
from q_emulate import QEmu, VIDEO_SIZE


class Gen:
    """Builds a valid stream by driving a QDecoder one operation at a time."""

    def __init__(self, dec, rng, has_comp=True):
        self.d, self.r = dec, rng
        self.has_comp = has_comp

    def pc(self):
        return self.r.randrange(0, 0xF8)

    def _two(self, *tail):
        c0 = self.pc()
        c1 = self.pc()
        while c1 == c0:
            c1 = self.pc()
        return bytes([c0, c1, *tail])

    def pc_not(self, *bad):
        while True:
            v = self.r.randrange(0, 256)
            if v not in bad:
                return v

    def choose(self):
        d, r = self.d, self.r
        W, H = d.W, d.H
        rem = d.cols + 1                       # blocks left in this row
        idx = d.idx
        cands = []

        def add(w, f):
            cands.append((w, f))
        add(8, lambda: self._two(r.randrange(0, 0x80)))
        add(6, lambda: self._two(r.randrange(0x80, 0x100), r.randrange(0, 256)))
        add(4, lambda: (lambda c: bytes([c, c]))(self.pc()))
        add(5, lambda: bytes([0xFF]))
        if idx >= 1:
            add(4, lambda: bytes([0xFE, r.randrange(0, min(255, idx))]))
        if idx + 2 < W * H:
            add(4, lambda: bytes([0xFD, r.randrange(0, min(255, W * H - idx - 1))]))
        add(4, self._esc4)
        add(3, lambda: (lambda i: bytes([0xFA, i & 255, i >> 8]))(r.randrange(0, W * H)))
        if self.has_comp:
            add(3, lambda: bytes([0xF8, 0xF8]))
            add(2, lambda: bytes([0xF8, 0xFB]) + bytes(r.choice([0xF8, r.randrange(0, 0xF8)]) for _ in range(16)))
            add(3, lambda: bytes([0xF8, self.pc_not(0xFB, 0xF8), r.randrange(256), r.randrange(256)]))
        if rem >= 2:
            add(5, self._esc1)
            add(5, self._esc3)
        total = sum(w for w, f in cands)
        x = r.uniform(0, total)
        for w, f in cands:
            x -= w
            if x <= 0:
                return f()
        return cands[-1][1]()

    def _mot_ok(self, dst, b):
        m = self.d.mot[b]
        return 0 <= dst + m and abs(m) < 0x20000 and dst + m + 3 * 640 + 8 < VIDEO_SIZE - 0x10000

    def _esc4(self):
        d, r = self.d, self.r
        for _ in range(50):
            b = r.randrange(0, 256)
            if self._mot_ok(d.dst, b):
                return bytes([0xFC, b])
        return bytes([0xFF])

    def _esc1(self):
        d, r = self.d, self.r
        st = d.state
        rem = d.cols + 1
        if st == COMP_OVL:
            return bytes([0xF9, r.randrange(2, 5)])
        if st in (T0, BLK4, SOLID, BLK3) and d.idx < 1:
            return bytes([0xFF])
        lo = 1 if st in (SKIP, ABS) else 2
        if rem < lo:
            return bytes([0xFF])
        n = r.randrange(lo, rem + 1)
        if st == BACK and d.idx - d.s[d.p - 1] - 1 < 0:
            return bytes([0xFF])
        if st == FWD and d.idx + d.s[d.p - 1] + 1 + n >= d.W * d.H:
            return bytes([0xFF])
        if st == MOTION and not all(self._mot_ok(d.dst + 4 * k, d.s[d.p - 1]) for k in range(n)):
            return bytes([0xFF])
        return bytes([0xF9, n])

    def _esc3(self):
        d, r = self.d, self.r
        st = d.state
        rem = d.cols + 1
        raw = d.raw_mode or st in (T0, SKIP, COMP_COPY, COMP_OVL)
        if raw:
            n = r.randrange(1, rem + 1)
            body = b"".join(bytes([self.pc()]) + bytes(r.randrange(256) for _ in range(15)) for _ in range(n))
            return bytes([0xFB, n]) + body
        if rem < 2:
            return bytes([0xFF])
        n = 2 * r.randrange(1, rem // 2 + 1)
        if st == SOLID:
            return bytes([0xFB, n]) + bytes(self.pc() for _ in range(n))
        if st == BACK:
            if d.idx < 1:
                return bytes([0xFF])
            return bytes([0xFB, n]) + bytes(r.randrange(0, min(255, d.idx + k)) for k in range(n))
        if st == FWD:
            if d.idx + 2 * n + 2 >= d.W * d.H:
                return bytes([0xFF])
            return bytes([0xFB, n]) + bytes(r.randrange(0, max(1, min(255, d.W * d.H - d.idx - n - 2))) for k in range(n))
        if st == MOTION:
            out = []
            for k in range(n):
                for _ in range(50):
                    b = r.randrange(256)
                    if self._mot_ok(d.dst + 4 * k, b):
                        break
                else:
                    return bytes([0xFF])
                out.append(b)
            return bytes([0xFB, n]) + bytes(out)
        if st == ABS:
            out = b""
            for k in range(n):
                i = r.randrange(0, d.W * d.H)
                out += bytes([i & 255, i >> 8])
            return bytes([0xFB, n]) + out
        if st == BLK4:
            return bytes([0xFB, n]) + bytes(r.randrange(256) for _ in range(2 * n))
        if st == BLK3:
            out = b""
            for k in range(n):
                if r.random() < 0.5:
                    out += bytes([r.randrange(0, 0x80)])
                else:
                    out += bytes([r.randrange(0x80, 0x100), r.randrange(256)])
            return bytes([0xFB, n]) + out
        return bytes([0xFF])


def run(dump_dir, iters=200, seed=1, W=10, H=5, window=False, raw=False):
    tables = load_tables(dump_dir)
    rng = random.Random(seed)
    emu = QEmu(dump_dir, W, H)
    bad = 0
    for it in range(iters):
        qmap = bytes(rng.randrange(256) for _ in range(256))
        video0 = bytes(rng.randrange(256) for _ in range(H * ROW + 4 * PITCH))
        comp = bytes(rng.randrange(256) for _ in range(H * ROW + 4 * PITCH))
        v = bytearray(video0 + bytes(VIDEO_SIZE - len(video0)))
        dec = QDecoder(tables, W, H, qmap, v, composite=bytearray(comp + bytes(0x10000)))
        stream = bytearray()
        dec.s = stream
        dec.p = 0
        min_col, min_row, max_col, max_row = 0, 0, W - 1, H
        if window:
            min_col = rng.randrange(0, W)
            max_col = rng.randrange(0, W - min_col)
            min_row = rng.randrange(0, H)
            max_row = rng.randrange(1, H - min_row + 1)
        dec.idx = min_row * W + min_col
        dec.cols = max_col
        dec.rows = max_row
        dec.start = dec.dst = dec.rowstart = dec.baddr(dec.idx)
        dec.row_i = 0
        dec.max_col = max_col
        dec.min_col = min_col
        dec.raw_mode = raw
        dec.state = T0
        dec.done = False
        gen = Gen(dec, rng)
        guard = 0
        while not dec.done and guard < 5000:
            guard += 1
            stream += gen.choose()
            dec.step()
        stream_b = bytes(stream)
        emu.reset_memory()
        emu.set_qmap(qmap)
        emu.set_composite(comp)
        if window:
            emu.set_window(min_col, min_row, max_col, max_row)
        if raw:
            emu.set_raw_mode()
        pix, info = emu.decode_frame(stream_b, video=video0)
        if pix is None:
            emu.reset_memory(); emu.set_qmap(qmap); emu.set_composite(comp)
            emu.decode_frame(stream_b, video=video0, trace=True)
            print('  trail:', [hex(a) for a in emu.trail[-12:]], 'stream', stream_b.hex())
        ref = bytes(dec.v[:H * ROW])
        if pix is None:
            print("iter", it, "emulator error", info)
            bad += 1
            continue
        got = pix[:H * ROW]
        used = info[0]
        if got != ref or used != len(stream_b):
            bad += 1
            diffs = [i for i in range(len(ref)) if ref[i] != got[i]]
            print("iter", it, "MISMATCH stream", len(stream_b), "used", used, "first diffs", diffs[:3])
            if bad <= 1:
                print("  stream:", stream_b.hex())
                open("mismatch_stream.bin", "wb").write(stream_b)
            if bad > 5:
                break
    print("done, mismatches:", bad, "of", iters)


if __name__ == "__main__":
    run(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 100, int(sys.argv[3]) if len(sys.argv) > 3 else 1)
