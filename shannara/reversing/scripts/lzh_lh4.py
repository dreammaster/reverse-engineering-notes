"""
LHA -lh4- style decoder (static per-block canonical Huffman + 4 KB sliding window, MSB-first bit order), written from
analysis/lzh_decode_spec.md (the engine's CXBUF2BUFEXPAND).  DICBIT = 12, NP = 13, PBIT = 4, NT = 19, TBIT = 5, NC = 510, CBIT = 9.

    from lzh_lh4 import expand
    pixels = expand(src_bytes, dst_len)       # raises ValueError if the stream is invalid or produces a different size
"""

DICBIT = 12
DICSIZ = 1 << DICBIT
MAXMATCH = 256
THRESHOLD = 3
NC = 255 + MAXMATCH + 2 - THRESHOLD      # 510
CBIT = 9
NP = DICBIT + 1                           # 13
PBIT = 4
NT = 19
TBIT = 5


class _Bits:
    def __init__(self, data):
        self.d = data
        self.pos = 0
        self.bitbuf = 0          # 32-bit MSB-aligned
        self.sub = 0
        self.cnt = 0
        self.fillbuf(0)
        # initial load of 32 bits
        self.bitbuf = 0
        self.sub = 0
        self.cnt = 0
        self.fillbuf(32)

    def _next(self):
        if self.pos < len(self.d):
            v = self.d[self.pos]
            self.pos += 1
            return v
        return None

    def fillbuf(self, n):
        self.bitbuf = (self.bitbuf << n) & 0xFFFFFFFF
        while n > self.cnt:
            n -= self.cnt
            self.bitbuf |= (self.sub << n) & 0xFFFFFFFF
            v = self._next()
            if v is not None:
                self.sub = v
            self.cnt = 8
        self.cnt -= n
        self.bitbuf |= (self.sub >> self.cnt) & 0xFFFFFFFF

    def getbits(self, n):
        x = self.bitbuf >> (32 - n)
        self.fillbuf(n)
        return x


def _make_table(nchar, bitlen, tablebits, table, left, right):
    count = [0] * 17
    weight = [0] * 17
    start = [0] * 18
    for i in range(nchar):
        count[bitlen[i]] += 1
    start[1] = 0
    for i in range(1, 17):
        start[i + 1] = (start[i] + (count[i] << (16 - i))) & 0xFFFF
    if start[17] & 0xFFFF:
        raise ValueError("bad table")
    jutbits = 16 - tablebits
    for i in range(1, tablebits + 1):
        start[i] >>= jutbits
        weight[i] = 1 << (tablebits - i)
    for i in range(tablebits + 1, 17):
        weight[i] = 1 << (16 - i)
    i = start[tablebits + 1] >> jutbits
    if i != 0:
        k = 1 << tablebits
        while i != k:
            table[i] = 0
            i += 1
    avail = nchar
    mask = 1 << (15 - tablebits)
    for ch in range(nchar):
        length = bitlen[ch]
        if length == 0:
            continue
        k = start[length]
        nextcode = k + weight[length]
        if length <= tablebits:
            if nextcode > (1 << tablebits):
                raise ValueError("bad table")
            for i in range(k, nextcode):
                table[i] = ch
        else:
            p_is_table = True
            idx = k >> jutbits
            ptr = ("t", idx)
            i = length - tablebits
            kk = k
            while i != 0:
                kind, pos = ptr
                cur = table[pos] if kind == "t" else (right[pos] if kind == "r" else left[pos])
                if cur == 0:
                    left[avail] = right[avail] = 0
                    if kind == "t":
                        table[pos] = avail
                    elif kind == "r":
                        right[pos] = avail
                    else:
                        left[pos] = avail
                    cur = avail
                    avail += 1
                if kk & mask:
                    ptr = ("r", cur)
                else:
                    ptr = ("l", cur)
                kk = (kk << 1) & 0xFFFF
                i -= 1
            kind, pos = ptr
            if kind == "t":
                table[pos] = ch
            elif kind == "r":
                right[pos] = ch
            else:
                left[pos] = ch
        start[length] = nextcode


class _Dec:
    def __init__(self, data):
        self.b = _Bits(data)
        self.blocksize = 0
        self.c_len = [0] * NC
        self.c_table = [0] * 4096
        self.pt_len = [0] * 19
        self.pt_table = [0] * 256
        self.left = [0] * (2 * NC - 1)
        self.right = [0] * (2 * NC - 1)

    def read_pt_len(self, nn, nbit, i_special):
        b = self.b
        n = b.getbits(nbit)
        if n == 0:
            c = b.getbits(nbit)
            for i in range(nn):
                self.pt_len[i] = 0
            for i in range(256):
                self.pt_table[i] = c
            return
        i = 0
        while i < n:
            c = b.bitbuf >> 29
            if c == 7:
                mask = 1 << 28
                while b.bitbuf & mask:
                    mask >>= 1
                    c += 1
            b.fillbuf(3 if c < 7 else c - 3)
            self.pt_len[i] = c
            i += 1
            if i == i_special:
                c = b.getbits(2)
                while c > 0:
                    self.pt_len[i] = 0
                    i += 1
                    c -= 1
        while i < nn:
            self.pt_len[i] = 0
            i += 1
        _make_table(nn, self.pt_len, 8, self.pt_table, self.left, self.right)

    def read_c_len(self):
        b = self.b
        n = b.getbits(CBIT)
        if n == 0:
            c = b.getbits(CBIT)
            for i in range(NC):
                self.c_len[i] = 0
            for i in range(4096):
                self.c_table[i] = c
            return
        i = 0
        while i < n:
            c = self.pt_table[b.bitbuf >> 24]
            if c >= NT:
                mask = 1 << 23
                while True:
                    c = self.right[c] if b.bitbuf & mask else self.left[c]
                    mask >>= 1
                    if c < NT:
                        break
            b.fillbuf(self.pt_len[c])
            if c <= 2:
                if c == 0:
                    c = 1
                elif c == 1:
                    c = b.getbits(4) + 3
                else:
                    c = b.getbits(CBIT) + 20
                while c > 0:
                    self.c_len[i] = 0
                    i += 1
                    c -= 1
            else:
                self.c_len[i] = c - 2
                i += 1
        while i < NC:
            self.c_len[i] = 0
            i += 1
        _make_table(NC, self.c_len, 12, self.c_table, self.left, self.right)

    def decode_c(self):
        b = self.b
        if self.blocksize == 0:
            self.blocksize = b.getbits(16)
            self.read_pt_len(NT, TBIT, 3)
            self.read_c_len()
            self.read_pt_len(NP, PBIT, -1)
        self.blocksize -= 1
        j = self.c_table[b.bitbuf >> 20]
        if j >= NC:
            mask = 1 << 19
            while True:
                j = self.right[j] if b.bitbuf & mask else self.left[j]
                mask >>= 1
                if j < NC:
                    break
        b.fillbuf(self.c_len[j])
        return j

    def decode_p(self):
        b = self.b
        j = self.pt_table[b.bitbuf >> 24]
        if j >= NP:
            mask = 1 << 23
            while True:
                j = self.right[j] if b.bitbuf & mask else self.left[j]
                mask >>= 1
                if j < NP:
                    break
        b.fillbuf(self.pt_len[j])
        if j != 0:
            j = (1 << (j - 1)) + b.getbits(j - 1)
        return j


def expand(src, dst_len):
    d = _Dec(bytes(src))
    out = bytearray()
    while len(out) < dst_len:
        c = d.decode_c()
        if c <= 255:
            out.append(c)
        else:
            length = c - (255 - THRESHOLD)
            dist = d.decode_p() + 1
            if dist > len(out):
                raise ValueError("distance before start")
            for _ in range(length):
                out.append(out[-dist])
                if len(out) >= dst_len:
                    break
    if len(out) != dst_len:
        raise ValueError("size mismatch")
    return bytes(out)
