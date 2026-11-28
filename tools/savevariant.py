#!/usr/bin/env python3
"""Puts the data (VBIN) that tests/vis/savevariant.exe wrote into a savegame container again: the entries of a savegame are
the data and the screenshot; the data of the savegame `template` are replaced, the screenshot stays.
Usage: tools/savevariant.py <template savegame.dat> <new data file> <output savegame.dat>"""
import hashlib
import struct
import sys

PASSWORD = b"SAVEGAMEPWD30"


def entries(path):
    data = open(path, "rb").read()
    count = struct.unpack(">I", data[4:8])[0]
    size = count * 16 + 6
    key = hashlib.md5(PASSWORD).hexdigest()[:16].encode()
    plain = bytes(data[8 + i] ^ key[i % 16] for i in range(size))
    base = 8 + size
    result = []
    for i in range(count):
        offset, length, stored, flag = struct.unpack(">IIII", plain[3 + i * 16:3 + i * 16 + 16])
        result.append((data[base + offset:base + offset + length], stored, flag))
    return result


def main():
    template, new_data, output = sys.argv[1:4]
    parts = entries(template)
    parts[0] = (open(new_data, "rb").read(),) + parts[0][1:]
    directory = bytearray(b"HDR")
    body = bytearray()
    for content, _, flag in parts:
        directory += struct.pack(">IIII", len(body), len(content), len(content), flag)
        body += content
    directory += b"END"
    key = hashlib.md5(PASSWORD).hexdigest()[:16].encode()
    encrypted = bytes(directory[i] ^ key[i % 16] for i in range(len(directory)))
    open(output, "wb").write(b"VIS3" + struct.pack(">I", len(parts)) + encrypted + bytes(body))
    print("wrote", output, len(body) + len(encrypted) + 8, "bytes")


main()
