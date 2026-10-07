#!/usr/bin/env python3
"""List the draw-list records emitted by the unrolled wall renderer routines of mm3.asm (sub_1DB3D, sub_17F38, sub_1862A, sub_18BF1).

usage: mm3_scenelist.py [mm3.asm] [function ...]
For each record (x, y, flags, frame) written through SI the guarding flag byte (last 'cmp byte_XXXX, 0') is printed.
Caveat: records are listed in emission order but the guard shown can be the wrong flag where the code branches into shared tails (check the asm; e.g. wall style 3 / byte_332FD emits frame 1 + byte_2884D without being listed here).
Flags that are not immediates (for example `2 | byte_28875`) are printed as the register expression found just before the store.
"""
import re
import sys


def signed(v):
    return v - 65536 if v >= 32768 else v


def parse(asm, func):
    lines = asm.split("\n")
    s = next(i for i, l in enumerate(lines) if l.startswith(func + " ") and " proc" in l)
    e = next(i for i in range(s, len(lines)) if lines[i].startswith(func + " ") and " endp" in lines[i])
    cond = None
    stores = []
    dx_expr = None
    regsrc = {}
    hdr = None
    out = []
    for i in range(s, e):
        l = lines[i]
        m = re.search(r"cmp +(byte_[0-9A-F]+|word_[0-9A-F]+)(?: ptr)?, 0\s*$", l.split(";")[0])
        if m:
            cond = m.group(1)
        m = re.search(r"mov +al, (byte_[0-9A-F]+)", l)
        if m:
            cond = m.group(1)
        m = re.search(r"mov +dx, ([0-9A-F]+)h?\s*(;.*)?$", l.split(";")[0] + "")
        if m:
            dx_expr = "%d" % int(m.group(1).rstrip("h"), 16) if re.search(r"[A-F]|h$", m.group(1)) else m.group(1)
        m = re.search(r"or +dx, (\S+)", l.split(";")[0])
        if m and dx_expr is not None:
            dx_expr = "%s|%s" % (dx_expr, m.group(1))
        m = re.search(r"mov +(ax|dx), (word_[0-9A-F]+|\[bx[^\]]*\])", l.split(";")[0])
        if m:
            regsrc[m.group(1)] = m.group(2)
        if re.search(r"mov +\[si\+2\], ax", l.split(";")[0]):
            hdr = "set=%s:%s" % (regsrc.get("ax"), "?")
            continue
        if re.search(r"mov +\[si\], dx", l.split(";")[0]) and hdr is not None:
            out.append((cond, "SPRITESET", "%s/%s" % (regsrc.get("ax"), regsrc.get("dx")), "", ""))
            hdr = None
            stores = []
            continue
        m = re.search(r"mov +word ptr \[si\], ([0-9A-Fa-f]+h?)", l.split(";")[0])
        if m and m.group(1).upper() == "0FFFFH":
            stores = []
            continue
        if m:
            t = m.group(1)
            v = int(t[:-1], 16) if t.endswith("h") else int(t)
            stores.append(str(signed(v)))
        elif re.search(r"mov +\[si\], dx", l.split(";")[0]):
            stores.append(dx_expr or "dx")
        elif re.search(r"mov +\[si\], ax", l.split(";")[0]):
            stores.append("ax")
        if len(stores) == 4:
            out.append((cond,) + tuple(stores))
            stores = []
    return out


def main():
    args = sys.argv[1:]
    path = args[0] if args and args[0].endswith(".asm") else "mm3.asm"
    funcs = [a for a in args if not a.endswith(".asm")] or ["sub_1DB3D"]
    asm = open(path, encoding="latin1").read()
    for f in funcs:
        print("; %s" % f)
        for r in parse(asm, f):
            print("%-14s x=%-5s y=%-4s flags=%-10s frame=%s" % r)


if __name__ == "__main__":
    main()
