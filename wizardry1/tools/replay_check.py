#!/usr/bin/env python3
"""
Console regression: run the real game under the oracle for a keystroke script,
then replay its recorded driver/window-manager calls through wiz::Console and
compare every composed screen (cells + VRAM CRC).

    replay_check.py <wiz1.exe> [keys]

Exit 0 = identical, 1 = mismatch, 77 = skipped (no game disk / python deps).
"""
import os, subprocess, sys, tempfile, shutil

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
DISK = r'C:\games\wizard15\WIZ1.DSK'
if not os.path.exists(DISK) or not os.path.exists(os.path.join(ROOT, 'extracted', 'wiz1', 'SYSTEM.PASCAL')):
    print('skipped: game disk / extracted files not present')
    sys.exit(77)
exe = sys.argv[1]
keys = sys.argv[2] if len(sys.argv) > 2 else r'S\r\rGA\r\x1bA\r\x1bT\r\x1bB\r\x1bE'
tmp = tempfile.mkdtemp(prefix='wizreplay')
try:
    r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'wizvm.py'), 'run', '--ops',
                        '--keys', keys, '--steps', '80000000', '--shotdir', tmp],
                       capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout, r.stderr); sys.exit(1)
    for f in os.listdir(tmp):                       # only .ops/.grid are needed
        if f.endswith('.png'): os.remove(os.path.join(tmp, f))
    r = subprocess.run([exe, 'console-replay', os.path.join(ROOT, 'extracted', 'wiz1'), tmp],
                       capture_output=True, text=True)
    print(r.stdout[-3000:])
    sys.exit(r.returncode)
finally:
    shutil.rmtree(tmp, ignore_errors=True)
