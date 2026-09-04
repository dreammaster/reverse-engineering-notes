"""
200.MONSTERS — DOS monster portraits.

Layout (from SYSTEM.INTERP's CONUNIT driver, docs/pmachine.md §CONUNIT):

  * PIC is 1-based; record n = bytes (PIC-1)*512 .. +512.  32 records,
    30..31 are disk slack.  WIZ1 uses PIC {1, 4..30}.
  * A portrait is a 6-wide x 5-tall grid of 16x8 px 1bpp tiles = 96 x 40 px.
  * tile t of a record is at offset  t * STRIDE  (STRIDE = word_13DC<<1 in
    the interpreter; = 16 unless a DOSBox check says otherwise).

Usage:
    python tools/monsters.py info   extracted/wiz1/200.MONSTERS
    python tools/monsters.py show   extracted/wiz1/200.MONSTERS <PIC> [stride] [cols] [rows]
    python tools/monsters.py pgm    extracted/wiz1/200.MONSTERS <PIC> out.pgm [stride]
"""
import sys

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


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else ""
    if cmd == "info":
        info(sys.argv[2])
    elif cmd == "show":
        show(*sys.argv[2:])
    elif cmd == "pgm":
        pgm(*sys.argv[2:])
    else:
        print(__doc__)
