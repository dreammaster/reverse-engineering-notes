"""
Maps the compiler errors of the generated logic (a build log) to the functions they are in and writes reversing/analysis/xanth_logic_manual.tsv: the functions that the
decompiler's output does not compile for yet.  gen_logic.py replaces them by stubs (and lists them in manual_review.txt) until they are fixed in the decompiler or
written by hand.

    python find_bad_functions.py build.log
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
LOGIC = os.path.abspath(os.path.join(HERE, "..", "..", "src", "xanth", "logic"))
OUT = os.path.join(HERE, "..", "analysis", "xanth_logic_manual.tsv")
TAB = chr(9)
NL = chr(10)


def function_ranges(path):
    """(first line, last line, name) of every function in a generated file"""
    out = []
    cur = None
    for i, line in enumerate(open(path, encoding="utf-8"), 1):
        m = re.match(r"^// (\S+)$", line.rstrip())
        if m:
            if cur:
                out.append((cur[0], i - 1, cur[1]))
            cur = (i, re.sub(r"[^A-Za-z0-9_]", "_", m.group(1).rstrip("?")))
    if cur:
        out.append((cur[0], 10 ** 9, cur[1]))
    return out


def main():
    log = open(sys.argv[1], encoding="utf-8", errors="replace").read().split(NL)
    bad = {}
    cache = {}
    for line in log:
        m = re.match(r"^(.*?[/\\]logic[/\\]([\w.]+\.cpp)):(\d+):\d+: error: (.*)$", line)
        if not m:
            continue
        name, ln, msg = m.group(2), int(m.group(3)), m.group(4)
        if name in ("not_decompiled.cpp", "tables.cpp"):
            continue
        if name not in cache:
            cache[name] = function_ranges(os.path.join(LOGIC, name))
        for a, b, fn in cache[name]:
            if a <= ln <= b:
                bad.setdefault(fn, msg[:90])
    existing = {}
    if os.path.exists(OUT):
        for line in open(OUT, encoding="utf-8"):
            p = line.rstrip(NL).split(TAB)
            if len(p) >= 2:
                existing[p[0]] = p[1]
    existing.update(bad)
    with open(OUT, "w", encoding="utf-8", newline=NL) as fh:
        for n in sorted(existing):
            fh.write(n + TAB + existing[n] + NL)
    print(len(bad), "new,", len(existing), "in total")


if __name__ == "__main__":
    main()
