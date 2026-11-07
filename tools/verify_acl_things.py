#!/usr/bin/env python3
"""Deliver thing events 7 and 9 (touched by the player: arg 0) to every thing that has handlers, in the original (emulator, dispatcher 1000:7634) and in
tools/acl_vm.py (fire_event with cur = the thing), and compare thing flags and script variables.   python3 tools/verify_acl_things.py [level]"""
import sys, os, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from acl_oracle import *
import things, acl, acl_vm

level = int(sys.argv[1]) if len(sys.argv) > 1 else 1
o = Oracle(level)
e = o.e
secs, names, th, rooms = things.load_level(ROOT + '/cadaver1/DATA/%d' % level)
SEQ = (7, 9)
ok = partial = bad = 0
for t, rec in enumerate(secs[3]['records']):
    if t not in o.recptr or len(rec) < 17 or not rec[11]:
        continue
    try:
        hs = acl.thing_handlers(rec)
    except Exception:
        continue
    if not any(h['type'] in SEQ for h in hs):
        continue
    o.restore()
    rp = o.recptr[t]
    base = struct.unpack('<H', e.rd(0x1F17, 0x5A, 2))[0]
    e.wr(0x1F17, base, b''.join(struct.pack('<3H', ty, rp, 0) for ty in SEQ))
    e.wr(0x1F17, 0x84, struct.pack('<H', base + 6 * len(SEQ)))
    e.wr(0x1F17, 0x226, struct.pack('<H', len(SEQ)))
    room = e.rd(0x1F17, 0x232, 1)[0]
    before = o.state()
    try:
        call(e, 0x7634, ds=0x1F17)
    except Exception as ex:
        print('thing', t, 'emulator error', ex); continue
    after = o.state()
    vm = acl_vm.VM({k: bytearray(v) for k, v in before['rec'].items()}, cur=t, room=room)
    vm.vars = bytearray(bytes(before['glob'][0x611 + i] for i in range(0x3F)) + b'\0')
    vm.selected = struct.unpack('<H', e.rd(0x1F17, 0x27A, 2))[0]
    vm.reg = before['glob'][0x704]
    vm.rand = lambda lo, hi: after['glob'][0x704]
    arm = [True] * len(hs)
    for ty in SEQ:
        acl_vm.fire_event(vm, hs, ty, 0, arm, cur=t, follow=False)
    errs = []
    for k, r in vm.things.items():
        if (r[3] & ~0x08) != (after['rec'][k][3] & ~0x08) or r[15] != after['rec'][k][15]:
            errs.append(('thing', k, hex(r[3]), hex(after['rec'][k][3])))
    for i in range(0x3F):
        if i != 0x628 - 0x611 and vm.vars[i] != after['glob'][0x611 + i]:
            errs.append(('var', i, vm.vars[i], after['glob'][0x611 + i]))
    if errs:
        bad += 1
        print('thing', t, 'unsupported', sorted(set(vm.unsupported)), errs[:3])
    elif vm.unsupported:
        partial += 1
    else:
        ok += 1
print('level %d: %d things identical, %d identical although other ops were skipped, %d differ' % (level, ok, partial, bad))
