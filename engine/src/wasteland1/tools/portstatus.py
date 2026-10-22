#!/usr/bin/env python3
"""portstatus.py [-l] : how much of WL.EXE is ported. Lists unported functions whose callees are all ported (the next
candidates, bottom-up) with sizes. -l lists every unported function."""
import os
import re
import sys

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.join(here, "..", "..", "..", "..", "ida_scripts")
funcs = {}      # addr -> (name, callees)
for line in open(os.path.join(root, "dump_callgraph.txt")):
    m = re.match(r"([0-9A-F]+) (\S+) (\d+) : ?(.*)", line)
    if m:
        funcs[int(m.group(1), 16) - 0x10000] = (m.group(2), m.group(4).split())
byname = {v[0]: k for k, v in funcs.items()}
sizes = {}
cur = None
for line in open(os.path.join(root, "dump_wasteland.txt"), errors="replace"):
    m = re.match(r";==== (\S+) @ ([0-9A-F]+) size=(\d+)", line)
    if m:
        sizes[int(m.group(2), 16) - 0x10000] = int(m.group(3))
ported = set()
for line in open(os.path.join(here, "recomp", "ports.txt")):
    line = line.split("#")[0].split()
    if line:
        ported.add(int(line[0], 16))
total = sum(sizes.values())
done = sum(sizes.get(a, 0) for a in ported)
print("%d of %d functions ported, %d of %d code bytes" % (len(ported & set(funcs)), len(funcs), done, total))
callees = {a: [byname[c] for c in v[1] if c in byname] for a, v in funcs.items()}
rest = [a for a in funcs if a not in ported]
if "-l" in sys.argv:
    for a in sorted(rest):
        print("%04X %-28s %4d" % (a, funcs[a][0], sizes.get(a, 0)))
else:
    cand = [a for a in rest if all(c in ported or c == a for c in callees[a])]
    print("unported functions whose callees are all ported (%d):" % len(cand))
    for a in sorted(cand):
        print("%04X %-28s %4d  callers: %s" % (a, funcs[a][0], sizes.get(a, 0), ""))
