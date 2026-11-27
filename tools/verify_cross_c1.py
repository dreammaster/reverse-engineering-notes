#!/usr/bin/env python3
"""Cadaver 1 level 1: the port's door crossings against the logs of tools/oracle_landing.py (the original: the player placed next to each exit and walked through):
the room reached (or none) must agree, `-> None` = the original did not cross.   python3 tools/oracle_landing.py 0 120 > land.log;  python3 tools/verify_cross_c1.py land.log"""
import os, re, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ok = bad = 0
for l in open(sys.argv[1]):
    m = re.match(r'(\d+) (\d+) door (\d+) (\w) \((-?\d+), (-?\d+)\) -> (.*)', l)
    if not m: continue
    room, k, door, dd, px, py = int(m[1]), int(m[2]), int(m[3]), m[4], int(m[5]), int(m[6])
    res = m[7]
    orig = re.match(r'\((\d+),', res)
    orig_room = int(orig[1]) if orig else room
    sim = 'wait:2,unlock:-1,tp:%d:%d,wait:2,%s:30,dump:0' % (px, py, {'N': 'up', 'S': 'down', 'E': 'right', 'W': 'left'}[dd])
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    out = subprocess.run([ROOT + '/build/cadaver', '-g', ROOT + '/cadaver1', '--scene', 'walk', '--level', '1', '--room', str(room), '--sim', sim, '--screenshot', '/tmp/x.bmp'], env=env, capture_output=True).stderr.decode()
    d = re.findall(r'DUMP (-?\d+) (-?\d+) (-?\d+) (-?\d+)', out)
    pr = int(d[-1][3]) if d else None
    if pr == orig_room: ok += 1
    else: bad += 1; print('DIFF room', room, 'door', door, dd, 'original ->', orig_room, 'port ->', pr)
print('%d crossings agree, %d differ' % (ok, bad))
