"""List every patched run between an original and a patched binary with the disassembly of both around it.

python tools/patch_sites.py ORIG PATCHED [BASE] [BEFORE] [AFTER]
Runs closer than 8 bytes are merged; each is disassembled from BEFORE bytes earlier (to let ndisasm resynchronise).
"""
import subprocess
import sys


def main():
    orig, new = sys.argv[1], sys.argv[2]
    base = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0
    before = int(sys.argv[4], 0) if len(sys.argv) > 4 else 0x18
    after = int(sys.argv[5], 0) if len(sys.argv) > 5 else 0x10
    a, b = open(orig, "rb").read(), open(new, "rb").read()
    n = min(len(a), len(b))
    runs = []
    for i in range(n):
        if a[i] != b[i]:
            if runs and i - runs[-1][1] <= 8:
                runs[-1][1] = i
            else:
                runs.append([i, i])
    for s, e in runs:
        print("##### %#x-%#x (cs/ovl offset %#x)" % (s, e, s - base))
        for tag, p in (("ORIG", orig), ("NEW ", new)):
            start = max(0, s - before)
            out = subprocess.run(["ndisasm", "-b", "16", "-o", hex(start - base), "-e", str(start), p], capture_output=True, text=True).stdout.splitlines()
            print("  " + tag)
            for line in out:
                addr = int(line.split()[0], 16)
                if addr > e - base + after:
                    break
                mark = "*" if s - base <= addr <= e - base else " "
                print("   %s %s" % (mark, line))
    if len(b) > len(a):
        print("##### patched file is %d bytes longer (appended at %#x)" % (len(b) - len(a), len(a)))


main()
