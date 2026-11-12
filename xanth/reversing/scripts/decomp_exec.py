"""Symbolic evaluation of basic blocks (decomp.py)."""
import re

from decomp import (L, STRIP, REG16, REG8, SEGS, JCC, K, is_k, simp, cmp_expr, negate_cond, parse_operand, Unsupported, walk, halve)
from decomp_eval import CALLSIG, clean_target

UNDEF = ("undef",)


class State:
    def __init__(self):
        self.regs = {}          # ax bx cx dx es -> expr
        self.stack = []         # pushed words (expressions), top last
        self.flags = None       # ('cmp', a, b) | ('res', r) | ('cfnz', a)
        self.shadow = {}        # si / di: the value last assigned in straight-line code (they are C variables, this is what the string instructions work on)

    def copy(self):
        s = State()
        s.regs = dict(self.regs)
        s.stack = list(self.stack)
        s.flags = self.flags
        s.shadow = dict(self.shadow)
        return s


def has_mem(e):
    for x in walk(e):
        if x[0] in ("glob", "mem"):
            return True
    return False


def has_glob(e):
    for x in walk(e):
        if x[0] in ("glob", "mem"):
            return True
    return False


def reads(e, name):
    for x in walk(e):
        if x[0] in ("loc", "glob") and x[1] == name:
            return True
    return False


class Evaluator:
    def __init__(self, func, insns, labels, locs, tables, entry_insn):
        self.f = func
        self.insns = insns
        self.labels = labels
        self.locs = locs        # name -> (size, offset, type)
        self.tables = tables
        self.entry_insn = entry_insn
        self.ncall = 0
        self.skip = set()       # `add sp, N` instructions that belong to a call

    # ---- helpers
    def tmp(self, stmts, e):
        self.f.ntemp += 1
        t = ("tmp", self.f.ntemp)
        stmts.append(("set", t, e))
        return t

    def materialize(self, st, stmts, pred):
        """replace register / stack expressions for which pred(expr) is true by temporaries"""
        for r, e in list(st.regs.items()):
            if pred(e) and e[0] not in ("tmp", "k"):
                st.regs[r] = self.tmp(stmts, e)
        for i, e in enumerate(st.stack):
            if pred(e) and e[0] not in ("tmp", "k"):
                st.stack[i] = self.tmp(stmts, e)

    def reg_get16(self, st, r, blk):
        if r in ("si", "di"):
            return st.shadow.get(r, ("loc", r, 0, 2))
        if r in ("bp", "sp"):
            raise Unsupported("use of " + r)
        if r == "ds":
            return st.regs.get("ds", ("dseg",))
        if r == "cs":
            return ("cseg",)
        if r == "ss":
            return ("sseg",)
        e = st.regs.get(r)
        if e is None:
            return ("in", r, blk)
        if e[0] == "hi0":
            return ("and", e[1], K(0xFF))
        if e[0] == "lo8set":
            return e[2]       # a byte value used as a word (byte argument pushed with garbage in ah): only the low byte is meaningful
        if e[0] == "hi8set":
            raise Unsupported("16-bit read after 8-bit write " + r)
        return e

    def reg_get8(self, st, r, blk):
        base, hi = REG8[r]
        e = st.regs.get(base)
        if e is None:
            e = ("in", base, blk)
        if hi == 0:
            if e[0] == "hi0":
                return ("lo8", e[1])
            if e[0] == "lo8set":
                return e[2]
            if e[0] in ("zx8", "sx8"):
                return e[1]
            if e[0] == "k":
                return K(e[1] & 0xFF)
            return ("lo8", e)
        if e[0] in ("zx8", "hi0"):
            return K(0)
        if e[0] == "k":
            return K((e[1] >> 8) & 0xFF)
        if e[0] == "lo8set":
            if e[1] == UNDEF:
                raise Unsupported("read of ah")
            return ("hi8", e[1])
        return ("hi8", e)

    def reg_set8(self, st, r, e, blk):
        base, hi = REG8[r]
        cur = st.regs.get(base)
        e = simp(e)
        if hi == 0:
            if cur is not None and cur[0] == "hi0":
                st.regs[base] = ("zx8", e)
            elif cur is not None and cur[0] == "k" and e[0] == "k":
                st.regs[base] = K((cur[1] & 0xFF00) | (e[1] & 0xFF))
            elif cur is not None and cur[0] == "lo8set":
                st.regs[base] = ("lo8set", cur[1], e)
            else:
                st.regs[base] = ("lo8set", cur if cur is not None else UNDEF, e)
        else:
            if e[0] == "k" and e[1] == 0:
                if cur is not None and cur[0] == "lo8set":
                    st.regs[base] = ("zx8", cur[2])
                    return
                if cur is not None and cur[0] == "zx8":
                    return
                if cur is not None and cur[0] == "k":
                    st.regs[base] = K(cur[1] & 0xFF)
                    return
                st.regs[base] = ("hi0", cur if cur is not None else ("in", base, blk))
                return
            if cur is not None and cur[0] == "k" and e[0] == "k":
                st.regs[base] = K((cur[1] & 0xFF) | ((e[1] & 0xFF) << 8))
                return
            raise Unsupported("write to %s" % r)

    # ---- operand access
    def mem_expr(self, st, op, blk, size):
        """expression reading the memory operand"""
        nm = op.sym.lstrip("-") if op.sym else None
        if nm is not None and nm in self.locs and op.base in ([], ["bp"]) and op.seg in (None, "ss"):
            sz = self.locs[nm][0] or 2
            return ("loc", nm, op.disp, size or sz)
        if op.base == ["bp"] and op.sym is None:
            return ("loc", "bp%+d" % op.disp, 0, size or 2)
        if not op.base and nm is not None and op.seg in (None, "ds"):
            return ("glob", nm, op.disp, size or 2)
        seg = {"es": st.regs.get("es", ("in", "es", blk)), "ds": st.regs.get("ds", ("dseg",)), "ss": ("sseg",), "cs": ("cseg",), None: st.regs.get("ds", ("dseg",))}[op.seg]
        off = K(op.disp) if op.disp else None
        for b in op.base:
            v = self.reg_get16(st, b, blk)
            off = v if off is None else simp(("add", v, off))
        if nm is not None:
            sy = ("addr", nm)
            off = sy if off is None else simp(("add", off, sy))
        if off is None:
            off = K(0)
        return ("mem", seg, off, size or 2)

    def read(self, st, text, blk, size=None):
        op = parse_operand(text, self.locs)
        if op.kind == "reg":
            r = op.reg
            if r in REG8:
                return self.reg_get8(st, r, blk)
            return self.reg_get16(st, r, blk)
        if op.kind == "imm":
            return K(op.val)
        if op.kind == "offset":
            return ("addr", op.sym) if not op.disp else simp(("add", ("addr", op.sym), K(op.disp)))
        if op.kind == "segof":
            return ("segof", op.sym)
        return self.mem_expr(st, op, blk, op.size or size)

    def operand_size(self, text, other_text=None):
        op = parse_operand(text, self.locs)
        if op.kind == "reg":
            return 1 if op.reg in REG8 else 2
        if op.kind == "mem" and op.size:
            return op.size
        if other_text is not None:
            o2 = parse_operand(other_text, self.locs)
            if o2.kind == "reg":
                return 1 if o2.reg in REG8 else 2
        return None

    def store(self, st, stmts, text, val, blk, size=None):
        op = parse_operand(text, self.locs)
        val = simp(val)
        if op.kind == "reg":
            r = op.reg
            if r in REG8:
                self.reg_set8(st, r, val, blk)
            elif r in ("si", "di"):
                self.materialize(st, stmts, lambda e: reads(e, r))
                stmts.append(("set", ("loc", r, 0, 2), val))
                st.shadow[r] = val
            elif r in ("es", "ds"):
                st.regs[r] = val
            elif r == "ss":
                raise Unsupported("assignment to ss")
            else:
                st.regs[r] = val
            return
        lhs = self.mem_expr(st, op, blk, op.size or size)
        if lhs[0] in ("loc", "glob"):
            name = lhs[1]
            self.materialize(st, stmts, lambda e: reads(e, name) or (lhs[0] == "glob" and any(x[0] == "mem" for x in walk(e))))
        else:
            self.materialize(st, stmts, has_mem)
        stmts.append(("set", lhs, val))
        if st.flags is not None and st.flags[0] == "res" and st.flags[1] == val and lhs[0] in ("loc", "glob"):
            st.flags = ("res", lhs)

    # ---- block evaluation
    def is_prologue(self, i):
        return i - self.entry_insn < 8 and all(self.insns[j].mn in ("push", "mov", "sub") for j in range(self.entry_insn, i))

    def is_epilogue(self, i):
        j = i
        while j < len(self.insns):
            m = self.insns[j]
            if m.mn in ("retf", "retn", "ret"):
                return True
            if m.mn == "pop" or (m.mn == "mov" and m.ops[0].strip() == "sp"):
                j += 1
                continue
            return False
        return False

    def run(self, b, st_in):
        st = st_in.copy()
        stmts = []
        blk = b.id
        last = b.insns[-1]
        for i in b.insns:
            x = self.insns[i]
            try:
                self.step(st, stmts, x, i, blk, i == last, b)
            except Unsupported as ex:
                raise Unsupported("%s (%s)" % (ex, x.text))
        if b.term is None:
            b.term = ("goto", b.succ[0]) if len(b.succ) == 1 else ("end",)
        b.stmts = stmts
        return st

    def result(self, st, stmts, text, r, blk, size):
        self.store(st, stmts, text, r, blk, size)

    def carry(self, st):
        f = st.flags
        if f is None:
            raise Unsupported("carry flag from an unknown instruction")
        if f[0] == "cmp":
            if is_k(f[2]) and f[2][1] == 1:
                return cmp_expr("eq", f[1], K(0))
            return cmp_expr("ult", f[1], f[2])
        if f[0] == "cfnz":
            return cmp_expr("ne", f[1], K(0))
        raise Unsupported("carry after " + f[0])

    def jump_cond(self, st, mn):
        cc = JCC[mn]
        f = st.flags
        if f is None:
            raise Unsupported("conditional jump without flags")
        if f[0] == "cmp":
            a, bb = f[1], f[2]
            if cc == "ult" and is_k(bb) and bb[1] == 1:
                return cmp_expr("eq", a, K(0))
            if cc == "uge" and is_k(bb) and bb[1] == 1:
                return cmp_expr("ne", a, K(0))
            return cmp_expr(cc, a, bb)
        if f[0] == "res":
            r = f[1]
            if cc in ("eq", "ne", "lt", "ge", "le", "gt"):
                return cmp_expr(cc, r, K(0))
            if cc == "neg":
                return cmp_expr("lt", r, K(0))
            if cc == "nneg":
                return cmp_expr("ge", r, K(0))
            if cc == "ugt":
                return cmp_expr("ne", r, K(0))
            if cc == "ule":
                return cmp_expr("eq", r, K(0))
            raise Unsupported("flag %s after a result" % cc)
        if f[0] == "cfnz":
            if cc == "ult":
                return cmp_expr("ne", f[1], K(0))
            if cc == "uge":
                return cmp_expr("eq", f[1], K(0))
        raise Unsupported("condition %s" % cc)

    def step(self, st, stmts, x, i, blk, term, b):
        mn, ops = x.mn, x.ops
        if x.idx in self.skip and mn in ("movsw", "movsb", "adc"):
            return
        if mn == "mov":
            size = self.operand_size(ops[0], ops[1])
            t0, t1 = ops[0].strip(), ops[1].strip()
            if t0 == "sp" or t1 == "sp":
                st.stack = []
                return
            if t0 == "bp":
                return
            self.store(st, stmts, ops[0], self.read(st, ops[1], blk, size), blk, size)
        elif mn == "push":
            t = ops[0].strip()
            if t == "bp" or (t in ("si", "di") and self.is_prologue(i)):
                return
            st.stack.append(self.read(st, ops[0], blk, 2))
        elif mn == "pop":
            t = ops[0].strip()
            if t in ("bp", "si", "di") and self.is_epilogue(i):
                return
            if not st.stack:
                raise Unsupported("pop from an empty stack")
            self.store(st, stmts, ops[0], st.stack.pop(), blk, 2)
        elif mn in ("add", "sub", "and", "or", "xor", "adc", "sbb"):
            if ops[0].strip() == "sp":
                if mn == "add" and x.idx not in self.skip:
                    n = L.num(ops[1]) // 2
                    del st.stack[max(0, len(st.stack) - n):]
                return
            size = self.operand_size(ops[0], ops[1])
            same = ops[0].strip() == ops[1].strip()
            if same and mn in ("sub", "xor") and ops[0].strip() in REG8:
                self.store(st, stmts, ops[0], K(0), blk, size)
                st.flags = ("res", K(0))
                return
            a = self.read(st, ops[0], blk, size)
            bb = self.read(st, ops[1], blk, size)
            if mn in ("and", "or", "xor") and ops[0].strip() in REG8 and ops[0].strip().endswith("l") and parse_operand(ops[1], self.locs).kind == "imm":
                base = REG8[ops[0].strip()][0]
                cur = st.regs.get(base)
                if cur is not None and cur[0] not in ("lo8set", "hi0", "zx8", "sx8", "k"):
                    k = bb[1] & 0xFF
                    r16 = simp((mn, cur, K(0xFF00 | k if mn == "and" else k)))
                    st.regs[base] = r16
                    st.flags = ("res", ("lo8", r16))
                    return
            if mn in ("add", "and", "or"):
                r = simp((mn, a, bb))
                st.flags = ("res", r)
            elif mn in ("xor", "sub"):
                r = K(0) if same else simp((mn, a, bb))
                st.flags = ("cmp", a, bb) if mn == "sub" else ("res", r)
            elif mn == "sbb":
                cf = self.carry(st)
                r = ("neg", ("b2i", cf)) if same else simp(("sub", simp(("sub", a, bb)), ("b2i", cf)))
                st.flags = ("res", r)
            else:
                cf = self.carry(st)
                r = simp(("add", simp(("add", a, bb)), ("b2i", cf)))
                st.flags = ("res", r)
            self.store(st, stmts, ops[0], r, blk, size)
        elif mn == "cmp":
            size = self.operand_size(ops[0], ops[1])
            st.flags = ("cmp", self.read(st, ops[0], blk, size), self.read(st, ops[1], blk, size))
        elif mn == "test":
            size = self.operand_size(ops[0], ops[1])
            a = self.read(st, ops[0], blk, size)
            bb = self.read(st, ops[1], blk, size)
            st.flags = ("res", a if a == bb else simp(("and", a, bb)))
        elif mn in ("inc", "dec"):
            size = self.operand_size(ops[0])
            r = simp(("add" if mn == "inc" else "sub", self.read(st, ops[0], blk, size), K(1)))
            st.flags = ("res", r)
            self.store(st, stmts, ops[0], r, blk, size)
        elif mn == "neg":
            size = self.operand_size(ops[0])
            a = self.read(st, ops[0], blk, size)
            st.flags = ("cfnz", a)
            self.store(st, stmts, ops[0], simp(("neg", a)), blk, size)
        elif mn == "not":
            size = self.operand_size(ops[0])
            a = self.read(st, ops[0], blk, size)
            self.store(st, stmts, ops[0], simp(("not", a)), blk, size)
        elif mn == "shr" and ops[0].strip() == "cx" and ops[1].strip() == "1" and self.string_copy(st, stmts, x, blk):
            return
        elif mn in ("movsw", "movsb") and x.pfx == "rep" and x.idx not in self.skip:
            unit = 2 if mn == "movsw" else 1
            count = self.reg_get16(st, "cx", blk)
            es = st.regs.get("es", ("in", "es", blk))
            ds = st.regs.get("ds", ("dseg",))
            dst = self.ptr(es, self.reg_get16(st, "di", blk))
            src = self.ptr(ds, self.reg_get16(st, "si", blk))
            self.ncall += 1
            nbytes = simp(("mul", count, K(unit))) if unit != 1 else count
            stmts.append(("call", (blk, 1000 + self.ncall), "memcpy", [dst, src, nbytes]))
            st.regs["cx"] = K(0)
            st.shadow.pop("di", None)
            st.shadow.pop("si", None)
        elif mn == "scasb":
            self.scasb(st, x, blk)
        elif mn in ("shl", "sal", "shr", "sar"):
            size = self.operand_size(ops[0])
            a = self.read(st, ops[0], blk, size)
            n = self.read(st, ops[1], blk, 1) if len(ops) > 1 else K(1)
            r = simp(({"shl": "shl", "sal": "shl", "shr": "shr", "sar": "sar"}[mn], a, n))
            st.flags = ("res", r)
            self.store(st, stmts, ops[0], r, blk, size)
        elif mn == "lea":
            op = parse_operand(ops[1], self.locs)
            e = self.mem_expr(st, op, blk, None)
            e = ("addrof", e) if e[0] in ("loc", "glob") else (e[2] if e[0] == "mem" else e)
            self.store(st, stmts, ops[0], e, blk, 2)
        elif mn == "xchg" and ops[0].strip() in ("ax", "bx", "cx", "dx") and ops[1].strip() in ("ax", "bx", "cx", "dx"):
            ra, rb = ops[0].strip(), ops[1].strip()
            va, vb = st.regs.get(ra), st.regs.get(rb)
            for r, v2 in ((ra, vb), (rb, va)):
                if v2 is None:
                    st.regs.pop(r, None)         # swapping with a register that holds nothing useful: the other one holds nothing either
                else:
                    st.regs[r] = v2
        elif mn == "xchg":
            a = self.read(st, ops[0], blk, 2)
            bb = self.read(st, ops[1], blk, 2)
            if any(reads(v, r) for v in (a, bb) for r in ("si", "di")) and {ops[0].strip(), ops[1].strip()} <= {"si", "di"}:
                t = self.tmp(stmts, a)
                self.store(st, stmts, ops[0], bb, blk, 2)
                self.store(st, stmts, ops[1], t, blk, 2)
            else:
                self.store(st, stmts, ops[0], bb, blk, 2)
                self.store(st, stmts, ops[1], a, blk, 2)
        elif mn == "cwd":
            st.regs["dx"] = simp(("sx16", self.reg_get16(st, "ax", blk)))
        elif mn == "cbw":
            st.regs["ax"] = ("sx8", self.reg_get8(st, "al", blk))
        elif mn in ("les", "lds"):
            op = parse_operand(ops[1], self.locs)
            lo = self.mem_expr(st, op, blk, 2)
            op2 = parse_operand(ops[1], self.locs)
            op2.disp += 2
            hi = self.mem_expr(st, op2, blk, 2)
            self.store(st, stmts, ops[0], lo, blk, 2)
            st.regs["es" if mn == "les" else "ds"] = hi
        elif mn in ("div", "idiv", "mul", "imul") and len(ops) == 1:
            size = self.operand_size(ops[0]) or 2
            den = self.read(st, ops[0], blk, size)
            if mn in ("mul", "imul"):
                if size == 1:
                    st.regs["ax"] = simp(("mul", self.reg_get8(st, "al", blk), den))
                else:
                    a = self.reg_get16(st, "ax", blk)
                    st.regs["dx"] = ("mulhi", a, den)
                    st.regs["ax"] = simp(("mul", a, den))
                st.flags = None
            else:
                if size != 2:
                    raise Unsupported("byte division")
                ax = self.reg_get16(st, "ax", blk)
                dx = st.regs.get("dx")
                signed = mn == "idiv"
                if signed and not (dx is not None and dx[0] == "sx16" and dx[1] == ax):
                    raise Unsupported("32-bit signed division")
                if not signed and dx != K(0):
                    raise Unsupported("32-bit unsigned division")
                st.regs["ax"] = ("sdiv" if signed else "udiv", ax, den)
                st.regs["dx"] = ("smod" if signed else "umod", ax, den)
                st.flags = None
        elif mn in ("cmc", "cld", "std", "cli", "sti", "nop", "clc", "stc"):
            return
        elif mn == "call":
            self.call(st, stmts, x, blk)
        elif mn == "jmp":
            self.do_jmp(st, x, blk, b)
        elif mn in ("jo", "jno"):
            # signed-compare idiom `jl X / jo X`: overflow cannot happen for these operands, so the jump is never (jo) / always (jno) taken
            fall, tgt = b.succ
            keep, drop = (fall, tgt) if mn == "jo" else (tgt, fall)
            b.succ = [keep]
            b.term = ("goto", keep)
            if drop is not None and drop != keep and b.id in self.f.blocks[drop].pred:
                self.f.blocks[drop].pred.remove(b.id)
        elif mn in JCC:
            cond = self.jump_cond(st, mn)
            fall, tgt = b.succ
            b.term = ("if", cond, tgt, fall)
        elif mn in ("retf", "retn", "ret"):
            b.term = ("ret", st.regs.get("ax"), st.regs.get("dx"))
        else:
            raise Unsupported("instruction " + mn)

    def do_jmp(self, st, x, blk, b):
        op = STRIP.sub("", x.ops[0].strip())
        m = re.match(r"^cs:(\w+)\[bx\]$", op)
        if m:
            idx = self.reg_get16(st, "bx", blk)
            if idx[0] == "shl" and idx[2] == K(1):
                idx = idx[1]
            elif idx[0] == "mul" and idx[2] == K(2):
                idx = idx[1]
            elif idx[0] == "add" and idx[1] == idx[2]:
                idx = idx[1]
            elif halve(idx) is not None:
                idx = halve(idx)
            else:
                raise Unsupported("jump table index")
            b.term = ("jtable", m.group(1), idx)
        elif re.match(r"^\$\+[23]$", op):
            pass
        elif len(b.succ) == 1:
            b.term = ("goto", b.succ[0])
        else:
            b.term = ("tail", op, st.regs.get("ax"), st.regs.get("dx"), list(st.stack))

    def call(self, st, stmts, x, blk):
        op = x.ops[0].strip()
        tgt = clean_target(op)
        self.ncall += 1
        cid = (blk, self.ncall)
        indirect = tgt in ("ax", "bx", "cx", "dx", "si", "di") or tgt.startswith(("dword ptr", "word ptr", "es:", "cs:", "ds:", "ss:", "[")) or ("[" in tgt)
        nxt = self.insns[self.insns.index(x) + 1] if False else None
        idx = x.idx
        nx = self.insns[idx + 1] if idx + 1 < len(self.insns) else None
        nwords = None
        nx2 = self.insns[idx + 2] if idx + 2 < len(self.insns) else None
        if nx is not None and nx.mn == "add" and nx.ops[0].strip() == "sp" and L.is_num(nx.ops[1]):
            nwords = L.num(nx.ops[1]) // 2
            self.skip.add(nx.idx)
        elif nx is not None and nx2 is not None and nx.mn == "mov" and L.is_num(nx.ops[1]) and nx2.mn == "add" and nx2.ops[0].strip() == "sp" and nx2.ops[1].strip() == nx.ops[0].strip():
            nwords = L.num(nx.ops[1]) // 2        # `mov ax, 6 / add sp, ax`
            self.skip.add(nx2.idx)
        if nwords is None:
            # no cleanup after the call: either `mov sp, bp` follows (every pending word is an argument) or the callee's usual argument count applies
            if nx is not None and nx.mn == "mov" and nx.ops[0].strip() == "sp":
                nwords = len(st.stack)
            elif indirect:
                nwords = 0
            else:
                fit = sorted(n for n in (CALLSIG.get(tgt) or ()) if n <= len(st.stack))
                if len(fit) == 1 or (fit and len(CALLSIG.get(tgt)) == len(fit)):
                    nwords = fit[-1]
                else:
                    nwords = len(st.stack)
        if nwords > len(st.stack):
            raise Unsupported("call %s needs %d words, %d pending" % (tgt, nwords, len(st.stack)))
        args = []
        for _ in range(nwords):
            args.append(st.stack.pop())
        if indirect:
            tgt = ("indirect", self.read(st, op.replace("far ptr", "").replace("dword ptr", "dword ptr"), blk, 4) if not tgt.startswith("bx") else ("reg", tgt))
        # a call changes memory and clobbers every register (the result comes back in ax / dx)
        st.regs.clear()
        self.materialize(st, stmts, has_glob)
        stmts.append(("call", cid, tgt, args))
        st.regs["ax"] = ("res", cid, "ax")
        st.regs["dx"] = ("res", cid, "dx")
        st.flags = None


    # ---- inline string functions (strlen / strcpy / strcat of the compiler's runtime)
    def ptr(self, seg, off):
        """a far pointer from its two halves"""
        if seg[0] == "res" and off[0] == "res" and seg[1] == off[1] and off[2] == "ax" and seg[2] == "dx":
            return ("res", off[1], "ax")
        if seg[0] == "loc" and off[0] == "loc" and seg[1] == off[1] and seg[2] == off[2] + 2:
            return ("loc", off[1], off[2], 4)
        if off[0] == "addr" and seg[0] in ("dseg", "segof"):
            return off
        return ("fptr", seg, off)

    def scasb(self, st, x, blk):
        if x.pfx not in ("repne", "repnz"):
            raise Unsupported("scasb without repne")
        cx = st.regs.get("cx")
        al = self.reg_get8(st, "al", blk)
        if cx != K(0xFFFF) or al != K(0):
            raise Unsupported("scasb that is not a strlen")
        es = st.regs.get("es", ("in", "es", blk))
        di = self.reg_get16(st, "di", blk)
        p = self.ptr(es, di)
        st.regs["cx"] = ("scasb_cx", p)
        st.shadow["di"] = simp(("add", di, simp(("add", ("strlen", p), K(1)))))
        st.flags = None

    def string_copy(self, st, stmts, x, blk):
        """shr cx,1 / rep movsw / adc cx,cx / rep movsb: a block copy of cx bytes from ds:si to es:di"""
        idx = x.idx
        n = self.insns[idx + 1:idx + 4]
        if len(n) < 3 or not (n[0].mn == "movsw" and n[0].pfx == "rep" and n[1].mn == "adc" and n[2].mn == "movsb" and n[2].pfx == "rep"):
            return False
        count = self.reg_get16(st, "cx", blk)
        es = st.regs.get("es", ("in", "es", blk))
        ds = st.regs.get("ds", ("dseg",))
        dst = self.ptr(es, self.reg_get16(st, "di", blk))
        src = self.ptr(ds, self.reg_get16(st, "si", blk))
        self.ncall += 1
        cid = (blk, 1000 + self.ncall)
        if dst[0] == "fptr" and dst[2][0] == "add" and dst[2][2][0] == "strlen":      # destination = end of the destination string: strcat
            base = self.ptr(dst[1], dst[2][1])
            if dst[2][2][1] == base or True:
                stmts.append(("call", cid, "strcat", [base, src]))
        elif count == simp(("add", ("strlen", src), K(1))):
            stmts.append(("call", cid, "strcpy", [dst, src]))
        else:
            stmts.append(("call", cid, "memcpy", [dst, src, count]))
        self.skip.update(i.idx for i in n)
        st.regs["cx"] = K(0)
        st.shadow.pop("di", None)
        st.shadow.pop("si", None)
        return True
