#!/usr/bin/env python3
"""List every CONUNIT (unit 13) UNITREAD/UNITWRITE call site in the p-code.

Usage: conunit_calls.py [pcode_dir]
Reads extracted/pcode/*.pd (tools/pcode_dis.py output); for each `LDO 60 ; CONUNIT`
prints the proc, the operand list up to the CSP, and the decoded blocknumber
(the 4th UNITxxx argument) when it is a constant.
"""
import re, sys, glob, os, collections

d = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', 'extracted', 'pcode')
ins_re = re.compile(r'^\s+([0-9a-f]{4})\s+((?:[0-9a-f]{2} ?)+?)\s{2,}(\S+)\s*(.*)$')
hdr_re = re.compile(r'^; ---- (\w+):proc (\d+) ')
stats = collections.defaultdict(list)

def const(op, arg):
    if op == 'SLDC':
        return int(arg.split()[0])
    return None

for f in sorted(glob.glob(os.path.join(d, '*.pd'))):
    seg = None; proc = None; buf = []
    for line in open(f, encoding='utf-8', errors='replace'):
        m = hdr_re.match(line)
        if m:
            seg, proc = m.group(1), int(m.group(2)); buf = []; continue
        m = ins_re.match(line)
        if not m: continue
        addr, _, op, arg = m.groups()
        buf.append((addr, op, arg.split(';')[0].strip()))
        if op == 'CSP' and arg.split()[0] in ('UNITWRITE', 'UNITREAD'):
            # find LDO 60 within the last 14 instrs
            idx = None
            for i in range(len(buf) - 2, max(-1, len(buf) - 16), -1):
                if buf[i][1] == 'LDO' and buf[i][2] == '60':
                    idx = i; break
            if idx is None: continue
            ops = buf[idx + 1:-1]
            # args: buf, len, blk, mode, ctl  -> blk is 3rd-from... take SLDC consts
            consts = [const(o[1], o[2]) for o in ops]
            stats[(arg.split()[0], seg, proc)].append((buf[idx][0], [(o[1], o[2]) for o in ops]))
for (kind, seg, proc), sites in sorted(stats.items()):
    for addr, ops in sites:
        print(f'{seg:9s} proc {proc:3d} @{addr} {kind:9s} ' + '; '.join(f'{o} {a}'.strip() for o, a in ops))
