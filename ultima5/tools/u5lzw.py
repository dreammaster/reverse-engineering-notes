"""Ultima V resource decompression (the LZW used by the *.16 / *.4 / *.OOL ... files): u32 LE length, then an LSB-first
LZW stream with 9..12 bit codes (256 = clear, 257 = end), exactly as src/common/lzw.c."""
import struct


def lzw_decompress(data):
    out_len = struct.unpack_from("<I", data, 0)[0]
    pos = 4
    bitbuf = 0
    bits = 0
    prefix = [0] * 4096
    suffix = [0] * 4096
    code_size, next_code, prev = 9, 258, -1
    out = bytearray()

    def expand(code):
        s = []
        while code >= 256:
            s.append(suffix[code])
            code = prefix[code]
        s.append(code)
        s.reverse()
        return s

    while len(out) < out_len:
        while bits < code_size:
            if pos >= len(data):
                raise ValueError("truncated stream")
            bitbuf |= data[pos] << bits
            pos += 1
            bits += 8
        code = bitbuf & ((1 << code_size) - 1)
        bitbuf >>= code_size
        bits -= code_size
        if code == 256:
            code_size, next_code, prev = 9, 258, -1
            continue
        if code == 257:
            break
        if code < next_code:
            s = expand(code)
        elif code == next_code and prev >= 0:
            s = expand(prev)
            s.append(s[0])
        else:
            raise ValueError("bad code")
        out += bytes(s)
        if prev >= 0 and next_code < 4096:
            prefix[next_code] = prev
            suffix[next_code] = s[0]
            next_code += 1
            if code_size < 12 and next_code == (1 << code_size):
                code_size += 1
        prev = code
    if len(out) != out_len:
        raise ValueError("length mismatch %d != %d" % (len(out), out_len))
    return bytes(out)
