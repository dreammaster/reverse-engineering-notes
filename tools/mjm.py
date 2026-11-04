#!/usr/bin/env python3
"""
Decoder for Cadaver's "mjm" container files (*.LNG, *.MP1-3, ANDATA.DAT, PCMAN.DAT,
ROUTINE*.TAD / *.PAK).  Format (see docs/formats.md):

    'm' 'j' 'm' <method>        method '0' = stored (rest of file is raw data)
                                       '1' = PKWARE DCL "implode" stream follows
                                       '2' = Rob Northern RNC ("RNC\\x01") stream follows
    ... payload ...

Method 1 is the stock PKWARE Data Compression Library format (the same
algorithm as zlib's contrib/blast): header byte 0 = literal mode (0 binary,
1 ASCII), byte 1 = dictionary size bits (4..6).
"""
import sys, struct

# --- PKWARE DCL explode (port of zlib/contrib/blast) ------------------------
_LITLEN = [11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10, 12, 10, 8, 23, 8,
           9, 7, 6, 7, 8, 7, 6, 55, 8, 23, 24, 12, 11, 7, 9, 11, 12, 6, 7, 22, 5,
           7, 24, 6, 11, 9, 6, 7, 22, 7, 11, 38, 7, 9, 8, 25, 11, 8, 11, 9, 12,
           8, 12, 5, 38, 5, 38, 5, 11, 7, 5, 6, 21, 6, 10, 53, 8, 7, 24, 10, 27,
           44, 253, 253, 253, 252, 252, 252, 13, 12, 45, 12, 45, 12, 61, 12, 45,
           44, 173]
_LENLEN = [2, 35, 36, 53, 38, 23]
_DISTLEN = [2, 20, 53, 230, 247, 151, 248]
_LENBASE = [3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264]
_EXTRA = [0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8]


def _construct(rep):
    """Build canonical Huffman decode tables from blast's run-length spec."""
    lengths = []
    for r in rep:
        n = (r >> 4) + 1
        l = r & 15
        lengths += [l] * n
    count = [0] * 16
    for l in lengths:
        count[l] += 1
    offs = [0] * 16
    for l in range(1, 15):
        offs[l + 1] = offs[l] + count[l]
    symbol = [0] * len(lengths)
    for s, l in enumerate(lengths):
        if l:
            symbol[offs[l]] = s
            offs[l] += 1
    return count, symbol


_LIT = _construct(_LITLEN)
_LEN = _construct(_LENLEN)
_DIST = _construct(_DISTLEN)


class _Bits:
    def __init__(self, d, p):
        self.d, self.p, self.buf, self.cnt = d, p, 0, 0

    def bits(self, need):
        val = self.buf
        while self.cnt < need:
            if self.p >= len(self.d):
                raise EOFError
            val |= self.d[self.p] << self.cnt
            self.p += 1
            self.cnt += 8
        self.buf = val >> need
        self.cnt -= need
        return val & ((1 << need) - 1)

    def decode(self, tbl):
        count, symbol = tbl
        code = first = index = 0
        for l in range(1, 16):
            code |= self.bits(1) ^ 1
            c = count[l]
            if code - c < first:
                return symbol[index + (code - first)]
            index += c
            first += c
            first <<= 1
            code <<= 1
        raise ValueError("bad code")


def dcl_explode(data, pos=0):
    lit = data[pos]
    dbits = data[pos + 1]
    if lit > 1 or not 4 <= dbits <= 6:
        raise ValueError("bad DCL header %r" % data[pos:pos + 2])
    b = _Bits(data, pos + 2)
    out = bytearray()
    while True:
        if b.bits(1):
            sym = b.decode(_LEN)
            ln = _LENBASE[sym] + b.bits(_EXTRA[sym])
            if ln == 519:
                break
            sh = 2 if ln == 2 else dbits
            dist = b.decode(_DIST) << sh
            dist += b.bits(sh)
            dist += 1
            if dist > len(out):
                raise ValueError("distance too far back")
            for _ in range(ln):
                out.append(out[-dist])
        else:
            out.append(b.decode(_LIT) if lit else b.bits(8))
    return bytes(out)


def mjm_unpack(data):
    if data[:3] != b'mjm':
        raise ValueError("not an mjm file")
    m = data[3:4]
    if m == b'0':
        return data[4:]
    if m == b'1':
        return dcl_explode(data, 4)
    if m == b'2':
        sys.path.insert(0, __file__.rsplit('/', 1)[0])
        import rnc1
        return rnc1.unpack(data[4:])
    raise ValueError("unknown mjm method %r" % m)


if __name__ == '__main__':
    for f in sys.argv[1:]:
        d = open(f, 'rb').read()
        u = mjm_unpack(d)
        print(f, len(d), '->', len(u))
