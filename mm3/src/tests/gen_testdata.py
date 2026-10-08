#!/usr/bin/env python3
"""Builds a synthetic .CC archive (cipher + LZHUF members) and the expected contents for test_cc.c.
usage: gen_testdata.py OUTDIR
Reuses the reference decoder's model (tools/mm3_cc.py) for the encoder side, so a C round trip proves the C port
decodes what the Python reference decodes.  Writes OUTDIR/test.cc and OUTDIR/<NAME>.expected for each member."""
import os
import random
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tools"))
import mm3_cc as cc  # noqa: E402


def lzhuf_encode(data, fill=0x20):
    """Literal-only LZHUF encoder using the decoder's adaptive Huffman model (match codes are not generated)."""
    z = cc.Lzhuf(b"", 0)
    out = bytearray([fill, fill, (len(data) >> 8) & 255, len(data) & 255])
    acc, nb = 0, 0

    def put(bit):
        nonlocal acc, nb
        acc = (acc << 1) | bit
        nb += 1
        if nb == 8:
            out.append(acc)
            acc, nb = 0, 0

    def encode_char(c):
        bits = []
        node = z.prnt[c + cc.T]  # position of the leaf's slot; its parity is the branch taken
        while True:
            bits.append(node & 1)
            node = z.prnt[node]
            if node == cc.R:
                break
        for b in reversed(bits):
            put(b)
        z.update(c)

    for byte in data:
        encode_char(byte)
    # flush
    while nb:
        put(0)
    return bytes(out)


def build(members):
    n = len(members)
    toc = bytearray()
    payloads = bytearray()
    base = 2 + n * 8
    for name, blob in members:
        off = base + len(payloads)
        toc += struct.pack("<H", cc.name_id(name)) + bytes([off & 255, (off >> 8) & 255, off >> 16]) + struct.pack("<H", len(blob)) + b"\0"
        payloads += blob
    # inverse of: dec = ((c<<2 | c>>6) & 255) - (0x54 + 0x99*i)
    out = bytearray()
    for i, p in enumerate(toc):
        v = (p + 0x54 + 0x99 * i) & 255
        out.append(((v >> 2) | (v << 6)) & 255)
    return struct.pack("<H", n) + bytes(out) + bytes(payloads)


def main():
    outdir = sys.argv[1]
    os.makedirs(outdir, exist_ok=True)
    rnd = random.Random(1)
    members = []
    plain = bytes(rnd.choice(b"ABCDEFGH \n") for _ in range(3000))
    skew = bytes(rnd.choice(b"aaaaaaaabc") for _ in range(20000))
    noise = bytes(rnd.randrange(256) for _ in range(900))
    stored = b"stored member, not lzhuf"
    for name, data in (("TEXT.MAZ", plain), ("SKEW.DAT", skew), ("NOISE.BIN", noise)):
        packed = lzhuf_encode(data)
        assert cc.lzhuf_decode(packed) == data, name
        members.append((name, packed))
        open(os.path.join(outdir, name + ".expected"), "wb").write(data)
    members.append(("MAZE.NAM", stored))
    open(os.path.join(outdir, "MAZE.NAM.expected"), "wb").write(stored)
    open(os.path.join(outdir, "test.cc"), "wb").write(build(members))


if __name__ == "__main__":
    main()
