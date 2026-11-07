#!/usr/bin/env python3
"""Replay random key sequences in the original (emulator) and in tools/pushmodel.py (pushing model) and compare every object position
after every game step until the room changes.   python3 tools/verify_push.py SEED NPHASES"""
import sys,random
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from pushmodel import *
KEYS={'right':(0x4D,1,0),'left':(0x4B,-1,0),'down':(0x50,0,1),'up':(0x48,0,-1)}
seed=int(sys.argv[1]); random.seed(seed)
phases=[(random.choice(list(KEYS)),random.randrange(20,120)) for _ in range(int(sys.argv[2]))]
e=boot(); W=World(e)
def snap():
    ds=bytes(e.uc.mem_read(DS,0x10000)); p=ds[0x2a]|ds[0x2b]<<8
    return [tuple(ds[p+50*i:p+50*i+4]) for i in range(ds[0x224])]
events=[]
cur=[None]
def hk(uc,addr,size,user):
    events.append((snap(),e.rd(0x1F17,0x232,2)))
e.uc.hook_add(UC_HOOK_CODE,hk,None,e.lin(0x1000,0x0B61),e.lin(0x1000,0x0B61))
try: e.uc.ctl_flush_tb()
except Exception: pass
seq=[]  # (phase name, state at event)
for name,ticks in phases:
    k=KEYS[name]
    e.scan_queue.append(k[0]); t0=e.ticks; end=t0+ticks; rel=False
    while e.ticks<end+12:
        if e.ticks>=end and not rel: e.scan_queue.append(k[0]|0x80); rel=True
        n0=len(events); e.run(max_instr=1500)
        for st in events[n0:]: seq.append((name,st[0],st[1]))
bad=0; moved=0
state=snap_state=[tuple(E['box'][:4]) for E in W.ent]
prev=state
room=e.rd(0x1F17,0x232,2) if False else None
rooms=0
lastroom=seq[0][2] if seq else None
for name,st,rm in seq:
    k=KEYS[name]
    if rm!=lastroom:
        lastroom=rm; rooms+=1
        # new room: rebuild model from the emulator state at that moment is not possible offline; re-create from entries
        W=None; prev=st; continue
    if W is None: continue
    if st==prev or len(st)!=len(W.ent): continue
    # model step
    r=W.step(k[1],k[2]); mine=[tuple(E['box'][:4]) for E in W.ent]
    moved+=1
    if mine!=st:
        bad+=1
        if bad<=5: print('DIVERGE',name,r,[(i,st[i],mine[i]) for i in range(len(st)) if st[i]!=mine[i]][:3])
        for E,cc in zip(W.ent,st): E['box'][:4]=list(cc)
    prev=st
print('room changes',rooms);print('phases',phases,'events',len(seq),'state changes',moved,'divergences',bad)
