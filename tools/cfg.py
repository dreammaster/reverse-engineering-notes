#!/usr/bin/env python3
"""Print the code of one entry point of a large, jump-table-driven function: every basic block
that can be reached from `<entry-label>` without passing one of the `--stop` labels, in address
order, through the compact reader (av.py).
Usage: cfg.py <file.asm> <func-first-line> <func-last-line> <entry-label> [--stop lbl,lbl,..] [--max N]
Blocks end at a label or after jmp/retn; jcc targets and fall-through are followed."""
import re
import subprocess
import sys

path, first, last, entry = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
stop = set()
maxblocks = 400
args = sys.argv[5:]
while args:
    a = args.pop(0)
    if a == "--stop":
        stop = set(args.pop(0).split(","))
    elif a == "--max":
        maxblocks = int(args.pop(0))

lines = {}
with open(path, encoding="utf-8", errors="replace") as f:
    for n, line in enumerate(f, 1):
        if n < first:
            continue
        if n > last:
            break
        lines[n] = line.rstrip("\r\n")

labelline = {}
for n, l in lines.items():
    m = re.match(r"^(loc_[0-9A-F]+|def_[0-9A-Za-z_]+):", l)
    if m:
        labelline[m.group(1)] = n

order = sorted(labelline.items(), key=lambda kv: kv[1])
blockend = {}
for i, (lab, n) in enumerate(order):
    blockend[lab] = (order[i + 1][1] - 1) if i + 1 < len(order) else last


def effective_end(lab):
    """The line of the block's first unconditional jump/return: what follows is landing-pad code."""
    n0 = labelline[lab]
    n1 = blockend[lab]
    for n in range(n0, n1 + 1):
        code = lines.get(n, "").split(";")[0].strip()
        if re.match(r"^jmp\s", code) or code.startswith(("retn", "rep retn")):
            return n, True
    return n1, False


def succs(lab):
    n0 = labelline[lab]
    end, terminated = effective_end(lab)
    res = []
    for n in range(n0, end + 1):
        code = lines.get(n, "").split(";")[0].strip()
        m = re.match(r"^(j\w+)\s+(?:short\s+)?(loc_[0-9A-F]+|def_[0-9A-Za-z_]+)$", code)
        if m:
            res.append(m.group(2))
    if not terminated and end < last:
        nxt = [k for k, v in labelline.items() if v == end + 1]
        if nxt:
            res.append(nxt[0])
    return res


seen = []
todo = [entry]
while todo and len(seen) < maxblocks:
    b = todo.pop()
    if b in seen or b in stop or b not in labelline:
        continue
    seen.append(b)
    for s in succs(b):
        if s not in seen:
            todo.append(s)

for b in sorted(seen, key=lambda x: labelline[x]):
    e = effective_end(b)[0]
    print("=== %s  (lines %d-%d)  -> %s" % (b, labelline[b], e, ",".join(succs(b))))
    out = subprocess.run([sys.executable, __file__.replace("cfg.py", "av.py"), path, str(labelline[b]), str(e)],
                         capture_output=True, text=True, encoding="utf-8", errors="replace").stdout
    sys.stdout.write(out)
