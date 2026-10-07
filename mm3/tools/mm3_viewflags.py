#!/usr/bin/env python3
"""Decode prepareIndoorView of mm3.asm: for each unrolled view-slot block, which face-flag bytes each wall style (1-7) sets.

usage: mm3_viewflags.py [mm3.asm]
Blocks appear in view-slot order (VIEW_DX/VIEW_DY order, docs/view.md).  A 'case N' handler of a block's 7-way jump table is the wall style N.
"""
import re
import sys


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "mm3.asm"
    lines = open(path, encoding="latin1").read().split("\n")
    s = next(i for i, l in enumerate(lines) if l.startswith("prepareIndoorView proc"))
    e = next(i for i in range(s, len(lines)) if lines[i].startswith("prepareIndoorView endp"))
    blocks = []
    cur = None
    for i in range(s, e):
        l = lines[i]
        m = re.search(r"jmp +cs:(jpt_[0-9A-F]+)\[bx\]", l)
        if m and "switch jump" in l:
            cur = {"jpt": m.group(1), "cases": {}}
            blocks.append(cur)
            continue
        if cur is None:
            continue
        m = re.search(r"; jumptable [0-9A-F]+ case ([0-9, ]+)", l)
        if m:
            ids = [int(x) for x in m.group(1).replace(" ", "").split(",") if x]
            j = i
            flags = []
            while j < e and "def_" not in lines[j].split(";")[0] and not re.search(r"^\s*jmp +short def_", lines[j]):
                t = lines[j].split(";")[0]
                mm = re.search(r"(?:inc|mov) +(byte_[0-9A-F]+)", t)
                if mm and mm.group(1) not in flags:
                    flags.append(mm.group(1))
                if re.search(r"^\s*jmp", t) and j > i:
                    break
                j += 1
            for c in ids:
                cur["cases"][c] = flags
    for n, b in enumerate(blocks):
        print("block %2d:" % n, "  ".join("%d:%s" % (c, "+".join(f) if f else "-") for c, f in sorted(b["cases"].items())))


if __name__ == "__main__":
    main()
