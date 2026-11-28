#!/usr/bin/env python3
"""A thrown thing that meets a non-solid thing in flight pushes it (1000:16D3A -> 165A1): the diary of level 1 room 0 is thrown (as in verify_throw.py) with thing 412 (the coin, weight 1) placed in its
path at flight height in the original (engine entry + position record written) and in the port (`--sim place:412:x:y:z`, gravity off: the original starts no fall by itself).  Compared: the thing's
x and z at every flight step and where 412 ends up.  Two cases: thrown left with 412 at (36, 24, 36): pushed 4 to the left; thrown up with 412 at (51, 9, 36): the push would put it into the wall (y0 < 7),
so nothing moves and the diary falls straight down.
    python3 tools/verify_flypush.py"""
import os, re, struct, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from emu_call import *
from dosemu import Emu
CASES = [('left', 0x4B, 52, 26, 36, 24, 36), ('up', 0x48, 52, 26, 51, 9, 36)]
bad = 0
for name, fkey, tx, ty, px, py, pz in CASES:
    e = Emu(ROOT + '/unpacked/cadaver1/CAD.EXE', ROOT + '/cadaver1')
    e.pending_keys = [0x0231, 0x011b]
    rw = lambda o: struct.unpack('<H', bytes(e.uc.mem_read(DS + o, 2)))[0]
    rb = lambda o, n: bytes(e.uc.mem_read(DS + o, n))
    steps, rec = [], [False]
    def h(uc, addr, size, user):
        if rec[0]: b = rb(rw(0x2a) + 50 * 18, 6); steps.append((b[0], b[5]))
    e.uc.hook_add(UC_HOOK_CODE, h, None, e.lin(0x1000, 0x247), e.lin(0x1000, 0x247))
    while e.ticks < 900: e.run(max_instr=20000)
    def run(n):
        t = e.ticks
        while e.ticks - t < n: e.run(max_instr=20000)
    def hold(k, n): e.scan_queue.append(k); run(n); e.scan_queue.append(k | 0x80)
    hold(0x50, 60); hold(0x4D, 80); run(6); hold(0x4F, 2); run(6); hold(0x4F, 2); run(6)
    e.wr(0x1F17, 0x58a, struct.pack('<H', 5)); hold(0x23, 2); run(6)
    pr = rw(0x5c); en = rw(0x2a)
    e.wr(0x1F17, pr, bytes([tx, ty, 0])); e.wr(0x1F17, en, bytes([tx, ty, tx - 6, ty - 6])); hold(fkey, 2); run(6)
    sl = rw(0x2a) + 50 * 18
    e.wr(0x1F17, 0x8e, struct.pack('<H', struct.unpack('<H', rb(sl + 6, 2))[0])); e.wr(0x1F17, 0x90, struct.pack('<H', struct.unpack('<H', rb(sl + 8, 2))[0]))
    cnt = rw(0x224)
    j = [j for j in range(cnt) if tuple(rb(rw(0x2a) + 50 * j, 6)) == (40, 23, 39, 22, 2, 0)][0]      # thing 412's engine entry
    s412 = rw(0x2a) + 50 * j; rp = struct.unpack('<H', rb(s412 + 10, 2))[0]
    e.wr(0x1F17, s412, bytes([px, py, px - 1, py - 1, pz + 2, pz])); e.wr(0x1F17, rp, bytes([px, py, pz]))
    rec[0] = True
    hold(0x4F, 2); run(60)
    fl = [s for s in steps if s[0] != 0xFF]
    if os.environ.get('DBG'): print('steps', len(steps), steps[:6], 'player', tuple(rb(rw(0x5c), 3)))
    ox = [s[0] for s in fl]; oz = [s[1] for s in fl]
    # the original's list repeats a sample now and then (two ticks per sample, one step per two ticks): compare the series with repeats removed
    dd = lambda l: [v for i, v in enumerate(l) if i == 0 or v != l[i - 1]]
    final412 = tuple(rb(rp, 2))
    seq = "down:60,right:80,wait:6,end:2,wait:6,end:2,wait:6,space:2,wait:10,right:2,wait:20,end:2,wait:10,space:2,wait:10,tp:%d:%d,%s:2,wait:6,place:412:%d:%d:%d,end:2,wait:80" % (tx, ty, name, px, py, pz)
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', CAD_SNDLOG='1', CAD_NOGRAV='1')
    out = subprocess.run([os.path.join(ROOT, 'build', 'cadaver'), '-g', os.path.join(ROOT, 'cadaver1'), '--scene', 'walk', '--level', '1', '--room', '0', '--sim', seq, '--screenshot', '/tmp/x.bmp'], env=env, capture_output=True).stderr.decode()
    fly = [tuple(map(int, m.groups())) for m in re.finditer(r'FLY 5 at (-?\d+),(-?\d+),(-?\d+)', out)]
    pz_ = [f[2] for f in fly]; pxx = [f[0] for f in fly]
    m = re.search(r'FLYPUSH 5 pushes thing 412 to (\d+),(\d+)', out)
    port412 = (int(m[1]), int(m[2])) if m else (px, py)
    o_x, o_z = dd(ox)[1:], dd(oz)[1:]                       # the original's first sample is the thing's starting place (z 29), the port logs from the first step
    n = min(len(o_z), len(dd(pz_)), 14)
    ok_x = (len(dd(ox)) == 1 and dd(pxx) == dd(ox)) or o_x[:n] == dd(pxx)[:n]         # no horizontal move at all (the blocked case): the same constant x
    ok = o_z[:n] == dd(pz_)[:n] and ok_x and final412 == port412 and n >= 8
    print('%-5s original 412 -> %s, port 412 -> %s; x %s | %s; z %s | %s  %s' % (name, final412, port412, o_x[:8], dd(pxx)[:8], o_z[:8], dd(pz_)[:8], 'OK' if ok else 'DIFF'))
    bad += not ok
sys.exit(1 if bad else 0)
