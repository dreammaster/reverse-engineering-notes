#!/usr/bin/env python3
"""Export the digitised sounds S1.S..S7.S of MM3.CC (raw unsigned 8-bit PCM, played at ~8008 Hz by the Blaster/Covox drivers) to WAV files.

usage: mm3_samples.py OUTDIR [MM3.CC]
"""
import os
import sys
import wave

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm3_cc  # noqa: E402

RATE = 8008  # 1193182 / 149 (PIT divisor 95h used by BLASTER.DRV's sample ISR)


def main():
    out = sys.argv[1]
    cc = sys.argv[2] if len(sys.argv) > 2 else r"D:\GOG Games\Might and Magic 3\MM3.CC"
    os.makedirs(out, exist_ok=True)
    d = open(cc, "rb").read()
    toc = {i: (o, s) for i, o, s in mm3_cc.read_toc(d)}
    for n in range(1, 8):
        name = "S%d.S" % n
        ent = toc.get(mm3_cc.name_id(name))
        if not ent:
            continue
        data, _ = mm3_cc.member(d, *ent)
        path = os.path.join(out, "S%d.wav" % n)
        with wave.open(path, "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(1)
            w.setframerate(RATE)
            w.writeframes(bytes(data))
        print("%s: %d samples, %.1f s -> %s" % (name, len(data), len(data) / RATE, path))


if __name__ == "__main__":
    main()
