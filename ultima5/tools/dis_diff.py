"""Disassemble the same window of an original and a patched binary side by side (needs ndisasm).

python tools/dis_diff.py ORIG PATCHED FILE_OFFSET [LENGTH] [BASE]   (BASE is subtracted for the shown address, e.g. 0x800 for ULTIMA.EXE)
"""
import subprocess
import sys


def dis(path, off, length, base):
    # start a little before the window so that the first instruction is likely aligned
    out = subprocess.run(["ndisasm", "-b", "16", "-o", hex(off - base), "-e", str(off), "-k", "0,0", path], capture_output=True, text=True)
    return out.stdout


def main():
    orig, new, off = sys.argv[1], sys.argv[2], int(sys.argv[3], 0)
    length = int(sys.argv[4], 0) if len(sys.argv) > 4 else 0x60
    base = int(sys.argv[5], 0) if len(sys.argv) > 5 else 0
    for tag, p in (("ORIGINAL", orig), ("PATCHED", new)):
        print("=== %s %s @ %#x" % (tag, p, off))
        r = subprocess.run(["ndisasm", "-b", "16", "-o", hex(off - base), "-e", str(off), p], capture_output=True, text=True).stdout.splitlines()
        shown = 0
        for line in r:
            parts = line.split()
            addr = int(parts[0], 16)
            if addr - (off - base) >= length:
                break
            print(line)


main()
