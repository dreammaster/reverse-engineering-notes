#!/usr/bin/env python3
"""Fuzz the implemented opcodes of tools/acl_vm.py against the original handlers (emulator):  python3 tools/verify_acl_ops.py [level] [n]"""
import sys, os, random, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from acl_oracle import *
import acl, acl_vm

level = int(sys.argv[1]) if len(sys.argv) > 1 else 4
N = int(sys.argv[2]) if len(sys.argv) > 2 else 8
o = Oracle(level)
ids = list(o.recptr)
IMPL = [1, 26, 15, 16, 17, 24, 18, 19, 20, 25, 21, 54, 55, 68, 67, 30, 31, 32, 33, 5, 38, 39, 40, 43]
random.seed(3)
bad = 0
for op in IMPL:
    for trial in range(N):
        n = acl.OPLEN[op]
        t = random.choice(ids)
        cur = random.choice(ids)
        if op in (5,): a = bytes([random.randrange(1, 100)])
        elif op in (38, 39): a = bytes([random.randrange(0, 8), random.randrange(0, 200)])
        elif op == 40: a = bytes([random.randrange(0, 8), random.randrange(0, 4), random.randrange(0, 10)])
        elif op == 43: a = struct.pack('>H', t) + bytes([random.choice([0xFE, o.e.rd(0x1F17, o.recptr[t] + 10, 1)[0], 3])])
        elif n == 0: a = b''
        else: a = struct.pack('>H', t)
        ds_pokes = [(o.recptr[t] + 3, random.randrange(256)), (o.recptr[t] + 15, random.randrange(256)), (o.recptr[cur] + 3, random.randrange(256))]
        vp = [(0x611 + i, random.randrange(0, 10)) for i in range(8)]
        b, af, used = o.run_op(op, a, cur=cur, poke=ds_pokes + vp + [(0x605, random.randrange(0, 3))])
        if af is None:
            print('oracle error', op); continue
        vm = acl_vm.VM({k: bytearray(v) for k, v in b['rec'].items()}, cur=cur, room=o.e.rd(0x1F17, 0x232, 1)[0])
        vm.vars = bytearray(b'' .join(bytes([b['glob'][0x611 + i]]) for i in range(0x3F)) + b'\0')
        vm.c605 = b['glob'][0x605]
        vm.op(op, a)
        # compare tracked state: rec[3], rec[15] of all things, vars, c605
        errs = []
        for k, r in vm.things.items():
            if r[3] != af['rec'][k][3] or r[15] != af['rec'][k][15]:
                errs.append(('rec', k, bytes(r[3:4]) + bytes(r[15:16]), bytes(af['rec'][k][3:4]) + bytes(af['rec'][k][15:16])))
        for i in range(0x3F):
            if vm.vars[i] != af['glob'][0x611 + i]:
                errs.append(('var', i, vm.vars[i], af['glob'][0x611 + i]))
        if vm.c605 != af['glob'][0x605]:
            errs.append(('c605', vm.c605, af['glob'][0x605]))
        if errs:
            bad += 1
            print('MISMATCH op', op, 'thing', t, 'cur', cur, a.hex(), errs[:3])
print('implemented ops checked: %d, mismatching trials: %d' % (len(IMPL), bad))
