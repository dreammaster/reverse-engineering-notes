"""
Instruction effects, control flow graphs and the interprocedural register analysis of the WL.EXE decompiler.

Registers are tracked as bytes (al ah bl bh cl ch dl dh), words (si di bp), the segment registers es/ds and the flags.
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "recomp"))
import recomp  # noqa: E402  (the instruction decoder)

REG8 = ["al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"]
REG16 = ["ax", "cx", "dx", "bx", "sp", "bp", "si", "di"]
SREG = ["es", "cs", "ss", "ds"]
PAIRS = {"ax": ("al", "ah"), "bx": ("bl", "bh"), "cx": ("cl", "ch"), "dx": ("dl", "dh")}
FLAGS = ["cf", "zf", "sf", "of", "pf", "af", "df"]
ALLFLAGS = frozenset(FLAGS)
ARITH_FLAGS = frozenset(["cf", "zf", "sf", "of", "pf", "af"])
BYTES = ["al", "ah", "bl", "bh", "cl", "ch", "dl", "dh"]
GENERAL = frozenset(BYTES + ["si", "di", "bp"])

CONDFLAGS = {
    "jo": {"of"}, "jno": {"of"}, "jb": {"cf"}, "jnb": {"cf"}, "jz": {"zf"}, "jnz": {"zf"}, "jbe": {"cf", "zf"},
    "ja": {"cf", "zf"}, "js": {"sf"}, "jns": {"sf"}, "jp": {"pf"}, "jnp": {"pf"}, "jl": {"sf", "of"},
    "jge": {"sf", "of"}, "jle": {"zf", "sf", "of"}, "jg": {"zf", "sf", "of"},
}


def regs_of(op):
    """byte/word register names named by a reg8/reg16/sreg operand"""
    if op.kind == "reg8":
        return [REG8[op.n]]
    if op.kind == "reg16":
        n = REG16[op.n]
        return list(PAIRS.get(n, (n,)))
    if op.kind == "sreg":
        return [SREG[op.n]]
    return []


def mem_regs(op):
    """registers a memory operand reads (base registers and the segment register); the high byte of bx is a weak use"""
    out = set()
    for b in op.base:
        if b == "bx":
            out.update({"bl", "bh^"})
        else:
            out.add(b)
    out.add(op.seg)
    return out


def weak(r):
    return r + "^"


def is_weak(r):
    return r.endswith("^")


class Effects:
    __slots__ = ("uses", "defs", "unsupported")

    def __init__(self):
        self.uses = set()
        self.defs = set()
        self.unsupported = None


def effects(ins):
    """registers and flags read and written by one instruction (calls, returns and jumps are handled by the analysis)"""
    e = Effects()
    op = ins.op
    a, b = ins.a, ins.b
    U, D = e.uses, e.defs

    def use(o):
        if o is None:
            return
        if o.kind == "mem":
            U.update(mem_regs(o))
        elif o.kind == "reg16" and REG16[o.n] in PAIRS:
            lo, hi = PAIRS[REG16[o.n]]     # a word read of ax/bx/cx/dx: the high byte is assumed to be zero unless defined
            U.add(lo)
            U.add(weak(hi))
        else:
            U.update(regs_of(o))

    def define(o):
        if o is None:
            return
        if o.kind == "mem":
            U.update(mem_regs(o))
        else:
            D.update(regs_of(o))

    if op == "mov":
        use(b)
        define(a)
    elif op in ("add", "sub", "and", "or", "xor", "adc", "sbb", "cmp", "test"):
        use(a)
        use(b)
        if op in ("adc", "sbb"):
            U.add("cf")
        if op not in ("cmp", "test"):
            define(a)
        D.update(ARITH_FLAGS)
    elif op in ("inc", "dec"):
        use(a)
        define(a)
        D.update(ARITH_FLAGS - {"cf"})
    elif op == "neg":
        use(a)
        define(a)
        D.update(ARITH_FLAGS)
    elif op == "not":
        use(a)
        define(a)
    elif op in ("shl", "shr", "sar", "rol", "ror", "rcl", "rcr"):
        use(a)
        use(b)
        define(a)
        if op in ("rcl", "rcr"):
            U.add("cf")
        D.update({"cf", "of"} if op in ("rol", "ror", "rcl", "rcr") else ARITH_FLAGS)
    elif op == "push":
        use(a)
    elif op == "pop":
        define(a)
    elif op == "xchg":
        use(a)
        use(b)
        define(a)
        define(b)
    elif op == "lea":
        U.update(mem_regs(b) - {b.seg})
        define(a)
    elif op in ("les", "lds"):
        use(b)
        define(a)
        D.add("es" if op == "les" else "ds")
    elif op == "cbw":
        U.add("al")
        D.add("ah")
    elif op == "cwd":
        U.add("ah")
        D.update({"dl", "dh"})
    elif op == "pushf":
        U.update(ALLFLAGS)
    elif op == "popf":
        D.update(ALLFLAGS)
    elif op == "sahf":
        U.add("ah")
        D.update(ARITH_FLAGS)
    elif op == "lahf":
        U.update(ARITH_FLAGS)
        D.add("ah")
    elif op in ("clc", "stc"):
        D.add("cf")
    elif op == "cmc":
        U.add("cf")
        D.add("cf")
    elif op in ("cld", "std"):
        D.add("df")
    elif op in ("cli", "sti", "nop"):
        pass
    elif op in ("mul", "imul"):
        use(a)
        U.add("al")
        D.update({"al", "ah"} | ({"dl", "dh"} if ins.width == 16 else set()) | {"cf", "of"})
        if ins.width == 16:
            U.add("ah")
    elif op in ("div", "idiv"):
        use(a)
        U.update({"al", "ah"})
        if ins.width == 16:
            U.update({"dl", "dh"})
        D.update({"al", "ah"} | ({"dl", "dh"} if ins.width == 16 else set()) | ARITH_FLAGS)
    elif op == "xlat":
        U.update({"al", "bl", "bh", ins.extra or "ds"})
        D.add("al")
    elif op == "in":
        D.update({"al", "ah"} if ins.width == 16 else {"al"})
        if ins.a is None:
            U.update({"dl", "dh"})
    elif op == "out":
        U.update({"al", "ah"} if ins.width == 16 else {"al"})
        if ins.a is None:
            U.update({"dl", "dh"})
    elif op in ("movs", "cmps", "stos", "lods", "scas"):
        src = ins.extra or "ds"
        U.update({"df"})
        if ins.rep:
            U.update({"cl", "ch"})
            D.update({"cl", "ch"})
        if op == "movs":
            U.update({"si", "di", src, "es"})
            D.update({"si", "di"})
        elif op == "stos":
            U.update({"di", "es", "al"} | ({"ah"} if ins.width == 16 else set()))
            D.add("di")
        elif op == "lods":
            U.update({"si", src})
            D.update({"si", "al"} | ({"ah"} if ins.width == 16 else set()))
        elif op == "cmps":
            U.update({"si", "di", src, "es"})
            D.update({"si", "di"} | ARITH_FLAGS)
        elif op == "scas":
            U.update({"di", "es", "al"} | ({"ah"} if ins.width == 16 else set()))
            D.update({"di"} | ARITH_FLAGS)
        if ins.rep in ("rep", "repne") and op in ("cmps", "scas"):
            U.add("zf")
    elif op == "int":
        e.unsupported = "int"
    elif op in ("call", "calli", "callf", "callfi", "jmp", "jmpi", "jmpf", "jmpfi", "retn", "retf", "iret", "hlt", "bad", "loop",
                "loope", "loopne", "jcxz") or op in CONDFLAGS:
        pass
    else:
        e.unsupported = op
    return e


# ---------------------------------------------------------------------------------------------------------------------
class Block:
    def __init__(self, start):
        self.start = start
        self.insns = []
        self.succ = []          # successor block starts (fallthrough / jump targets)
        self.term = None        # terminator kind: 'fall', 'jmp', 'jcc', 'ret', 'tail', 'indirect', 'halt'
        self.tail = None        # entry called as a tail call


class Function:
    def __init__(self, entry):
        self.entry = entry
        self.insns = {}
        self.blocks = {}
        self.order = []
        self.calls = {}         # call site address -> callee entry (direct calls only)
        self.tailcalls = {}     # instruction address -> callee entry
        self.declined = None    # reason the function cannot be decompiled
        self.uses_in = set()    # live registers at entry (the inputs)
        self.defs_all = set()   # every register/flag written on some path (including callees)
        self.out = set()        # registers/flags callers read after the call (the outputs)


class Program:
    def __init__(self, image, entries):
        self.dec = recomp.Decoder(image)
        self.cache = {}
        self.entries = set(entries)
        self.funcs = {}
        self.build_all()

    def insn(self, a):
        if a not in self.cache:
            self.cache[a] = self.dec.decode(a)
        return self.cache[a]

    # -- discovery ----------------------------------------------------------------------------------------------
    def build_all(self):
        todo = sorted(self.entries)
        while todo:
            e = todo.pop()
            if e in self.funcs:
                continue
            f = self.build(e)
            self.funcs[e] = f
            for c in f.calls.values():
                if c not in self.entries:
                    self.entries.add(c)
                    todo.append(c)
            for t in f.tailcalls.values():
                if t not in self.entries:
                    self.entries.add(t)
                    todo.append(t)

    def build(self, entry):
        f = Function(entry)
        leaders = {entry}
        work = [entry]
        while work:
            a = work.pop()
            while True:
                if a in f.insns:
                    break
                ins = self.insn(a)
                f.insns[a] = ins
                op = ins.op
                nxt = a + ins.size
                if op in ("retn", "retf", "iret", "hlt", "bad", "jmpi", "jmpf", "jmpfi"):
                    break
                if op == "call" and ins.target is not None:
                    f.calls[a] = ins.target
                if ins.target is not None and op != "call":
                    t = ins.target
                    if t in self.entries and t != entry:
                        f.tailcalls[a] = t
                    else:
                        leaders.add(t)
                        work.append(t)
                    if op == "jmp":
                        break
                if op in CONDFLAGS or op in ("loop", "loope", "loopne", "jcxz"):
                    leaders.add(nxt)
                a = nxt
                if a in self.entries and a != entry:
                    f.tailcalls[ins.addr + 0x10000] = a    # fall through into another function (key: address | 10000h)
                    break
        # split into basic blocks
        for a in sorted(f.insns):
            pass
        starts = sorted(l for l in leaders if l in f.insns)
        for s in starts:
            f.blocks[s] = Block(s)
        for s in starts:
            blk = f.blocks[s]
            a = s
            while True:
                ins = f.insns[a]
                blk.insns.append(ins)
                op = ins.op
                nxt = a + ins.size
                if op in ("retn", "retf", "iret"):
                    blk.term = "ret"
                    break
                if op in ("hlt", "bad"):
                    blk.term = "halt"
                    break
                if op in ("jmpi", "jmpf", "jmpfi"):
                    blk.term = "indirect"
                    break
                if op == "jmp":
                    t = ins.target
                    if a in f.tailcalls:
                        blk.term = "tail"
                        blk.tail = f.tailcalls[a]
                    else:
                        blk.term = "jmp"
                        blk.succ = [t]
                    break
                if op in CONDFLAGS or op in ("loop", "loope", "loopne", "jcxz"):
                    blk.term = "jcc"
                    t = ins.target
                    if a in f.tailcalls:
                        blk.term = "jcc-tail"
                        blk.tail = f.tailcalls[a]
                        blk.succ = [nxt]
                    else:
                        blk.succ = [nxt, t]
                    break
                if (a + 0x10000) in f.tailcalls:
                    blk.term = "tail"
                    blk.tail = f.tailcalls[a + 0x10000]
                    break
                a = nxt
                if a in f.blocks:
                    blk.term = "fall"
                    blk.succ = [a]
                    break
        f.order = sorted(f.blocks)
        return f
