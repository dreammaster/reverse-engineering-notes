"""ppm2png.py in.ppm out.png [scale]: converts a binary PPM (P6) to a PNG, enlarged by an integer scale (default 3). No dependencies."""
import struct
import sys
import zlib


def main():
    src, dst = sys.argv[1], sys.argv[2]
    scale = int(sys.argv[3]) if len(sys.argv) > 3 else 3
    data = open(src, 'rb').read()
    head = data.split(b'\n', 3)
    w, h = map(int, head[1].split())
    px = head[3]
    rows = bytearray()
    for y in range(h):
        row = bytearray()
        for x in range(w):
            row += px[(y * w + x) * 3:(y * w + x) * 3 + 3] * scale
        rows += (b'\0' + bytes(row)) * scale

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body))
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w * scale, h * scale, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(bytes(rows))) + chunk(b'IEND', b'')
    open(dst, 'wb').write(png)


main()
