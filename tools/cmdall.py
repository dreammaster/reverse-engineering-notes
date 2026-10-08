#!/usr/bin/env python3
"""Everything one script command has: its syntax (with the description of the command), and its Do and Redo
(and CanUndo/Undo if it has its own) through the lean reader, with the strings they use.
Usage: cmdall.py <file.asm> <CommandName>      e.g. cmdall.py Deponia_Linux.asm StartAction"""
import re
import subprocess
import sys

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
here = __file__.rsplit("cmdall.py", 1)[0]
asm, name = sys.argv[1], sys.argv[2]
cls = "Cmd" + name
pat_start = re.compile(r"^(_Z\w*%s\w*?) proc near" % re.escape(cls))
import os
import pickle

cache = os.path.join(os.path.dirname(os.path.abspath(asm)), "build", "procs.pickle")
procs = {}
if os.path.exists(cache) and os.path.getmtime(cache) > os.path.getmtime(asm):
    procs = pickle.load(open(cache, "rb"))
else:
    cur = None
    with open(asm, encoding="utf-8", errors="replace") as f:
        for n, line in enumerate(f, 1):
            if not line.startswith("_Z"):
                continue
            m = re.match(r"^(_Z[0-9A-Za-z_]+)\s+proc near", line)
            if m:
                cur = (m.group(1), n)
                continue
            m = re.match(r"^(_Z[0-9A-Za-z_]+)\s+endp", line)
            if m and cur and cur[0] == m.group(1):
                procs[cur[0]] = (cur[1], n)
                cur = None
    os.makedirs(os.path.dirname(cache), exist_ok=True)
    pickle.dump(procs, open(cache, "wb"))


def run(tool, first, last, extra=()):
    return subprocess.run([sys.executable, here + tool, asm, str(first), str(last)] + list(extra),
                          capture_output=True, text=True, encoding="utf-8", errors="replace").stdout


want = []
for sym, (a, b) in procs.items():
    if re.search(r"Cmd%s_GetSyntax" % re.escape(name), sym):
        want.append(("SYNTAX", sym, a, b))
    elif re.match(r"^_ZN\d+%s(2Do|4Redo|4Undo|7CanUndo|7CleanUp)Ev$" % re.escape(cls), sym) or \
            re.match(r"^_ZNK?\d+%s(2Do|4Redo|4Undo|7CanUndo|7CleanUp)Ev$" % re.escape(cls), sym):
        want.append(("CODE", sym, a, b))
want.sort(key=lambda w: (0 if w[0] == "SYNTAX" else 1, w[2]))
for kind, sym, a, b in want:
    print("=" * 8, sym, a, b)
    if kind == "SYNTAX":
        print(run("cmdsyn.py", a, b, ["--docs"]))
    else:
        print(run("avc.py", a, b))
        s = run("strs.py", a, b)
        if s.strip():
            print("--strings--")
            print(s)
