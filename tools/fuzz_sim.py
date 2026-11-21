#!/usr/bin/env python3
"""Random --sim sequences against a sanitizer build of the port:  cmake -S src -B /tmp/asan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='-fsanitize=address,undefined' && make -C /tmp/asan;  python3 tools/fuzz_sim.py SEED COUNT"""
import random,subprocess,sys,os
exe='/tmp/asan/cadaver'
root=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
env=dict(os.environ,SDL_VIDEODRIVER='dummy',ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='print_stacktrace=1')
toks=['right','left','up','down','rdown','ldown','rup','lup','end','rend','space','ret','hold','wait','map','info','save','load','magicmap','wait','wait']
random.seed(int(sys.argv[1]) if len(sys.argv)>1 else 1)
bad=0
for i in range(int(sys.argv[2]) if len(sys.argv)>2 else 30):
    game=random.choice(['cadaver1','cadaver2'])
    lv=random.randint(1,5 if game=='cadaver1' else 4)
    tata=game=='cadaver2' and random.random()<0.5
    if tata: lv=random.randint(1,4)
    room=random.randint(0,60)
    seq=','.join('%s:%d'%(random.choice(toks),random.choice([2,5,20,80,300])) for _ in range(80))
    cmd=[exe,'-g',os.path.join(root,game),'--scene','walk','--level',str(lv),'--room',str(room),'--sim',seq,'--screenshot','/dev/null.bmp']+(['--tata'] if tata else [])
    r=subprocess.run(cmd,env=env,capture_output=True,timeout=300)
    err=r.stderr.decode(errors='ignore')
    if r.returncode or 'runtime error' in err or 'AddressSanitizer' in err:
        bad+=1; print('FAIL',game,lv,room,tata,r.returncode); print(err[:1500]); print(seq); break
print('done, failures',bad)
