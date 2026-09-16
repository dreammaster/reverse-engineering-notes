"""
Standalone tool (run with a normal Python 3, NOT via run_ida_script.ps1
-- it just reads game/PICTURES.VGA off disk, no IDA/idb involvement).

Extracts a raw indexed-color region from PICTURES.VGA and writes it as a
grayscale PNG (index value used directly as gray level -- no real VGA
palette recovered yet, but enough to reveal shape/structure), using only
the standard library (zlib for PNG's deflate stream).

Offset/width/height come from the g_pictureDir table documented in
docs/file-formats.md and named in name_picture_system.py: 16-byte
entries at DS:0x782E (linear 0x3508E), word width @+8, word height
@+0xA, dword file offset (low @+0xC, high @+0xE).

Usage:
    py extract_pic.py game/PICTURES.VGA 0x0 318 198 splash.png
"""
import struct
import sys
import zlib


def write_gray_png(path, width, height, data):
    def chunk(tag, payload):
        return (
            struct.pack(">I", len(payload))
            + tag
            + payload
            + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)
        )

    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0)
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # no filter
        raw.extend(data[y * width:(y + 1) * width])
    idat = zlib.compress(bytes(raw), 9)
    with open(path, "wb") as f:
        f.write(sig)
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", idat))
        f.write(chunk(b"IEND", b""))


def extract(vga_path, offset, width, height, out_path):
    with open(vga_path, "rb") as f:
        f.seek(offset)
        data = f.read(width * height)
    if len(data) < width * height:
        print(f"WARNING: short read ({len(data)} < {width*height})")
        data = data + b"\x00" * (width * height - len(data))
    write_gray_png(out_path, width, height, data)
    print(f"wrote {out_path} ({width}x{height}, offset {offset:#x})")


if __name__ == "__main__":
    vga = sys.argv[1]
    offset = int(sys.argv[2], 0)
    width = int(sys.argv[3])
    height = int(sys.argv[4])
    out = sys.argv[5]
    extract(vga, offset, width, height, out)
