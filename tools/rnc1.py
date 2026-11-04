#!/usr/bin/env python3
"""
Rob Northen Compression (RNC, "Pro-Pack") method 1 decompressor.

Cadaver 2 ships its sound drivers (ADLIB.BIN, BEEPER.BIN, ROLAND.BIN,
SBLASTER.BIN) as raw RNC1 blobs; Cadaver 1's equivalents are stored
uncompressed.  The header's unpacked-data CRC16 is checked so a wrong
guess about the bit-stream details can't pass silently.

    python rnc1.py in.bin out.bin
"""
import struct
import sys


def crc16(data):
    crc = 0
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


class BitReader:
    def __init__(self, data, pos):
        self.d = data
        self.p = pos
        self.buf = 0
        self.cnt = 0

    def word(self):
        w = 0
        if self.p < len(self.d):
            w = self.d[self.p]
        if self.p + 1 < len(self.d):
            w |= self.d[self.p + 1] << 8
        self.p += 2
        return w

    def bit(self):
        if self.cnt == 0:
            self.buf = self.word()
            self.cnt = 16
        b = self.buf & 1
        self.buf >>= 1
        self.cnt -= 1
        return b

    def bits(self, n):
        v = 0
        for i in range(n):
            v |= self.bit() << i
        return v

    def byte(self):
        b = self.d[self.p]
        self.p += 1
        return b


def read_table(br):
    num = br.bits(5)
    lens = [br.bits(4) for _ in range(num)]
    codes = {}
    code = 0
    for l in range(1, 17):
        for i, ll in enumerate(lens):
            if ll == l:
                codes[(l, code)] = i
                code += 1
        code <<= 1
    return codes


def huff(br, tbl):
    code = 0
    for l in range(1, 17):
        code = (code << 1) | br.bit()
        if (l, code) in tbl:
            i = tbl[(l, code)]
            if i < 2:
                return i
            return (1 << (i - 1)) | br.bits(i - 1)
    raise ValueError("bad huffman code")


def unpack(blob):
    if blob[:3] != b"RNC" or blob[3] != 1:
        raise ValueError("not an RNC1 blob")
    ulen, plen, ucrc, pcrc = struct.unpack(">IIHH", blob[4:16])
    if crc16(blob[18:18 + plen]) != pcrc:
        raise ValueError("packed CRC mismatch")
    br = BitReader(blob, 18)
    br.bits(2)
    out = bytearray()
    while len(out) < ulen:
        raw = read_table(br)
        dist = read_table(br)
        ln = read_table(br)
        chunks = br.bits(16)
        while True:
            n = huff(br, raw)
            for _ in range(n):
                out.append(br.byte())
            chunks -= 1
            if chunks <= 0:
                break
            pos = huff(br, dist) + 1
            n = huff(br, ln) + 2
            for _ in range(n):
                out.append(out[-pos])
    out = bytes(out[:ulen])
    if crc16(out) != ucrc:
        raise ValueError("unpacked CRC mismatch")
    return out


if __name__ == "__main__":
    data = open(sys.argv[1], "rb").read()
    res = unpack(data)
    open(sys.argv[2], "wb").write(res)
    print("%s: %d -> %d bytes, CRC ok" % (sys.argv[1], len(data), len(res)))
