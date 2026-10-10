import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ASM = os.path.join(ROOT, "Deponia_Linux.asm")
FIELDS = os.path.join(ROOT, "src", "deponia1", "vstables", "fieldIds.h")

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from uvdisp import ins, labels, entry

inv = {}
for n, i in labels.items():
    inv.setdefault(i, []).append(n)


def lab_before(i):
    j = i
    while j >= 0 and j not in inv:
        j -= 1
    return (inv[j][0], i - j) if j >= 0 else None


CMP = re.compile(r'cmp\s+\[rsp\+\w+\+var_C54\],\s*([0-9A-Fa-f]+)h?$')


def succs(pc, V):
    lab, t = ins[pc]
    m = re.match(r'(j\w+)\s+(?:short\s+)?(\w+)$', t)
    if m:
        op, tgt = m.groups()
        if tgt not in labels:
            return []
        if op == 'jmp':
            return [labels[tgt]]
        if pc > 0:
            pm = CMP.match(ins[pc - 1][1])
            if pm:
                g = pm.group(1)
                b = int(g[:-1] if g.endswith('h') else g, 16)
                a = V
                cond = {'jz': a == b, 'jnz': a != b, 'jle': a <= b, 'jg': a > b, 'jl': a < b, 'jge': a >= b,
                        'jbe': a <= b, 'ja': a > b, 'jb': a < b, 'jae': a >= b}[op]
                return [labels[tgt]] if cond else [pc + 1]
        return [labels[tgt], pc + 1]
    if t.startswith('ret'):
        return []
    return [pc + 1]


def reach(V, start=None):
    start = entry(V) if start is None else start
    seen = set()
    stack = [start]
    while stack:
        p = stack.pop()
        if p in seen or p >= len(ins):
            continue
        seen.add(p)
        stack.extend(succs(p, V))
    return seen


if __name__ == '__main__':
    prev = None
    for V in range(0x63, 0xBB):
        r = reach(V)
        print(hex(V), len(r), end='')
        if prev is not None:
            print('  only-here', len(prev - r), 'new', len(r - prev), end='')
        print()
        prev = r


def regions(V):
    """labels of the blocks reached, in address order of first instruction"""
    r = sorted(reach(V))
    out = []
    for p in r:
        if p in inv:
            out.append(inv[p][0])
    return out
