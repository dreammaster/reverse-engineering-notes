"""
Standalone tool (run with a normal Python 3, NOT via run_ida_script.ps1
-- it just reads game files off disk, no IDA/idb involvement).

Extracts a raw indexed-color region from PICTURES.VGA and writes it as a
PNG, using only the standard library (zlib for PNG's deflate stream).
With --palette, writes a proper indexed-color (PNG color type 3) image
using the game's real VGA DAC palette; without it, falls back to a
grayscale rendering (index value as gray level).

Offset/width/height come from the g_pictureDir table documented in
docs/file-formats.md and named in name_picture_system.py: 16-byte
entries at DS:0x782E (linear 0x3508E), word width @+8, word height
@+0xA, dword file offset (low @+0xC, high @+0xE).

The real palette lives in WORLD.DAT at file offset 0x8270A, 768 bytes
(256 RGB triples, 6-bit VGA DAC values 0-63 each) -- found by tracing
ShowIntroPicture's palette load (reads it via the FileEntry resource
stub sub_27CB0, see docs/file-formats.md's "Palette" section).

Usage:
    py extract_pic.py game/PICTURES.VGA 0x0 318 198 splash.png ^
        --palette game/WORLD.DAT 0x8270A
"""
import struct
import sys
import zlib


def _chunk(tag, payload):
    return (
        struct.pack(">I", len(payload))
        + tag
        + payload
        + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)
    )


def write_gray_png(path, width, height, data):
    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0)
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # no filter
        raw.extend(data[y * width:(y + 1) * width])
    idat = zlib.compress(bytes(raw), 9)
    with open(path, "wb") as f:
        f.write(sig)
        f.write(_chunk(b"IHDR", ihdr))
        f.write(_chunk(b"IDAT", idat))
        f.write(_chunk(b"IEND", b""))


def write_indexed_png(path, width, height, data, palette_rgb):
    """palette_rgb: 256 (r,g,b) tuples, each component already 0-255."""
    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 3, 0, 0, 0)
    plte = b"".join(struct.pack("BBB", r, g, b) for r, g, b in palette_rgb)
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # no filter
        raw.extend(data[y * width:(y + 1) * width])
    idat = zlib.compress(bytes(raw), 9)
    with open(path, "wb") as f:
        f.write(sig)
        f.write(_chunk(b"IHDR", ihdr))
        f.write(_chunk(b"PLTE", plte))
        f.write(_chunk(b"IDAT", idat))
        f.write(_chunk(b"IEND", b""))


def load_palette(path, offset):
    with open(path, "rb") as f:
        f.seek(offset)
        raw = f.read(768)
    if len(raw) < 768:
        raise ValueError(f"short palette read: {len(raw)} < 768")
    # 6-bit VGA DAC values (0-63) -> 8-bit (0-255)
    return [
        (raw[i] * 255 // 63, raw[i + 1] * 255 // 63, raw[i + 2] * 255 // 63)
        for i in range(0, 768, 3)
    ]


def extract(vga_path, offset, width, height, out_path, palette=None):
    with open(vga_path, "rb") as f:
        f.seek(offset)
        data = f.read(width * height)
    if len(data) < width * height:
        print(f"WARNING: short read ({len(data)} < {width*height})")
        data = data + b"\x00" * (width * height - len(data))
    if palette:
        write_indexed_png(out_path, width, height, data, palette)
    else:
        write_gray_png(out_path, width, height, data)
    print(f"wrote {out_path} ({width}x{height}, offset {offset:#x}, "
          f"{'indexed' if palette else 'grayscale'})")


if __name__ == "__main__":
    args = sys.argv[1:]
    palette = None
    if "--palette" in args:
        i = args.index("--palette")
        pal_path, pal_offset = args[i + 1], int(args[i + 2], 0)
        palette = load_palette(pal_path, pal_offset)
        del args[i:i + 3]
    vga, offset, width, height, out = args
    extract(vga, int(offset, 0), int(width), int(height), out, palette)
