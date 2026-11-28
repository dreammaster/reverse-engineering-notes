#!/usr/bin/env python3
"""The fall paths of a thing in the air (1000:16BFC): record flags2 bit 10h selects the path FB9A (-6 -6 -6 -7 -7 -7 -7 -8 ..., the fall table from its 13th entry), otherwise FB8E (-1 -1 -2 -2 -3 ...).
Thing 412 of level 1 room 0 (the coin) gets flags2 = 10h or 0 and is put at z = 60 in the original (engine entry + record written, then 16BFC called to start the fall, z read at every game step)
and in the port (`--sim flags2:412:V,place:412:44:60:60`, its gravity scan starts the fall); the z sequences must be equal.     python3 tools/verify_fall.py"""
import os, re, struct, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from emu_call import *
from dosemu import Emu
bad = 0
for f2 in (0x10, 0):
    e = Emu(ROOT + '/unpacked/cadaver1/CAD.EXE', ROOT + '/cadaver1'); e.pending_keys = [0x0231, 0x011b]
    n = [0]
    e.uc.hook_add(UC_HOOK_CODE, lambda *a: n.__setitem__(0, n[0] + 1), None, e.lin(0x1000, 0x247), e.lin(0x1000, 0x247))
    while e.ticks < 900: e.run(max_instr=20000)
    def steps(k):
        t = n[0]
        while n[0] - t < k: e.run(max_instr=2000, slice_instr=2000)
    steps(6)
    rw = lambda o: struct.unpack('<H', bytes(e.uc.mem_read(DS + o, 2)))[0]
    rb = lambda o, k: bytes(e.uc.mem_read(DS + o, k))
    cnt = rw(0x224); sl = rw(0x2a)
    j = [j for j in range(cnt) if tuple(rb(sl + 50 * j, 6)) == (40, 23, 39, 22, 2, 0)][0]
    ent = sl + 50 * j; rp = struct.unpack('<H', rb(ent + 10, 2))[0]
    e.wr(0x1F17, rp + 15, bytes([f2])); e.wr(0x1F17, ent, bytes([44, 60, 43, 59, 62, 60])); e.wr(0x1F17, rp, bytes([44, 60, 60]))
    call(e, 0x6BFC, esi=ent, ds=0x1F17)
    oz = []
    for i in range(30):
        steps(1); oz.append(rb(ent + 5, 1)[0])
    while oz and oz[-1] == 0 and len(oz) > 1 and oz[-2] == 0: oz.pop()
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', CAD_SNDLOG='1')
    out = subprocess.run([os.path.join(ROOT, 'build', 'cadaver'), '-g', os.path.join(ROOT, 'cadaver1'), '--scene', 'walk', '--level', '1', '--room', '0', '--sim', 'wait:10,flags2:412:%d,place:412:44:60:60,wait:60' % f2, '--screenshot', '/tmp/x.bmp'], env=env, capture_output=True).stderr.decode()
    pz = [int(m.group(1)) for m in re.finditer(r'FLY 412 at \d+,\d+,(-?\d+) ', out)]
    ok = oz == pz
    print('flags2 %02x: original %s\n           port     %s  %s' % (f2, oz, pz, 'OK' if ok else 'DIFF'))
    bad += not ok
sys.exit(1 if bad else 0)
