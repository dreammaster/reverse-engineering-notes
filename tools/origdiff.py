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
  --engine OUT.tsv     for builds compiled differently (-O2 against -O0): only the calls to engine functions (T* methods, free functions of the
                       engine) that both executables define and that are bigger than 24 bytes in the first one (smaller ones are inlined
                       in an -O2 build), compared as sequences; the rows are the functions whose sequences differ, most different first

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


def engine_mode(f1, f2, common, out):
    own = set(n for n in common if f1[n].size > 24 and (klass(n).startswith("T") or re.match(r"^_Z\d+[A-Za-z]", n)))
    own = set(n for n in own if not re.match(r"^_ZN?K?\d+(std|__gnu|wx)", n))

    # what the first (-O2) build has inlined is a function of its own in the second (-O0): its calls are put in place of the call
    norm1 = set(normalize_name(n) for n in f1)
    by_norm2 = {}
    for n, f in f2.items():
        by_norm2.setdefault(normalize_name(n), f)

    def expand(f, depth=0):
        # the call, and what it calls in turn (three levels): the first build may have inlined any of the levels
        out = []
        for c in f.calls():
            out.append(c)
            g = by_norm2.get(c)
            if depth < 3 and g is not None:
                out.extend(expand(g, depth + 1))
        return out

    def seq(f, deep=False):
        return [c for c in (expand(f) if deep else f.calls()) if c in own]
    rows = []
    for n in common:
        a, b = seq(f1[n]), seq(f2[n], True)
        if not a and not b:
            continue
        # an -O0 build (the second) makes every call that an -O2 build (the first) has inlined: what the second has more of is no
        # difference. What the first has and the second has not cannot be explained that way.
        missing = collections.Counter(a) - collections.Counter(b)
        if missing:
            rows.append((ratio(a, b), len(a), len(b), n, missing))
    rows.sort(key=lambda t: (-sum(t[4].values()), t[0]))
    with open(out, "w") as fh:
        fh.write("name\tratio\tcalls_first\tcalls_second\tcalls only in the first\n")
        for r, a, b, n, missing in rows:
            fh.write("%s\t%.3f\t%d\t%d\t%s\n" % (n, r, a, b, ", ".join("%s x%d" % (c, k) for c, k in missing.items())))
    total = sum(1 for n in common if seq(f1[n]) or seq(f2[n], True))
    print("functions with engine calls: %d; the first calls something the second does not: %d" % (total, len(rows)))

    # the other way: a call of the second build to a big function (over 200 bytes, which the -O2 build does not inline) that the
    # first build's function and what it calls (three levels) never make
    big = set(n for n in own if f1[n].size > 200)
    by_norm1 = {}
    for n, f in f1.items():
        by_norm1.setdefault(normalize_name(n), f)

    def expand1(f, depth=0):
        res = []
        for c in f.calls():
            res.append(c)
            g = by_norm1.get(c)
            if depth < 3 and g is not None:
                res.extend(expand1(g, depth + 1))
        return res
    rev = []
    for n in common:
        extra = collections.Counter(c for c in f2[n].calls() if c in big) - collections.Counter(expand1(f1[n]))
        if extra:
            rev.append((n, extra))
    rev.sort(key=lambda t: -sum(t[1].values()))
    with open(out + ".rev.tsv", "w") as fh:
        fh.write("name\tcalls only in the second\n")
        for n, extra in rev:
            fh.write("%s\t%s\n" % (n, ", ".join("%s x%d" % (c, k) for c, k in extra.items())))
    print("the second calls a big function that the first does not: %d (%s.rev.tsv)" % (len(rev), out))
    print("written %s" % out)
    return 0


def main():
    args = sys.argv[1:]
    out = None
    classes = False
    worst = None
    flt = ""
    show_name = None
    engine_out = None
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
        elif a == "--engine":
            engine_out = args[i + 1]
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
    if engine_out:
        return engine_mode(f1, f2, common, engine_out)
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
