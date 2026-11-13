#!/usr/bin/env python3
"""Smoke test of the SDL walk scene: for every level of both games run the rooms with the most creatures (plus room 0) headless for 300 ticks and report
crashes (non-zero exit codes).   python3 tools/smoke_walk.py BUILD/cadaver"""
import collections, os, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import things, objsprites, roomview

exe = sys.argv[1]
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
env = dict(os.environ, SDL_VIDEODRIVER='dummy')
bad = []
for game, sets in (('cadaver1', [('DATA', 5)]), ('cadaver2', [('DATA', 5), ('TATA', 4)])):
    for sub, n in sets:
        for lv in range(1, n + 1):
            d = os.path.join(root, game, sub, str(lv))
            secs, names, th, rooms = things.load_level(d)
            bl = objsprites.blocks(roomview.level_files(d)[0])
            cnt = collections.Counter()
            for t, r in enumerate(th):
                if len(r) >= 16:
                    bi = (r[6] << 8) | r[7]
                    if bi < len(bl) and len(bl[bi][1]) >= 32 and bl[bi][1][0x16] == 3 and not r[3] & 0x80:
                        cnt[r[10]] += 1
            for room in [r for r, _ in cnt.most_common(3)] + [0]:
                cmd = [exe, '-g', os.path.join(root, game), '--scene', 'walk', '--level', str(lv), '--room', str(room), '--sim', 'wait:300']
                if sub == 'TATA':
                    cmd.append('--tata')
                rc = subprocess.run(cmd + ['--screenshot', os.devnull + '.bmp'], env=env, capture_output=True, timeout=120).returncode
                if rc:
                    bad.append((game, sub, lv, room, rc))
print('crashes:', bad if bad else 'none')
sys.exit(1 if bad else 0)
