#!/usr/bin/env python3
"""Compares two ORIGINAL executables function by function (the same features as tools/crosscompare.py: calls, numbers, texts,
shape), to find which functions of the engine were changed between two builds, whatever the reconstruction says.
Both executables must have been read by tools/crosscompare.py once (it caches them as build/cross/original-<name>.pkl: run
`tools/crosscompare.py <game> --original <executable>` for each).

Usage: tools/origdiff.py <first.pkl> <second.pkl> [-o out.tsv] [--classes] [--worst N [FILTER]] [--show NAME]
  (default)            the summary: how many functions are the same / changed, and per class
  -o out.tsv           every function in both, least alike first
  --classes            per class: functions in both, same, changed
  --worst N [FILTER]   the N least alike functions whose name contains FILTER
  --show NAME          both versions of one function side by side (names as in the .tsv)

A function is 'same' when its calls, texts and shape agree completely; the numbers are not required to (the offsets of the members of
a class move when a member is added, which changes numbers in functions whose source did not change). 'moved only' lists those that
differ in numbers alone."""
import collections
import difflib
import pickle
import re
import sys

CTOR_DTOR = re.compile(r"(C[0-2]|D[0-2])E")


def normalize_name(name):
    name = name.split("@")[0]
    return CTOR_DTOR.sub(lambda m: m.group(1)[0] + "?E", name)


def kind_of(m):
    if m.startswith("j") and m != "jmp":
        return "jcc"
    if m.startswith("set"):
        return "setcc"
    if m.startswith("cmov"):
        return "cmov"
    if m in ("movzbl", "movzwl", "movsbl", "movswl", "movslq", "movzbq", "movsbq", "movzx", "movsx", "movsbw", "movzbw", "cltq", "cwtl", "cltd", "cqto"):
        return "ext"
    if m.startswith(("movs", "movz")) and m not in ("movss", "movsd", "movsq", "movsl", "movsw", "movsb"):
        return "ext"
    if m in ("nop", "nopw", "nopl", "xchg", "data16", "ud2"):
        return "nop"
    return m


class Func:
    __slots__ = ("name", "start", "size", "insns")

    def calls(self):
        return [normalize_name(t) for (m, t, c, s, j, o) in self.insns if t is not None and m.startswith(("call", "jmp"))
                and not (m.startswith("jmp") and t.startswith("<L"))]

    def consts(self):
        out = []
        for (m, t, c, s, j, o) in self.insns:
            out.extend(c)
        return out

    def strings(self):
        return [s for (m, t, c, s, j, o) in self.insns if s]

    def kinds(self):
        return [kind_of(m) for (m, t, c, s, j, o) in self.insns]


def ratio(a, b):
    if not a and not b:
        return 1.0
    if not a or not b:
        return 0.0
    if len(a) * len(b) > 4_000_000:
        a, b = a[:2000], b[:2000]
    return difflib.SequenceMatcher(None, a, b, autojunk=False).ratio()


def bag_ratio(a, b):
    if not a and not b:
        return 1.0
    ca, cb = collections.Counter(a), collections.Counter(b)
    return 2.0 * sum((ca & cb).values()) / (len(a) + len(b))


def compare(f, g):
    r = {"calls": ratio(f.calls(), g.calls()), "consts": bag_ratio(f.consts(), g.consts()),
         "strings": bag_ratio(f.strings(), g.strings()), "shape": ratio(f.kinds(), g.kinds())}
    r["score"] = 0.4 * r["calls"] + 0.25 * r["consts"] + 0.15 * r["strings"] + 0.2 * r["shape"]
    return r


def klass(mangled):
    m = re.match(r"_ZN[K]?(\d+)(.*)", mangled)
    if m:
        return m.group(2)[:int(m.group(1))]
    return "(free)"


def same(r):
    return r["calls"] == 1.0 and r["strings"] == 1.0 and r["shape"] == 1.0


def load(path):
    sys.modules["__main__"].Func = Func  # the pickles were written by a script, where the class was __main__.Func
    funcs, symbols = pickle.load(open(path, "rb"))
    return funcs


def show(f, g, name):
    def fmt(x):
        return ["%s %s" % (i[0], i[1] or ",".join(str(c) for c in i[2]) or (i[3] or "")) for i in x.insns]
    a, b = fmt(f), fmt(g)
    sm = difflib.SequenceMatcher(None, [x.split(" ")[0] for x in a], [x.split(" ")[0] for x in b], autojunk=False)
    print("== %s  instructions %d / %d" % (name, len(a), len(b)))
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op == "equal":
            for k in range(i2 - i1):
                if a[i1 + k] != b[j1 + k]:
                    print("   %-50s | %s" % (a[i1 + k], b[j1 + k]))
            continue
        for k in range(max(i2 - i1, j2 - j1)):
            print("%s  %-50s | %s" % ("-" if op == "delete" else "+" if op == "insert" else "~",
                                      a[i1 + k] if i1 + k < i2 else "", b[j1 + k] if j1 + k < j2 else ""))
    print("   first  calls:", ", ".join(f.calls()))
    print("   second calls:", ", ".join(g.calls()))


def main():
    args = sys.argv[1:]
    out = None
    classes = False
    worst = None
    flt = ""
    show_name = None
    pos = []
    i = 0
    while i < len(args):
        a = args[i]
        if a == "-o":
            out = args[i + 1]
            i += 1
        elif a == "--classes":
            classes = True
        elif a == "--worst":
            worst = int(args[i + 1])
            i += 1
            if i + 1 < len(args) and not args[i + 1].startswith("-") and len(pos) >= 2:
                flt = args[i + 1]
                i += 1
        elif a == "--show":
            show_name = args[i + 1]
            i += 1
        else:
            pos.append(a)
        i += 1
    if len(pos) != 2:
        print(__doc__)
        return 1
    f1, f2 = load(pos[0]), load(pos[1])
    common = sorted(set(f1) & set(f2))
    if show_name:
        for n in common:
            if show_name == n or (show_name in n and not show_name in common):
                show(f1[n], f2[n], n)
                break
        return 0
    results = {n: compare(f1[n], f2[n]) for n in common}
    ranked = sorted(common, key=lambda n: (results[n]["score"], -f1[n].size))
    if out:
        with open(out, "w") as fh:
            fh.write("name\tscore\tcalls\tconsts\tstrings\tshape\tinsns_first\tinsns_second\n")
            for n in ranked:
                r = results[n]
                fh.write("%s\t%.3f\t%.2f\t%.2f\t%.2f\t%.2f\t%d\t%d\n" % (n, r["score"], r["calls"], r["consts"], r["strings"], r["shape"],
                                                                      len(f1[n].insns), len(f2[n].insns)))
    if worst:
        for n in [n for n in ranked if flt in n][:worst]:
            r = results[n]
            print("%.3f  calls %.2f consts %.2f strings %.2f shape %.2f  insns %d/%d  %s" % (
                r["score"], r["calls"], r["consts"], r["strings"], r["shape"], len(f1[n].insns), len(f2[n].insns), n))
        return 0
    n_same = sum(1 for n in common if same(results[n]))
    n_moved = sum(1 for n in common if same(results[n]) and results[n]["consts"] < 1.0)
    n_ident = sum(1 for n in common if results[n]["score"] == 1.0)
    print("first: %d functions, second: %d, in both: %d" % (len(f1), len(f2), len(common)))
    print("  identical in every feature: %d; same calls, texts and shape: %d (of them with other numbers only: %d)" % (n_ident, n_same, n_moved))
    print("  changed: %d (calls differ: %d, texts differ: %d, shape differs: %d)" % (
        len(common) - n_same, sum(1 for n in common if results[n]["calls"] < 1.0), sum(1 for n in common if results[n]["strings"] < 1.0),
        sum(1 for n in common if results[n]["shape"] < 1.0)))
    if classes:
        by = collections.defaultdict(lambda: [0, 0])
        for n in common:
            c = klass(n)
            by[c][0] += 1
            by[c][1] += not same(results[n])
        print("class                         in both  changed")
        for c, (t, ch) in sorted(by.items(), key=lambda kv: (-kv[1][1], kv[0])):
            if c.startswith("T") and ch:
                print("%-30s %5d %7d" % (c, t, ch))
    if out:
        print("written %s" % out)
    return 0


sys.exit(main())
