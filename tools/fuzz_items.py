#!/usr/bin/env python3
"""Random runs with a carried spell / potion / weapon (CAD_CARRY) against the sanitizer build:  python3 tools/fuzz_items.py SEED COUNT   (see fuzz_sim.py for the build)"""
import random, subprocess, sys, os, re
exe = '/tmp/asan/cadaver'
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='print_stacktrace=1')
toks = ['right:3', 'left:3', 'up:3', 'down:3', 'end', 'rend', 'space', 'ret', 'hold', 'wait:20', 'wait:60', 'right:8', 'key:s', 'key:y', 'key:2', 'key:l', 'key:2']
random.seed(int(sys.argv[1]) if len(sys.argv) > 1 else 1)
bad = 0
runs = 0
cache = {}
for i in range(int(sys.argv[2]) if len(sys.argv) > 2 else 20):
    game = random.choice(['cadaver1', 'cadaver2'])
    tata = game == 'cadaver2' and random.random() < 0.7
    lv = random.randint(1, 4 if tata else 5)
    key = (game, tata, lv)
    if key not in cache:
        cmd = [exe, '-g', os.path.join(root, game), '--scene', 'walk', '--level', str(lv), '--room', '0', '--sim', 'wait:2', '--screenshot', os.devnull + '.bmp'] + (['--tata'] if tata else [])
        out = subprocess.run(cmd, env=dict(env, CAD_SPELLS='1'), capture_output=True, text=True, timeout=300).stderr
        cache[key] = [int(m.group(1)) for m in re.finditer(r'SPELLTHING (\d+) ', out)]
    if not cache[key]:
        continue
    t = random.choice(cache[key])
    room = random.randint(0, 60)
    sim = ','.join(['hold'] + [random.choice(toks) for _ in range(random.randint(10, 30))])
    runs += 1
    cmd = [exe, '-g', os.path.join(root, game), '--scene', 'walk', '--level', str(lv), '--room', str(room), '--sim', sim, '--screenshot', os.devnull + '.bmp'] + (['--tata'] if tata else [])
    r = subprocess.run(cmd, env=dict(env, CAD_CARRY='%d:%d:%d' % (t, random.randint(0, 31), random.randint(0, 255)), CAD_ENERGY='300'), capture_output=True, text=True, timeout=300, cwd='/tmp')
    if r.returncode != 0 or 'runtime error' in r.stderr or 'AddressSanitizer' in r.stderr:
        bad += 1
        print('FAIL', game, tata, lv, room, t, sim, r.returncode)
        print(r.stderr[-1500:])
print("done, runs", runs, "failures", bad)
