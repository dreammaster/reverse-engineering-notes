#!/usr/bin/env python3
"""Compare the translated game-rules routines (src/gen/rules_gen.c) with the original code in the emulator.

usage: mm3_rules_check.py MM3.EXE DATADIR RULES_RUN [N] [SEED]
Random party states (real roster characters with random mutations, mutated party block) and random arguments; for each call the
return value (dx:ax) and the whole data segment after the call must be identical."""
import os
import random
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm3_cc  # noqa: E402
import mm3_emu  # noqa: E402

DSEG = mm3_emu.DSEG
PARTY_CHARS = 0xB9D6      # active character copies, 12Fh bytes each
PTY = 0xE8EA              # party block (MAZE.PTY)
CHAR_SIZE = 0x12F
LINEAR = {"ifProc": 0x3D32E, "getMaxHP": 0x4FB2C, "getMaxSP": 0, "getArmorClass": 0, "getAge": 0, "getStat": 0,
          "getCurrentLevel": 0, "conditionMod": 0, "itemScan": 0, "statBonus": 0, "checkSkill": 0x153EA}
VALID_ACTIONS = [3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20, 21, 23, 25, 34, 35] + list(range(37, 100))
NAMES = ["ifProc", "getMaxHP", "getMaxSP", "getArmorClass", "getAge", "getStat", "getCurrentLevel", "conditionMod", "itemScan",
         "statBonus", "checkSkill"]


def lookup_linear(tsv):
    import re
    out = {}
    idc = os.path.join(os.path.dirname(tsv), "..", "mm3.idc")
    for m in re.finditer(r'set_name\s*\(0[Xx]([0-9A-Fa-f]+),\s*"([^"]+)"', open(idc, encoding="latin1").read()):
        if m.group(2) in NAMES:
            out[m.group(2)] = int(m.group(1), 16)
    return out


def main():
    exe, data, runner = sys.argv[1:4]
    n = int(sys.argv[4]) if len(sys.argv) > 4 else 100
    rnd = random.Random(int(sys.argv[5]) if len(sys.argv) > 5 else 3)
    lin = lookup_linear(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "names", "mm3.tsv"))
    emu = mm3_emu.Emu(exe)
    emu.hook(0x3CE7A, lambda e: 1, "far")  # the yes/no dialog: answer 1 (the C host does the same)
    cur = open(os.path.join(data, "MM3.CUR"), "rb").read()
    mem = {i: mm3_cc.member(cur, o, s)[0] for i, o, s in mm3_cc.read_toc(cur)}
    roster = mem[mm3_cc.name_id("MAZE.CHR")]
    pty = mem[mm3_cc.name_id("MAZE.PTY")]
    tmp = tempfile.mkdtemp()
    bad = 0
    calls = 0
    for case in range(n):
        emu.reset_dgroup()
        # party: six random roster characters, lightly mutated
        for i in range(8):
            ch = bytearray(roster[rnd.randrange(30) * CHAR_SIZE:][:CHAR_SIZE])
            for _ in range(rnd.randrange(0, 12)):
                ch[rnd.randrange(CHAR_SIZE)] = rnd.randrange(256)
            if rnd.random() < 0.7:  # keep fields in their usual ranges more often
                ch[0x13] = rnd.randrange(10); ch[0x11] = rnd.randrange(5); ch[0x10] = rnd.randrange(2); ch[0x23] = rnd.randrange(1, 40)
            emu.wbytes(PARTY_CHARS + i * CHAR_SIZE, ch)
        p = bytearray(pty)
        for _ in range(rnd.randrange(0, 20)):
            p[rnd.randrange(0x17, len(p))] = rnd.randrange(256)
        p[0] = rnd.randrange(1, 7)
        emu.wbytes(PTY, p)
        snap = emu.rbytes(0, 0x10000)
        open(tmp + "/in.dg", "wb").write(snap)
        for _ in range(6):
            which = rnd.choice(NAMES)
            ch = rnd.randrange(6)
            off = PARTY_CHARS + ch * CHAR_SIZE
            if which == "ifProc":
                args = [rnd.choice(VALID_ACTIONS), rnd.choice([0, 1, 2, 5, 10, 21, 60, rnd.randrange(0, 70000) & 0xFFFF]), 0, rnd.randrange(3), ch]
            elif which in ("getMaxHP", "getMaxSP", "getCurrentLevel"):
                args = [off, DSEG]
            elif which == "getArmorClass":
                args = [off, rnd.randrange(2)]
            elif which == "getAge":
                args = [off, DSEG, rnd.randrange(2)]
            elif which == "getStat":
                args = [off, DSEG, rnd.randrange(7), rnd.randrange(2)]
            elif which in ("conditionMod", "itemScan"):
                args = [off, DSEG, rnd.randrange(0, 40)]
            elif which == "statBonus":
                args = [rnd.randrange(0, 60)]
            else:
                args = [rnd.randrange(0, 20)]
            emu.wbytes(0, snap)
            lin_addr = lin.get(which)
            if not lin_addr:
                continue
            try:
                ax = emu.call_linear(lin_addr, args)
            except Exception as ex:  # noqa: BLE001
                print("EMULATOR FAILURE %s%r: %s" % (which, args, str(ex)[:120]))
                bad += 1
                continue
            dx = emu.uc.reg_read(mm3_emu.UC_X86_REG_DX)
            want = emu.rbytes(0, 0x10000)
            r = subprocess.run([runner, tmp + "/in.dg", tmp + "/out.dg", which] + [str(a) for a in args], capture_output=True, text=True)
            got = open(tmp + "/out.dg", "rb").read()
            calls += 1
            if True:  # 16-bit results: dx is whatever the caller left in it (ifProc too: its result is a truth value in ax)
                dx_c = r.stdout.strip().split("dx=")[-1][:4] if "dx=" in r.stdout else ""
                r.stdout = r.stdout.split(" dx=")[0] + " dx=%s" % ("%04X" % dx)
            ok = got == want and r.stdout.strip() == "ax=%04X dx=%04X" % (ax, dx)
            if not ok:
                bad += 1
                open("/tmp/mm3_fail_%s_%d.dg" % (which, bad), "wb").write(snap)
                diff = [i for i in range(0x10000) if got[i] != want[i]]
                print("MISMATCH %s%r: emu ax=%04X dx=%04X, C %s; %d bytes differ%s" % (which, args, ax, dx, r.stdout.strip() or r.stderr.strip()[:80],
                      len(diff), (" first at %04X (want %02X got %02X)" % (diff[0], want[diff[0]], got[diff[0]])) if diff else ""))
                if bad > 6:
                    break
        if bad > 6:
            break
    print("%d calls, %d mismatches" % (calls, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
