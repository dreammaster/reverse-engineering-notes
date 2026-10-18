"""
Debugging aid: replays an ops file up to operation INDEX on the oracle, then records every byte written to the data object / BSS during operation INDEX (by the original
machine code through a unicorn memory hook, and by the Python stubs through mem_write) and compares that sequence with the native engine's write log for the same operation.
Prints the first write that differs, with the native function stack / the oracle's current function.

    python port/tests/write_diff.py OPS INDEX
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "reversing", "scripts"))
TOOL = os.path.join(ROOT, "port", "build", "engine_tool.exe")

from retail_game import Game   # noqa: E402
from unicorn import UC_HOOK_MEM_WRITE   # noqa: E402


def apply(g, p, base):
    kind, a = p[0], [int(x) for x in p[1:4]]
    if kind == "reset":
        g.restore(base)
    elif kind == "flag":
        ad = 0xCD8AC + ((a[0] - 1) >> 3)
        g.w8(ad, (g.r8(ad) | (1 << ((a[0] - 1) & 7))) if a[1] else (g.r8(ad) & ~(1 << ((a[0] - 1) & 7)) & 0xFF))
    elif kind == "goto":
        g.call(0x1EE10, eax=a[0])
    elif kind == "move":
        g.call(0x1B470, eax=a[0], edx=a[1], ebx=a[2])
    elif kind == "cmd":
        g.command(*a)
    elif kind == "attr":
        g.call(0x1A1B0 if a[2] else 0x1A1D8, eax=a[0], edx=a[1])
    elif kind == "phase":
        g.call(0x21AF4, eax=a[0], edx=a[1])
    elif kind == "call":
        g.call(a[0], eax=a[1], edx=a[2])
    g.text()


def main():
    lines = [l.split() for l in open(sys.argv[1]).read().splitlines() if l.strip() and not l.startswith("#")]
    idx = int(sys.argv[2])
    g = Game(os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game", "SHANSTR.DAT"))
    base = g.snapshot()
    for p in lines[:idx]:
        apply(g, p, base)
    log = []
    orig = g.uc.mem_write

    def hook_write(uc, access, address, size, value, user):
        if 0xA0000 <= address < 0xF0000:
            for k in range(size):
                log.append((address + k, (value >> (8 * k)) & 0xFF, g._pc))

    def mem_write(addr, data, *a, **k):
        if 0xA0000 <= addr < 0xF0000 and not getattr(g, "_in_replay", False):
            for k, b in enumerate(bytes(data)):
                log.append((addr + k, b, -1))
        return orig(addr, data, *a, **k)

    h = g.uc.hook_add(UC_HOOK_MEM_WRITE, hook_write)
    g.uc.mem_write = mem_write
    apply(g, lines[idx], base)
    g.uc.mem_write = orig
    g.uc.hook_del(h)
    ops = os.path.join(ROOT, "port", "build", "wd_ops.txt")
    open(ops, "w").write("\n".join(" ".join(p) for p in lines[:idx + 1]) + "\n")
    wl = os.path.join(ROOT, "port", "build", "wd_log.txt")
    env = dict(os.environ, ENGINE_WRITELOG=wl, ENGINE_TRACE_FROM=str(idx))
    subprocess.run([TOOL, os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game"), ops], capture_output=True, text=True, env=env)
    native = [(int(a, 16), int(v, 16)) for a, v in (l.split() for l in open(wl).read().splitlines())]
    print("oracle byte writes: %d   native byte writes: %d" % (len(log), len(native)))
    # the two machines write the same bytes in the same order unless a shim/stub differs; compare as sequences but resync on value-neutral writes
    k = 0
    while k < min(len(log), len(native)) and (log[k][0], log[k][1]) == native[k]:
        k += 1
    if k == min(len(log), len(native)) and len(log) == len(native):
        print("write sequences identical")
        return
    print("first difference at write #%d" % k)
    for j in range(max(0, k - 4), min(k + 6, max(len(log), len(native)))):
        o = log[j] if j < len(log) else None
        n = native[j] if j < len(native) else None
        print("  %s oracle %s  native %s" % ("->" if j == k else "  ", ("0x%X=%02x pc=%s" % (o[0], o[1], (g._func_at(o[2]) if o[2] > 0 else "python stub"))) if o else None, ("0x%X=%02x" % n) if n else None))


main()
