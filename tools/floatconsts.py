#!/usr/bin/env python3
"""Compares the floating point constants (speeds, tolerances, factors, limits) that the functions of two builds load from .rodata.
tools/origdiff.py --consts does not see them: they are not immediates but `movss 0x...(%rip),%xmm0  # <address>`.

Usage: tools/floatconsts.py <first.pkl> <first-exe> <second.pkl> <second-exe> [-o out.tsv] [--global]
  the .pkl files are the caches of tools/crosscompare.py (build/cross/original-*.pkl), the executables are the same builds.
  --global   only the values that one build loads somewhere in T* code and the other nowhere
  -o         per function: the values only the first loads, and only the second loads (the second -O0: what the first inlined
             is read from the functions of the second that the first does not define, four levels down)
The scalar instructions give the width (ss: 4 bytes, sd: 8 bytes); the values are rounded to 7 digits."""
import collections
import pickle
import re
import struct
import sys

ADDR = re.compile(r"#\s+([0-9a-f]+)\s")
SS = re.compile(r"^(mov|mul|add|sub|div|max|min|comi|ucomi|sqrt|cvtss2)ss$|^cvtss2s")
SD = re.compile(r"^(mov|mul|add|sub|div|max|min|comi|ucomi|sqrt)sd$|^cvtsd2s|^cvttsd2")
CTOR_DTOR = re.compile(r"(C[0-2]|D[0-2])E")


class Func:
    pass


def read_sections(path):
    d = open(path, "rb").read()
    shoff, = struct.unpack_from("<Q", d, 0x28)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", d, 0x3A)
    secs = []
    for i in range(shnum):
        name, typ, flags, addr, off, size = struct.unpack_from("<IIQQQQ", d, shoff + i * shentsize)
        secs.append((addr, off, size, typ))
    return d, [s for s in secs if s[0] and s[3] == 1]


def value_at(d, secs, addr, width):
    for a, off, size, typ in secs:
        if a <= addr and addr + width <= a + size:
            raw = d[off + addr - a: off + addr - a + width]
            return struct.unpack("<f" if width == 4 else "<d", raw)[0]
    return None


def norm(name):
    return CTOR_DTOR.sub(lambda m: m.group(1)[0] + "?E", name.split("@")[0])


def load(pkl, exe):
    sys.modules["__main__"].Func = Func
    funcs, _ = pickle.load(open(pkl, "rb"))
    d, secs = read_sections(exe)
    vals = {}
    for n, f in funcs.items():
        out = []
        for ins in f.insns:
            m = ins[0]
            width = 4 if SS.match(m) else 8 if SD.match(m) else 0
            if not width or "(%rip)" not in ins[5][1]:
                continue
            a = ADDR.search(ins[5][1])
            if not a:
                continue
            v = value_at(d, secs, int(a.group(1), 16), width)
            if v is not None and v == v:
                out.append(float("%.7g" % v))
        vals[n] = out
    return funcs, vals


def klass(n):
    m = re.match(r"_ZN[K]?(\d+)(.*)", n)
    return m.group(2)[:int(m.group(1))] if m else None


def main():
    args = sys.argv[1:]
    out = None
    glob = "--global" in args
    args = [a for a in args if a != "--global"]
    if "-o" in args:
        i = args.index("-o")
        out = args[i + 1]
        del args[i:i + 2]
    if len(args) != 4:
        print(__doc__)
        return 1
    f1, v1 = load(args[0], args[1])
    f2, v2 = load(args[2], args[3])
    if glob:
        a = collections.defaultdict(set)
        b = collections.defaultdict(set)
        for n, vs in v1.items():
            c = klass(n)
            if c and c.startswith("T"):
                for v in vs:
                    a[v].add(n)
        for n, vs in v2.items():
            c = klass(n)
            if c and c.startswith("T"):
                for v in vs:
                    b[v].add(n)
        print("distinct values in T* code: first %d, second %d" % (len(a), len(b)))
        for tag, x, y in (("only in the first", a, b), ("only in the second", b, a)):
            print("== " + tag)
            for v in sorted(set(x) - set(y)):
                print("  %g   %s" % (v, ", ".join(sorted(x[v]))[:150]))
        return 0
    norm1 = set(norm(n) for n in f1)
    by2 = {}
    for n in f2:
        by2.setdefault(norm(n), n)

    def closure(n, depth=0):
        res = list(v2.get(n, []))
        if depth < 4:
            for ins in f2[n].insns:
                t = ins[1]
                if t and ins[0].startswith("call"):
                    c = norm(t)
                    if c not in norm1 and c in by2:
                        res += closure(by2[c], depth + 1)
        return res

    rows = []
    for n in sorted(set(f1) & set(f2)):
        a = collections.Counter(v1[n])
        b = collections.Counter(closure(n))
        o1, o2 = sorted(set(a) - set(b)), sorted(set(b) - set(a))
        if o1 or o2:
            rows.append((n, o1, o2))
    print("functions in both: %d; with other floating point constants: %d" % (len(set(f1) & set(f2)), len(rows)))
    if out:
        with open(out, "w", encoding="utf8") as fh:
            fh.write("name\tonly in the first\tonly in the second\n")
            for n, o1, o2 in rows:
                fh.write("%s\t%s\t%s\n" % (n, " ".join("%g" % v for v in o1), " ".join("%g" % v for v in o2)))
        print("written %s" % out)
    return 0


sys.exit(main())
