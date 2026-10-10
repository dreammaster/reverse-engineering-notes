import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ASM = os.path.join(ROOT, "Deponia_Linux.asm")
FIELDS = os.path.join(ROOT, "src", "deponia1", "vstables", "fieldIds.h")

lines = open(ASM, encoding='utf-8', errors='replace').read().split('\n')[1508676:1523438]
ins = []
labels = {}
for i, l in enumerate(lines):
    if not l.strip() or l.strip().startswith(';'):
        continue
    m = re.match(r'^(loc_[0-9A-F]+|[A-Za-z_]\w*):', l)
    if m:
        labels[m.group(1)] = len(ins)
        rest = l[m.end():].split(';')[0].strip()
        if rest:
            ins.append((m.group(1), rest))
        continue
    t = l.split(';')[0].strip()
    if t:
        ins.append((None, t))


def find_cmp(imm):
    for i, (lab, t) in enumerate(ins):
        if re.match(r'cmp\s+\[rsp\+\w+\+var_C54\],\s*%s$' % imm, t):
            return i


def entry(V):
    pc = find_cmp('63h')
    flag = None
    for _ in range(1000):
        lab, t = ins[pc]
        m = re.match(r'cmp\s+\[rsp\+\w+\+var_C54\],\s*([0-9A-Fa-f]+)h?$', t)
        if m:
            g = m.group(1)
            imm = int(g[:-1] if g.endswith('h') else g, 16)
            flag = (V, imm)
            pc += 1
            continue
        m = re.match(r'(j\w+)\s+(?:short\s+)?(\w+)$', t)
        if m:
            op, tgt = m.groups()
            if op == 'jmp':
                pc = labels[tgt]
                continue
            a, b = flag
            cond = {'jz': a == b, 'jnz': a != b, 'jle': a <= b, 'jg': a > b, 'jl': a < b, 'jge': a >= b, 'jbe': a <= b,
                    'ja': a > b, 'jb': a < b, 'jae': a >= b}[op]
            pc = labels[tgt] if cond else pc + 1
            continue
        return pc


if __name__ == '__main__':
    for V in range(0x63, 0xBB):
        e = entry(V)
        lab = None
        j = e
        while j >= 0 and ins[j][0] is None:
            j -= 1
        print(hex(V), e, ins[j][0] if j >= 0 else None)
