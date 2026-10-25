"""Print the original disassembly next to the C port for ASM-marked functions (manual review aid).

    python audit/asm_review.py 08ca 08e6 ...        (ULTIMA.EXE code offsets, hex)
"""
import json
import os
import subprocess
import sys

import audit_functions as a

EXE = os.path.join(a.GAME_DIR, "ULTIMA.EXE")


def main():
    cf = a.load_c_functions()
    sizes = {f["start"] - 0x10000: f["size"] for f in json.load(open(os.path.join(a.OUT, "ULTIMA.json")))["functions"]}
    for arg in sys.argv[1:]:
        off = int(arg, 16)
        size = sizes.get(off, 64)
        info = cf[("ULTIMA", off)][0]
        print("=" * 100)
        print("%04x  %s  (%d bytes)   %s:%d" % (off, info["name"], size, info["file"], info["line"]))
        out = subprocess.run(["ndisasm", "-b16", "-o", hex(off), "-e", str(0x800 + off), EXE], capture_output=True, text=True).stdout.splitlines()
        for line in out:
            addr = int(line.split()[0], 16)
            if addr >= off + size:
                break
            print("   " + line)
        src = open(os.path.join(a.SRC, info["file"]), encoding="utf-8", errors="replace").read().splitlines()
        i = info["line"] - 1
        while i > 0 and src[i - 1].lstrip().startswith("//"):
            i -= 1
        print("--- C")
        print("\n".join(src[i:info["line"] - 1 + info["lines"]]))


if __name__ == "__main__":
    main()
