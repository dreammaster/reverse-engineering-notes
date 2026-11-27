#!/usr/bin/env python3
"""Pushing differential in the start room of level 1 (no room_enter hack: the original is booted normally): random direction segments, the player position and the boxes of
every object (x1 y1 x0 y0 zt zb: the engine slots of the original, `OBJ` lines of the port with CAD_DUMPBOX) are compared after every segment.
    python3 tools/verify_push2.py SEED [SEGMENTS]"""
import os, random, re, struct, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from emu_call import *
GAME = os.environ.get('GAME', 'cadaver1')           # cadaver2: the Payoff (GAME.EXE, language 1, then P at the restore prompt; TATA level 1)
PAY = GAME == 'cadaver2'
if PAY: DS = 0x1F1C0
seed = int(sys.argv[1]); nseg = int(sys.argv[2]) if len(sys.argv) > 2 else 10
random.seed(seed)
DIRS = {'right': (0x4D,), 'left': (0x4B,), 'up': (0x48,), 'down': (0x50,), 'rdown': (0x4D, 0x50), 'ldown': (0x4B, 0x50), 'rup': (0x4D, 0x48), 'lup': (0x4B, 0x48)}
NAMES = [d for d in DIRS if d not in ('up', 'lup')]
segs = [(random.choice(NAMES), random.choice([60, 90, 120])) for _ in range(nseg)]
START = os.environ.get('START', '48:22' if PAY else '')      # where the original puts the player at the start (Payoff: 48, 22); the port's own default placement differs
sim = 'wait:2,' + ('tp:%s,wait:2,' % START if START else '') + ','.join('%s:%d,wait:40,dump:0' % (d, n) for d, n in segs)
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', CAD_DUMPBOX='1')
out = subprocess.run([os.path.join(ROOT, 'build', 'cadaver'), '-g', os.path.join(ROOT, GAME), '--scene', 'walk', '--level', '1', '--room', '0', '--sim', sim, '--screenshot', '/tmp/x.bmp'] + (['--tata'] if PAY else []), env=env, capture_output=True).stderr.decode()
blocks, cur = [], None
for ln in out.splitlines():
    m = re.match(r'DUMP (-?\d+) (-?\d+) (-?\d+) (-?\d+)', ln)
    if m: cur = {'p': tuple(map(int, m.groups()))[:3], 'o': []}; blocks.append(cur); continue
    m = re.match(r'OBJ (\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (\d+)', ln)
    if m and cur is not None and int(m.group(8)) != 3: cur['o'].append(tuple(int(v) & 0xFF for v in m.groups()[1:7]))      # creatures (class 3) move on their own: left out
from dosemu import Emu
e = Emu(ROOT + ('/unpacked/cadaver2/GAME.EXE' if PAY else '/unpacked/cadaver1/CAD.EXE'), ROOT + '/' + GAME)
e.pending_keys = [0x0231, 0x1970] if PAY else [0x0231, 0x011b]
nstep = [0]
def _h(uc, addr, size, user): nstep[0] += 1
e.uc.hook_add(UC_HOOK_CODE, _h, None, e.lin(0x1000, 0x247), e.lin(0x1000, 0x247))      # one game step per call: the segments are counted in steps (n / 2), not in emulator ticks
while e.ticks < (3000 if PAY else 900): e.run(max_instr=20000)
rw = lambda o: struct.unpack('<H', bytes(e.uc.mem_read(DS + o, 2)))[0]
pr = rw(0x5c)
def run(n):
    t = e.ticks
    while e.ticks - t < n: e.run(max_instr=20000)
class Stalled(Exception): pass
def run_steps(k):
    t, slices = nstep[0], 0
    while nstep[0] - t < k:
        e.run(max_instr=2000, slice_instr=2000)
        slices += 1
        if slices > 3000 * (k + 2): raise Stalled()          # no game steps any more: the original left the room (an exit) or sits in a modal screen
bad = 0
RESYNC = os.environ.get('NORESYNC') is None      # after a drift (a game step more or less in the emulator's segment) the original's player and object boxes are overwritten with the port's, so that one drift does not spoil the rest of the run
def cls(sl, j):
    b = bytes(e.uc.mem_read(DS + sl + 50 * j, 10)); hs, ho = struct.unpack('<H', b[6:8])[0], struct.unpack('<H', b[8:10])[0]
    return e.uc.mem_read(hs * 16 + ho + 22, 1)[0]
near = lambda a, b: all(abs(x - y) <= 2 for x, y in zip(a, b))
for i, (d, n) in enumerate(segs):
    try:
        run_steps(1)                                              # start on a game step boundary
        for k in DIRS[d]: e.scan_queue.append(k)
        run_steps(n // 2 + 1)                                    # the key is seen by the next step: n / 2 steps of movement like the port's n ticks
        for k in DIRS[d]: e.scan_queue.append(k | 0x80)
        run(44)                                                   # let the 17-step room events of both programs run before comparing (ticks: the game step hook is not reached in every idle tick)
    except Stalled:
        e.save_frame('/tmp/push2_stall.png')
        print('%-6s %2d original stalled (left the room or a modal screen); the port is at %s in room %d' % (d, n, blocks[i]['p'] if i < len(blocks) else '?', 0)); break
    if i >= len(blocks): break
    sl = rw(0x2a); cnt = rw(0x224)
    objs = [tuple(e.uc.mem_read(DS + sl + 50 * j, 6)) for j in range(1, cnt) if cls(sl, j) != 3]
    objs = [tuple(o) for o in objs if o[0] != 0xFF]
    p = tuple(bytes(e.uc.mem_read(DS + pr, 3)))
    pb = sorted(blocks[i]['o'])
    moved = [o for o in pb if not any(near(o, q) for q in objs)]
    gone = [q for q in objs if not any(near(o, q) for o in pb)]
    dp = max(abs(a - b) for a, b in zip(p, blocks[i]['p']))
    st = 'OK' if dp == 0 and not moved and not gone else ('drift' if dp <= 2 and not moved and not gone else 'DIFF')
    print('%-6s %2d port %s original %s objects: port-only %d original-only %d  %s' % (d, n, blocks[i]['p'], p, len(moved), len(gone), st))
    if st == 'DIFF':
        bad += 1
        print('   port-only:', moved); print('   original-only:', gone)
        ex_p = [o for o in pb if o not in objs]; ex_o = [q for q in sorted(objs) if q not in pb]
        print('   exact mismatches (the tolerance of 2 hides them in the comparison above): port', ex_p, 'original', ex_o)
        print('   (the segments after the first difference are not comparable: the run is not re-synchronised)'); break
    if st == 'drift' and RESYNC:
        e.wr(0x1F17, pr, bytes(blocks[i]['p']))
        for j in range(1, cnt):
            if cls(sl, j) == 3: continue
            cur_b = tuple(e.uc.mem_read(DS + sl + 50 * j, 6))
            cand = [o for o in pb if near(o, cur_b)]
            if cand: e.wr(0x1F17, sl + 50 * j, bytes(min(cand, key=lambda o: sum(abs(a - b) for a, b in zip(o, cur_b)))))
print('differences:', bad)
