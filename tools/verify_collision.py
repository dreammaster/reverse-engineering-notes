import sys,random
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from emu_call import *
import collision
e=boot()
def snap(): return bytes(e.uc.mem_read(DS,0x10000))
rw=lambda ds,o: ds[o]|(ds[o+1]<<8)
ds=snap()
SS=e.r('ss'); print('ss',hex(SS))
es=rw(ds,0x26); di=rw(ds,0x28); sipos=rw(ds,0x5c)
hdr=e.rd(es,di,0x20); xs,ys,zs=hdr[0x10],hdr[0x12],hdr[0x14]
print('mover sizes',xs,ys,zs)
random.seed(1)
bad=0; kinds={}
for n in range(3000):
    x=random.randrange(0,100) if n%3 else random.randrange(-5,100)&0xFF
    y=random.randrange(0,100)
    z=random.choice([0,0,0,1,3,5,10,21,22,30,40,63,200])
    ds=snap()
    o=call(e,0x16E0,edx=((y&0xFF)<<8)|(x&0xFF),ecx=(z&0xFF)<<8,es=es,edi=di,esi=sipos,ds=0x1F17)
    # result list after call (memory modified by the call): read before restoring? call() restores regs only; memory stays
    l=rw(ds,0x3c)
    new=snap()
    cnt=new[l]; flag=new[l+1]
    zf=(o['eflags']>>6)&1
    ptrs=[rw(new,l+2+2*i) for i in range(cnt)] if cnt<0xF0 else []
    ss=bytes(e.uc.mem_read(SS*16,0x10000)) if False else bytes(e.uc.mem_read(SS*16,0x10000))
    ref=collision.collision_test(ds,x&0xFF,y&0xFF,z&0xFF,xs,ys,zs,sipos,ss)
    got=dict(zf=zf,count=cnt,flag=flag if cnt<0xF0 else 0,ptrs=ptrs)
    exp=dict(zf=ref['zf'],count=ref['count'],flag=ref['flag'],ptrs=ref['ptrs'])
    kinds[(cnt if cnt>=0xF0 else 'n',flag)]=kinds.get((cnt if cnt>=0xF0 else 'n',flag),0)+1
    if got!=exp:
        bad+=1
        if bad<8: print('MISMATCH',(x,y,z),got,exp)
print('bad',bad,kinds)
