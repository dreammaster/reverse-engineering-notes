import sys,struct
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from emu_call import *
import collision
from collision import s8,overlap
class World:
    def __init__(s,e):
        s.ds=bytearray(e.uc.mem_read(DS,0x10000)); s.ss=bytes(e.uc.mem_read(e.r('ss')*16,0x10000))
        rw=lambda o: s.ds[o]|s.ds[o+1]<<8
        s.n=rw(0x224); s.p=rw(0x2a); s.strength=rw(0x59c)
        s.ent=[]
        for i in range(s.n):
            en=s.ds[s.p+50*i:s.p+50*i+50]
            seg=en[6]|en[7]<<8; off=en[8]|en[9]<<8
            h=e.rd(seg,off,0x20)
            rc=en[10]|en[11]<<8
            s.ent.append(dict(i=i,pos=list(s.ds[rc:rc+3]),box=list(en[:6]),marker=en[0x14],rec=en[10]|en[11]<<8,w=h[0xd],sz=(h[0x10],h[0x12],h[0x14])))
        s.pl=s.ent[0]
    def test(s,x,y,z,sz,self_rec):
        # run reference collision on current ds with entries patched
        for E in s.ent:
            b=E['box']; o=s.p+50*E['i']
            for k in range(6): s.ds[o+k]=b[k]&255
        return collision.collision_test(bytes(s.ds),x&255,y&255,z&255,sz[0],sz[1],sz[2],self_rec,s.ss)
    def push(s,E,dx,dy,budget,depth=0):
        """try to move E by (dx,dy); returns (ok,budget,moved list)"""
        budget-=E['w']
        if budget<0 or depth>=32: return False,budget,[]
        x=E['pos'][0]+dx; y=E['pos'][1]+dy; z=E['pos'][2]
        r=s.test(x,y,z,E['sz'],E['rec'])
        moved=[E]
        if r['count'] in (0xFF,0xFE): return False,budget,[]
        if r['flag']==0xFD: return False,budget,[]
        for rec in r['ptrs']:
            F=[f for f in s.ent if f['rec']==rec][0]
            if F is s.pl: continue
            ok,budget,m=s.push(F,dx,dy,budget,depth+1)
            if not ok: return False,budget,[]
            moved+=m
        return True,budget,moved
    def step(s,dx,dy):
        pl=s.pl
        x=pl['pos'][0]+dx; y=pl['pos'][1]+dy
        r=s.test(x,y,pl['pos'][2],pl['sz'],pl['rec'])
        if r['count'] in (0xFF,0xFE): return 'blocked'
        if r['flag']==0xFD: return 'solid'
        budget=s.strength; moved=[pl]
        for rec in r['ptrs']:
            F=[f for f in s.ent if f['rec']==rec][0]
            ok,budget,m=s.push(F,dx,dy,budget)
            if not ok: return 'pushfail'
            moved+=m
        for E in moved:
            bb=E['box']; bb[0]+=dx; bb[1]+=dy; bb[2]+=dx; bb[3]+=dy; E['pos'][0]+=dx; E['pos'][1]+=dy
        return 'moved'
