#!/usr/bin/env python3
"""Compares two builds of the engine (ELF files with their dynamic symbols, e.g. Deponia and Deponia2).
Usage: tools/compare_binaries.py <first> <second> [--list]   (needs nm and c++filt on the PATH, e.g. /c/mingw64/bin)
Prints the size and hash, how many symbols and T* classes the builds share, how many functions have the same size (a
rough sign that the code is the same) and, per class, where they differ most. --list also prints the classes and
the methods that only one of them has."""
import hashlib
import os
import re
import subprocess
import sys
from collections import Counter, defaultdict


def nm(path):
    """name -> size of the functions that the file defines (dynamic symbols)."""
    out = subprocess.run(["nm", "-D", "-S", "--defined-only", path], capture_output=True, text=True, check=True).stdout
    functions = {}
    names = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[2] in "tTwW":
            functions[parts[3]] = int(parts[1], 16)
        if parts:
            names.add(parts[-1])
    return functions, names


def demangle(names):
    names = sorted(names)
    out = subprocess.run(["c++filt"], input="\n".join(names), capture_output=True, text=True, check=True).stdout.splitlines()
    return dict(zip(names, out))


def class_of(mangled):
    m = re.match(r"_ZN[K]?(\d+)(.*)", mangled)
    if not m:
        return None
    return m.group(2)[:int(m.group(1))]


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) != 2:
        print(__doc__)
        return 1
    paths = args
    infos = []
    for p in paths:
        data = open(p, "rb").read()
        functions, names = nm(p)
        infos.append((p, len(data), hashlib.sha1(data).hexdigest(), functions, names))
        print("%s: %d bytes, sha1 %s, %d functions, %d symbols" % (os.path.basename(p), len(data), hashlib.sha1(data).hexdigest()[:12], len(functions), len(names)))

    f1, f2 = infos[0][3], infos[1][3]
    n1, n2 = infos[0][4], infos[1][4]
    common = set(f1) & set(f2)
    same = [n for n in common if f1[n] == f2[n]]
    print("symbols in common %d (only in first %d, only in second %d)" % (len(n1 & n2), len(n1 - n2), len(n2 - n1)))
    print("functions in common %d, of them the same size %d (%.0f%%)" % (len(common), len(same), 100.0 * len(same) / max(1, len(common))))

    d1, d2 = demangle(n1), demangle(n2)
    m1 = {v for v in d1.values() if re.match(r"T[A-Za-z0-9_]*::", v)}
    m2 = {v for v in d2.values() if re.match(r"T[A-Za-z0-9_]*::", v)}
    c1 = {v.split("::")[0] for v in m1}
    c2 = {v.split("::")[0] for v in m2}
    print("T* classes: first %d, second %d, common %d" % (len(c1), len(c2), len(c1 & c2)))
    print("T* methods: first %d, second %d, same signature %d" % (len(m1), len(m2), len(m1 & m2)))

    by_class = defaultdict(lambda: [0, 0])
    for n in common:
        c = class_of(n)
        if c and c.startswith("T"):
            by_class[c][0] += 1
            by_class[c][1] += f1[n] == f2[n]
    print("classes with the most functions of another size (different, common, class):")
    for different, total, c in sorted(((v[0] - v[1], v[0], k) for k, v in by_class.items()), reverse=True)[:20]:
        print("  %4d of %4d  %s" % (different, total, c))

    for label, only in (("first", m1 - m2), ("second", m2 - m1)):
        counts = Counter(v.split("::")[0] for v in only)
        print("methods only in the %s, by class: %s" % (label, ", ".join("%s %d" % kv for kv in counts.most_common(12))))
    if "--list" in sys.argv:
        print("classes only in the first:", " ".join(sorted(c1 - c2)))
        print("classes only in the second:", " ".join(sorted(c2 - c1)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
