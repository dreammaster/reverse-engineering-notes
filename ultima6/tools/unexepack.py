#!/usr/bin/env python3
"""
Unpack a Microsoft EXEPACK-compressed DOS executable ("RB" signature stub) back
into a plain MZ executable, so IDA can analyse the real code.

Ultima 6's GAME.EXE is packed this way (the stub's first bytes disassemble as
`push dx / inc dx` because they are really the "RB" signature).

Layout of a packed EXE (all offsets within the load image, i.e. after the MZ
header):

    [packed data .......... ][ stub: 9-word header | unpacker code | relocs ]
                              ^ CS:0000 from the MZ header

    stub header words: real_IP, real_CS, mem_start, exepack_size, real_SP,
                       real_SS, dest_len (paragraphs), skip_len, 'RB'

The packed data is decoded *backwards* from its last non-0xFF byte into a
buffer of dest_len paragraphs, written backwards from its end.  Commands (read
backwards): <fill byte> <count16> <cmd>  with cmd & 0xFE == 0xB0 (fill), or
<count16> <cmd> followed by count literal bytes with cmd & 0xFE == 0xB2 (copy);
bit 0 of cmd marks the last command.  The relocation table follows the stub
code as 16 groups (one per 64K of image) of <count16> <offset16 * count>.

Usage: unexepack.py <packed.exe> <unpacked.exe>
"""

import struct
import sys


def unpack(data):
    sig, last, pages, nreloc, hdr, minalloc, maxalloc, ss, sp, csum, ip, cs, rtoff, ovl = struct.unpack(
        "<2sHHHHHHHHHHHHH", data[:28])
    if sig != b"MZ":
        raise ValueError("not an MZ executable")
    hdr_size = hdr * 16
    img_end = (pages - 1) * 512 + (last or 512)
    image = data[hdr_size:img_end]
    stub = cs * 16 + ip * 0 + 0           # stub header lives at CS:0000
    (real_ip, real_cs, mem_start, exepack_size, real_sp, real_ss, dest_len, skip_len,
     rb) = struct.unpack("<8H2s", image[stub:stub + 18])
    if rb != b"RB":
        raise ValueError("no EXEPACK signature at CS:0000")

    # Packed data ends at the stub; trailing 0xFF bytes are paragraph padding.
    src = stub - skip_len * 16 + 15
    while image[src] == 0xFF:
        src -= 1
    dest = bytearray(dest_len * 16)
    dst = len(dest) - skip_len * 16 + 15
    while True:
        cmd = image[src]
        count = image[src - 2] | (image[src - 1] << 8)
        src -= 3
        op = cmd & 0xFE
        if op == 0xB0:
            fill = image[src]
            src -= 1
            for _ in range(count):
                dest[dst] = fill
                dst -= 1
        elif op == 0xB2:
            for _ in range(count):
                dest[dst] = image[src]
                dst -= 1
                src -= 1
        else:
            raise ValueError("bad EXEPACK command %02x at %x" % (cmd, src))
        if cmd & 1:
            break
    # The head of the file that the packer left untouched is carried over as is.
    dest[:src + 1] = image[:src + 1]

    # Relocation table: starts where the unpacker code ends.  The unpacker
    # ends with the "Packed file is corrupt" message just before it; every
    # known variant of the stub puts it at offset exepack_size - table size, so
    # locate it by parsing backwards-compatible: scan for the 16-group table
    # that ends exactly at exepack_size.
    stub_bytes = image[stub:stub + exepack_size]
    relocs = None
    for start in range(18, len(stub_bytes) - 31, 1):
        pos = start
        out = []
        ok = True
        for grp in range(16):
            if pos + 2 > len(stub_bytes):
                ok = False
                break
            cnt = struct.unpack_from("<H", stub_bytes, pos)[0]
            pos += 2
            if pos + 2 * cnt > len(stub_bytes):
                ok = False
                break
            for i in range(cnt):
                off = struct.unpack_from("<H", stub_bytes, pos + 2 * i)[0]
                out.append((grp * 0x1000, off))
            pos += 2 * cnt
        if ok and pos == len(stub_bytes):
            # Prefer the table that begins right after the printable message.
            prefix = stub_bytes[max(18, start - 8):start]
            if relocs is None or prefix.endswith(b"corrupt") or prefix[-1:] in b"\x00":
                relocs = out
                if prefix.endswith(b"corrupt"):
                    break
    if relocs is None:
        raise ValueError("relocation table not found")

    # Build the plain MZ.
    n = len(relocs)
    header_len = (0x1C + 4 * n + 511) // 512 * 512
    total = header_len + len(dest)
    need_paras = max(real_ss + (real_sp + 15) // 16, dest_len) - dest_len
    new_min = max(need_paras, 0)
    mz = struct.pack("<2sHHHHHHHHHHHHH", b"MZ", total % 512, (total + 511) // 512, n, header_len // 16,
                     new_min, 0xFFFF, real_ss, real_sp, 0, real_ip, real_cs, 0x1C, 0)
    mz += b"".join(struct.pack("<HH", off, seg) for seg, off in relocs)
    mz = mz.ljust(header_len, b"\0")
    return mz + bytes(dest)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1
    with open(sys.argv[1], "rb") as f:
        out = unpack(f.read())
    with open(sys.argv[2], "wb") as f:
        f.write(out)
    print("wrote %s (%d bytes)" % (sys.argv[2], len(out)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
