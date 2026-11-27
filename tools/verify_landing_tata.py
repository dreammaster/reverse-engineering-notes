#!/usr/bin/env python3
"""Landing after a door in the Payoff's TATA levels against the original (the Payoff counterpart of oracle_landing.py + verify_landing.py): GAME.EXE is booted into the level, for every
room 1000:DAB1 enters it, the player is put next to each exit and walks through it; the position in the new room is compared with tools/landing.py (= the port's room_landing).
    python3 tools/verify_landing_tata.py [LEVEL] [FIRST_ROOM LAST_ROOM]"""
import os, re, struct, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from emu_call import *
from dosemu import Emu
import things, mjm, objsprites, roomexits, landing
DSP = 0x1F1C
LV = int(sys.argv[1]) if len(sys.argv) > 1 else 1
e = Emu(ROOT + '/unpacked/cadaver2/GAME.EXE', ROOT + '/cadaver2')
e.pending_keys = [0x0231, 0x1970]
if LV != 1:
    e.uc.hook_add(UC_HOOK_CODE, lambda uc, a, sz, u: e.wr(DSP, 0x708, bytes([LV - 1])), None, e.lin(0x1000, 0xCB24), e.lin(0x1000, 0xCB24))
while e.ticks < 3000: e.run(max_instr=20000)
full = bytes(e.uc.mem_read(0, 0xA0000)); regs = {r: e.r(r) for r in REGS}
def restore():
    e.uc.mem_write(0, full)
    for r, v in regs.items(): e.w(r, v)
d = ROOT + '/cadaver2/TATA/%d' % LV
secs, names, th, rooms = things.load_level(d)
rec = secs[0]['records']
rd = lambda: bytes(e.uc.mem_read(DSP * 16, 0x10000))
KEY = {'N': 0x48, 'S': 0x50, 'E': 0x4D, 'W': 0x4B}
first = int(sys.argv[2]) if len(sys.argv) > 3 else 0; last = int(sys.argv[3]) if len(sys.argv) > 3 else len(rec)
ok = bad = skipped = 0
OFF = int(os.environ.get('OFF', '0'))      # shifts the player's place along the wall when it crosses (the original keeps the low three bits of the place)
PORT = os.environ.get('PORT') is not None      # PORT=1: also run the port (build/cadaver) through every door and compare room and place with the original, refusals included
def port_cross(room, px, py, dd):
    sim = 'wait:2,unlock:-1,tp:%d:%d,wait:2,%s:30,dump:0' % (px, py, {'N': 'up', 'S': 'down', 'E': 'right', 'W': 'left'}[dd])
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    out = subprocess.run([ROOT + '/build/cadaver', '-g', ROOT + '/cadaver2', '--tata', '--scene', 'walk', '--level', str(LV), '--room', str(room), '--sim', sim, '--screenshot', '/tmp/x.bmp'], env=env, capture_output=True).stderr.decode()
    m = re.findall(r'DUMP (-?\d+) (-?\d+) (-?\d+) (-?\d+)', out)
    return (int(m[-1][3]), (int(m[-1][0]), int(m[-1][1])), int(m[-1][2])) if m else None
for room in range(first, last):
    r = rec[room]
    if len(r) < 22 or r[4] * r[5] == 0: continue
    restore()
    e.wr(DSP, 0x232, struct.pack('<H', room))
    try: call(e, 0x1DAB1, edx=room, ds=DSP)
    except Exception: continue
    ds = rd(); ss = bytes(e.uc.mem_read(e.r('ss') * 16, 0x10000))
    if ds[0x232] | ds[0x233] << 8 != room: continue
    bp = ds[0x3a] | ds[0x3b] << 8; nex = ds[0x22e]
    exits = [ss[bp + 50 * i: bp + 50 * i + 50] for i in range(nex)]
    snap = bytes(e.uc.mem_read(0, 0xA0000)); rg = {q: e.r(q) for q in REGS}
    w8, h8 = r[4] * 8, r[5] * 8
    for k, ex in enumerate(exits):
        x1, y1, x0, y0 = [v if v < 128 else v - 256 for v in ex[:4]]
        if y1 < 8: dd = 'N'; px, py = (x0 + x1) // 2 + 3, 12
        elif y0 >= h8 - 1: dd = 'S'; px, py = (x0 + x1) // 2 + 3, h8 - 1
        elif x1 < 8: dd = 'W'; px, py = 12, (y0 + y1) // 2 + 3
        else: dd = 'E'; px, py = w8 - 1, (y0 + y1) // 2 + 3
        if dd in 'NS': px += OFF
        else: py += OFF
        e.uc.mem_write(0, snap)
        for q, v in rg.items(): e.w(q, v)
        ds = rd(); p = ds[0x5c] | ds[0x5d] << 8; en = ds[0x2a] | ds[0x2b] << 8
        for dk in range(len(secs[1]['records'])): e.wr(DSP, 0x5194 + 8 * dk + 2, b'\0\0')   # the lock words of all door records (8-byte records from ds:5194 in TATA level 1): locked doors would stop the walk (that rule is checked separately)
        e.wr(DSP, p, bytes([px & 255, py & 255, 0])); e.wr(DSP, en, bytes([px & 255, py & 255, (px - 6) & 255, (py - 6) & 255]))
        e.scan_queue.append(KEY[dd]); t0 = e.ticks; res = None
        while e.ticks < t0 + 80:
            e.run(max_instr=20000)
            ds = rd()
            if ds[0x232] | ds[0x233] << 8 != room:
                for _ in range(3): e.run(max_instr=20000)
                ds = rd(); p = ds[0x5c] | ds[0x5d] << 8
                res = (ds[0x232] | ds[0x233] << 8, tuple(ds[p:p + 3])); break
        e.scan_queue.append(KEY[dd] | 0x80)
        door = ex[0x16]
        if PORT:
            pr = port_cross(room, px, py, dd)
            orig = (res[0], tuple(res[1][:2]), res[1][2]) if res else (room, (px, py), 0)
            print('  port', pr, 'original', orig, 'OK' if pr and pr[0] == orig[0] and pr[2] == orig[2] else 'DIFF', 'room', room, 'door', door, dd)   # room and z: the walks differ in length
        if res is None: skipped += 1; print(room, k, 'door', door, dd, 'no crossing'); continue
        nr, (x, y, z) = res
        if os.environ.get('DBG'): print('  crossing', room, k, 'exit box', tuple(ex[:6]), 'door', door, 'from', (px, py), dd, '->', res)
        nrec = rec[nr]
        mp1 = mjm.mjm_unpack(open(d + '/LEV%d.MP1' % LV, 'rb').read()); B = objsprites.blocks(mp1)
        hd = lambda i: struct.unpack('>16H', B[i][1][:32])[8:11]
        cand = [q for q in roomexits.room_exits(nrec, secs[1]['records'], hd, nrec[21], nrec[20]) if q[6] == door]
        if not cand: bad += 1; print(room, k, 'door', door, '-> room', nr, 'no exit for the door there'); continue
        pred = landing.landing(cand[0], nrec)
        if pred == (x, y): ok += 1
        else: bad += 1; print('MISMATCH room', room, 'door', door, dd, '->', nr, (x, y, z), 'port', pred)
print('TATA level %d: %d landings agree, %d differ, %d without a crossing' % (LV, ok, bad, skipped))
