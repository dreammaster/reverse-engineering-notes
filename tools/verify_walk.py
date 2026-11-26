#!/usr/bin/env python3
"""Differential test of walking: random direction segments in one room of Cadaver 1, run in the port (--sim ... dump:0) and in the original (emulator, room_enter 1000:DA60, scancodes);
the player positions after every segment are compared (a segment is a few steps, so +-1 unit of timing drift is tolerated and reported separately).
    python3 tools/verify_walk.py LEVEL ROOM SEED [SEGMENTS]"""
import sys, os, re, random, subprocess, struct
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from emu_call import *
lv, room, seed = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]); nseg = int(sys.argv[4]) if len(sys.argv) > 4 else 12
random.seed(seed)
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
exe = os.path.join(ROOT, 'build', 'cadaver')
def port(sim):
    r = subprocess.run([exe, '-g', os.path.join(ROOT, 'cadaver1'), '--scene', 'walk', '--level', str(lv), '--room', str(room), '--sim', sim, '--screenshot', '/tmp/x.bmp'], env=env, capture_output=True, timeout=120)
    return [tuple(map(int, m.groups())) for m in re.finditer(r'DUMP (-?\d+) (-?\d+) (-?\d+) (-?\d+)', r.stderr.decode())]
DIRS = {'right': (0x4D,), 'left': (0x4B,), 'up': (0x48,), 'down': (0x50,), 'rdown': (0x4D, 0x50), 'ldown': (0x4B, 0x50), 'rup': (0x4D, 0x48), 'lup': (0x4B, 0x48)}
# the three left-going directions are left out unless WITH_LEFT is set: after room_enter 1000:DA60 the emulated game opens its map screen (a modal loop) about 5 steps into a left walk in most
# rooms (cause not found: the redraw bookkeeping of 15465 runs on stale state), which stops the player and makes every later segment differ (long walks can stall the same way at other times, so a DIFF needs a look with tools/verify_climb.py-style single runs before it is believed; the climb comparison and the grab comparison do not have this problem)
names = [d for d in DIRS if os.environ.get('WITH_LEFT') or 'left' not in d and d != 'ldown' and d != 'lup']
segs = [(random.choice(names), random.choice([8, 16, 24, 40, 60])) for _ in range(nseg)]
start = port('wait:4,dump:0')[-1][:3]
sim = 'wait:4,' + ','.join('%s:%d,wait:6,dump:0' % (d, n) for d, n in segs)
pres = port(sim)
e = boot(lv)
e.wr(0x1F17, 0x232, struct.pack('<H', room))
call(e, 0x1DA60, edx=room, ds=0x1F17)
rw = lambda o: struct.unpack('<H', bytes(e.uc.mem_read(DS + o, 2)))[0]
pr = rw(0x5C)
e.wr(0x1F17, pr, bytes(start))
def run(n):
    t0 = e.ticks
    while e.ticks - t0 < n: e.run(max_instr=20000)
run(10)
bad = 0
for i, (d, n) in enumerate(segs):
    for k in DIRS[d]: e.scan_queue.append(k)
    run(n)
    for k in DIRS[d]: e.scan_queue.append(k | 0x80)
    run(6)
    o = tuple(bytes(e.uc.mem_read(DS + pr, 3)))
    p = pres[i]
    if p[3] != room or rw(0x232) != room:
        print('room change (port %d, original %d) after %s %d: stop' % (p[3], rw(0x232), d, n)); break
    p = p[:3]
    diff = max(abs(o[0] - p[0]), abs(o[1] - p[1]), abs(o[2] - p[2]))
    st = 'OK' if diff == 0 else ('drift' if diff <= 2 else 'DIFF')
    if st == 'DIFF': bad += 1
    print('%-6s %2d  port %s  original %s  %s' % (d, n, p, o, st))
    # re-sync the original to the port so one drift does not accumulate
    e.wr(0x1F17, pr, bytes([p[0] & 255, p[1] & 255, p[2] & 255]))
print('differences:', bad)
