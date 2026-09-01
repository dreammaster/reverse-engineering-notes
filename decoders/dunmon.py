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

The 5 "frames" per block are the 5 view DEPTHS (near..far), each a
hand-drawn smaller redraw -- NOT an animation cycle.  The 6 blocks are
6 distinct monsters, not frames of one (DUNMONA: streaker / two-headed
turtle / octopoid / one-eyed stalker / segmented worm / horned demon;
DUNMONB: dragon / horse-thing / horned swordsman / owlbear / caped
antenna-beast / fanged tentacle-thing).

NO palette animation exists.  drawViewSprite runs once per player action
(renderDungeonView is a per-turn redraw), the sprite is static, and CGA
colour is set ONCE by renderDungeonView -> rtm_FE29: port 0x3D8 = 0x0A
(graphics mode), 0x3D9 = 0x30 (palette 1: black/cyan/magenta/white,
black border), plus the matching BIOS INT 10h,AH=0Bh.  No register
cycling anywhere; the overworld does the same one-shot rtm_FE29 with two
bytes taken from the map data (ds:1E2A[+4] mode, [+2] colour).
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


def write_png(path, rows):
    """rows = list of list of (r,g,b)."""
    import zlib
    h = len(rows)
    w = len(rows[0])
    raw = bytearray()
    for r in rows:
        raw.append(0)
        for (R, G, B) in r:
            raw += bytes((R, G, B))

    def ch(t, d):
        return (struct.pack(">I", len(d)) + t + d
                + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF))
    open(path, "wb").write(
        b"\x89PNG\r\n\x1a\n"
        + ch(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
        + ch(b"IDAT", zlib.compress(bytes(raw), 9))
        + ch(b"IEND", b""))


def sheet(path, outfile, scale=2):
    """One PNG: 6 monster rows x 5 depth columns."""
    p = load_payload(path)
    bg = (30, 30, 40)
    cw, cht, pad = 90, 72, 4
    W = (cw * N_DEPTHS + pad) * scale
    H = (cht * N_BLOCKS + pad) * scale
    img = [[bg] * W for _ in range(H)]
    for b in range(N_BLOCKS):
        block = p[b * BLOCK_WORDS * 2:(b + 1) * BLOCK_WORDS * 2]
        for pd, (xb, yr, data) in enumerate(block_frames(block)):
            gx = pd * cw + pad // 2
            gy = b * cht + pad // 2
            for y, prow in enumerate(to_indices(xb, yr, data)):
                for x, idx in enumerate(prow):
                    col = bg if idx == 0 else PAL[idx]
                    for sy in range(scale):
                        for sx in range(scale):
                            Y, X = (gy + y) * scale + sy, (gx + x) * scale + sx
                            if 0 <= Y < H and 0 <= X < W:
                                img[Y][X] = col
    write_png(outfile, img)
    print(f"wrote {outfile}")


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    path = args[0] if args else r"C:\games\lota\DUNMONA.BSV"
    outdir = args[1] if len(args) > 1 else "."
    os.makedirs(outdir, exist_ok=True)
    stem = os.path.splitext(os.path.basename(path))[0].lower()
    p = load_payload(path)

    if "--sheet" in sys.argv:
        sheet(path, os.path.join(outdir, f"{stem}_sheet.png"))
        return

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
    print(f"wrote {N_BLOCKS*N_DEPTHS} BMPs to {outdir}  (--sheet for a combined PNG)")


if __name__ == "__main__":
    main()
