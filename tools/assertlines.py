#!/usr/bin/env python3
"""Compares the line numbers of the assertions of two builds, file by file: where the source of the engine has more or fewer lines.

The engine's x_assert(cond, "expression", "/home/.../src/<file>.cpp", line) carries the line of the source, so the same assertion
(same file, same expression) in two builds says how many lines were added or removed before it. All the assertions of a file at the
same line: nothing was added or removed above the last of them.

Usage: tools/assertlines.py <first.pkl> <second.pkl> [--detail FILE ...]   (the caches that tools/crosscompare.py writes, build/cross/original-*.pkl)
Prints per source file the line of the first build and the shift of the second (second - first), at each place where the shift changes.
--detail lists every assertion of the files named (a part of the path is enough)."""
import collections
import pickle
import sys


class Func:
    pass


def load(path):
    sys.modules["__main__"].Func = Func
    funcs, _ = pickle.load(open(path, "rb"))
    found = collections.defaultdict(set)  # (file, expression) -> lines
    for f in funcs.values():
        ins = f.insns
        for k, (m, t, c, s, j, o) in enumerate(ins):
            if not (t and m.startswith("call") and (t.startswith("_Z8x_assert") or t.startswith("__assert_fail"))):
                continue
            strings, numbers = [], []
            for q in range(max(0, k - 9), k):
                if ins[q][0].startswith(("call", "j")):
                    strings, numbers = [], []
                    continue
                if ins[q][3]:
                    strings.append(ins[q][3])
                numbers += ins[q][2]
            files = [x for x in strings if x.startswith("/home/")]
            exprs = [x for x in strings if not x.startswith("/home/")]
            if files and numbers:
                found[(files[0].split("branchPillars/src/")[-1], exprs[0] if exprs else "")].update(numbers)
    return found


def main():
    args = sys.argv[1:]
    detail = []
    if "--detail" in args:
        i = args.index("--detail")
        detail = args[i + 1:]
        args = args[:i]
    if len(args) != 2:
        print(__doc__)
        return 1
    a, b = load(args[0]), load(args[1])
    by_file = collections.defaultdict(list)
    for key in a:
        if key in b and len(a[key]) == 1 and len(b[key]) == 1:
            by_file[key[0]].append((min(a[key]), min(b[key]) - min(a[key])))
    for name in sorted(by_file):
        if name.startswith(("ext/", "libbox2d")):
            continue
        out, last = [], None
        for line, shift in sorted(by_file[name]):
            if shift != last:
                out.append("%d:%+d" % (line, shift))
                last = shift
        print("%-45s %3d asserts   %s" % (name, len(by_file[name]), " ".join(out)))
    for part in detail:
        print("==", part)
        for key in sorted(set(a) | set(b)):
            if part in key[0]:
                print("  %-60r first %s   second %s" % (key[1][:60], " ".join("%x" % x for x in sorted(a.get(key, []))),
                                                       " ".join("%x" % x for x in sorted(b.get(key, [])))))
    return 0


sys.exit(main())
