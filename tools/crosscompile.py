#!/usr/bin/env python3
"""Compiles the reconstructed sources of a game for Linux x86-64 (the platform of the original executable) with a cross
compiler, so that the code can be compared with the original's (tools/crosscompare.py).

The cross toolchain lives outside the repository (default C:\\dev\\crosstools, override with VIS_CROSSTOOLS); tools/crosssetup.py
makes it:
  zigpkg/ziglang/zig.exe  clang of the ziglang wheel; it cross compiles to Linux and writes ELF objects
  sysroot/                the headers of Debian 7 (wheezy): libstdc++6-4.7-dev, libc6-dev, linux-libc-dev - what GCC 4.7.2 of the
                          original (`readelf -p .comment Deponia`) was built with, with the old std::string (a reference count)
  tools/crossshim/        a std::variant (4.7 has none), the config.h that ScummVM's headers want
Usage: tools/crosscompile.py [game] [-j N] [-std=gnu++NN] [-O2] [-X clangflag] [--tag NAME] [file.cpp ...]   (objects go to build/cross/<game>[-NAME]/)"""
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.environ.get("VIS_CROSSTOOLS", r"C:\dev\crosstools")
ZIG = os.path.join(TOOLS, "zigpkg", "ziglang", "zig.exe")
SYS = os.path.join(TOOLS, "sysroot")
SYS8 = os.path.join(TOOLS, "sysroot8")
SCUMMVM = os.environ.get("VIS_SCUMMVM", r"C:\dev\scummvm")
TP = os.path.join(ROOT, "third_party")

# files that are not part of what the original was made of (they are for Windows, or for the stubs)
SKIP = {"WxStub.cpp", "luaStdLibs.cpp"}  # (Windows, and <filesystem> that 4.7 does not have)

args = sys.argv[1:]
jobs = 8
verbose = False
tag = ""
extra = []
std = "-std=gnu++17"
opt = "-O2"
files = []
game = "deponia1"
i = 0
while i < len(args):
    a = args[i]
    if a == "-j":
        jobs = int(args[i + 1])
        i += 1
    elif a == "-v":
        verbose = True
    elif a == "--tag":
        tag = "-" + args[i + 1]
        i += 1
    elif a == "-X":
        extra.append(args[i + 1])
        i += 1
    elif a.startswith("-std="):
        std = a
    elif a.startswith("-O"):
        opt = a
    elif a.endswith(".cpp"):
        files.append(os.path.abspath(a))
    elif not a.startswith("-"):
        game = a
    i += 1

SRC = os.path.join(ROOT, "src", game)
OBJ = os.path.join(ROOT, "build", "cross", game + tag)

# (the plain clang driver of zig, not `zig c++`, which puts the headers of its own glibc before the ones of the sysroot)
COMMON = ["clang", "-target", "x86_64-linux-gnu", "-x", "c++", "-nostdinc", "-nostdinc++"]
LIBC = ["-isystem", SYS + "/usr/lib/gcc/x86_64-linux-gnu/4.7/include",
        "-isystem", SYS + "/usr/lib/gcc/x86_64-linux-gnu/4.7/include-fixed",
        "-isystem", SYS + "/usr/include/x86_64-linux-gnu",
        "-isystem", SYS + "/usr/include"]
HEADERS = {
    "gcc4.7": ["-isystem", SYS + "/usr/include/c++/4.7",
               "-isystem", SYS + "/usr/include/c++/4.7/x86_64-linux-gnu",
               "-isystem", SYS + "/usr/include/c++/4.7/backward"] + LIBC,
    "gcc8": ["-isystem", SYS8 + "/usr/include/c++/8",
             "-isystem", SYS8 + "/usr/include/x86_64-linux-gnu/c++/8",
             "-isystem", SYS8 + "/usr/include/c++/8/backward",
             "-D_GLIBCXX_USE_CXX11_ABI=0", "-include", os.path.join(ROOT, "tools", "crossshim", "glibc213.h")] + LIBC,
}
OPTIONS = ["-I", os.path.join(ROOT, "tools", "crossshim"), "-I", SRC, "-I", SCUMMVM, "-I", os.path.join(TP, "freetype", "include"), "-I", os.path.join(TP, "harfbuzz", "src"),
           "-I", os.path.join(TP, "libwebp", "src"), "-I", os.path.join(TP, "libvpx"), "-I", os.path.join(TP, "opusfile", "include"),
           "-I", os.path.join(TP, "opus", "include"), "-I", os.path.join(TP, "ogg", "include"),
           "-I", os.path.join(TP, "vorbis", "include"), "-I", os.path.join(TP, "SDL2", "include"),
           std, opt, "-fomit-frame-pointer"] + extra + [ "-fno-pic", "-fno-addrsig", "-w", "-Wno-dynamic-exception-spec", "-Wno-non-pod-varargs",
           "-DHAVE_CONFIG_H", "-I", os.path.join(ROOT, "tools", "crossshim"), "-c"]

if not files:
    for root, _, names in os.walk(SRC):
        for n in names:
            if n.endswith(".cpp") and n not in SKIP:
                files.append(os.path.join(root, n))
    files.sort()


def compile_one(src):
    rel = os.path.relpath(src, SRC) if src.startswith(SRC) else os.path.basename(src)
    obj = os.path.join(OBJ, rel[:-4] + ".o")
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    error = ""
    for profile in ("gcc4.7",):
        r = subprocess.run([ZIG] + COMMON + HEADERS[profile] + OPTIONS + [src, "-o", obj], capture_output=True, text=True)
        if r.returncode == 0:
            return rel, profile, ""
        if not error:
            error = r.stderr
        last = r.stderr
    return rel, "", last


bad = 0
used = []
with ThreadPoolExecutor(jobs) as pool:
    for rel, profile, err in pool.map(compile_one, files):
        if not profile:
            bad += 1
            first = err.splitlines()[:30] if verbose else [l for l in err.splitlines() if "error" in l][:2]
            print("FAILED", rel, *first, sep="\n   ")
        else:
            used.append("%s\t%s" % (rel.replace("\\", "/"), profile))
os.makedirs(OBJ, exist_ok=True)
with open(os.path.join(OBJ, "profiles.txt"), "w") as f:
    f.write("\n".join(used) + "\n")
print("%d of %d files compiled (%d with the GCC 4.7 headers, %d with the GCC 8 ones)" % (
    len(files) - bad, len(files), sum(1 for u in used if u.endswith("gcc4.7")), sum(1 for u in used if u.endswith("gcc8"))))
sys.exit(1 if bad else 0)
