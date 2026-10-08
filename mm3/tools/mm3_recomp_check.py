#!/usr/bin/env python3
"""Check translated C routines (src/gen, tools/asm2c.py) against the original code running in the emulator.

usage: mm3_recomp_check.py MM3.EXE MM3.CUR RUNNER FUNC LINEAR [N]
For N positions of the indoor maps: prepare the view in the emulator, snapshot DGROUP at the entry of FUNC (a routine called
by renderIndoorView with one word argument), run FUNC alone in the emulator and with the C runner from the same snapshot,
and compare the whole data segment and the return value.
"""
import os
import random
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm3_vieworacle as vo  # noqa: E402
import mm3_cc  # noqa: E402


def main():
    exe, cur, runner, func, linear = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5], 16)
    n = int(sys.argv[6]) if len(sys.argv) > 6 else 50
    o = vo.ViewOracle(exe, cur)
    e = o.emu
    rnd = random.Random(1)
    cases = []
    for map_id in range(1, 41):
        name = mm3_cc.name_id("MAZE%02d.DAT" % map_id)
        if name not in o.members:
            continue
        dat = o.members[name]
        # open cells of the 16x16 page (no wall word 1111h only)
        cells = [(x, y) for y in range(16) for x in range(16)]
        for _ in range(3):
            x, y = rnd.choice(cells)
            cases.append((map_id, x, y, rnd.randrange(4)))
    rnd.shuffle(cases)
    cases = cases[:n]
    tmp = tempfile.mkdtemp()
    bad = 0
    for idx, (map_id, x, y, f) in enumerate(cases):
        o.setup(map_id, x, y, f)
        e.call_linear(vo.LINEAR["prepareIndoorView"])
        snap = {}

        def grab(emu):
            snap["dg"] = emu.rbytes(0, 0x10000)
            snap["arg"] = emu.arg(0, True)
        e.watch(linear, grab)
        e.call_linear(vo.LINEAR["renderIndoorView"])
        e.watches.clear()
        if "dg" not in snap:
            print("case", idx, "routine not reached"); continue
        # original: restore the snapshot and run the routine alone
        e.wbytes(0, snap["dg"])
        # other routines it calls must be hooked the same way as in the oracle (already)
        ax = e.call_linear(linear, [snap["arg"]])
        want = e.rbytes(0, 0x10000)
        open(tmp + "/in.dg", "wb").write(snap["dg"])
        r = subprocess.run([runner, tmp + "/in.dg", func, str(snap["arg"]), tmp + "/out.dg"], capture_output=True, text=True)
        got = open(tmp + "/out.dg", "rb").read()
        reg = r.stdout.strip()
        ok = got == want and reg.startswith("ax=%04X" % ax)
        if not ok:
            bad += 1
            diffs = [i for i in range(0x10000) if got[i] != want[i]]
            print("MISMATCH case %d map %d (%d,%d) f%d: %s, %d bytes differ, first at %04X (want %02X got %02X); %s (emu ax=%04X)" % (
                idx, map_id, x, y, f, "regs" if got == want else "memory", len(diffs), diffs[0] if diffs else 0,
                want[diffs[0]] if diffs else 0, got[diffs[0]] if diffs else 0, reg, ax))
    print("%d cases, %d mismatches" % (len(cases), bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
