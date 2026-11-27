#!/usr/bin/env python3
"""Make a save file with the original game (CAD.EXE in the emulator): boot the level, optionally walk, give money, press F5 (the pay-to-save prompt is answered, the disk / slot prompt is
skipped by returning from 1000:C1AB with slot 1) and copy the file the game writes.  The game directory is a symlink copy in a temp dir so that the repository is not touched.
    python3 tools/make_orig_save.py OUT.sav [LEVEL] [WALK]      WALK = comma list of KEY:TICKS, KEY one of up down left right end  (e.g. down:60,right:80,end:2,end:2)
Prints the player record and the collision boxes of the engine entries at the moment of the save as  STATE {...}  (what tools/verify_origsave.py compares)."""
import json, os, shutil, struct, subprocess, sys, tempfile
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from emu_call import *
from dosemu import Emu
out = sys.argv[1]; LV = int(sys.argv[2]) if len(sys.argv) > 2 else 1; walk = [w.split(':') for w in sys.argv[3].split(',')] if len(sys.argv) > 3 and sys.argv[3] else []
tmp = tempfile.mkdtemp()
subprocess.run(['cp', '-rs', os.path.join(ROOT, 'cadaver1') + '/.', tmp], check=True)
e = Emu(ROOT + '/unpacked/cadaver1/CAD.EXE', tmp)
e.pending_keys = [0x0231, 0x011b]
if LV != 1: e.uc.hook_add(UC_HOOK_CODE, lambda uc, a, sz, u: e.wr(0x1F17, 0x708, bytes([LV - 1])), None, e.lin(0x1000, 0xC4CB), e.lin(0x1000, 0xC4CB))
opened = []
orig = e.on_open
e.on_open = lambda h, dn, hp: (opened.append(hp), orig(h, dn, hp))
armed = [False]                                   # 1000:C1AB is the restore prompt at start-up as well: only fake it for the save
def fake_prompt(uc, a, sz, u):
    if not armed[0]: return                    # 1000:C1AB, the slot prompt: file name 'SAVE.SAV' + slot digit, slot 1, ZF clear
    ss = e.r('ss') & 0xFFFF; sp = e.r('esp') & 0xFFFF
    ret = struct.unpack('<H', bytes(e.uc.mem_read(ss * 16 + sp, 2)))[0]
    e.wr(ss, 0x4D09, b'SAVE.SAV\0'); e.w('esp', sp + 2); e.w('eip', ret)
    e.w('ebx', (e.r('ebx') & ~0xFF) | 0x31); e.w('eflags', e.r('eflags') & ~0x40)
e.uc.hook_add(UC_HOOK_CODE, fake_prompt, None, e.lin(0x1000, 0xC1AB), e.lin(0x1000, 0xC1AB))
nstep = [0]
e.uc.hook_add(UC_HOOK_CODE, lambda *a: nstep.__setitem__(0, nstep[0] + 1), None, e.lin(0x1000, 0x247), e.lin(0x1000, 0x247))
while e.ticks < 900: e.run(max_instr=20000)
def run(k):
    t = e.ticks
    while e.ticks - t < k: e.run(max_instr=20000)
KEY = {'up': 0x48, 'down': 0x50, 'left': 0x4B, 'right': 0x4D, 'end': 0x4F}
t0 = nstep[0]
while nstep[0] - t0 < 6: e.run(max_instr=2000, slice_instr=2000)         # a few game steps: the room is entered and the entries exist
for k, n in walk:
    e.scan_queue.append(KEY[k]); run(int(n)); e.scan_queue.append(KEY[k] | 0x80); run(6)
e.wr(0x1F17, 0x248, struct.pack('<H', 1000))
rw = lambda o: struct.unpack('<H', bytes(e.uc.mem_read(DS + o, 2)))[0]
pr = rw(0x5c); sl = rw(0x2a)
state = {'room': rw(0x232), 'player': list(e.uc.mem_read(DS + pr, 3)), 'ents': [list(e.uc.mem_read(DS + sl + 50 * j, 6)) for j in range(1, rw(0x224))]}
armed[0] = True
e.scan_queue.append(0x3F); run(4); e.scan_queue.append(0xBF); run(40)
e.keys.append(0x1579); run(300)                  # Y: pay
if not any(os.path.basename(o).upper().startswith('SAVE.SAV') for o in opened):
    e.save_frame('/tmp/mos_fail.png'); sys.exit('the game did not write a save file (frame in /tmp/mos_fail.png; opened: %s)' % opened[-4:])
saves = [o for o in opened if os.path.basename(o).upper().startswith('SAVE.SAV')]      # SAVE.SAV or SAVE.SAV1
if not saves: sys.exit('no save file among the opened files: %s' % opened[-3:])
shutil.copy(saves[-1], out)
shutil.rmtree(tmp, ignore_errors=True)
print('STATE', json.dumps(state))
