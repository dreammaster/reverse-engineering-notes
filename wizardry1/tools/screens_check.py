#!/usr/bin/env python3
"""
End-to-end screen regression: run a keystroke scenario in the p-code oracle
(tools/wizvm.py) and in the C++ port, then compare every screen the player
sees at a key wait or a pause beat (composed cells + VRAM CRC) -- see
tools/cmpgrids.py.

    screens_check.py <wiz1.exe> [scenario ...]

Exit 0 = identical, 1 = mismatch, 77 = skipped (no game disk).
"""
import os, subprocess, sys, tempfile, shutil

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
DATA = os.path.join(ROOT, 'extracted', 'wiz1')
DISK = r'C:\games\wizard15\WIZ1.DSK'

TITLE = r'S\r\r'          # the oracle starts at the title + two disk prompts

# name -> dict(cmd=engine command, keys=..., patch=roster pokes, skip=oracle screens before the
#              engine's first, prefix=oracle keys that reach the engine's start screen, env=extra env)
SCEN = {
    'title_static': dict(cmd='title-test', keys='S', skip=0, prefix=''),
    'title_anim':   dict(cmd='title-test', keys='', skip=0, prefix='', env={'WIZ1_MAXDUMPS': '70'}),
    'castle_tavern': dict(cmd='castle-test', keys=r'GA\rRLAE'),
    'castle_party':  dict(cmd='castle-test', keys=r'GAABC\rR2DLA1B\rL\rE'),
    'castle_levelup': dict(cmd='castle-test', keys=r'GAA\rLA1B', patch='0:62:1500,0:67:2'),
    # Temple: TEMPEST (slot 1) is DEAD / ASHES; THESUS + LYSANDER join and pool gold to pay
    'shops_temple':  dict(cmd='castle-test', keys=r'GAAC\rLTA1\rE', patch='1:20:5,1:67:0'),
    'shops_temple2': dict(cmd='castle-test', keys=r'GAAC\rLTA2\r\rE', patch='1:20:6,1:67:0'),
    'shops_boltac':  dict(cmd='castle-test', keys=r'GAABC\rLB1BPPL2SP1UP1ILL3PLE'),
    'shops_edge':    dict(cmd='castle-test', keys=r'GAA\rLEMXC'),
    'roller_menu':   dict(cmd='castle-test', keys=r'ETR\rTIA'),
    'roller_inspect': dict(cmd='castle-test', keys=r'ETIAL\rIB'),
    'roller_pw':     dict(cmd='castle-test', keys=r'ETIAAXYZ\rXYZ\r\rL\rIAAAB\rQ\rL'),
    'roller_delete': dict(cmd='castle-test', keys=r'ETIEDNDY\rETR\r'),
    'roller_class':  dict(cmd='castle-test', keys=r'ETIACL\rL'),
    'roller_create': dict(cmd='castle-test', keys=r'ETCZORK\r\rAA++++++++++++\r++++++++++\r++++\rCAYLR\r'),
    'roller_create_n': dict(cmd='castle-test', keys=r'ETCBEAR\rPW\rPW\rBC++++\r++++++++\rC\rEN'),
    # the maze: Edge of Town -> M)AZE -> CAMP -> L)EAVE -> the first view (the stairs prompt)
    'maze_enter':    dict(cmd='castle-test', keys=r'GAAC\rLEML'),
    'maze_up':       dict(cmd='castle-test', keys=r'GAAC\rLEMLYE'),            # stairs up -> the castle again
    'maze_quit':     dict(cmd='castle-test', keys=r'GAAC\rLEMLNWQNQYE'),       # Q)UIT the expedition
    'maze_camp':     dict(cmd='castle-test', keys=r'GAAC\rLEMLNWWCLWNCL'),     # C)AMP in the maze and back
    # both fighters paralysed: the roster check at the maze's first row redraw finds nobody on their
    # feet -> the cemetery (SHOPS:27) and back to the castle
    'maze_wiped':    dict(cmd='castle-test', keys=r'GAAC\rLEML\r\r\rE',
                          patch='0:20:3,1:20:3,2:20:3,3:20:3'),
    'camp_sheet':    dict(cmd='castle-test', keys=r'GAAC\rI1TL'),               # the Tavern's INSPECT: the sheet
    'camp_maze':     dict(cmd='castle-test', keys=r'GAAC\rLEMLNWC1T25\r1D1PTX\rL\rL'),
    # a priest and a mage in camp: cast DIOS on the mage, a prefix match, an unknown name, USE
    'camp_spells':   dict(cmd='castle-test', keys=r'GAFB\rLEMLNC1SDIOS\r2SK\rSZZ\rSDIOS\r1U1L\rL'),
    'maze_walk':     dict(cmd='castle-test', keys=r'GAAC\rLEMLNWWWDWWAWWXWWWWAWSSOOT5\rW'),
}


def run(exe, name, tmp):
    sc = SCEN[name]
    skip = sc.get('skip', 3)
    prefix = sc.get('prefix', TITLE)
    patch = sc.get('patch', '')
    od, ed = os.path.join(tmp, 'o'), os.path.join(tmp, 'e')
    os.makedirs(ed, exist_ok=True)
    args = [sys.executable, os.path.join(ROOT, 'tools', 'wizvm.py'), 'run', '--ops', '--keys', prefix + sc['keys'],
            '--steps', '80000000', '--shotdir', od]
    if patch: args += ['--patch', patch]
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode: print(r.stdout, r.stderr); return 1
    for f in os.listdir(od):
        if f.endswith('.png') or f.endswith('.ops'): os.remove(os.path.join(od, f))
    env = dict(os.environ, **sc.get('env', {}))
    if sc['cmd'] == 'castle-test':
        # the oracle's RANDOM state at the first Castle screen (key wait #skip+1)
        first = sorted(f for f in os.listdir(od) if f.endswith('.grid'))[skip]
        rng = [l for l in open(os.path.join(od, first)).read().split('\n') if l.startswith('rng')][0].split()[1]
        env['WIZ1_RNG'] = rng
    if patch: env['WIZ1_PATCH'] = patch
    r = subprocess.run([exe, sc['cmd'], DATA, sc['keys'], ed], capture_output=True, text=True, env=env)
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
