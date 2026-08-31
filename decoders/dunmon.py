#!/usr/bin/env python3
"""Decode `DUNMONA.BSV` / `DUNMONB.BSV` -- the first-person dungeon-view
monster sprites (`DUN.EXE` / `bmDUNG`).

    python decoders/dunmon.py [C:\\games\\lota\\DUNMONA.BSV] [outdir]

`DUNMONA` = the monster set for dungeon levels 0-3, `DUNMONB` = levels
4-7 (`loadDungeonMonsters` picks on `ds:1AE2h >= 0x400`).

Both files: 7-byte BSAVE header `[FD][seg][off][len]`, then a 14136-byte
payload = **exactly 6 blocks x 2356 B (0x49A words)**, one per
monster-type slot. `loadDungeonMonsters` `BLOAD`s the payload into
`spriteBank` at word offset 0x1240; `DUNOBJ.BSV` region A word
`0x190 + type*2` points at `0x1240 + type*0x49A`.

Each block:
    words 0..4 : frame-offset table (word offsets rel. to block start)
                 = 9, 725, 973, 1083, 1148   -- index = view depth P
                 (0 = adjacent ... 4 = farthest)
    word  5    : 0x49A  -- block length / end marker
    words 6..8 : zero padding
    word  9 .. : the 5 sprites, back to back, each a stock Microsoft
                 BASIC GET/PUT array:
                     dw xBits     ; width in BITS = pixelWidth * 2 (2 bpp)
                     dw yRows     ; height in scanlines
                     db yRows * ceil(xBits/8) bytes
                 Pixels: 2 bpp packed left-to-right (MSB pair = leftmost
                 pixel), rows top-to-bottom, byte-aligned per row, stored
                 LINEARLY -- NOT CGA field-interleaved (PUT splits the
                 even/odd fields itself). Colour 0 = transparent.

Frame geometry (identical in every block of both files):
    P0  82 x 68   P1  48 x 41   P2  32 x 27   P3  24 x 21   P4  16 x 14
"""
import struct
import sys
import os

BLOCK_WORDS = 0x49A          # 1178 words = 2356 bytes per monster block
N_BLOCKS = 6
N_DEPTHS = 5
PAL = [(0, 0, 0), (0x55, 0xFF, 0xFF), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0xFF)]


def load_payload(path):
    d = open(path, "rb").read()
    if d[:1] == b"\xFD":
        seg, off, ln = struct.unpack("<HHH", d[1:7])
        p = d[7:7 + ln]
    else:
        p = d
    assert len(p) == BLOCK_WORDS * 2 * N_BLOCKS, \
        f"{path}: payload {len(p)} != {BLOCK_WORDS*2*N_BLOCKS}"
    return p


def block_frames(block):
    """Return [(xbits, yrows, bytes), ...] for the 5 depth frames."""
    tbl = struct.unpack("<6H", block[:12])
    out = []
    for p in range(N_DEPTHS):
        o = tbl[p] * 2
        xb, yr = struct.unpack("<HH", block[o:o + 4])
        bpr = (xb + 7) // 8
        data = block[o + 4:o + 4 + bpr * yr]
        out.append((xb, yr, data))
    return out


def to_indices(xbits, yrows, data):
    """GET-array bitmap -> yrows lists of (xbits//2) palette indices."""
    bpr = (xbits + 7) // 8
    px = xbits // 2
    rows = []
    for y in range(yrows):
        base = y * bpr
        row = []
        for x in range(px):
            b = data[base + (x >> 2)]
            row.append((b >> (6 - 2 * (x & 3))) & 3)
        rows.append(row)
    return rows


def write_bmp(path, rows, scale=1):
    h = len(rows)
    w = len(rows[0]) if h else 0
    W, H = w * scale, h * scale
    rb = (W * 3 + 3) & ~3
    pad = rb - W * 3
    size = rb * H
    with open(path, "wb") as f:
        f.write(b"BM" + struct.pack("<IHHI", 14 + 40 + size, 0, 0, 54))
        f.write(struct.pack("<IiiHHIIiiII", 40, W, H, 1, 24, 0, size,
                            2835, 2835, 0, 0))
        for y in range(H - 1, -1, -1):
            srow = rows[y // scale]
            for x in range(W):
                r, g, b = PAL[srow[x // scale]]
                f.write(bytes((b, g, r)))
            f.write(b"\0" * pad)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\DUNMONA.BSV"
    outdir = sys.argv[2] if len(sys.argv) > 2 else "."
    os.makedirs(outdir, exist_ok=True)
    stem = os.path.splitext(os.path.basename(path))[0].lower()
    p = load_payload(path)
    upscale = {0: 4, 1: 5, 2: 7, 3: 9, 4: 12}
    for b in range(N_BLOCKS):
        block = p[b * BLOCK_WORDS * 2:(b + 1) * BLOCK_WORDS * 2]
        frames = block_frames(block)
        dims = " ".join(f"P{p_}={xb//2}x{yr}" for p_, (xb, yr, _) in enumerate(frames))
        print(f"{stem} block {b}: {dims}")
        for pd, (xb, yr, data) in enumerate(frames):
            rows = to_indices(xb, yr, data)
            out = os.path.join(outdir, f"{stem}_m{b}_P{pd}.bmp")
            write_bmp(out, rows, scale=upscale[pd])
    print(f"wrote {N_BLOCKS*N_DEPTHS} BMPs to {outdir}")


if __name__ == "__main__":
    main()
