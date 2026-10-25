#!/usr/bin/env python3
"""
Inventory of every procedure in SYSTEM.PASCAL that draws, directly or through
a helper, built from the p-code listings (extracted/pcode/*.pd).

  direct   -- the proc issues UNITREAD/UNITWRITE(CONUNIT, ..., selector, ...)
  via      -- it calls (CXP/CLP/CIP/CGP) a proc that draws
  roots    -- the WIZARDRY resident UI procs everything else funnels through

Writes docs/drawing-inventory.md (markdown) when given --md.
"""
import os, re, sys, collections

ROOT = os.path.join(os.path.dirname(__file__), '..')
PCODE = os.path.join(ROOT, 'extracted', 'pcode')
hdr = re.compile(r'^; ---- (\w+):proc (\d+)(?:\s+=\s+(\w+))?\s+entry')
ins = re.compile(r'^\s+([0-9a-f]{4})\s+(?:[0-9a-f]{2} ?)+\s+(\S+)\s*(.*)$')

procs = {}                       # (seg, n) -> dict
cur = None
for fn in sorted(os.listdir(PCODE)):
    if not fn.endswith('.pd'): continue
    for line in open(os.path.join(PCODE, fn), encoding='utf-8', errors='replace'):
        m = hdr.match(line)
        if m:
            cur = (m.group(1), int(m.group(2)))
            procs[cur] = dict(name=m.group(3) or '', calls=set(), direct=collections.Counter(),
                              buf=[])
            continue
        if cur is None: continue
        m = ins.match(line)
        if not m: continue
        op, arg = m.group(2), m.group(3).split(';')[0].strip()
        P = procs[cur]
        P['buf'].append((op, arg))
        if op == 'CXP':
            seg, n = arg.split()[0].split(',')
            P['calls'].add((seg, int(n)))
        elif op in ('CLP', 'CIP', 'CGP'):
            P['calls'].add((cur[0], int(arg.split()[0])))
        elif op == 'CSP' and arg.split()[0] in ('UNITREAD', 'UNITWRITE'):
            b = P['buf']
            for i in range(len(b) - 2, max(-1, len(b) - 16), -1):
                if b[i][0] == 'LDO' and b[i][1].split()[0] == '60':
                    ops = b[i + 1:-1]
                    # 6-value call: buf, X, selector, Z, W after the unit LDO
                    vals = []
                    # selector = third pushed value after the unit: find SLDC items
                    sel = None
                    k = 0
                    for o, a in ops:
                        if o in ('SLDC',) and False: pass
                    # simple: the selector is the 3rd operand group; handle common shapes
                    consts = [(o, a) for o, a in ops]
                    # position-based: [buf-ish, SLDC 0 (X), SLDC sel, Z..., W]
                    for j, (o, a) in enumerate(consts):
                        if o == 'SLDC' and a == '0' and j + 1 < len(consts) and consts[j + 1][0] == 'SLDC':
                            sel = int(consts[j + 1][1]); break
                    P['direct'][(arg.split()[0], sel)] += 1
                    break

# transitive closure
DRAW_SELS = {1, 2, 3, 4, 5, 13, 14, 15, 17, 18, 23, 30}      # selectors that touch the screen
for v in procs.values():
    v['direct'] = collections.Counter({k: n for k, n in v['direct'].items()
                                       if k[0] == 'UNITWRITE' and k[1] in DRAW_SELS})
draws = {k for k, v in procs.items() if v['direct']}
changed = True
while changed:
    changed = False
    for k, v in procs.items():
        if k not in draws and v['calls'] & draws:
            draws.add(k); changed = True

def label(k):
    v = procs[k]
    return f"{k[0]}:{k[1]}" + (f" ({v['name']})" if v['name'] else '')

if __name__ == '__main__':
    md = '--md' in sys.argv
    bysegs = collections.defaultdict(list)
    for k in sorted(procs):
        bysegs[k[0]].append(k)
    out = []
    SEL = {1: 'init', 2: 'layout', 3: 'present', 4: 'putchar', 5: 'DRAWLINE', 13: 'blit13',
           14: 'loadcharset', 17: 'clearattr', 18: 'loadmonsters', 19: 'pause', 22: 'ioctl?',
           23: 'titlebitmap', 24: 'sound', 25: 'vdisk', 26: 'strcat', 29: 'strpos',
           30: 'putstring', 31: 'vdisk2'}
    tot_direct = sum(1 for k in procs if procs[k]['direct'])
    out.append(f"{len(procs)} procedures; {tot_direct} call CONUNIT directly; {len(draws)} draw (directly or transitively)\n")
    for seg, ks in bysegs.items():
        out.append(f"\n### {seg}\n")
        out.append("| proc | name | direct CONUNIT calls | draws via helper |")
        out.append("|--:|---|---|---|")
        for k in ks:
            v = procs[k]
            if k not in draws: continue
            d = ', '.join(f"{('RD' if a=='UNITREAD' else 'WR')} {s}:{SEL.get(s, '?')}" + (f" x{n}" if n > 1 else '')
                          for (a, s), n in sorted(v['direct'].items(), key=lambda t: (t[0][1] or 0)))
            via = sorted(c for c in v['calls'] if c in draws)
            vs = ', '.join(f"{c[0]}:{c[1]}" if c[0] != seg else str(c[1]) for c in via[:8]) + (' ...' if len(via) > 8 else '')
            out.append(f"| {k[1]} | {v['name']} | {d} | {vs} |")
    text = '\n'.join(out)
    if md:
        open(os.path.join(ROOT, 'docs', 'drawing-inventory.md'), 'w', encoding='utf-8').write(
            "# Drawing-code inventory (generated by tools/drawcalls.py)\n\n" + text + "\n")
    print(text[:3000])
