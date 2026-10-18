"""
Instruction-level differential debugging: runs ONE command on the unicorn oracle and on the transpiled C++ engine, records the registers
before every executed instruction of the game logic on both sides, and prints the first divergence (the transpiler's bug, or a shim difference).

    python port/tests/trace_diff.py VERB DOBJ IOBJ [commands before it ...]     e.g. trace_diff.py 3 3 0

The C++ side needs port/build/engine_tool.exe built from generated code with trace hooks (the default).
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "reversing", "scripts"))
from retail_game import Game   # noqa: E402
from unicorn import UC_HOOK_CODE   # noqa: E402
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP  # noqa: E402

TOOL = os.path.join(ROOT, "port", "build", "engine_tool.exe")


def main():
    prefix = []
    if sys.argv[1] == "--ops":      # --ops FILE INDEX: replay the first INDEX lines of FILE untraced, then trace line INDEX
        lines = [l for l in open(sys.argv[2]).read().splitlines() if l.strip()]
        idx = int(sys.argv[3])
        prefix, last = lines[:idx], lines[idx]
        sys.argv = [sys.argv[0]] + last.split()[1:4]
        kind = last.split()[0]
        assert kind in ("cmd", "goto"), "only cmd and goto lines can be traced"
    else:
        kind = "cmd"
    v, d, i = (int(x) for x in sys.argv[1:4])
    g = Game(os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game", "SHANSTR.DAT"))
    log = []
    hooked = set(g.hooks)

    def tr(uc, addr, size, ud):
        if addr in hooked:
            return
        log.append("%x %x %x %x %x %x %x %x %x" % (addr, uc.reg_read(UC_X86_REG_EAX), uc.reg_read(UC_X86_REG_ECX), uc.reg_read(UC_X86_REG_EDX),
                                                   uc.reg_read(UC_X86_REG_EBX), uc.reg_read(UC_X86_REG_ESI), uc.reg_read(UC_X86_REG_EDI),
                                                   uc.reg_read(UC_X86_REG_EBP), uc.reg_read(UC_X86_REG_ESP)))
    base = g.snapshot()
    for line in prefix:
        p = line.split()
        if p[0] == "reset":
            g.restore(base)
        elif p[0] == "cmd":
            g.command(int(p[1]), int(p[2]), int(p[3]))
        elif p[0] == "flag":
            a = 0xCD8AC + ((int(p[1]) - 1) >> 3)
            b = 1 << ((int(p[1]) - 1) & 7)
            g.w8(a, (g.r8(a) | b) if int(p[2]) else (g.r8(a) & ~b & 0xFF))
        elif p[0] == "move":
            g.call(0x1B470, eax=int(p[1]), edx=int(p[2]), ebx=int(p[3]))
        elif p[0] == "phase":
            g.call(0x21AF4, eax=int(p[1]), edx=int(p[2]))
        elif p[0] == "call":
            g.call(int(p[1]), eax=int(p[2]), edx=int(p[3]))
        elif p[0] == "goto":
            g.call(0x1EE10, eax=int(p[1]))
        elif p[0] == "attr":
            g.call(0x1A1B0 if int(p[3]) else 0x1A1D8, eax=int(p[1]), edx=int(p[2]))
        g.text()
    g.uc.hook_add(UC_HOOK_CODE, tr, begin=0x10000, end=0x75000)   # the transpiled (game-logic) range
    if kind == "goto":
        g.call(0x1EE10, eax=v)
    else:
        g.command(v, d, i)
    print("oracle text:", repr(g.text()), "instructions traced:", len(log))
    ops = os.path.join(ROOT, "port", "build", "one.txt")
    open(ops, "w").write("\n".join(prefix + ["%s %d %d %d" % (kind, v, d, i)]) + "\n")
    tf = os.path.join(ROOT, "port", "build", "trace_cpp.txt")
    env = dict(os.environ, ENGINE_TRACE_FILE=tf, ENGINE_TRACE_FROM=str(len(prefix)))
    p = subprocess.run([TOOL, os.path.join(ROOT, "game", "SHANNARA.EXE"), os.path.join(ROOT, "game"), ops], capture_output=True, text=True, env=env)
    print("C++ result:", p.stdout.strip()[:160])
    cpp = open(tf).read().split("\n")[:-1]
    print("C++ instructions traced:", len(cpp))
    for k, (a, b) in enumerate(zip(log, cpp)):
        if a != b:
            print("first divergence at instruction #%d" % k)
            for j in range(max(0, k - 3), min(k + 2, len(log), len(cpp))):
                print("  %s oracle: %s\n  %s C++   : %s" % ("->" if j == k else "  ", log[j], "->" if j == k else "  ", cpp[j]))
            fa = g._func_at(int(log[k].split()[0], 16))
            print("  in", fa)
            return 1
    print("no divergence in the common prefix" if len(log) == len(cpp) else "traces differ in length (%d vs %d)" % (len(log), len(cpp)))
    return 0


sys.exit(main())
