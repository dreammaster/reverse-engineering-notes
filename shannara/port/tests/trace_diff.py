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
    g.uc.hook_add(UC_HOOK_CODE, tr, begin=0x10000, end=0x75000)   # the transpiled (game-logic) range
    g.command(v, d, i)
    print("oracle text:", repr(g.text()), "instructions traced:", len(log))
    ops = os.path.join(ROOT, "port", "build", "one.txt")
    open(ops, "w").write("cmd %d %d %d\n" % (v, d, i))
    tf = os.path.join(ROOT, "port", "build", "trace_cpp.txt")
    env = dict(os.environ, ENGINE_TRACE_FILE=tf)
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
