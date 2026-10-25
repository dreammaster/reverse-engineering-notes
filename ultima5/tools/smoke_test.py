"""
Headless smoke tests for the MinGW build (build/u5.exe).

    python tools/smoke_test.py [--game-dir C:/games/ultima5] [scenario ...]

Each scenario runs the game headless (U5_HEADLESS=1, U5_FAST=1) with a scripted key feed (see src/key/key_win32.c),
takes screenshots, and checks the process exits cleanly and that the screenshots are not blank.  Scenarios run in
run/ (a working copy of the game data; the original game directory is never modified).
"""
import argparse
import hashlib
import json
import os
import shutil
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
    "play": ("j{sleep:6000}{shot:play1.png}{up}{sleep:700}{left}{sleep:700}{shot:play2.png}{quit}", ["play1.png", "play2.png"], 180),
    "stats": ("j{sleep:6000}z{sleep:1500}{shot:stats_select.png}1{sleep:1500}{shot:stats1.png}{esc}{sleep:800}{quit}",
              ["stats_select.png", "stats1.png"], 180),
    "look": ("j{sleep:6000}{right}{sleep:300}{down}{sleep:300}{down}{sleep:300}l{sleep:300}{down}{sleep:1500}{shot:look.png}"
             "k{sleep:300}{down}{sleep:1500}{shot:klimb.png}{quit}", ["look.png", "klimb.png"], 180),
    # "paula-outside" fixture: standing outside the start room; walking south to the map edge asks to leave
    "overworld": ("j{sleep:6000}" + "{down}{sleep:200}" * 14 + "{sleep:1000}{shot:leave.png}y{sleep:3000}{down}{sleep:1500}{sleep:5000}"
                  "{shot:overworld1.png}{down}{sleep:300}{sleep:5000}{shot:overworld2.png}z{sleep:800}1{sleep:1500}{shot:overworld_stats.png}{quit}",
                  ["leave.png", "overworld1.png", "overworld2.png", "overworld_stats.png"], 240),
    # "paula-abbey" fixture: one square east of Empath Abbey on the overworld
    "abbey": ("j{sleep:6000}{left}{sleep:2000}e{sleep:8000}{shot:abbey_gate.png}" + "{up}{sleep:700}" * 4 + "{left}{sleep:700}"
              "{shot:abbey_hall.png}t{sleep:500}{left}{sleep:2500}{shot:abbey_talk.png}{quit}",
              ["abbey_gate.png", "abbey_hall.png", "abbey_talk.png"], 240),
    # "paula-jester" fixture: Hardluck the abbey jester one square north (an empty line ends a conversation)
    "talk": ("j{sleep:6000}t{sleep:500}{up}{sleep:2500}{shot:talk_greet.png}name{enter}{sleep:3000}{shot:talk_name.png}"
             "job{enter}{sleep:3000}{shot:talk_job.png}bye{enter}{sleep:3000}{shot:talk_bye.png}{quit}",
             ["talk_greet.png", "talk_name.png", "talk_job.png", "talk_bye.png"], 240),
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


GOLDEN = os.path.join(ROOT, "tests", "golden.json")
# regions that legitimately vary between runs: wind banner, text cursor line
MASKS = [(96, 362, 300, 388), (384, 340, 640, 392)]


def screen_hash(path):
    w, h, raw = load_png_rgb(path)
    stride = 1 + w * 3
    rows = []
    for y in range(h):
        row = bytearray(raw[y * stride + 1:(y + 1) * stride])
        for x0, y0, x1, y1 in MASKS:
            if y0 <= y < y1:
                row[x0 * 3:x1 * 3] = bytes((x1 - x0) * 3)
        rows.append(bytes(row))
    return hashlib.sha1(b"".join(rows)).hexdigest()


def install_fixture(name):
    """Copy tests/fixtures/<name>/* over the saves in run/SAVEGAME (scenario starts from that character)."""
    src = os.path.join(ROOT, "tests", "fixtures", name)
    for f in os.listdir(src):
        shutil.copy2(os.path.join(src, f), os.path.join(RUN, "SAVEGAME", f))


def run_scenario(name, keys, shots, timeout, golden=None, update=None):
    env = dict(os.environ, U5_HEADLESS="1", U5_FAST="1", U5_SEED="7", U5_VCLOCK="1", U5_KEYS=keys)
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
        if golden is not None and s in golden:
            if screen_hash(p) != golden[s]:
                return "screenshot %s differs from golden" % s
        elif update is not None:
            update[s] = screen_hash(p)
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game-dir", default="C:/games/ultima5")
    ap.add_argument("--update-golden", action="store_true", help="record screenshot hashes in tests/golden.json")
    ap.add_argument("scenarios", nargs="*")
    args = ap.parse_args()

    if not os.path.exists(EXE):
        sys.exit("build/u5.exe not found; run mingw32-make first")
    subprocess.run([sys.executable, os.path.join(ROOT, "tools", "u5tool.py"), "rundir", args.game_dir, RUN], check=True)

    golden = json.load(open(GOLDEN)) if os.path.exists(GOLDEN) else {}
    update = {} if args.update_golden else None
    failed = 0
    for name in args.scenarios or SCENARIOS:
        keys, shots, timeout = SCENARIOS[name]
        if name in ("play", "stats", "look"):
            install_fixture("paula")
        elif name == "talk":
            install_fixture("paula-jester")
        elif name == "abbey":
            install_fixture("paula-abbey")
        elif name == "overworld":
            install_fixture("paula-outside")
        err = run_scenario(name, keys, shots, timeout, None if args.update_golden else golden, update)
        print("%-8s %s" % (name, "ok" if err is None else "FAIL: " + err))
        failed += err is not None
    if update:
        golden.update(update)
        json.dump(golden, open(GOLDEN, "w"), indent=1, sort_keys=True)
        print("recorded %d golden hashes" % len(update))
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
