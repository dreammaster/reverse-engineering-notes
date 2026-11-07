"""Experiment on the original: for every exit of rooms [first, last) put the player next to the door, walk through it and print the
landing place in the new room.  python3 tools/oracle_landing.py FIRST LAST > land_FIRST.log   (then tools/verify_landing.py)"""
import sys,pickle
sys.path.insert(0,__import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from emu_call import *
import things,roomexits,objsprites,mjm
lv=1
e=boot(lv)
full=bytes(e.uc.mem_read(0,0xA0000)); regs={r:e.r(r) for r in REGS}
def restore():
    e.uc.mem_write(0,full)
    for r,v in regs.items(): e.w(r,v)
secs,names,th,rooms=things.load_level(__import__('os').path.join(ROOT,'cadaver1','DATA',str(lv)))
KEY={'N':(0x48),'S':(0x50),'E':(0x4D),'W':(0x4B)}
def rd(): 
    ds=bytes(e.uc.mem_read(DS,0x10000)); return ds
out=[]
rl=list(range(int(sys.argv[1]),int(sys.argv[2])))
for room in rl:
    r=secs[0]['records'][room]
    if len(r)<22: continue
    restore()
    e.wr(0x1F17,0x232,struct.pack('<H',room)); call(e,0x1DA60,edx=room,ds=0x1F17)
    ds=rd(); ss=bytes(e.uc.mem_read(e.r('ss')*16,0x10000))
    bp=ds[0x3a]|ds[0x3b]<<8; nex=ds[0x22e]
    exits=[ss[bp+50*i:bp+50*i+50] for i in range(nex)]
    snap=bytes(e.uc.mem_read(0,0xA0000)); rg={q:e.r(q) for q in REGS}
    w8,h8=r[4]*8,r[5]*8
    for k,ex in enumerate(exits):
        x1,y1,x0,y0=[v if v<128 else v-256 for v in ex[:4]]
        # direction: exit beyond which edge
        if y1<8: d='N'; px,py=(x0+x1)//2+3,12
        elif y0>=h8-1: d='S'; px,py=(x0+x1)//2+3,h8-1
        elif x1<8: d='W'; px,py=12,(y0+y1)//2+3
        else: d='E'; px,py=w8-1,(y0+y1)//2+3
        e.uc.mem_write(0,snap)
        for q,v in rg.items(): e.w(q,v)
        ds=rd(); p=ds[0x5c]|ds[0x5d]<<8; en=ds[0x2a]|ds[0x2b]<<8
        e.wr(0x1F17,p,bytes([px&255,py&255,0])); e.wr(0x1F17,en,bytes([px&255,py&255,(px-6)&255,(py-6)&255]))
        # stop real-time input state
        e.scan_queue.append(KEY[d]); t0=e.ticks; res=None
        while e.ticks<t0+80:
            e.run(max_instr=20000)
            ds=rd()
            if ds[0x232]|ds[0x233]<<8!=room:
                for _ in range(3): e.run(max_instr=20000)
                ds=rd(); p=ds[0x5c]|ds[0x5d]<<8
                res=(ds[0x232]|ds[0x233]<<8,tuple(ds[p:p+3])); break
        e.scan_queue.append(KEY[d]|0x80)
        out.append((room,k,ex[0x16],d,(px,py),res))
        print(room,k,'door',ex[0x16],d,(px,py),'->',res,flush=True)
pickle.dump(out,open('land_%s.pkl'%sys.argv[1],'wb'))
