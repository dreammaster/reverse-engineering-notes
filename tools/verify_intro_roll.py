#!/usr/bin/env python3
"""Compares the port's story roll (--scene intro, CAD_INTRO_T = seconds into the roll) with the running INTRO.EXE: the emulator runs the packed INTRO.EXE, and every time the
interpreter enters the page-break wait (a `!` in the script: the loop at 1000:023A, Cadaver 2: 02DA) the frame is saved and the wait counter cs:001E is forced to 0 so that the roll goes on.
Those frames are compared (outside the candle columns, colour tolerance 3 for the emulator's DAC rounding) with the port's frame just before the CR that follows each page break.
    python3 tools/verify_intro_roll.py cadaver1|cadaver2          (needs build/cadaver; the emulator takes a few minutes)
Result: Cadaver 1: the title and the six pages after it match exactly; Cadaver 2: the first pages match exactly or to one scroll row (the capture lands one 70 Hz step later), the credits page
differs by the scroll offset of the capture."""
import os, struct, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from dosemu import Emu
from PIL import Image, ImageChops
game = sys.argv[1] if len(sys.argv) > 1 else 'cadaver1'
isp = game == 'cadaver2'
isr, lo, hi = (0x1056, 0x2DA, 0x310) if isp else (0xFF5, 0x23A, 0x270)
e = Emu(os.path.join(ROOT, game, 'INTRO.EXE'), os.path.join(ROOT, game))
cs_lin = e.lin(0x1000, 0x1E)
frames, n = [], 0
while len(frames) < 7 and n < 3000:
    e.run(max_instr=20000); n += 1
    ip = struct.unpack('<H', bytes(e.uc.mem_read(e.lin(e.r('ss'), e.r('sp')), 2)))[0] if e.r('ip') == isr else e.r('ip')
    w = struct.unpack('<h', bytes(e.uc.mem_read(cs_lin, 2)))[0]
    if w < 0 and lo <= ip < hi:
        fn = '/tmp/vir_emu%d.png' % len(frames)
        e.save_frame(fn); frames.append(fn)
        e.uc.mem_write(cs_lin, struct.pack('<h', 0))
        for _ in range(6): e.run(max_instr=20000)
d = open(os.path.join(ROOT, 'unpacked', game, 'INTRO.EXE'), 'rb').read()
i = d.find(b'          cadaver'); j = d.find(b'#', i)
lines, cur = [], b''
for c in d[i:j]:
    if c == 10: continue
    if c == 13: lines.append(cur); cur = b''
    else: cur += bytes([c])
t, waits = 0.0, []
for l in lines:
    if b'!' in l: t += 540 / 50.16; waits.append(t - 0.05)
    t += 6 / 70.086
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
off = 1 if isp else 0                  # the Cadaver 2 loop also sees the start of the script (empty window) first
for k, tw in enumerate(waits):
    subprocess.run([os.path.join(ROOT, 'build', 'cadaver'), '-g', os.path.join(ROOT, game), '--scene', 'intro', '--screenshot', '/tmp/vir_port%d.bmp' % k], env=dict(env, CAD_INTRO_T=str(tw)), capture_output=True)
    if k + off >= len(frames): break
    a = Image.open('/tmp/vir_port%d.bmp' % k).convert('RGB'); b = Image.open(frames[k + off]).convert('RGB')
    dd = ImageChops.difference(a.crop((40, 0, 280, 200)), b.crop((40, 0, 280, 200)))
    print('page', k, 'differing pixels', sum(1 for p in dd.getdata() if max(p) > 3))
