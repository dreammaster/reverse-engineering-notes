"""
PKWARE Data Compression Library "implode" stream decoder (the format Companions of Xanth's .PIC pictures use - XANTH.EXE
contains "PKWARE Data Compression Library(tm)").  Python port of Mark Adler's blast.c: 2-byte header (literal mode 0/1, dictionary
size 4/5/6 bits), then LSB-first bit stream of literals and length/distance pairs, end code length 519.

    from dcl_explode import explode
    out = explode(data)                      # returns (bytes, number of input bytes consumed)
"""

_LITLEN = [11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10, 12, 10, 8, 23, 8, 9, 7, 6, 7, 8, 7, 6, 55, 8, 23, 24, 12, 11,
           7, 9, 11, 12, 6, 7, 22, 5, 7, 24, 6, 11, 9, 6, 7, 22, 7, 11, 38, 7, 9, 8, 25, 11, 8, 11, 9, 12, 8, 12, 5, 38, 5, 38,
           5, 11, 7, 5, 6, 21, 6, 10, 53, 8, 7, 24, 10, 27, 44, 253, 253, 253, 252, 252, 252, 13, 12, 45, 12, 45, 12, 61, 12,
           45, 44, 173]
_LENLEN = [2, 35, 36, 53, 38, 23]
_DISTLEN = [2, 20, 53, 230, 247, 151, 248]
_BASE = [3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264]
_EXTRA = [0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8]
_MAXBITS = 13


class _Huff:
    def __init__(self, rep):
        length = []
        for b in rep:
            left = (b >> 4) + 1
            ln = b & 15
            length += [ln] * left
        n = len(length)
        self.count = [0] * (_MAXBITS + 1)
        for ln in length:
            self.count[ln] += 1
        offs = [0] * (_MAXBITS + 1)
        for ln in range(1, _MAXBITS):
            offs[ln + 1] = offs[ln] + self.count[ln]
        self.symbol = [0] * n
        for sym in range(n):
            if length[sym] != 0:
                self.symbol[offs[length[sym]]] = sym
                offs[length[sym]] += 1


class _State:
    def __init__(self, data):
        self.d = data
        self.p = 0
        self.bitbuf = 0
        self.bitcnt = 0

    def bits(self, need):
        val = self.bitbuf
        while self.bitcnt < need:
            if self.p >= len(self.d):
                raise ValueError("out of input")
            val |= self.d[self.p] << self.bitcnt
            self.p += 1
            self.bitcnt += 8
        self.bitbuf = val >> need
        self.bitcnt -= need
        return val & ((1 << need) - 1)

    def decode(self, h):
        code = first = index = 0
        bitbuf = self.bitbuf
        left = self.bitcnt
        ln = 1
        nxt = 1
        while True:
            while left > 0:
                left -= 1
                code |= (bitbuf & 1) ^ 1
                bitbuf >>= 1
                count = h.count[nxt]
                nxt += 1
                if code < first + count:
                    self.bitbuf = bitbuf
                    self.bitcnt = (self.bitcnt - ln) & 7
                    return h.symbol[index + (code - first)]
                index += count
                first += count
                first <<= 1
                code <<= 1
                ln += 1
            left = (_MAXBITS + 1) - ln
            if left == 0:
                break
            if self.p >= len(self.d):
                raise ValueError("out of input")
            bitbuf = self.d[self.p]
            self.p += 1
            if left > 8:
                left = 8
        raise ValueError("bad code")


_LIT = _Huff(_LITLEN)
_LEN = _Huff(_LENLEN)
_DIST = _Huff(_DISTLEN)


def explode(data, max_out=1 << 24):
    s = _State(bytes(data))
    lit = s.bits(8)
    if lit > 1:
        raise ValueError("bad literal flag")
    dictbits = s.bits(8)
    if dictbits < 4 or dictbits > 6:
        raise ValueError("bad dictionary size")
    out = bytearray()
    while True:
        if s.bits(1):
            sym = s.decode(_LEN)
            ln = _BASE[sym] + s.bits(_EXTRA[sym])
            if ln == 519:
                break
            sh = 2 if ln == 2 else dictbits
            dist = s.decode(_DIST) << sh
            dist += s.bits(sh)
            dist += 1
            if dist > len(out):
                raise ValueError("distance too far")
            for _ in range(ln):
                out.append(out[-dist])
        else:
            out.append(s.decode(_LIT) if lit else s.bits(8))
        if len(out) > max_out:
            raise ValueError("too large")
    return bytes(out), s.p
