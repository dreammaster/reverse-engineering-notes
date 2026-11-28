"""
fuzz_playlog.py FIRST LAST [STEPS]: plays random command scripts (verb dobj iobj, whole turns through execute_command) in the standalone game (xanth_playlog) and replays them in the
oracle (host_test `init` with HOST_PLAYLOG), then compares after every command the room, a hash of the saved state, the event queue and the text printed.

    python fuzz_playlog.py 1 40 40

A script stops at the first long text or conversation (the oracle cannot run those turns without a player: it hangs in its next word wrap), so many scripts are short.  Where the
oracle did stall (an empty text and no turn) the seed is counted as 'stalled', not as a difference.  Needs engine/host/host_test.exe (built from host_test.cpp) and the standalone
build (env XANTH_BUILD = build folder, default %TEMP%/xb); run from anywhere, files go to %TEMP%/pl.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
XANTH = os.path.abspath(os.path.join(HERE, "..", ".."))
BUILD = os.environ.get("XANTH_BUILD", os.path.join(tempfile.gettempdir(), "xb"))
OUT = os.path.join(tempfile.gettempdir(), "pl")
TAB = chr(9)
FS = chr(31)


def oracle(script, log, commands):
    env = dict(os.environ)
    env.update(HOST_TICKMASK="15", HOST_DRAW="1", HOST_INPUT="1000000:316E,2000000:316E,4000000:316E,8000000:316E", HOST_MAIN="1", PLAYLOG_DETAIL="1",
               HOST_PLAYLOG=script, HOST_PLAYLOG_OUT=log, HOST_STEPS=str(45000000 + commands * 3000000 + 6000000))
    subprocess.run([os.path.join(XANTH, "engine", "host", "host_test.exe"), "game", "game_out/image.bin", "reversing/analysis/xanth_objects.tsv", "init"], cwd=XANTH, env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=900)


def clean(text):
    return FS.join(e for e in text.split(FS) if not e.startswith("Font_WriteString|"))


def load(path):
    # a text that contains a line break continues on the next line of the log: such lines (not "step<TAB>...") belong to the previous record
    out = []
    for line in open(path, "rb").read().decode("latin1").split(chr(10)):
        head = line.split(TAB)[0]
        if out and not (head.split()[:1] and head.split()[0].isdigit() and TAB in line):
            out[-1] += chr(10) + line
        else:
            out.append(line)
    return out


def main():
    first, last = int(sys.argv[1]), int(sys.argv[2])
    steps = int(sys.argv[3]) if len(sys.argv) > 3 else 40
    os.makedirs(OUT, exist_ok=True)
    env = dict(os.environ, PLAYLOG_DETAIL="1")
    same = stalled = 0
    for seed in range(first, last + 1):
        script, mine, orc = (os.path.join(OUT, "f%d.txt" % seed), os.path.join(OUT, "oursf%d.log" % seed), os.path.join(OUT, "orcf%d.log" % seed))
        subprocess.run([os.path.join(BUILD, "xanth_playlog.exe"), "game", script, mine, "gen", str(steps), str(seed)], cwd=XANTH, env=env)
        n = len([l for l in open(script) if l.strip()])
        oracle(script, orc, n)
        a, b = load(mine), load(orc)
        verdict = "identical"
        for x, y in zip(a, b):
            fx, fy = x.split(TAB), y.split(TAB)
            if len(fx) > 5:
                fx[5] = clean(fx[5])
            if len(fy) > 5:
                fy[5] = clean(fy[5])
            if fx != fy:
                verdict = "stalled" if len(fy) > 5 and not fy[5].strip() else "DIFFERENT at %s" % fx[0]
                break
        print("seed %d (%d commands): %s" % (seed, n, verdict))
        same += verdict == "identical"
        stalled += verdict == "stalled"
    print("%d identical, %d oracle stalls, %d different" % (same, stalled, last - first + 1 - same - stalled))


main()
