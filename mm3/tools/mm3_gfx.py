#!/usr/bin/env python3
"""Export MM3 sprite resources (.ICN .FAC .OUT .VGA .TIL .BRD .PIC .MON) and raw screens (.RAW) to PNG.

usage: mm3_gfx.py FILE_OR_DIR OUTDIR [MM3.CC]        (the palette comes from member 8F99h of MM3.CC at offset 39Ch)
Format: docs/mm3-re.md section 5 (literal / skip / fill scanline codec, two layers per frame).
"""
import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm3_cc  # noqa: E402


def png(path, w, h, pixels, palette):
    """pixels: bytes of palette indices (0 = transparent); writes an RGBA PNG."""
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        for i in pixels[y * w:(y + 1) * w]:
            r, g, b = palette[i]
            raw += bytes((r, g, b, 0 if i == 0 else 255))

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) +
                chunk(b"IDAT", zlib.compress(bytes(raw), 6)) + chunk(b"IEND", b""))


def decode_layer(d, off):
    """-> (x_off, y_off, w, h, rows) where rows is a list of bytearrays of width w (0 = transparent)."""
    x_off, w, y_off, h = struct.unpack_from("<4H", d, off)
    p = off + 8
    rows = []
    for _ in range(h):
        row = bytearray(w)
        ln = struct.unpack_from("<H", d, p)[0]
        if ln == 0:
            p += 2
            rows.append(row)
            continue
        end = p + 2 + ln
        x = struct.unpack_from("<H", d, p + 2)[0]
        q = p + 4
        while q < end:
            op = d[q]
            q += 1
            if op < 0x80:
                n = op + 1
                for k in range(n):
                    if x + k < w:
                        row[x + k] = d[q + k]
                q += n
                x += n
            elif op < 0xC0:
                x += (op & 0x3F) + 1
            else:
                n = (op & 0x3F) + 3
                v = d[q]
                q += 1
                for k in range(n):
                    if x + k < w:
                        row[x + k] = v
                x += n
        p = end
        rows.append(row)
    return x_off, y_off, w, h, rows


def decode_sprite(d):
    """-> list of (w, h, pixels) frames (layers composited into their union rectangle)."""
    n = struct.unpack_from("<H", d, 0)[0]
    frames = []
    for i in range(n):
        o1, o2 = struct.unpack_from("<2H", d, 2 + 4 * i)
        layers = [decode_layer(d, o1)]
        if o2:
            layers.append(decode_layer(d, o2))
        x0 = min(l[0] for l in layers)
        y0 = min(l[1] for l in layers)
        x1 = max(l[0] + l[2] for l in layers)
        y1 = max(l[1] + l[3] for l in layers)
        w, h = x1 - x0, y1 - y0
        canvas = bytearray(w * h)
        for lx, ly, lw, lh, rows in layers:
            for yy, row in enumerate(rows):
                base = (ly - y0 + yy) * w + (lx - x0)
                for xx, v in enumerate(row):
                    if v:
                        canvas[base + xx] = v
        frames.append((w, h, bytes(canvas)))
    return frames


def load_palette(cc_path):
    cc = open(cc_path, "rb").read()
    for ident, off, size in mm3_cc.read_toc(cc):
        if ident == 0x8F99:
            mod, _ = mm3_cc.member(cc, off, size)
            raw = mod[0x39C:0x39C + 768]
            return [(raw[3 * i] * 4, raw[3 * i + 1] * 4, raw[3 * i + 2] * 4) for i in range(256)]
    raise SystemExit("palette module not found")


def export(path, outdir, palette):
    d = open(path, "rb").read()
    base = os.path.splitext(os.path.basename(path))[0]
    os.makedirs(outdir, exist_ok=True)
    if len(d) == 64000:
        png(os.path.join(outdir, base + ".png"), 320, 200, d, palette)
        return 1
    frames = decode_sprite(d)
    for i, (w, h, px) in enumerate(frames):
        if w and h:
            png(os.path.join(outdir, "%s_%02d.png" % (base, i)), w, h, px, palette)
    return len(frames)


def main():
    src, out = sys.argv[1], sys.argv[2]
    cc = sys.argv[3] if len(sys.argv) > 3 else r"D:\GOG Games\Might and Magic 3\MM3.CC"
    palette = load_palette(cc)
    files = [src] if os.path.isfile(src) else [os.path.join(src, f) for f in sorted(os.listdir(src)) if f.upper().endswith((".ICN", ".FAC", ".OUT", ".VGA", ".TIL", ".BRD", ".PIC", ".MON", ".RAW"))]
    for f in files:
        try:
            n = export(f, out, palette)
            print("%s: %d image(s)" % (os.path.basename(f), n))
        except Exception as e:  # noqa: BLE001
            print("%s: failed (%s)" % (os.path.basename(f), e))


if __name__ == "__main__":
    main()
