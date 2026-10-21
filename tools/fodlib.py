"""Prototype readers for Fountain of Dreams data files (used to validate the reverse engineering)."""
import struct, os

GAME = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "fod", "game")


def lzss(data, pos=0):
    """LZSS as in KEH lzssDecode: u32 output size, then flag-byte stream. Ring 4096 init 0xFEE, space-filled."""
    size = struct.unpack_from("<I", data, pos)[0]
    pos += 4
    ring = bytearray(b"\x20" * 4096)
    r = 0xFEE
    out = bytearray()
    flags = 0
    bits = 0
    while len(out) < size:
        if bits == 0:
            flags = data[pos]; pos += 1; bits = 8
        bit = flags & 1; flags >>= 1; bits -= 1
        if bit:
            b = data[pos]; pos += 1
            out.append(b); ring[r] = b; r = (r + 1) & 0xFFF
        else:
            lo = data[pos]; hi = data[pos + 1]; pos += 2
            off = lo | ((hi & 0xF0) << 4)
            for i in range((hi & 0x0F) + 3):
                b = ring[(off + i) & 0xFFF]
                out.append(b); ring[r] = b; r = (r + 1) & 0xFFF
                if len(out) >= size:
                    break
    return bytes(out[:size])


def toc(name, count):
    d = open(os.path.join(GAME, name), "rb").read()
    ents = [struct.unpack_from("<HH", d, i * 4) for i in range(count)]
    return d, ents


def chunk(name, count, idx, compressed=False):
    """Raw (MAP) or LZSS-decoded (SCR/ANI?) chunk idx of a *MAP/*SCR/*ANI file."""
    d, ents = toc(name, count)
    off, size = ents[idx]
    if size == 0:
        return None
    raw = d[off * 16: off * 16 + size]
    return lzss(raw) if compressed else raw


if __name__ == "__main__":
    for idx in range(28):
        m = chunk("FMAP", 28, idx)
        if m:
            w, h, flags = struct.unpack_from("<HHH", m, 0)
            print("FMAP", idx, len(m), "w=%d h=%d flags=%04X" % (w, h, flags),
                  "ofs", struct.unpack_from("<5H", m, 0x404), "counts", struct.unpack_from("<3H", m, 0x40E),
                  "expected cells end", 0x414 + w * h * 8)
