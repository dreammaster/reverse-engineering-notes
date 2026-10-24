"""
Headless smoke tests for the MinGW build (build/u5.exe).

    python tools/smoke_test.py [--game-dir C:/games/ultima5] [scenario ...]

Each scenario runs the game headless (U5_HEADLESS=1, U5_FAST=1) with a scripted key feed (see src/key/key_win32.c),
takes screenshots, and checks the process exits cleanly and that the screenshots are not blank.  Scenarios run in
run/ (a working copy of the game data; the original game directory is never modified).
"""
import argparse
import os
import struct
import subprocess
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "build", "u5.exe")
RUN = os.path.join(ROOT, "run")

# name -> (keys, [screenshots expected], timeout seconds)
SCENARIOS = {
    "boot": ("{sleep:500}{shot:boot.png}{quit}", ["boot.png"], 120),
    # 'J' at the very first key poll is the original "Journey Onward" shortcut; the pristine SAVED.GAM has no
    # character yet, so the game prints "No active game. Please create a character..."
    "journey": ("j{sleep:1500}{shot:noactive.png}{quit}", ["noactive.png"], 120),
}


def load_png_rgb(path):
    """Decode the PNGs written by GRAP_WIN32_SaveScreenshot (8-bit RGB, filter 0)."""
    data = open(path, "rb").read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    pos, idat, w, h = 8, b"", 0, 0
    while pos < len(data):
        n, typ = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if typ == b"IHDR":
            w, h = struct.unpack(">II", body[:8])
        elif typ == b"IDAT":
            idat += body
        pos += 12 + n
    raw = zlib.decompress(idat)
    return w, h, raw


def distinct_colors(path):
    w, h, raw = load_png_rgb(path)
    stride = 1 + w * 3
    colors = set()
    for y in range(h):
        row = raw[y * stride + 1:(y + 1) * stride]
        for x in range(0, len(row), 3):
            colors.add(row[x:x + 3])
    return len(colors)


def run_scenario(name, keys, shots, timeout):
    env = dict(os.environ, U5_HEADLESS="1", U5_FAST="1", U5_KEYS=keys)
    for s in shots:
        p = os.path.join(RUN, s)
        if os.path.exists(p):
            os.remove(p)
    try:
        r = subprocess.run([EXE], cwd=RUN, env=env, timeout=timeout, capture_output=True)
    except subprocess.TimeoutExpired:
        return "TIMEOUT after %ds" % timeout
    if r.returncode != 0:
        return "exit code %d" % r.returncode
    for s in shots:
        p = os.path.join(RUN, s)
        if not os.path.exists(p):
            return "missing screenshot " + s
        if distinct_colors(p) < 2:
            return "blank screenshot " + s
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game-dir", default="C:/games/ultima5")
    ap.add_argument("scenarios", nargs="*")
    args = ap.parse_args()

    if not os.path.exists(EXE):
        sys.exit("build/u5.exe not found; run mingw32-make first")
    subprocess.run([sys.executable, os.path.join(ROOT, "tools", "u5tool.py"), "rundir", args.game_dir, RUN], check=True)

    failed = 0
    for name in args.scenarios or SCENARIOS:
        keys, shots, timeout = SCENARIOS[name]
        err = run_scenario(name, keys, shots, timeout)
        print("%-8s %s" % (name, "ok" if err is None else "FAIL: " + err))
        failed += err is not None
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
