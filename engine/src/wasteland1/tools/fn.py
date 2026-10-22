#!/usr/bin/env python3
"""fn.py ADDR... : prints the disassembly of the given functions (offsets inside CS, hex) from ida_scripts/dump_wasteland.txt
with data segment offsets for the IDA names (word_21A2F -> ds:3C0F), and the callers/callees from dump_callgraph.txt."""
import os
import re
import sys

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "..", "ida_scripts")
dump = open(os.path.join(root, "dump_wasteland.txt"), errors="replace").read().split(";==== ")
funcs = {}
for chunk in dump[1:]:
    head = chunk.split("\n", 1)[0]
    m = re.search(r"@ ([0-9A-F]+)", head)
    if m:
        funcs[int(m.group(1), 16)] = ";==== " + chunk.rstrip()
names = {}
for line in open(os.path.join(root, "wl_dsnames.txt")):
    p = line.split()
    if len(p) >= 2:
        names[p[1]] = int(p[0], 16)


def annotate(text):
    def rep(m):
        n = m.group(0)
        mm = re.match(r"(?:byte|word|dword|unk|off)_([0-9A-F]+)$", n)
        if mm:
            a = int(mm.group(1), 16) - 0x1CE20
            if 0 <= a < 0x10000:
                return "%s{ds:%04X}" % (n, a)
        if n in names:
            return "%s{ds:%04X}" % (n, names[n])
        return n
    return re.sub(r"\b[A-Za-z_][A-Za-z_0-9?.]*", rep, text)


for arg in sys.argv[1:]:
    a = int(arg, 16)
    if a < 0x10000:
        a += 0x10000
    if a not in funcs:
        print("no function at %X" % a)
        continue
    print(annotate(funcs[a]))
    print()
