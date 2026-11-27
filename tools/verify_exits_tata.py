#!/usr/bin/env python3
"""The exit boxes of every room of the Payoff's TATA level 1 against the original: GAME.EXE is booted to the first room (P at the restore prompt), then for each room the room-entry
routine 1000:DAB1 (the Payoff's counterpart of CAD.EXE's DA60) is called with ds:232 = the room and the entries it builds at ss:[ds:3A] are compared with tools/roomexits.py.
    python3 tools/verify_exits_tata.py [LEVEL 1..4]   (the level number is forced by writing ds:708 before the file-name patcher 1000:CB24 runs)"""
import os, struct, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from emu_call import *
from dosemu import Emu
import things, mjm, objsprites, roomexits
DSP = 0x1F1C
e = Emu(ROOT + '/unpacked/cadaver2/GAME.EXE', ROOT + '/cadaver2')
LV = int(sys.argv[1]) if len(sys.argv) > 1 else 1
e.pending_keys = [0x0231, 0x1970]
if LV != 1:
    e.uc.hook_add(UC_HOOK_CODE, lambda uc, a, sz, u: e.wr(DSP, 0x708, bytes([LV - 1])), None, e.lin(0x1000, 0xCB24), e.lin(0x1000, 0xCB24))
while e.ticks < 3000: e.run(max_instr=20000)
full = bytes(e.uc.mem_read(0, 0xA0000)); regs = {r: e.r(r) for r in REGS}
d = ROOT + '/cadaver2/TATA/%d' % LV
secs, names, th, rooms = things.load_level(d)
mp1 = mjm.mjm_unpack(open(d + '/LEV%d.MP1' % LV, 'rb').read())
B = objsprites.blocks(mp1)
hd = lambda i: struct.unpack('>16H', B[i][1][:32])[8:11]
bad = n_ok = 0
for n, r in enumerate(secs[0]['records']):
    if len(r) < 22 or r[4] * r[5] == 0: continue
    e.uc.mem_write(0, full)
    for k, v in regs.items(): e.w(k, v)
    e.wr(DSP, 0x232, struct.pack('<H', n))
    try: call(e, 0x1DAB1, edx=n, ds=DSP)
    except Exception as ex: print(n, 'err', ex); continue
    ds = bytes(e.uc.mem_read(DSP * 16, 0x10000)); ss = bytes(e.uc.mem_read(e.r('ss') * 16, 0x10000))
    rw = lambda b, o: b[o] | b[o + 1] << 8
    if rw(ds, 0x232) != n: continue
    ex = [ss[rw(ds, 0x3a) + 50 * i: rw(ds, 0x3a) + 50 * i + 50] for i in range(rw(ds, 0x22e))]
    got = [tuple(x[:6]) + (x[0x16],) for x in ex]
    exp = roomexits.room_exits(r, secs[1]['records'], hd, r[21], r[20])
    if got == exp: n_ok += 1
    else: bad += 1; print('room', n, 'original', got, 'port', exp)
print('TATA level %d: %d rooms agree, %d differ' % (LV, n_ok, bad))
sys.exit(1 if bad else 0)
