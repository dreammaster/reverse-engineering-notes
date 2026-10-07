#!/usr/bin/env python3
"""Parallel, incremental build of one game's reconstructed sources (what tools/build.sh does one file at
a time). Objects and dependency files go to build/<game>-<bits>/ (ignored by git).
Usage: tools/pbuild.py [game] [-32] [-r]    e.g. tools/pbuild.py deponia1 -32 -r
  -32  build with the 32-bit MinGW (C:\\mingw32); default is C:\\mingw64
  -r   run the program afterwards (the smoke test: it must exit with 0)"""
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
args = [a for a in sys.argv[1:] if not a.startswith("-")]
game = args[0] if args else "deponia1"
bits = 32 if "-32" in sys.argv else 64
SRC = os.path.join(ROOT, "src", game)
OBJ = os.path.join(ROOT, "build", "%s-%d" % (game, bits))
CXX = (r"C:\mingw64\bin\g++.exe" if bits == 64 else r"C:\mingw32\bin\g++.exe")
FLAGS = ["-std=c++17", "-Wall", "-Wextra", "-I", SRC, "-MMD"]

sources = []
for root, _, files in os.walk(SRC):
    for f in files:
        if f.endswith(".cpp"):
            p = os.path.join(root, f)
            sources.append((p, os.path.join(OBJ, os.path.relpath(p, SRC)[:-4] + ".o")))


def stale(src, obj):
    if not os.path.exists(obj):
        return True
    t = os.path.getmtime(obj)
    if os.path.getmtime(src) > t:
        return True
    d = obj[:-2] + ".d"
    if not os.path.exists(d):
        return True
    text = open(d, encoding="utf-8", errors="replace").read().replace("\\\n", " ")
    for dep in text.split(":", 1)[1].split():
        if os.path.exists(dep) and os.path.getmtime(dep) > t:
            return True
    return False


def compile_one(item):
    src, obj = item
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    r = subprocess.run([CXX] + FLAGS + ["-c", src, "-o", obj], capture_output=True, text=True)
    return src, (r.stdout + r.stderr) if r.returncode != 0 else None, r.stderr


todo = [s for s in sources if stale(*s)]
print("compiling %d of %d files (%d-bit)" % (len(todo), len(sources), bits), flush=True)
failed = False
with ThreadPoolExecutor(os.cpu_count() or 4) as ex:
    for src, err, warn in ex.map(compile_one, todo):
        if err:
            failed = True
            print("FAILED", src)
            print("\n".join(l for l in err.splitlines() if "error" in l)[:4000])
        elif "warning" in warn:
            print("warnings in", os.path.relpath(src, SRC))
            print("\n".join(l for l in warn.splitlines() if "warning" in l)[:1500])
if failed:
    sys.exit(1)
exe = os.path.join(SRC, "%s.exe" % game)
r = subprocess.run([CXX, "-o", exe] + [o for _, o in sources], capture_output=True, text=True)
if r.returncode != 0:
    print(r.stderr[:4000])
    sys.exit(1)
print("built", exe)
if "-r" in sys.argv:
    env = dict(os.environ)
    env["PATH"] = os.path.dirname(CXX) + os.pathsep + env["PATH"]
    code = subprocess.run([exe], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode
    print("smoke test exit code", code)
    sys.exit(code)
