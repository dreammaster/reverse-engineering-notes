#!/usr/bin/env python3
"""Drives the original CAD.EXE in the emulator through scancodes: walk right to the thing 412 in level 1 room 0, choose verb 10 (grab) and walk left; prints the player record (ds:5C) and the
entry slots that change every step (the grabbed thing follows the player, ds:74 holds its record). The port equivalent: CAD_SNDLOG=1 CAD_POSLOG=1 build/cadaver -g cadaver1 --scene walk --level 1 --room 0
--sim right:80,wait:6,end:2,wait:6,right:2,wait:20,end:2,wait:6,left:60.   python3 tools/verify_grab.py"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emu_call import *
e=boot(1)
rw=lambda o: struct.unpack('<H',bytes(e.uc.mem_read(DS+o,2)))[0]
rb=lambda o,n: bytes(e.uc.mem_read(DS+o,n))
R,L,END=0x4D,0x4B,0x4F
ev=[]  # (tick_offset, code)
def add(t,c): ev.append((t,c))
t=0
add(t,R); t+=80; add(t,R|0x80)
t+=6; add(t,END); t+=2; add(t,END|0x80)
t+=6; add(t,R); t+=2; add(t,R|0x80)
t+=20; add(t,END); t+=2; add(t,END|0x80)
t+=6; add(t,L); 
T0=e.ticks
base=t
import collections
log=[]
def snap():
    pr=rw(0x5c); sl=rw(0x2a); n=rw(0x224)
    ents=[rb(sl+50*i,6) for i in range(n)]
    return (e.ticks-T0, rb(pr,3).hex(), rw(0x74), [x.hex() for x in ents])
last=None
total=base+30
evi=0
while e.ticks-T0<total:
    while evi<len(ev) and e.ticks-T0>=ev[evi][0]:
        e.scan_queue.append(ev[evi][1]); evi+=1
    e.run(max_instr=20000)
    s=snap()
    if (s[1:]!=last) :
        print(s[0],'player',s[1],'ds74',hex(s[2]),'chg', [ (i,x) for i,x in enumerate(s[3]) if last is None or x!=last[2][i]][:3])
        last=s[1:]
