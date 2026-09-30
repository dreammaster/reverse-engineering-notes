"""LZW decoder matching MM2's sub_12242 (variable 9..12 bit codes, LSB-first, 100h = clear, 101h = end)."""

def lzw_decode(data: bytes) -> bytes:
    out = bytearray()
    bitpos = 0
    nbits = 9
    next_code = 0x102
    limit = 0x200
    table = {}
    prev = None
    def read():
        nonlocal bitpos
        byte = bitpos >> 3
        if byte + 3 > len(data) + 2:
            return None
        chunk = int.from_bytes(data[byte:byte + 3].ljust(3, b"\0"), "little")
        v = (chunk >> (bitpos & 7)) & ((1 << nbits) - 1)
        bitpos += nbits
        return v
    while True:
        code = read()
        if code is None or code == 0x101:
            break
        if code == 0x100:
            nbits, next_code, limit, table, prev = 9, 0x102, 0x200, {}, None
            continue
        if code < 0x100:
            entry = bytes([code])
        elif code in table:
            entry = table[code]
        elif prev is not None and code == next_code:
            entry = prev + prev[:1]
        else:
            raise ValueError(f"bad code {code:#x} at bit {bitpos}")
        out += entry
        if prev is not None:
            table[next_code] = prev + entry[:1]
            next_code += 1
            if next_code >= limit and nbits < 12:
                nbits += 1
                limit <<= 1
        prev = entry
    return bytes(out)
