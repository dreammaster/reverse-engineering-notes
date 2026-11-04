#!/usr/bin/env python3
"""
Decoder for Cadaver *.PC1 pictures (LOGO.PC1, LOADING2.PC1).  See docs/formats.md.

File = sequence of blocks:  u16le length (0 = end), then `length` bytes of LZ stream.
Stream ops (INTRO.EXE seg000:0B09):
    c = byte
    c <  0x80 : copy c literal bytes
    c >= 0x80 : n = c & 0x7F  (n = u16 if c == 0xFF)  ; off = u16
                copy n bytes from  out[block_start + off ...]   (byte by byte, may overlap)
Decoded picture: 2 bytes (unused), 16 Atari-ST palette words (big endian 0x0RGB,
3 bits per channel), then 4000 x (4 plane words) ST-interleaved 16-pixel groups.

    python pc1.py in.pc1 out.png
"""
import struct, sys


def lz(data):
    out = bytearray()
    p = 0
    while True:
        (blen,) = struct.unpack_from('<H', data, p)
        p += 2
        if blen == 0:
            break
        end = p + blen
        base = len(out)
        while p < end:
            c = data[p]; p += 1
            if c < 0x80:
                out += data[p:p + c]; p += c
            else:
                if c == 0xFF:
                    n = struct.unpack_from('<H', data, p)[0]; p += 2
                else:
                    n = c & 0x7F
                off = struct.unpack_from('<H', data, p)[0]; p += 2
                for i in range(n):
                    out.append(out[base + off + i])
    return bytes(out)


def decode(data):
    """-> (palette[16] of (r,g,b) 0-255, pixels 320x200 bytes of colour index)"""
    raw = lz(data)
    pal = []
    for i in range(16):
        (w,) = struct.unpack_from('>H', raw, 2 + i * 2)
        pal.append((((w >> 8) & 7) * 255 // 7, ((w >> 4) & 7) * 255 // 7, (w & 7) * 255 // 7))
    px = bytearray(320 * 200)
    base = 34
    for g in range(4000):
        planes = struct.unpack_from('>4H', raw, base + g * 8)
        y, xg = divmod(g, 20)
        for b in range(16):
            c = 0
            for pl in range(4):
                c |= ((planes[pl] >> (15 - b)) & 1) << pl
            px[y * 320 + xg * 16 + b] = c
    return pal, bytes(px)


if __name__ == '__main__':
    from PIL import Image
    pal, px = decode(open(sys.argv[1], 'rb').read())
    im = Image.frombytes('P', (320, 200), px)
    im.putpalette([c for rgb in pal for c in rgb])
    im.save(sys.argv[2])
