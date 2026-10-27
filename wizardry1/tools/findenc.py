import random, subprocess, os, sys
exe = r'C:\dev\wizardry\wizardry1\engine\build\wiz1.exe'
data = r'C:\dev\wizardry\wizardry1\extracted\wiz1'
random.seed(int(sys.argv[1]) if len(sys.argv) > 1 else 5)
env = dict(os.environ)
bs = chr(92)
for t in range(300):
    walk = ''.join(random.choice('WWWWADX') for _ in range(80))
    keys = 'GAAC' + bs + 'rLEMLN' + walk
    r = subprocess.run([exe, 'castle-test', data, keys], capture_output=True, text=True, env=env)
    line = r.stdout.strip().splitlines()[-1] if r.stdout.strip() else ''
    if 'xgoto 8 ' in line:
        print(t, walk, line); break
