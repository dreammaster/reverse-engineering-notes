#!/usr/bin/env python3
"""Loads a save written by the original game into the port and compares the player's place and the collision boxes of the room's things with the engine entries the emulator had at the
moment of the save (tools/make_orig_save.py).   python3 tools/verify_origsave.py [LEVEL] [WALK]"""
import json, os, re, subprocess, sys, tempfile
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = os.environ.get('GAME', 'cadaver1')
LV = sys.argv[1] if len(sys.argv) > 1 else '1'; walk = sys.argv[2] if len(sys.argv) > 2 else ''
d = tempfile.mkdtemp()
r = subprocess.run([sys.executable, ROOT + '/tools/make_orig_save.py', d + '/cadaver.sav', LV, walk], capture_output=True, text=True)
m = re.search(r'STATE (.*)', r.stdout)
if not m: sys.exit('make_orig_save failed: ' + r.stdout + r.stderr)
st = json.loads(m[1])
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', CAD_DUMPBOX='1')
out = subprocess.run([ROOT + '/build/cadaver', '-g', ROOT + '/' + GAME, '--scene', 'walk', '--level', '1', '--room', '0', '--sim', 'wait:4,load:0,wait:2,dump:0,state:0', '--screenshot', d + '/x.bmp'] + (['--tata'] if GAME == 'cadaver2' else []), env=env, capture_output=True, text=True, cwd=d).stderr
dm = re.findall(r'DUMP (-?\d+) (-?\d+) (-?\d+) (-?\d+)', out)[-1]
port = {'player': [int(dm[0]), int(dm[1]), int(dm[2])], 'room': int(dm[3])}
objs = sorted(tuple(int(v) & 0xFF for v in m.groups()[1:7]) for m in re.finditer(r'OBJ (\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (\d+)', out) if int(m.group(8)) != 3)
orig = sorted(tuple(e) for e in st['ents'] if e[0] != 0xFF)
orig = [o for o in orig]
sm = re.search(r'STATE money (\d+) score (\d+) energy (-?\d+) day (\d+) held (\d+) carried([\d ]*) locks([\w= ]*) timers([\d= ]*)', out)
ex = st['extra']
pc = [int(x) for x in sm[6].split()]
pl = {int(a): int(b, 16) for a, b in (kv.split('=') for kv in sm[7].split())}
pt = {int(a): int(b) for a, b in (kv.split('=') for kv in sm[8].split())}
same = [('money', int(sm[1]) == ex['money']), ('score', int(sm[2]) == ex['score']), ('energy', int(sm[3]) == ex['energy']), ('day', int(sm[4]) == ex['day']), ('held', int(sm[5]) == ex['held']),
        ('carried', sorted(pc) == sorted(ex['carried'])), ('locks', pl == {int(k): v for k, v in ex['locks'].items()}), ('timers', pt == {int(k): v for k, v in ex['timers'].items()})]
print('state:', ' '.join('%s %s' % (k, 'ok' if v else 'DIFF (port %s original %s)' % ({'money': sm[1], 'score': sm[2], 'energy': sm[3], 'day': sm[4], 'held': sm[5], 'carried': pc, 'locks': len(pl), 'timers': pt}[k], ex[k] if k not in ('locks', 'carried') else (len(ex[k]) if k == 'locks' else ex[k]))) for k, v in same))
ok = all(v for k, v in same) and (port['player'] == st['player'] or (st['player'][2] > 0 and port['player'][:2] == st['player'][:2])) and port['room'] == st['room']
near = lambda a, b: all(abs(x - y) <= 1 for x, y in zip(a, b))
missing = [o for o in orig if not any(near(o, p) for p in objs)]
extra = [p for p in objs if not any(near(o, p) for o in orig)]
print('level', LV, 'room', port['room'], 'player port', port['player'], 'original', st['player'], '| objects: %d port, %d original, %d without a partner (original), %d (port)' % (len(objs), len(orig), len(missing), len(extra)))
print('OK' if ok and not missing else 'DIFF', missing[:5], extra[:5])
sys.exit(0 if ok and not missing else 1)
