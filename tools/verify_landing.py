"""Compare tools/landing.py with the logs of tools/oracle_landing.py:  python3 tools/verify_landing.py land_*.log"""
import sys,re,glob,os
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
import things,mjm,objsprites,roomexits,landing,struct
secs,names,th,rooms=things.load_level(ROOT+'/cadaver1/DATA/1')
mp1=mjm.mjm_unpack(open(ROOT+'/cadaver1/DATA/1/LEV1.MP1','rb').read())
B=objsprites.blocks(mp1)
hd=lambda i:struct.unpack('>16H',B[i][1][:32])[8:11]
ok=bad=none=0
for f in [f for a in sys.argv[1:] for f in glob.glob(a)]:
    for l in open(f):
        m=re.match(r'(\d+) (\d+) door (\d+) (\w) \((-?\d+), (-?\d+)\) -> (.*)',l)
        if not m: continue
        room,k,door,d=int(m[1]),int(m[2]),int(m[3]),m[4]
        res=m[7]
        if res=='None': none+=1; continue
        mm=re.match(r'\((\d+), \((\d+), (\d+), (\d+)\)\)',res)
        nr,x,y=int(mm[1]),int(mm[2]),int(mm[3])
        rec=secs[0]['records'][nr]
        ex=[e for e in roomexits.room_exits(rec,secs[1]['records'],hd,rec[21],rec[20]) if e[6]==door]
        if not ex: print('no exit',l.strip()); bad+=1; continue
        pred=landing.landing(ex[0],rec)
        if pred==(x,y): ok+=1
        else: bad+=1; print('MISMATCH',l.strip(),'pred',pred)
print('ok',ok,'bad',bad,'none',none)
