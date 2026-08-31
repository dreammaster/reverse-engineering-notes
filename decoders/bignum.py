#!/usr/bin/env python3
"""Decode `BIGNUM.DAT` -- the large-digit font `GMB2.EXE` ("Flip-Flop
Parlour") uses for its GOLD / BET / winnings readouts.

    python decoders/bignum.py [C:\\games\\lota\\BIGNUM.DAT] [out.bmp]

`BIGNUM.DAT` has **no BSAVE header** -- it is a raw **112 x 15 px CGA
mode-4 bitmap**: 28 bytes per row (2 bpp, 4 px/byte) x 15 rows = 420
bytes exactly (autocorrelation locks the row stride at 28). It is one
horizontal strip of digit glyphs (~10-11 px pitch), blitted a digit at
a time by `GMB2`'s `drawBigNumberPanel`.
"""
import struct
import sys

BPR = 28                 # bytes per row
W, H = BPR * 4, 15       # 112 x 15 px
PAL = [(0, 0, 0), (0x55, 0xFF, 0xFF), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0xFF)]


def decode(path):
    d = open(path, "rb").read()
    if len(d) != BPR * H:
        print(f"warning: expected {BPR*H} bytes, got {len(d)}", file=sys.stderr)
    scr = bytearray(W * H)
    for r in range(H):
        for c in range(BPR):
            b = d[r * BPR + c]
            for p in range(4):
                scr[r * W + c * 4 + p] = (b >> (6 - p * 2)) & 3
    return scr


def write_bmp(path, buf, w, h, scale=1):
    if scale > 1:
        big = bytearray(w * scale * h * scale)
        for y in range(h):
            for x in range(w):
                v = buf[y * w + x]
                for dy in range(scale):
                    rb = (y * scale + dy) * w * scale
                    for dx in range(scale):
                        big[rb + x * scale + dx] = v
        buf, w, h = big, w * scale, h * scale
    rb = (w * 3 + 3) & ~3
    pad = rb - w * 3
    size = rb * h
    with open(path, "wb") as f:
        f.write(b"BM" + struct.pack("<IHHI", 14 + 40 + size, 0, 0, 54))
        f.write(struct.pack("<IiiHHIIiiII", 40, w, h, 1, 24, 0, size,
                            2835, 2835, 0, 0))
        for y in range(h - 1, -1, -1):
            for x in range(w):
                r, g, b = PAL[buf[y * w + x]]
                f.write(bytes((b, g, r)))
            f.write(b"\0" * pad)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\BIGNUM.DAT"
    out = sys.argv[2] if len(sys.argv) > 2 else "bignum.bmp"
    scr = decode(path)
    # ASCII preview
    for r in range(H):
        print("".join("#" if scr[r * W + x] else "." for x in range(W)))
    write_bmp(out, scr, W, H, scale=1)
    write_bmp(out.replace(".bmp", "_6x.bmp"), scr, W, H, scale=6)
    print(f"\nwrote {out} ({W}x{H}) and _6x")


if __name__ == "__main__":
    main()
