#!/usr/bin/env python3
"""Deliver the room events of a visit (28 first visit, 6 entry, 14 timer x2) to every room in the original (emulator, dispatcher 1000:7634) and
in tools/acl_vm.py and compare the effect on thing flags and script variables.   python3 tools/verify_acl_rooms.py [level]"""
import sys, os, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from acl_oracle import *
import things, acl, acl_vm

level = int(sys.argv[1]) if len(sys.argv) > 1 else 4
o = Oracle(level)
e = o.e
secs, names, th, rooms = things.load_level(ROOT + '/cadaver1/DATA/%d' % level)
ok = bad = partial = 0
for n, rr in enumerate(secs[0]['records']):
    if len(rr) < 0x21 or not rr[0x1F]:
        continue
    try:
        hs = acl.room_handlers(rr)
    except Exception:
        continue
    SEQ = (28, 6, 14, 14)
    if not any(h['type'] in SEQ for h in hs):
        continue
    o.restore()
    r = call(e, 0x410E, eax=3, edx=n, ds=0x1F17)
    rp = r['edi'] & 0xFFFF
    base = struct.unpack('<H', e.rd(0x1F17, 0x5A, 2))[0]
    e.wr(0x1F17, base, b''.join(struct.pack('<3H', t | 0x4000, rp, 0) for t in SEQ))
    e.wr(0x1F17, 0x84, struct.pack('<H', base + 6 * len(SEQ)))
    e.wr(0x1F17, 0x226, struct.pack('<H', len(SEQ)))
    e.wr(0x1F17, 0x232, struct.pack('<H', n))
    before = o.state()
    try:
        call(e, 0x7634, ds=0x1F17)
    except Exception as ex:
        print('room', n, 'emulator error', ex); continue
    after = o.state()
    vm = acl_vm.VM({k: bytearray(v) for k, v in before['rec'].items()}, cur=0, room=n)
    vm.vars = bytearray(bytes(before['glob'][0x611 + i] for i in range(0x3F)) + b'\0')
    vm.selected = struct.unpack('<H', e.rd(0x1F17, 0x27A, 2))[0]
    vm.reg = before['glob'][0x704]
    vm.rand = lambda lo, hi: after['glob'][0x704]      # the game's random numbers cannot be reproduced: take the emulator's draw
    arm = [True] * len(hs)
    for t in SEQ:
        acl_vm.fire_event(vm, hs, t, 0, arm)
    errs = []
    for k, rec in vm.things.items():
        if (rec[3] & ~0x08) != (after['rec'][k][3] & ~0x08) or rec[15] != after['rec'][k][15]:
            errs.append(('thing', k, hex(rec[3]), hex(after['rec'][k][3])))
    for i in range(0x3F):
        if i == 0x628 - 0x611:
            continue                                      # ds:628 = message-shown flag (user interface)
        if vm.vars[i] != after['glob'][0x611 + i]:
            errs.append(('var', i, vm.vars[i], after['glob'][0x611 + i]))
    if errs:
        bad += 1
        print('room', n, 'unsupported ops used', sorted(set(vm.unsupported)), 'differences', errs[:4])
    elif vm.unsupported:
        partial += 1
    else:
        ok += 1
print('level %d: %d rooms identical (only implemented ops), %d identical although other ops were skipped, %d differ' % (level, ok, partial, bad))
