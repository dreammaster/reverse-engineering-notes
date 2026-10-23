#!/usr/bin/env python3
"""Compares, over the whole code of the engine (the functions of the T* classes), the text literals and the numbers that two builds refer to:
which texts / numbers does one build use somewhere and the other nowhere. A sign that a message, a field or a limit was added or removed,
whatever the optimizer did to the single function. Used on builds of the same optimization level, or on the -O0 build as the second.

Usage: tools/globaldiff.py <first.pkl> <second.pkl> [--min-number 0x30]
  the caches of tools/crosscompare.py (build/cross/original-*.pkl)
Texts that sit next to a source file path (the arguments of an assert) are left out; so are numbers over 0xFFFFF (addresses)."""
import collections
import pickle
import re
import subprocess
import sys


class Func:
    def strings(self):
        return [x[3] for x in self.insns if x[3]]


def klass(n):
    m = re.match(r"_ZN[K]?(\d+)(.*)", n)
    return m.group(2)[:int(m.group(1))] if m else None


def load(path):
    sys.modules["__main__"].Func = Func
    funcs, _ = pickle.load(open(path, "rb"))
    texts = collections.defaultdict(set)
    numbers = collections.defaultdict(set)
    assertish = set()
    for n, f in funcs.items():
        ss = f.strings()
        for i, s in enumerate(ss):
            if s.startswith("/home/"):
                for k in (i - 1, i + 1, i + 2):
                    if 0 <= k < len(ss):
                        assertish.add(ss[k])
        c = klass(n)
        if c and c.startswith("T"):
            for s in ss:
                texts[s].add(n)
            for ins in f.insns:
                for v in ins[2]:
                    numbers[v].add(c)
    return texts, numbers, assertish


def main():
    args = sys.argv[1:]
    low = 0x30
    if "--min-number" in args:
        i = args.index("--min-number")
        low = int(args[i + 1], 0)
        del args[i:i + 2]
    if len(args) != 2:
        print(__doc__)
        return 1
    t1, n1, a1 = load(args[0])
    t2, n2, a2 = load(args[1])
    bad = a1 | a2

    def keep(s):
        return not s.startswith("/home/") and s not in bad and len(s) >= 3

    o1 = sorted(s for s in t1 if s not in t2 and keep(s))
    o2 = sorted(s for s in t2 if s not in t1 and keep(s))
    names = sorted({n for s in o1 for n in t1[s]} | {n for s in o2 for n in t2[s]})
    dm = dict(zip(names, subprocess.run(["c++filt"], input="\n".join(names), capture_output=True, text=True).stdout.splitlines())) if names else {}
    print("texts in T* code: first %d, second %d; only first %d, only second %d" % (len(t1), len(t2), len(o1), len(o2)))
    for tag, lst, d in (("ONLY IN THE FIRST", o1, t1), ("ONLY IN THE SECOND", o2, t2)):
        print("==", tag)
        for s in lst:
            print("  %r   in %s" % (s[:110], ", ".join(sorted(dm[n][:40] for n in d[s]))[:100]))
    v1 = {v for v in n1 if low <= v <= 0xFFFFF}
    v2 = {v for v in n2 if low <= v <= 0xFFFFF}
    print("numbers >= %#x in T* code: first %d, second %d; only first %d, only second %d" % (low, len(v1), len(v2), len(v1 - v2), len(v2 - v1)))
    print("ONLY IN THE FIRST:", " ".join("%x(%s)" % (v, "/".join(sorted(n1[v]))[:24]) for v in sorted(v1 - v2)))
    print("ONLY IN THE SECOND:", " ".join("%x(%s)" % (v, "/".join(sorted(n2[v]))[:24]) for v in sorted(v2 - v1)))
    return 0


sys.exit(main())
