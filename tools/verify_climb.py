#!/usr/bin/env python3
"""Compares the port's climbing (1000:05F1) with the original: for every room of level N of Cadaver 1 where a headless walk of the port climbs onto an object, the player is put at the
spot in front of it in both programs (original: room_enter 1000:DA60 in the emulator, the player record ds:5C overwritten, one direction scancode held) and the first position change
is compared (x, y, z).    python3 tools/verify_climb.py LEVEL FIRST_ROOM LAST_ROOM+1   (needs build/cadaver)
Differences seen: objects that the port had pushed earlier in its walk, and a platform (level 3 room 13, thing 787) that appears some 30 ticks after the room is entered (timing)."""
import sys,subprocess,re,os,struct
ROOT_=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0,os.path.join(ROOT_,'tools'))
from emu_call import *
env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',CAD_SNDLOG='1')
KEY={(1,0):0x4D,(-1,0):0x4B,(0,1):0x50,(0,-1):0x48}
cases=[]
lv=int(sys.argv[1]); rooms=range(int(sys.argv[2]),int(sys.argv[3]))
for r in rooms:
    out=subprocess.run([os.path.join(ROOT_,'build','cadaver'),'-g',os.path.join(ROOT_,'cadaver1'),'--scene','walk','--level',str(lv),'--room',str(r),'--sim','right:100,left:200,up:100,down:200,right:100','--screenshot','/tmp/x.bmp'],env=env,capture_output=True,timeout=60).stderr.decode()
    for m in re.finditer(r'CLIMB onto (\d+): z (-?\d+) -> (-?\d+) at (\d+),(\d+) by (-?\d+),(-?\d+)',out):
        t,z0,z1,x,y,dx,dy=map(int,m.groups()); cases.append((r,t,z0,z1,x-dx,y-dy,dx,dy,x,y)); break
print(len(cases),'cases')
e=boot(lv)
full=bytes(e.uc.mem_read(0,0xA0000)); regs={k:e.r(k) for k in REGS}
rw=lambda o: struct.unpack('<H',bytes(e.uc.mem_read(DS+o,2)))[0]
for (r,t,z0,z1,sx,sy,dx,dy,tx,ty) in cases:
    e.uc.mem_write(0,full)
    for k,v in regs.items(): e.w(k,v)
    e.wr(0x1F17,0x232,struct.pack('<H',r))
    call(e,0x1DA60,edx=r,ds=0x1F17)
    pr=rw(0x5c); e.wr(0x1F17,pr,bytes([sx,sy,z0&0xFF]))
    for _ in range(10): e.run(max_instr=20000)
    key=KEY[(dx,dy)]
    dn={(1,0):'right',(-1,0):'left',(0,1):'down',(0,-1):'up'}[(dx,dy)]
    out=subprocess.run([os.path.join(ROOT_,'build','cadaver'),'-g',os.path.join(ROOT_,'cadaver1'),'--scene','walk','--level',str(lv),'--room',str(r),'--sim','wait:4,tp:%d:%d,wait:2,%s:8'%(sx,sy,dn),'--screenshot','/tmp/x.bmp'],env=dict(env,CAD_POSLOG='1'),capture_output=True,timeout=60).stderr.decode()
    m=re.search(r'CLIMB onto (\d+): z (-?\d+) -> (-?\d+) at (\d+),(\d+)',out)
    if m: tx,ty,z1=int(m.group(4)),int(m.group(5)),int(m.group(3))
    else:
        m=re.search(r'POS tick \d+ (\d+),(\d+) -> \+(-?\d+),(-?\d+)',out)
        if m and (int(m.group(1)),int(m.group(2)))==(sx,sy): tx,ty,z1=sx+dx,sy+dy,z0
        else: tx,ty,z1=None,None,None
    T0=e.ticks; e.scan_queue.append(key); res=None; rel=False
    while e.ticks-T0<12:
        e.run(max_instr=20000)
        if e.ticks-T0>=9 and not rel: e.scan_queue.append(key|0x80); rel=True
        b=bytes(e.uc.mem_read(DS+pr,3))
        if (b[0],b[1])!=(sx,sy): res=tuple(b); break
    print('L%d R%d thing %d start %d,%d,%d dir %d,%d: port -> %s,%s,%s  original -> %s %s'%(lv,r,t,sx,sy,z0,dx,dy,tx,ty,z1,res,'OK' if (res==(tx,ty,z1) or (res is None and tx is None)) else 'DIFF'))
