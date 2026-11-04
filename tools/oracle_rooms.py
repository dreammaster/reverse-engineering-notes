#!/usr/bin/env python3
"""
Ground truth from the original: enter every room of level N by calling CAD.EXE's room_enter (1000:DA60) inside the emulator and
dump the state it builds (bounds, wall limits, door rectangles, exit entries, objects) into rooms_lvN.pkl.
    python3 tools/oracle_rooms.py N        then   python3 tools/verify_exits.py N:rooms_lvN.pkl
"""
import sys,pickle
sys.path.insert(0,__import__('os').path.dirname(__import__('os').path.abspath(__file__))+'')
from emu_call import *
import things
lv=int(sys.argv[1])
e=boot(lv)
secs,names,th,rooms=things.load_level(__import__('os').path.join(ROOT,'cadaver1','DATA',str(lv)))
full=bytes(e.uc.mem_read(0,0xA0000)); regs={r:e.r(r) for r in REGS}
def restore():
    e.uc.mem_write(0,full)
    for r,v in regs.items(): e.w(r,v)
out={}
for n,r in enumerate(secs[0]['records']):
    if len(r)<22 or r[4]*r[5]==0: continue
    restore()
    e.wr(0x1F17,0x232,struct.pack('<H',n))
    try: call(e,0x1DA60,edx=n,ds=0x1F17)
    except Exception as ex: print(n,'err',ex); continue
    ds=bytes(e.uc.mem_read(DS,0x10000)); ss=bytes(e.uc.mem_read(e.r('ss')*16,0x10000))
    rw=lambda b,o:b[o]|b[o+1]<<8
    if rw(ds,0x232)!=n: continue
    ex=[ss[rw(ds,0x3a)+50*i:rw(ds,0x3a)+50*i+50] for i in range(rw(ds,0x22e))]
    ents0=rw(ds,0x2a); nob=rw(ds,0x224); proj=[]
    for i in range(nob):
        try: call(e,0x5A61,esi=ents0+50*i,ds=0x1F17)
        except Exception: pass
        eb=e.rd(0x1F17,ents0+50*i,50); proj.append((eb[0x0C]|eb[0x0D]<<8,eb[0x10],eb[0x11]))
    out[n]=dict(proj=proj,b=[ds[o] for o in (0x5D1,0x5D2,0x5DF,0x5E0,0x5F4,0x5F5)],z54=rw(ds,0x54),rects=ds[0x5E4:0x5F4],exits=ex,nobj=rw(ds,0x224),f9=ds[0x5F9],ents=ds[rw(ds,0x2a):rw(ds,0x2a)+50*rw(ds,0x224)],player=ds[rw(ds,0x5c):rw(ds,0x5c)+16])
pickle.dump(out,open('rooms_lv%d.pkl'%lv,'wb'))
print(lv,len(out),'rooms; distinct (5DF,5E0):',set((o['b'][2],o['b'][3]) for o in out.values()),'z54',set(o['z54'] for o in out.values()))
