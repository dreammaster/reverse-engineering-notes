#!/usr/bin/env python3
"""junk.py <wlrctest output> : merges the "junk candidates" of a `wlrctest --ported` run into ports.txt (the registers and
flags that differ between a port and the original in any run become its junk= list). Review the diff before committing:
a register that is a real result of the function must be fixed in the port instead."""
import os
import re
import sys

ports = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ports.txt")
cand = {}
active = False
for line in open(sys.argv[1], errors="replace"):
    if "junk candidates" in line:
        active = True
        continue
    if active:
        m = re.match(r"([0-9A-F]+)\s+(\S+)$", line.strip())
        if m:
            cand[int(m.group(1), 16)] = m.group(2).split(",")
out = []
for line in open(ports):
    raw = line.rstrip("\n")
    body = raw.split("#")[0].split()
    if body and int(body[0], 16) in cand:
        regs = sorted(set(cand[int(body[0], 16)]))
        old = [x for x in body[2:] if x.startswith("junk=")]
        have = set(old[0][5:].split(",")) if old else set()
        regs = sorted(have | set(regs))
        rest = [x for x in body[2:] if not x.startswith("junk=")]
        raw = " ".join(body[:2] + (["junk=" + ",".join(regs)] if regs else []) + rest)
    out.append(raw)
open(ports, "w", newline="\n").write("\n".join(out) + "\n")
