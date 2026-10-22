#!/usr/bin/env python3
"""Compares the conditions (comparisons with constants) of the reconstruction, compiled at -O0, with those of an original that was
compiled at -O0 too (Deponia 4): both are close transcriptions of the source, so a condition that one has and the other has not is
a candidate for a different source line (not for another optimizer). tools/origdiff.py --conds does the same for two originals, but
-O0 against -O2 (Deponia 4 against Deponia 1) drowns in switches, loop changes and merged tests.

Usage: tools/condcompare.py <original.pkl> <objects directory> [-o out.tsv] [--first <other.pkl>]
  original.pkl         the cache of tools/crosscompare.py (build/cross/original-Deponia4.pkl)
  --first other.pkl    a third build (Deponia 1, -O2) whose sources the reconstruction was written from: a test is a candidate only when it
                       is also missing in that build (the original has it and neither the reconstruction nor the other build has it), or
                       the reconstruction has it, the other build has it and the original has not
  objects directory    the objects of tools/crosscompile.py -O0 -X -fno-omit-frame-pointer --tag o0 (build/cross/deponia4-o0)
A test is the split that it makes: `x > 16` and `x >= 17` are the same test; signed and unsigned are not told apart; tests against 0,
and against numbers over 0xFFFFF (addresses), are left out. Functions with a jump table (a switch) in either are listed apart."""
import collections
import os
import pickle
import re
import subprocess
import sys

OBJDUMP = os.environ.get("VIS_OBJDUMP", r"C:\mingw64\bin\objdump.exe")
JCC = {"l": "<", "nge": "<", "le": "<=", "ng": "<=", "g": ">", "nle": ">", "ge": ">=", "nl": ">=", "b": "<", "nae": "<", "c": "<",
       "be": "<=", "na": "<=", "a": ">", "nbe": ">", "ae": ">=", "nb": ">=", "nc": ">=", "e": "==", "z": "==", "ne": "!=", "nz": "!="}
CMP_IMM = re.compile(r"^\$(-?0x[0-9a-f]+),")
CTOR_DTOR = re.compile(r"(C[0-2]|D[0-2])E")


class Func:
    pass


def conditions(insns):
    """insns: [(mnemonic, operands)]. The tests as ('r', t) or ('e', c); True when a jump table was seen."""
    out = []
    table = False
    for k, (mn, ops) in enumerate(insns):
        if mn.startswith("jmp") and ops.startswith("*"):
            table = True
        base = re.sub(r"[bwlq]$", "", mn) if mn.startswith("cmp") else mn
        if base != "cmp":
            continue
        m = CMP_IMM.match(ops.split("\t")[0].strip())
        if not m:
            continue
        c = int(m.group(1), 16)
        if 0x7FFFFFFF < c <= 0xFFFFFFFF:
            c -= 1 << 32
        if abs(c) > 0xFFFFF or c == 0:
            continue
        for q in range(k + 1, min(len(insns), k + 4)):
            nxt = insns[q][0]
            cond = None
            if nxt.startswith("j") and nxt != "jmp":
                cond = nxt[1:]
            elif nxt.startswith("set"):
                cond = nxt[3:]
            elif nxt.startswith("cmov"):
                cond = nxt[4:]
            if cond is None:
                if nxt.startswith(("call", "cmp", "test")):
                    break
                continue
            op = JCC.get(cond)
            if op == "==" or op == "!=":
                out.append(("e", c))
            elif op:
                out.append(("r", c if op in (">", "<=") else c - 1))
            break
    return out, table


def ours(directory):
    funcs = {}
    paths = []
    for root, _, names in os.walk(directory):
        paths += [os.path.join(root, n) for n in names if n.endswith(".o")]
    for path in sorted(paths):
        proc = subprocess.run([OBJDUMP, "-d", "-w", "--no-show-raw-insn", path], capture_output=True, text=True, encoding="latin1")
        cur = None
        for line in proc.stdout.splitlines():
            m = re.match(r"^[0-9a-f]+ <(.+)>:$", line)
            if m:
                cur = funcs.setdefault(m.group(1), [])
                continue
            m = re.match(r"^\s*[0-9a-f]+:\t([^\t]*)(?:\t(.*))?$", line)
            if m and cur is not None:
                parts = m.group(1).split(None, 1)
                if parts:
                    cur.append((parts[0], (parts[1] if len(parts) > 1 else "") + ("\t" + m.group(2) if m.group(2) else "")))
    return funcs


def main():
    args = sys.argv[1:]
    out = None
    first_path = None
    if "--first" in args:
        i = args.index("--first")
        first_path = args[i + 1]
        del args[i:i + 2]
    if "-o" in args:
        i = args.index("-o")
        out = args[i + 1]
        del args[i:i + 2]
    if len(args) != 2:
        print(__doc__)
        return 1
    sys.modules["__main__"].Func = Func
    original, _ = pickle.load(open(args[0], "rb"))
    mine = ours(args[1])
    first = None
    if first_path:
        first, _ = pickle.load(open(first_path, "rb"))
        by_first = {}
        for n, f in first.items():
            by_first.setdefault(CTOR_DTOR.sub(lambda m: m.group(1)[0] + "?E", n.split("@")[0]), f)
        cache = {}

        def first_conditions(f, depth=0):
            key = (id(f), depth)
            if key in cache:
                return cache[key]
            res = collections.Counter(conditions([(i[0], i[5][1]) for i in f.insns])[0])
            if depth < 3:
                for i in f.insns:
                    t = i[1]
                    if t and i[0].startswith("call"):
                        g = by_first.get(CTOR_DTOR.sub(lambda m: m.group(1)[0] + "?E", t.split("@")[0]))
                        if g is not None and g is not f:
                            res |= first_conditions(g, depth + 1)
            cache[key] = res
            return res
    rows = []
    switches = 0
    common = 0
    for n, f in original.items():
        if n not in mine:
            continue
        common += 1
        a, ta = conditions([(i[0], i[5][1]) for i in f.insns])
        b, tb = conditions(mine[n])
        if ta or tb:
            switches += 1
            continue
        ca, cb = collections.Counter(a), collections.Counter(b)
        if ca != cb:
            only_orig, only_ours = ca - cb, cb - ca
            if first is not None:
                if n not in first:
                    continue
                other = first_conditions(first[n])
                only_orig = only_orig - other
                only_ours = only_ours & (other - ca)
                if not only_orig and not only_ours:
                    continue
            rows.append((n, only_orig, only_ours, len(a), len(b)))
    rows.sort(key=lambda r: -(sum(r[1].values()) + sum(r[2].values())))
    fmt = lambda c: " ".join("%s%d" % (k, t) for (k, t), v in sorted(c.items()) for _ in range(v))
    print("functions in both: %d; with a jump table (left out): %d; with other conditions: %d" % (common, switches, len(rows)))
    if out:
        with open(out, "w", encoding="utf8") as fh:
            fh.write("name\toriginal\tours\tonly in the original\tonly in ours\n")
            for n, o, m, ca, cb in rows:
                fh.write("%s\t%d\t%d\t%s\t%s\n" % (n, ca, cb, fmt(o), fmt(m)))
        print("written %s" % out)
    return 0


sys.exit(main())
