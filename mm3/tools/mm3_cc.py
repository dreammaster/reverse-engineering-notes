#!/usr/bin/env python3
"""MM3 .CC archive reader (docs/mm3-re.md sections 2, 3): TOC cipher, LZHUF members, filename hash.

usage: mm3_cc.py list ARCHIVE [MM3.EXE]        list members (names recovered from the EXE's strings)
       mm3_cc.py extract ARCHIVE OUTDIR [MM3.EXE]
"""
import os
import re
import struct
import sys

N, F, THRESHOLD = 4096, 60, 2
N_CHAR = 256 - THRESHOLD + F
T = N_CHAR * 2 - 1
R = T - 1
MAX_FREQ = 0x8000

# static Huffman tables of LZHUF.C (position codes)
def _static_tables():
    code, ln, c = [], [], 0
    for bits, ncodes, rep in ((3, 1, 32), (4, 3, 16), (5, 8, 8), (6, 12, 4), (7, 24, 2), (8, 16, 1)):
        for _ in range(ncodes):
            code += [c] * rep
            ln += [bits] * rep
            c += 1
    return bytes(code), bytes(ln)


D_CODE, D_LEN = _static_tables()
assert len(D_CODE) == 256 and len(D_LEN) == 256, (len(D_CODE), len(D_LEN))


class Lzhuf:
    def __init__(self, data, pos):
        self.d, self.p = data, pos
        self.bits = 0
        self.nbits = 0
        self.freq = [0] * (T + 1)
        self.prnt = [0] * (T + N_CHAR)
        self.son = [0] * T
        for i in range(N_CHAR):
            self.freq[i] = 1
            self.son[i] = i + T
            self.prnt[i + T] = i
        i, j = 0, N_CHAR
        while j <= R:
            self.freq[j] = self.freq[i] + self.freq[i + 1]
            self.son[j] = i
            self.prnt[i] = self.prnt[i + 1] = j
            i += 2
            j += 1
        self.freq[T] = 0xFFFF
        self.prnt[R] = 0

    def getbit(self):
        if self.nbits == 0:
            self.bits = self.d[self.p] if self.p < len(self.d) else 0
            self.p += 1
            self.nbits = 8
        self.nbits -= 1
        return (self.bits >> self.nbits) & 1

    def getbyte(self):
        v = 0
        for _ in range(8):
            v = (v << 1) | self.getbit()
        return v

    def reconst(self):
        j = 0
        for i in range(T):
            if self.son[i] >= T:
                self.freq[j] = (self.freq[i] + 1) // 2
                self.son[j] = self.son[i]
                j += 1
        i, j = 0, N_CHAR
        while j < T:
            k = i + 1
            f = self.freq[j] = self.freq[i] + self.freq[k]
            k = j - 1
            while f < self.freq[k]:
                k -= 1
            k += 1
            n = j - k
            self.freq[k + 1:k + 1 + n] = self.freq[k:k + n]
            self.freq[k] = f
            self.son[k + 1:k + 1 + n] = self.son[k:k + n]
            self.son[k] = i
            i += 2
            j += 1
        for i in range(T):
            k = self.son[i]
            if k >= T:
                self.prnt[k] = i
            else:
                self.prnt[k] = self.prnt[k + 1] = i

    def update(self, c):
        if self.freq[R] == MAX_FREQ:
            self.reconst()
        c = self.prnt[c + T]
        while True:
            self.freq[c] += 1
            k = self.freq[c]
            l = c + 1
            if k > self.freq[l]:
                while k > self.freq[l + 1]:
                    l += 1
                self.freq[c] = self.freq[l]
                self.freq[l] = k
                i = self.son[c]
                self.prnt[i] = l
                if i < T:
                    self.prnt[i + 1] = l
                j = self.son[l]
                self.son[l] = i
                self.prnt[j] = c
                if j < T:
                    self.prnt[j + 1] = c
                self.son[c] = j
                c = l
            c = self.prnt[c]
            if c == 0:
                break

    def decode_char(self):
        c = self.son[R]
        while c < T:
            c = self.son[c + self.getbit()]
        c -= T
        self.update(c)
        return c

    def decode_position(self):
        i = self.getbyte()
        c = D_CODE[i] << 6
        j = D_LEN[i] - 2
        while j:
            i = (i << 1) | self.getbit()
            j -= 1
        return c | (i & 0x3F)


def lzhuf_decode(blob):
    """blob = member payload (fill, fill, u16 BE size, stream); returns bytes or None if it is not LZHUF."""
    if len(blob) < 5 or blob[0] != blob[1]:
        return None
    fill = blob[0]
    size = (blob[2] << 8) | blob[3]
    if size == 0:
        return None
    dec = Lzhuf(blob, 4)
    ring = bytearray([fill]) * N
    r = N - F
    out = bytearray()
    try:
        while len(out) < size:
            c = dec.decode_char()
            if c < 256:
                out.append(c)
                ring[r] = c
                r = (r + 1) & (N - 1)
            else:
                pos = (r - dec.decode_position() - 1) & (N - 1)
                ln = c - 255 + THRESHOLD
                for k in range(ln):
                    b = ring[(pos + k) & (N - 1)]
                    out.append(b)
                    ring[r] = b
                    r = (r + 1) & (N - 1)
    except IndexError:
        return None
    if dec.p > len(blob) + 2 or len(blob) - dec.p > 2:
        return None
    return bytes(out[:size])


def name_id(name):
    name = name.upper()
    t = ord(name[0])
    for ch in name[1:]:
        t = ((t & 0x7F) << 9) | ((t & 0xFF80) >> 7)
        t = (t + ord(ch)) & 0xFFFF
    return t


def read_toc(d):
    n = struct.unpack_from("<H", d, 0)[0]
    t = bytes(((((c << 2) | (c >> 6)) & 255) - (0x54 + 0x99 * i)) & 255 for i, c in enumerate(d[2:2 + n * 8]))
    ents = []
    for i in range(n):
        ident = struct.unpack_from("<H", t, i * 8)[0]
        off = t[i * 8 + 2] | (t[i * 8 + 3] << 8) | (t[i * 8 + 4] << 16)
        size = struct.unpack_from("<H", t, i * 8 + 5)[0]
        ents.append((ident, off, size))
    return ents


def exe_names(exe_path):
    names = {}
    if not exe_path:
        return names
    data = open(exe_path, "rb").read()
    for m in re.finditer(rb"[A-Za-z0-9_%.\-]{3,16}\.[A-Za-z]{1,3}\x00", data):
        s = m.group(0)[:-1].decode()
        cands = [s]
        if "%" in s:
            cands = [re.sub(r"%0?2?[du]", lambda mm: ("%02d" % i) if "02" in mm.group(0) else str(i), s) for i in range(100)]
        for c in cands:
            if "%" not in c:
                names[name_id(c)] = c.upper()
    return names


def member(d, off, size):
    blob = d[off:off + size]
    dec = lzhuf_decode(blob)
    return (dec, True) if dec is not None else (blob, False)


def main():
    cmd, arc = sys.argv[1], sys.argv[2]
    d = open(arc, "rb").read()
    ents = read_toc(d)
    if cmd == "list":
        names = exe_names(sys.argv[3] if len(sys.argv) > 3 else None)
        for ident, off, size in ents:
            m, packed = member(d, off, size)
            print("%04X %-14s off=%7d size=%5d  -> %5d%s" % (ident, names.get(ident, ""), off, size, len(m), "" if packed else " (stored)"))
    elif cmd == "extract":
        out = sys.argv[3]
        names = exe_names(sys.argv[4] if len(sys.argv) > 4 else None)
        os.makedirs(out, exist_ok=True)
        for ident, off, size in ents:
            m, _ = member(d, off, size)
            open(os.path.join(out, names.get(ident, "%04X.bin" % ident)), "wb").write(m)


if __name__ == "__main__":
    main()
