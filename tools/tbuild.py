#!/usr/bin/env python3
"""Incremental parallel build of the reconstructed sources + one test (tests/deponia1/<name>.cpp).
Usage: tools/tbuild.py <name.cpp> [-32] [-g] [-c] [-r]   (-> build/tests-<bits>/<name>.exe; -r runs it from tests/deponia1/work;
-c builds with gcov coverage into build/tests-<bits>c, see tools/coverage.py)"""
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

SRC = r"C:\dev\visionnaire\src\deponia1"
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCR = os.path.join(ROOT, "tests", "deponia1")
bits = 32 if "-32" in sys.argv else 64
OBJ = os.path.join(ROOT, "build", "tests-%d%s" % (bits, "g" if "-g" in sys.argv else "c" if "-c" in sys.argv else ""))
CXX = r"C:\mingw64\bin\g++.exe" if bits == 64 else r"C:\mingw32\bin\g++.exe"
# (the 32-bit build counts in SSE like the 64-bit one and the original: the x87 registers round differently, which changes
# the results of the comparisons in the geometry)
TP = os.path.join(ROOT, "third_party")
TPLIB = os.path.join(ROOT, "build", "third_party", str(bits))
FLAGS = ["-std=c++17", "-Wall", "-Wextra", "-I", SRC, "-I", r"C:\dev\scummvm", "-I", os.path.join(TP, "libwebp", "src"), "-I", os.path.join(TP, "opusfile", "include"), "-I", os.path.join(TP, "opus", "include"), "-I", os.path.join(TP, "ogg", "include"), "-I", os.path.join(TP, "freetype", "include"), "-I", os.path.join(TP, "harfbuzz", "src"), "-I", os.path.join(TP, "libvpx"), "-I", os.path.join(TP, "vorbis", "include"), "-I", os.path.join(TPLIB, "gen"), "-MMD"] + (["-msse2", "-mfpmath=sse"] if bits == 32 else []) + (["-g"] if "-g" in sys.argv else []) + (["--coverage", "-O0"] if "-c" in sys.argv else [])
LUA_EXCLUDE = {"scummvm_file.cpp", "lua_persist.cpp", "lua_persistence_util.cpp", "lua_unpersist.cpp", "liolib.cpp", "loslib.cpp", "loadlib.cpp", "double_serialization.cpp"}
LUA_SOURCES = [os.path.join(r"C:\dev\scummvm\common\lua", f) for f in sorted(os.listdir(r"C:\dev\scummvm\common\lua")) if f.endswith(".cpp") and f not in LUA_EXCLUDE] + ["C:/dev/visionnaire/tools/luashim/luashim.cpp"]

if not (os.path.exists(os.path.join(TPLIB, "libwebp.a")) and os.path.exists(os.path.join(TPLIB, "libfreetype.a")) and os.path.exists(os.path.join(TPLIB, "libharfbuzz.a")) and os.path.exists(os.path.join(TPLIB, "libvpx.a")) and os.path.exists(os.path.join(TPLIB, "libvorbis.a"))):
    subprocess.run([sys.executable, os.path.join(ROOT, "tools", "build_third_party.py")] + (["-32"] if bits == 32 else []), check=True)
test = [a for a in sys.argv[1:] if not a.startswith("-")][0]
test = os.path.join(SCR, test) if not os.path.isabs(test) else test

sources = []
for root, _, files in os.walk(SRC):
    for f in files:
        if f.endswith(".cpp"):
            p = os.path.join(root, f)
            if os.path.basename(p) == "mainSDL.cpp":
                continue
            sources.append((p, os.path.join(OBJ, os.path.relpath(p, SRC)[:-4] + ".o")))
for p in LUA_SOURCES:
    sources.append((p, os.path.join(OBJ, "lua", os.path.basename(p)[:-4] + ".o")))
sources.append((test, os.path.join(OBJ, "test_" + os.path.basename(test)[:-4] + ".o")))


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
    deps = text.split(":", 1)[1].split()
    for dep in deps:
        if os.path.exists(dep) and os.path.getmtime(dep) > t:
            return True
    return False


def compile_one(item):
    src, obj = item
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    r = subprocess.run([CXX] + FLAGS + (["-w"] if "scummvm" in src or "luashim" in src else []) + ["-c", src, "-o", obj], capture_output=True, text=True)
    if r.returncode != 0:
        return src, r.stdout + r.stderr
    return src, None


todo = [s for s in sources if stale(*s)]
print("compiling %d of %d" % (len(todo), len(sources)), flush=True)
failed = False
with ThreadPoolExecutor(12) as ex:
    for src, err in ex.map(compile_one, todo):
        if err:
            failed = True
            print("FAILED", src)
            print("\n".join(l for l in err.splitlines() if "error" in l or "undefined" in l)[:3000])
if failed:
    sys.exit(1)
exe = os.path.join(OBJ, os.path.basename(test)[:-4] + ".exe")
r = subprocess.run([CXX] + (["-g"] if "-g" in sys.argv else []) + (["--coverage"] if "-c" in sys.argv else []) + ["-o", exe] + [o for _, o in sources] + [os.path.join(TPLIB, "lib" + n + ".a") for n in ("opusfile", "opus", "vorbis", "ogg", "webp", "harfbuzz", "freetype", "vpx")], capture_output=True, text=True)
if r.returncode != 0:
    print(r.stderr[:4000])
    sys.exit(1)
print("built", exe)
if "-r" in sys.argv:
    env = dict(os.environ)
    env["PATH"] = os.path.dirname(CXX) + os.pathsep + env["PATH"]
    work = os.path.join(SCR, "work")
    os.makedirs(work, exist_ok=True)
    sys.exit(subprocess.run([exe], env=env, cwd=work).returncode)
