"""
Builds and runs every test_*.c in this directory using the gcc line from its header comment (the text after " *   gcc" up to
"&& ./test_x"), and prints one line per suite plus a summary. Needs gcc on PATH (MinGW: C:\\mingw64\\bin).

    python run_all.py [name-filter]
"""
import glob
import os
import re
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
os.chdir(here)
flt = sys.argv[1] if len(sys.argv) > 1 else ""
failed = []
total = 0
for path in sorted(glob.glob("test_*.c")):
    if flt not in path:
        continue
    text = open(path, encoding="utf-8", errors="replace").read(2000)
    m = re.search(r"\*\s+((?:gcc|cc) .*?)&&\s*(\./test_\w+)", text, re.S)
    if not m:
        print("NOBUILD", path)
        failed.append(path)
        continue
    cmd = re.sub(r"\\?\s*\n\s*\*\s*", " ", m.group(1)).strip()
    if cmd.startswith("cc "):
        cmd = "gcc " + cmd[3:]
    exe = m.group(2)
    total += 1
    build = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if build.returncode != 0:
        print("BUILD FAIL", path)
        print(build.stdout + build.stderr)
        failed.append(path)
        continue
    run = subprocess.run([os.path.join(here, exe[2:])], capture_output=True, text=True)
    status = "ok" if run.returncode == 0 else "FAIL"
    print(f"{status:5} {path}")
    if run.returncode != 0:
        print("\n".join(l for l in run.stdout.splitlines() if l.startswith("FAIL")))
        failed.append(path)
print(f"\n{total - len(failed)}/{total} suites passed")
sys.exit(1 if failed else 0)
