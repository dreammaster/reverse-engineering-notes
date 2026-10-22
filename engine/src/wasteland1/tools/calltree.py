#!/usr/bin/env python3
"""calltree.py ADDR [depth] : callees of a function (from ida_scripts/dump_callgraph.txt) with sizes; ported ones marked *."""
import os
import re
import sys

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.join(here, "..", "..", "..", "..", "ida_scripts")
funcs = {}
for line in open(os.path.join(root, "dump_callgraph.txt")):
    m = re.match(r"([0-9A-F]+) (\S+) (\d+) : ?(.*)", line)
    if m:
        funcs[int(m.group(1), 16) - 0x10000] = (m.group(2), m.group(4).split())
byname = {v[0]: k for k, v in funcs.items()}
sizes = {}
for line in open(os.path.join(root, "dump_wl_ext.txt"), errors="replace"):
    m = re.match(r";==== (\S+) @ ([0-9A-F]+) size=(\d+)", line)
    if m:
        sizes[int(m.group(2), 16) - 0x10000] = int(m.group(3))
ported = set()
for line in open(os.path.join(here, "recomp", "ports.txt")):
    line = line.split("#")[0].split()
    if line:
        ported.add(int(line[0], 16))
seen = set()


def show(a, d, maxd, indent):
    name = funcs.get(a, ("?", []))[0]
    mark = "*" if a in ported else " "
    print("%s%s%04X %s (%d)" % ("  " * indent, mark, a, name, sizes.get(a, 0)))
    if a in seen or d >= maxd or a in ported:
        return
    seen.add(a)
    for c in funcs.get(a, ("", []))[1]:
        if c in byname:
            show(byname[c], d + 1, maxd, indent + 1)


show(int(sys.argv[1], 16) % 0x10000, 0, int(sys.argv[2]) if len(sys.argv) > 2 else 2, 0)
