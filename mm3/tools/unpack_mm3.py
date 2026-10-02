#!/usr/bin/env python3
"""Unpack MM3.EXE: NWC LZW self-extractor (docs/mm3-re.md section 1) -> EXEPACK -> plain MZ executable.

usage: unpack_mm3.py MM3.EXE [stage1.exe [mm3_unpacked.exe]]
"""
import struct
import sys


def u16(b, o):
    return struct.unpack_from('<H', b, o)[0]


def lzw_decode(data, start):
    """Variable-width (9..12 bit) LSB-first LZW; 256 = clear, 257 = end."""
    out = bytearray()
    bitbuf = 0
    nbits = 0
    pos = start
    width = 9
    table = None
    nxt = 258
    prev = None

    def reset():
        return {}, 258, 9

    table, nxt, width = reset()
    while True:
        while nbits < width:
            if pos >= len(data):
                return bytes(out)
            bitbuf |= data[pos] << nbits
            pos += 1
            nbits += 8
        code = bitbuf & ((1 << width) - 1)
        bitbuf >>= width
        nbits -= width
        if code == 257:
            break
        if code == 256:
            table, nxt, width = reset()
            prev = None
            continue
        if code < 256:
            entry = bytes([code])
        elif code in table:
            entry = table[code]
        elif code == nxt and prev is not None:
            entry = prev + prev[:1]
        else:
            raise ValueError('bad LZW code %d at output %d' % (code, len(out)))
        out += entry
        if prev is not None and nxt < 4096:
            table[nxt] = prev + entry[:1]
            nxt += 1
            if nxt >= (1 << width) and width < 12:
                width += 1
        prev = entry
    return bytes(out)


def unpack_outer(f):
    cparhdr = u16(f, 8)
    stub = cparhdr * 16
    hdr = u16(f, stub + 0x4B)
    e_cblp, e_cp = u16(f, 2), u16(f, 4)
    # stored original header
    o_cblp, o_cp, o_cparhdr = u16(f, hdr + 2), u16(f, hdr + 4), u16(f, hdr + 8)
    header_bytes = o_cparhdr * 16
    declared = (o_cp - 1) * 512 + (o_cblp or 512)
    module_len = declared - header_bytes
    image = lzw_decode(f, hdr + header_bytes)
    packed_end = (e_cp - 1) * 512 + (e_cblp or 512)
    return f[hdr:hdr + header_bytes], image[:module_len], hdr, header_bytes, declared, packed_end


def unpack_exepack(exe):
    """MS EXEPACK (header with skip_len) -> plain MZ file bytes."""
    cparhdr = u16(exe, 8)
    base = cparhdr * 16
    ip, cs = u16(exe, 0x14), u16(exe, 0x16)
    ep = base + cs * 16
    real_ip, real_cs, _mem, exepack_size, real_sp, real_ss, dest_len = struct.unpack_from('<7H', exe, ep)
    if u16(exe, ep + 0x10) == 0x4252:
        hdr_len, skip = 0x12, u16(exe, ep + 0x0E) or 1
    elif u16(exe, ep + 0x0E) == 0x4252:
        hdr_len, skip = 0x10, 1
    else:
        raise ValueError('no EXEPACK signature')
    # compressed stream: from the load module start up to the EXEPACK header, trailing 0xFF = padding
    buf = exe[base:ep]
    src = len(buf) - 1
    while buf[src] == 0xFF:
        src -= 1
    out = bytearray()
    while True:
        cmd = buf[src]
        count = buf[src - 2] | (buf[src - 1] << 8)
        src -= 3
        op = cmd & 0xFE
        if op == 0xB0:
            fill = buf[src]
            src -= 1
            out += bytes([fill]) * count
        elif op == 0xB2:
            out += bytes(buf[src - count + 1:src + 1][::-1])
            src -= count
        else:
            raise ValueError('bad EXEPACK opcode %02x' % cmd)
        if cmd & 1:
            break
    out.reverse()
    out = bytearray(buf[:src + 1]) + out   # the part before the last chunk is stored verbatim
    # relocation table sits at the end of the EXEPACK block (after the "Packed file is corrupt" message)
    msg = exe.find(b'Packed file is corrupt', ep)
    p = msg + len(b'Packed file is corrupt')
    relocs = []
    for seg in range(16):
        n = u16(exe, p)
        p += 2
        for _ in range(n):
            relocs.append((seg * 0x1000, u16(exe, p)))
            p += 2
    return bytes(out), relocs, (real_ip, real_cs, real_sp, real_ss, dest_len), ep + exepack_size


def build_mz(image, relocs, real_ip, real_cs, real_sp, real_ss, minalloc=0, maxalloc=0xFFFF):
    nrel = len(relocs)
    hdr_size = (0x1C + nrel * 4 + 511) // 512 * 512  # 512-byte aligned header
    total = hdr_size + len(image)
    hdr = bytearray(hdr_size)
    struct.pack_into('<14H', hdr, 0, 0x5A4D, total % 512, (total + 511) // 512, nrel, hdr_size // 16, minalloc, maxalloc,
                     real_ss, real_sp, 0, real_ip, real_cs, 0x1C, 0)
    for i, (seg, off) in enumerate(relocs):
        struct.pack_into('<HH', hdr, 0x1C + i * 4, off, seg)
    return bytes(hdr) + image


def main():
    src = sys.argv[1]
    f = open(src, 'rb').read()
    hdr, img, hoff, hb, declared, packed_end = unpack_outer(f)
    stage1 = hdr + img
    overlay = f[max(packed_end, 0):]
    print('stage1: %d bytes, overlay %d bytes (from packed end %x)' % (len(stage1), len(overlay), packed_end))
    if len(sys.argv) > 2:
        open(sys.argv[2], 'wb').write(stage1)
    image, relocs, regs, _ = unpack_exepack(stage1)
    real_ip, real_cs, real_sp, real_ss, dest_len = regs
    print('EXEPACK: image %d bytes (dest_len %x paras), %d relocs, cs:ip=%04x:%04x ss:sp=%04x:%04x' % (
        len(image), dest_len, len(relocs), real_cs, real_ip, real_ss, real_sp))
    out = build_mz(image, relocs, real_ip, real_cs, real_sp, real_ss) + overlay
    if len(sys.argv) > 3:
        open(sys.argv[3], 'wb').write(out)
    print('output %d bytes' % len(out))


if __name__ == '__main__':
    main()
