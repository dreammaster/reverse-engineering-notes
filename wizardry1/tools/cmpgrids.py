#!/usr/bin/env python3
"""
Compare composed-screen dumps (NNN_KK.grid) from the oracle (tools/wizvm.py --ops)
and from the C++ engine (WIZ1 ... gridDir), screen by screen.

    cmpgrids.py <oracleDir> <engineDir> [--skip N]

Engine screen i is compared with oracle screen i+N (the oracle starts at the
title / disk prompts).  Prints the first differing cells of each bad screen.
Exit status 1 if any screen differs.
"""
import os, sys

def load(path):
    rows = open(path).read().split('\n')
    cells = [r.split() for r in rows[:25]]
    return cells, rows[25]

def grids(d):
    out = {}
    for f in sorted(os.listdir(d)):
        if f.endswith('.grid'):
            out[int(f[:3])] = f
    return out

def main():
    a = [x for x in sys.argv[1:] if not x.startswith('--')]
    skip = 0
    if '--skip' in sys.argv:
        skip = int(sys.argv[sys.argv.index('--skip') + 1])
        a = [x for x in a if x != str(skip)]
    od, ed = a[0], a[1]
    og, eg = grids(od), grids(ed)
    bad = 0
    for i in sorted(eg):
        j = i + skip
        if j not in og:
            print(f'engine screen {i}: no oracle screen {j}'); bad += 1; continue
        oc, ov = load(os.path.join(od, og[j]))
        ec, ev = load(os.path.join(ed, eg[i]))
        diffs = [(x, y, oc[y][x], ec[y][x]) for y in range(25) for x in range(40) if oc[y][x] != ec[y][x]]
        key_o, key_e = og[j][4:6], eg[i][4:6]
        status = 'ok' if not diffs and ov == ev else 'MISMATCH'
        if status != 'ok': bad += 1
        print(f'{i:3d} <-> {j:3d}  keys {key_e}/{key_o}  {status}'
              + (f'  {len(diffs)} cells' if diffs else '') + ('' if ov == ev else f'  vram {ev[5:]} vs {ov[5:]}'))
        for x, y, o, e in diffs[:8]:
            print(f'      ({x},{y}) oracle {o}  engine {e}')
    print('bad:', bad)
    sys.exit(1 if bad else 0)

main()
