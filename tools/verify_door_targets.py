#!/usr/bin/env python3
"""Static check of the target room of a door: the original resolves it geometrically (1000:5C53 / 15C0E, called from the walk step after an exit hit: the door's map cell
(record bytes 0, 1) is compared with the four sides of every room record -- cell = origin - 1 or origin + size, and the other coordinate inside the room's span -- and the
first room other than the current one wins), the port takes the other room that lists the same door id (`door_target`).  Both rules give the same room for every door of
levels 1-5 of both games and the four TATA levels (about 2,100 doors).
    python3 tools/verify_door_targets.py cadaver1/DATA/1 cadaver2/TATA/1 ..."""
import sys,os
ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0,ROOT+'/tools')
import things
def s8(v): return v-256 if v>127 else v
def geo(rooms, door, cur):
    x,y=door[0],door[1]
    hits=[]
    for i,r in enumerate(rooms):
        if len(r)<22: continue
        a,b=(r[1]-1)&255,(r[3]-1)&255
        c=cl=x; bl=y
        def inx(): return r[1]<=cl<r[1]+r[4]   # loc_15C9C : cl in [rec1, rec1+rec4)
        def iny(): return r[3]<=bl<r[3]+r[5]
        ok=False
        if cl==a: ok=iny()
        elif bl==b: ok=inx()
        else:
            a2=(a+1+r[4])&255; b2=(b+1+r[5])&255
            if cl==a2: ok=iny()
            elif bl==b2: ok=inx()
        if ok: hits.append(i)
    return hits
for path in sys.argv[1:]:
    secs,names,th,rooms=things.load_level(path)
    recs=secs[0]['records']; doors=secs[1]['records']
    bad=tot=0
    for ri,r in enumerate(recs):
        if len(r)<22: continue
        for i in range(7):
            d=(r[6+2*i]<<8)|r[7+2*i]
            if d==0xFFFF or d>=len(doors) or len(doors[d])<6: continue
            other=[j for j,q in enumerate(recs) if j!=ri and len(q)>=22 and any(((q[6+2*k]<<8)|q[7+2*k])==d for k in range(7))]
            h=[j for j in geo(recs,doors[d],ri) if j!=ri]
            tot+=1
            if (other[:1]!=h[:1]):
                bad+=1
                if bad<=6: print(path,'room',ri,'door',d,'listed-other',other,'geometric',h)
    print(path,'doors',tot,'differ',bad)
