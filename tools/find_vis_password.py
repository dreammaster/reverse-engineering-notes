#!/usr/bin/env python3
"""Finds the password of a game's data.vis in the engine executable of the game.
Usage: tools/find_vis_password.py <executable> <data.vis>
The directory of a VIS3 container (8 bytes: "VIS3" and the big endian count of the entries, then count * 16 + 6 bytes) is
XORed with the first 16 characters of the hexadecimal MD5 of the password, over and over; its text is "HDR", the entries and
"END". Every string of the executable (narrow, and wide as UTF-32: the password is a wide string in Deponia 2 and a narrow
one in Deponia 1) and every part of it that is at most 40 characters is tried until one makes the directory read HDR ... END."""
import hashlib
import re
import struct
import sys


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1
    binary = open(sys.argv[1], "rb").read()
    container = open(sys.argv[2], "rb").read()
    if container[:4] != b"VIS3":
        print("not a VIS3 container")
        return 1

    count = struct.unpack(">I", container[4:8])[0]
    size = count * 16 + 6
    cipher = container[8:8 + size]
    tail = size - 3

    def works(candidate):
        key = hashlib.md5(candidate).hexdigest()[:16].encode()
        # (the key is made of hexadecimal digits only, so the six bytes of "HDR" and "END" let about one string in sixteen million
        # through by chance: the entries are looked at too, each must lie inside the file)
        if bytes(cipher[j] ^ key[j % 16] for j in range(3)) != b"HDR" or bytes(cipher[tail + j] ^ key[(tail + j) % 16] for j in range(3)) != b"END":
            return False
        plain = bytes(cipher[j] ^ key[j % 16] for j in range(size))
        base = 8 + size
        for i in range(count):
            offset, length, _, _ = struct.unpack(">IIII", plain[3 + i * 16:3 + i * 16 + 16])
            if base + offset + length > len(container):
                return False
        return True

    seen = set()

    def search(strings, label):
        for run in strings:
            for a in range(min(len(run), 60)):
                for b in range(a + 3, min(len(run), a + 41) + 1):
                    candidate = run[a:b]
                    if candidate in seen:
                        continue
                    seen.add(candidate)
                    if works(candidate):
                        print("password %s (%s)" % (candidate.decode("ascii"), label))
                        return True
        return False

    if search((m.group() for m in re.finditer(rb"[\x20-\x7e]{3,200}", binary)), "narrow string"):
        return 0
    if search((m.group().decode("utf-32le").encode("ascii") for m in re.finditer(rb"(?:[\x20-\x7e]\x00\x00\x00){3,60}", binary)), "wide string"):
        return 0
    print("no password found (%d candidates)" % len(seen))
    return 2


if __name__ == "__main__":
    sys.exit(main())
