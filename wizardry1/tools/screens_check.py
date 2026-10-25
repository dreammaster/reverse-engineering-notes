#!/usr/bin/env python3
"""
End-to-end screen regression: run a keystroke scenario in the p-code oracle
(tools/wizvm.py) and in the C++ port, then compare every screen the player
sees at a key wait (composed cells + VRAM CRC) -- see tools/cmpgrids.py.

    screens_check.py <wiz1.exe> [scenario ...]

Exit 0 = identical, 1 = mismatch, 77 = skipped (no game disk).
"""
import os, subprocess, sys, tempfile, shutil

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
DATA = os.path.join(ROOT, 'extracted', 'wiz1')
DISK = r'C:\games\wizard15\WIZ1.DSK'

# name: (engine command, oracle prefix keys, engine keys, roster patch, skip)
TITLE = r'S\r\r'          # the oracle starts at the title + two disk prompts
SCEN = {
    'castle_tavern': ('castle-test', 'GA\rRLAE', '', 3),
    'castle_party':  ('castle-test', r'GAABC\rR2DLA1B\rL\rE', '', 3),
    'castle_levelup': ('castle-test', r'GAA\rLA1B', '0:62:1500,0:67:2', 3),
}

def run(exe, name, tmp):
    cmd, keys, patch, skip = SCEN[name]
    od, ed = os.path.join(tmp, 'o'), os.path.join(tmp, 'e')
    os.makedirs(ed, exist_ok=True)
    args = [sys.executable, os.path.join(ROOT, 'tools', 'wizvm.py'), 'run', '--ops', '--keys', TITLE + keys,
            '--steps', '80000000', '--shotdir', od]
    if patch: args += ['--patch', patch]
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode: print(r.stdout, r.stderr); return 1
    for f in os.listdir(od):
        if f.endswith('.png') or f.endswith('.ops'): os.remove(os.path.join(od, f))
    # the oracle's RANDOM state at the first Castle screen (key wait #skip+1)
    first = sorted(f for f in os.listdir(od) if f.endswith('.grid'))[skip]
    rng = [l for l in open(os.path.join(od, first)).read().split('\n') if l.startswith('rng')][0].split()[1]
    env = dict(os.environ, WIZ1_RNG=rng)
    if patch: env['WIZ1_PATCH'] = patch
    r = subprocess.run([exe, cmd, DATA, keys, ed], capture_output=True, text=True, env=env)
    if r.returncode: print(r.stdout, r.stderr); return 1
    r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'cmpgrids.py'), od, ed, '--skip', str(skip)],
                       capture_output=True, text=True)
    print(f'[{name}]'); print(r.stdout.rstrip())
    return r.returncode

if __name__ == '__main__':
    if not os.path.exists(DISK) or not os.path.exists(os.path.join(DATA, 'SCENARIO.DATA')):
        print('skipped: game disk / extracted files not present'); sys.exit(77)
    exe = sys.argv[1]
    names = sys.argv[2:] or list(SCEN)
    bad = 0
    for n in names:
        tmp = tempfile.mkdtemp(prefix='wizscr')
        try: bad |= run(exe, n, tmp)
        finally: shutil.rmtree(tmp, ignore_errors=True)
    sys.exit(1 if bad else 0)
