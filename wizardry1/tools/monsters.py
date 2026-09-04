"""
200.MONSTERS — DOS monster portraits.

Layout (from SYSTEM.INTERP's CONUNIT driver, docs/pmachine.md §CONUNIT):

  * PIC is 1-based; record n = bytes (PIC-1)*512 .. +512.  32 records,
    30..31 are disk slack.  WIZ1 uses PIC {1, 4..30}.
  * A portrait is a 6-wide x 5-tall grid of 16x8 px 1bpp tiles = 96 x 40 px.
  * tile t of a record is at offset  t * STRIDE  (STRIDE = word_13DC<<1 in
    the interpreter; = 16, confirmed live in DOSBox 2026-09-04 --
    word_13DC = 8, and cs:59F2 = 0x000,0x010..0x1D0).
  * bytes are plain 1bpp, MSB = leftmost pixel, byte 0 = left 8 px of the
    row, 2 bytes/row, 8 rows contiguous (blitter INTERP:0x1E9A, CGA mode
    6).  All 27 WIZ1 portraits render as coherent monster art at this
    geometry -- confirmed 2026-09-04.

Usage:
    python tools/monsters.py info   extracted/wiz1/200.MONSTERS
    python tools/monsters.py show   extracted/wiz1/200.MONSTERS <PIC> [stride] [cols] [rows]
    python tools/monsters.py pgm    extracted/wiz1/200.MONSTERS <PIC> out.pgm [stride]
    python tools/monsters.py png    extracted/wiz1/200.MONSTERS <PIC> out.png [scale]
    python tools/monsters.py sheet  extracted/wiz1/200.MONSTERS out.png [scale]
"""
import sys
import struct
import zlib

REC = 512
TILE_W, TILE_H = 16, 8
COLS, ROWS = 6, 5
STRIDE = 16


def tiles(rec, stride, cols, rows):
    """Return a (rows*TILE_H) x (cols*TILE_W) bit array."""
    h, w = rows * TILE_H, cols * TILE_W
    px = [[0] * w for _ in range(h)]
    for tr in range(rows):
        for tc in range(cols):
            t = tr * cols + tc
            base = t * stride
            for py in range(TILE_H):
                for bytecol in range(TILE_W // 8):
                    o = base + py * (TILE_W // 8) + bytecol
                    b = rec[o] if o < len(rec) else 0
                    for bit in range(8):
                        if (b >> (7 - bit)) & 1:
                            px[tr * TILE_H + py][tc * TILE_W + bytecol * 8 + bit] = 1
    return px


def show(path, pic, stride=STRIDE, cols=COLS, rows=ROWS):
    d = open(path, "rb").read()
    off = (int(pic) - 1) * REC
    px = tiles(d[off:off + REC], int(stride), int(cols), int(rows))
    for row in px:
        print("".join("#" if v else " " for v in row))


def info(path):
    d = open(path, "rb").read()
    print(f"{path}: {len(d)} bytes, {len(d)//REC} records of {REC}")
    for r in range(len(d) // REC):
        seg = d[r * REC:(r + 1) * REC]
        nz = sum(1 for b in seg if b)
        tag = "  (PIC %d)" % (r + 1) if nz else ""
        note = "  <- disk slack" if r >= 30 else ""
        print(f"  rec {r:2}: nonzero {nz:3}/512  hdr={seg[:16].hex()}{tag}{note}")


def pgm(path, pic, out, stride=STRIDE):
    d = open(path, "rb").read()
    off = (int(pic) - 1) * REC
    px = tiles(d[off:off + REC], int(stride), COLS, ROWS)
    h, w = len(px), len(px[0])
    with open(out, "wb") as f:
        f.write(f"P5\n{w} {h}\n255\n".encode())
        f.write(bytes(0 if v else 255 for row in px for v in row))
    print(f"wrote {out} ({w}x{h})")


def _png(path, w, h, gray):
    """Write an 8-bit grayscale PNG from a flat bytes-like `gray` (len w*h)."""
    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw.extend(gray[y * w:(y + 1) * w])
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 0, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        f.write(chunk(b"IEND", b""))


def _blit(gray, gw, px, x0, y0, scale):
    for y, prow in enumerate(px):
        for x, v in enumerate(prow):
            val = 255 if v else 0
            for dy in range(scale):
                base = (y0 + y * scale + dy) * gw + x0 + x * scale
                for dx in range(scale):
                    gray[base + dx] = val


def png(path, pic, out, scale=8):
    d = open(path, "rb").read()
    off = (int(pic) - 1) * REC
    px = tiles(d[off:off + REC], STRIDE, COLS, ROWS)
    scale = int(scale)
    w, h = len(px[0]) * scale, len(px) * scale
    gray = bytearray(w * h)
    _blit(gray, w, px, 0, 0, scale)
    _png(out, w, h, gray)
    print(f"wrote {out} ({w}x{h})")


def sheet(path, out, scale=3):
    d = open(path, "rb").read()
    scale, pad, cols = int(scale), 6, 6
    pics = [r + 1 for r in range(len(d) // REC)
            if r < 30 and any(d[r * REC:(r + 1) * REC])]
    cw, ch = COLS * TILE_W * scale + pad, ROWS * TILE_H * scale + pad
    rows = (len(pics) + cols - 1) // cols
    w, h = cols * cw, rows * ch
    gray = bytearray(b"\x40" * (w * h))
    for i, pic in enumerate(pics):
        off = (pic - 1) * REC
        px = tiles(d[off:off + REC], STRIDE, COLS, ROWS)
        _blit(gray, w, px, (i % cols) * cw + pad // 2,
              (i // cols) * ch + pad // 2, scale)
    _png(out, w, h, gray)
    print(f"wrote {out} ({w}x{h}, {len(pics)} portraits)")


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else ""
    if cmd == "info":
        info(sys.argv[2])
    elif cmd == "show":
        show(*sys.argv[2:])
    elif cmd == "pgm":
        pgm(*sys.argv[2:])
    elif cmd == "png":
        png(*sys.argv[2:])
    elif cmd == "sheet":
        sheet(*sys.argv[2:])
    else:
        print(__doc__)
