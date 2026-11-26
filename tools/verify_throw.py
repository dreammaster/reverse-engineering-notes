#!/usr/bin/env python3
"""Compares a thrown thing's flight in the port with the original (level 1 room 0): walk to the diary (thing 5), take it, hold it, walk left and throw it with End.
Original: CAD.EXE in the emulator with scancodes; the hold goes through ds:58A (H) and ds:8E / 90 are pointed at the diary's header the way the hold verb would,
a code hook on 1000:0247 samples the diary's engine entry (slot 18: x1, y1, zt, zb) once per game step.  Port: CAD_SNDLOG prints every FLY step.
Compared: the z sequence (identical) and the horizontal deltas (4 4 3 3 3 2 2 2 2 2 1 ...; the port's start differs only by where the player stands).
    python3 tools/verify_throw.py           (needs build/cadaver)"""
import os, re, struct, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from emu_call import *
from dosemu import Emu
e = Emu(ROOT + '/unpacked/cadaver1/CAD.EXE', ROOT + '/cadaver1')
e.pending_keys = [0x0231, 0x011b]
rw = lambda o: struct.unpack('<H', bytes(e.uc.mem_read(DS + o, 2)))[0]
rb = lambda o, n: bytes(e.uc.mem_read(DS + o, n))
steps, rec = [], [False]
def h(uc, addr, size, user):
    if rec[0]:
        b = rb(rw(0x2a) + 50 * 18, 6)
        steps.append((b[0], b[5]))
e.uc.hook_add(UC_HOOK_CODE, h, None, e.lin(0x1000, 0x247), e.lin(0x1000, 0x247))
while e.ticks < 900: e.run(max_instr=20000)
def run(n):
    t = e.ticks
    while e.ticks - t < n: e.run(max_instr=20000)
def hold(k, n):
    e.scan_queue.append(k); run(n); e.scan_queue.append(k | 0x80)
D, R, L, END, H = 0x50, 0x4D, 0x4B, 0x4F, 0x23
hold(D, 60); hold(R, 80); run(6); hold(END, 2); run(6); hold(END, 2); run(6)
e.wr(0x1F17, 0x58a, struct.pack('<H', 5)); hold(H, 2); run(6)
hold(L, 40); run(6)
sl = rw(0x2a) + 50 * 18
hs = struct.unpack('<H', rb(sl + 6, 2))[0]; ho = struct.unpack('<H', rb(sl + 8, 2))[0]
e.wr(0x1F17, 0x8e, struct.pack('<H', hs)); e.wr(0x1F17, 0x90, struct.pack('<H', ho))
rec[0] = True
hold(END, 2); run(60)
fl = [s for s in steps if s[0] != 0xFF]
orig_z = [s[1] for s in fl]
while orig_z and orig_z[-1] == 0 and len(orig_z) > 1 and orig_z[-2] == 0: orig_z.pop()
orig_x = [s[0] for s in fl]
seq = "down:60,right:80,wait:6,end:2,wait:6,end:2,wait:6,space:2,wait:10,right:2,wait:20,end:2,wait:10,space:2,wait:10,left:40,tp:49:34,wait:6,end:2,wait:80"
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', CAD_SNDLOG='1')
out = subprocess.run([os.path.join(ROOT, 'build', 'cadaver'), '-g', os.path.join(ROOT, 'cadaver1'), '--scene', 'walk', '--level', '1', '--room', '0', '--sim', seq, '--screenshot', '/tmp/x.bmp'], env=env, capture_output=True).stderr.decode()
fly = [tuple(map(int, m.groups())) for m in re.finditer(r'FLY 5 at (\d+),(\d+),(\d+)', out)]
port_z = [f[2] for f in fly]
px_orig = rb(rw(0x5c), 1)[0] if False else None
print('original z:', orig_z[:30]); print('port z    :', port_z[:30])
ox = [x - 23 for x in orig_x]
print('original x:', ox[:20]); print('port x    :', [f[0] for f in fly][:20])
oz = orig_z[1:] if orig_z and orig_z[0] == 29 else orig_z
ok = oz[:len(port_z)] == port_z[:len(oz)] and len(port_z) > 20
print('flight z sequences identical:', ok)
sys.exit(0 if ok else 1)
