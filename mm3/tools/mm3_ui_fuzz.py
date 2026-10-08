#!/usr/bin/env python3
"""Fuzz the C text/window engine (src/ui_text.c) against the original video module in the emulator.

usage: mm3_ui_fuzz.py MM3.EXE DATADIR UI_RUN [N] [SEED]
Random window geometries and strings (letters, spaces, punctuation, the control codes) on a real background picture; after every
command the 320x200 screen of the original and of the C port must be identical."""
import os
import random
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm3_cc  # noqa: E402
import mm3_vdrv_oracle as vo  # noqa: E402

WORDS = ["the", "party", "of", "adventurers", "stands", "before", "a", "great", "door", "Gold", "and", "gems", "sparkle", "in-the-dark",
         "path/way", "Isles", "Terra", "Might", "Magic", "quick", "brown", "fox", "jumps", "over", "lazy", "dog", "gypsy", "Quay", "yes", "no"]


def gen_text(rnd, level):
    out = bytearray()
    for _ in range(rnd.randrange(1, int(os.environ.get('MM3_FUZZ_ITEMS', '14')))):
        r = rnd.random()
        if r < 0.7 or level == 0:
            out += rnd.choice(WORDS).encode() + b" "
        elif r < 0.78:
            out += b"\x03" + bytes([rnd.choice(b"clrtf")])
        elif r < 0.82:
            out += b"\x0a"
        elif r < 0.85:
            out += b"\x01" if rnd.random() < 0.5 else b"\x02"
        elif r < 0.88:
            out += b"\x0c%02d" % rnd.randrange(0, 20)
        elif r < 0.90:
            out += b"\x07%03d" % rnd.randrange(0, 256)
        elif r < 0.92:
            out += b"\x04%03d" % rnd.randrange(1, 60)
        elif r < 0.94:
            out += b"\x09%03d" % rnd.randrange(0, 150)
        elif r < 0.96:
            out += b"\x0b%03d" % rnd.randrange(0, 80)
        elif r < 0.975:
            out += b"\x08" + bytes([rnd.choice(b"oO.-")])
        elif r < 0.985 and os.environ.get("MM3_FUZZ_EXTRA"):
            out += b"\x03d"
        elif r < 0.99 and os.environ.get("MM3_FUZZ_EXTRA"):
            out += b"\x0d"
        else:
            out += b"\x06"
    return bytes(out)


def main():
    exe, data, runner = sys.argv[1:4]
    n = int(sys.argv[4]) if len(sys.argv) > 4 else 40
    rnd = random.Random(int(sys.argv[5]) if len(sys.argv) > 5 else 1)
    o = vo.VdrvOracle(exe, os.path.join(data, "MM3.CC"))
    bg = o.members[mm3_cc.name_id("CREATE.RAW")]
    tmp = tempfile.mkdtemp()
    bad = 0
    for case in range(n):
        o.reset()
        o.set_screen(bg)
        cmds, lines = [], []
        depth = 0
        for _ in range(rnd.randrange(1, 6)):
            r = rnd.random()
            if r < 0.5 and depth < 4:
                w = rnd.randrange(60, 250); h = rnd.randrange(40, 120)
                x = rnd.randrange(0, 320 - w); y = rnd.randrange(0, 200 - h)
                col = rnd.randrange(0, 12)
                text = gen_text(rnd, 1) if rnd.random() < 0.85 else None
                cmds.append(("O", (x, y, w, h, col, text))); depth += 1
                lines.append("O %d %d %d %d %d %s" % (x, y, w, h, col, text.hex() if text is not None else "-"))
            elif r < 0.85:
                text = gen_text(rnd, 1)
                cmds.append(("P", text)); lines.append("P " + text.hex())
            elif depth:
                k = rnd.randrange(1, depth + 1)
                cmds.append(("C", k)); depth -= k; lines.append("C %d" % k)
        open(tmp + "/script.txt", "w").write("\n".join(lines) + "\n")
        open(tmp + "/init.scr", "wb").write(bg)
        subprocess.run([runner, data, tmp + "/script.txt", tmp + "/init.scr", tmp + "/out.bin"], check=True)
        got = open(tmp + "/out.bin", "rb").read()
        for i, (kind, arg) in enumerate(cmds):
            if kind == "O":
                x, y, w, h, col, text = arg
                o.open_window(x, y, w, h, col, 0, text)
            elif kind == "P":
                o.print_text(arg)
            else:
                o.close_windows(arg)
            want = o.screen()
            have = got[i * 64000:(i + 1) * 64000]
            if want != have:
                bad += 1
                diff = [j for j in range(64000) if want[j] != have[j]]
                print("MISMATCH case %d command %d (%s %r): %d pixels differ, first at (%d,%d)" % (
                    case, i, kind, arg, len(diff), diff[0] % 320, diff[0] // 320))
                print("  script:", lines)
                break
        if bad > 4:
            break
    print("%d cases, %d mismatches" % (n, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
