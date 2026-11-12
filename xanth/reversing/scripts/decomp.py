"""
Structured decompiler for the Xanth game-logic functions (object handlers, exit/event/conversation callbacks, verb handlers and the engine's own logic).

It reads xanth.asm through the lifter's loader (lift_handlers.load_asm), splits a function into basic blocks, evaluates every block symbolically (registers
and the argument stack hold expression trees, so `push x / push y / call f / add sp,4` becomes `f(y, x)` and `sub ax,19h / jz` becomes `x == 0x19`) and
re-creates if / else / switch / loop structure from the control-flow graph.  Whatever it cannot structure is left as `goto`, so the output is always
equivalent to the original; the oracle (engine/host) checks that.

    python decomp.py go_to_room?            # one function to stdout
    python decomp.py --stats                # which functions decompile
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.argv, _argv = [sys.argv[0]], sys.argv
import lift_handlers as L
sys.argv = _argv

STRIP = re.compile(r"^((short|near ptr|far ptr|near|far)\s+)+")
REG16 = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp")
REG8 = {"al": ("ax", 0), "ah": ("ax", 1), "bl": ("bx", 0), "bh": ("bx", 1), "cl": ("cx", 0), "ch": ("cx", 1), "dl": ("dx", 0), "dh": ("dx", 1)}
SEGS = ("es", "ds", "ss", "cs")
JCC = {"jz": "eq", "je": "eq", "jnz": "ne", "jne": "ne", "jl": "lt", "jnge": "lt", "jge": "ge", "jnl": "ge", "jle": "le", "jng": "le", "jg": "gt", "jnle": "gt",
       "jb": "ult", "jc": "ult", "jnae": "ult", "jae": "uge", "jnb": "uge", "jnc": "uge", "jbe": "ule", "jna": "ule", "ja": "ugt", "jnbe": "ugt",
       "js": "neg", "jns": "nneg", "jo": "ovf", "jno": "novf"}
NEGATE = {"eq": "ne", "ne": "eq", "lt": "ge", "ge": "lt", "le": "gt", "gt": "le", "ult": "uge", "uge": "ult", "ule": "ugt", "ugt": "ule", "neg": "nneg", "nneg": "neg",
          "ovf": "novf", "novf": "ovf"}


class Unsupported(Exception):
    pass


# --------------------------------------------------------------------------------------------------------------------------------- operands
class Op:
    __slots__ = ("kind", "reg", "val", "seg", "base", "disp", "sym", "size", "idx")

    def __init__(self, kind, **kw):
        self.kind = kind
        self.reg = self.val = self.seg = self.base = self.sym = self.size = self.idx = None
        self.disp = 0
        for k, v in kw.items():
            setattr(self, k, v)


def parse_int(t):
    t = t.strip()
    if t.startswith("'") and t.endswith("'") and len(t) == 3:
        return ord(t[1])
    neg = t.startswith("-")
    t = t.lstrip("-+")
    if t.lower().endswith("h"):
        v = int(t[:-1], 16)
    else:
        v = int(t)
    return -v if neg else v


def try_int(t):
    try:
        return parse_int(t)
    except ValueError:
        return None


def parse_operand(text, proc_locals):
    s = text.strip()
    size = None
    m = re.match(r"^(byte|word|dword)\s+ptr\s+(.*)$", s)
    if m:
        size = {"byte": 1, "word": 2, "dword": 4}[m.group(1)]
        s = m.group(2).strip()
    s = STRIP.sub("", s)
    if s in REG16 or s in REG8 or s in SEGS:
        return Op("reg", reg=s)
    m = re.match(r"^\(?offset\s+([A-Za-z_][\w?.$@]*)\s*([+-][0-9A-Fa-f]+h?)?\)?$", s)
    if m:
        o = Op("offset", sym=m.group(1))
        o.disp = parse_int(m.group(2)) if m.group(2) else 0
        return o
    m = re.match(r"^offset\s+(.*)$", s)
    if m:
        return Op("offset", sym=m.group(1).strip().strip("()"))
    m = re.match(r"^seg\s+(.*)$", s)
    if m:
        return Op("segof", sym=m.group(1).strip())
    v = try_int(s)
    if v is not None and not re.match(r"^[A-Za-z_]", s):
        return Op("imm", val=v)
    # memory: [seg:]sym[+disp][[reg...]] or [seg:][base+...+disp]
    seg = None
    m = re.match(r"^(es|ds|ss|cs):(.*)$", s)
    if m:
        seg, s = m.group(1), m.group(2)
    base, disp, sym, idx = [], 0, None, None
    # symbol with index: off_34E52[bx] / tbl[bx+si]
    m = re.match(r"^\(([A-Za-z_][\w?.$@]*)([+-][0-9A-Fa-f]+h?)?\)\[(.*)\]$", s)
    if m:        # (symbol+2)[bx]: an element of a table of dwords, high word
        d0 = parse_int(m.group(2)) if m.group(2) else 0
        regs = [r.strip() for r in m.group(3).split("+") if r.strip() in REG16]
        return Op("mem", seg=seg, base=regs, disp=d0, sym=m.group(1), size=size)
    m = re.match(r"^([A-Za-z_][\w?.$@]*)((?:[+-][0-9A-Fa-f]+h?)?)\[(.*)\]$", s)
    if m:
        sym = m.group(1)
        if m.group(2):
            disp += parse_int(m.group(2))
        for r in re.split(r"\+", m.group(3)):
            if r.strip() in REG16:
                base.append(r.strip())
        return Op("mem", seg=seg, base=base, disp=disp, sym=sym, size=size)
    m = re.match(r"^\[(.*)\]$", s)
    if m:
        body = m.group(1).replace("-", "+-")
        for part in body.split("+"):
            part = part.strip()
            if not part:
                continue
            if part in REG16:
                base.append(part)
            else:
                v = try_int(part)
                if v is not None and not re.match(r"^-?[A-Za-z_]", part):
                    disp += v
                else:
                    neg = part.startswith("-")
                    nm = part.lstrip("-")
                    if nm in proc_locals:
                        sym = ("-" if neg else "") + nm
                    else:
                        sym = nm
        return Op("mem", seg=seg, base=base, disp=disp, sym=sym, size=size)
    # direct global:  name, name+2, name.field
    m = re.match(r"^([A-Za-z_][\w?.$@]*)((?:[+-][0-9A-Fa-f]+h?)?)$", s)
    if m:
        d = parse_int(m.group(2)) if m.group(2) else 0
        return Op("mem", seg=seg, base=[], disp=d, sym=m.group(1), size=size)
    raise Unsupported("operand " + text)


# --------------------------------------------------------------------------------------------------------------------------------- expressions
def K(v):
    return ("k", v)


def is_k(e):
    return e[0] == "k"


def _linear(e, sign, terms, const):
    if e[0] == "add":
        const = _linear(e[1], sign, terms, const)
        return _linear(e[2], sign, terms, const)
    if e[0] == "sub":
        const = _linear(e[1], sign, terms, const)
        return _linear(e[2], -sign, terms, const)
    if e[0] == "k":
        return const + sign * e[1]
    if e[0] == "mul" and e[2][0] == "k" and e[1][0] != "k":
        return _linear(e[1], sign * e[2][1], terms, const)
    if e[0] == "neg" and e[1][0] != "b2i":
        return _linear(e[1], -sign, terms, const)
    for i, (c, t) in enumerate(terms):
        if t == e:
            terms[i] = (c + sign, t)
            return const
    terms.append((sign, e))
    return const


def _rebuild(terms, const):
    acc = None
    for c, t in terms:
        if c == 0:
            continue
        if abs(c) != 1:
            t = ("mul", t, K(abs(c)))
        if acc is None:
            acc = t if c > 0 else ("neg", t)
        else:
            acc = ("add" if c > 0 else "sub", acc, t)
    if acc is None:
        return K(const)
    if const > 0:
        acc = ("add", acc, K(const))
    elif const < 0:
        acc = ("sub", acc, K(-const))
    return acc


def halve(e):
    """e / 2 for an expression that is known to be even (a word index turned into a byte offset), or None"""
    terms = []
    const = _linear(e, 1, terms, 0)
    if const % 2 or any(c % 2 for c, _ in terms):
        return None
    return _rebuild([(c // 2, t) for c, t in terms], const // 2)


def simp(e):
    """light simplification of an expression tree"""
    op = e[0]
    if op in ("add", "sub") and not (e[1][0] == "k" and e[2][0] == "k"):
        terms = []
        const = _linear(e, 1, terms, 0)
        r = _rebuild(terms, const)
        if r[0] == "add" and r[1][0] == "neg" and r[1][1][0] == "b2i" and r[2][0] == "k":      # -(c) + k  ==  c ? k - 1 : k
            return ("sel", r[1][1][1], K(r[2][1] - 1), r[2])
        return r
    if op in ("add", "sub", "and", "or", "xor", "shl", "shr", "sar", "mul"):
        a, b = e[1], e[2]
        if is_k(a) and is_k(b):
            x, y = a[1], b[1]
            r = {"add": x + y, "sub": x - y, "and": x & y, "or": x | y, "xor": x ^ y, "shl": x << y, "shr": (x & 0xFFFF) >> y, "sar": x >> y, "mul": x * y}[op]
            return K(r if r >= 0 else r)
        if op == "add":
            if a == b:
                return simp(("mul", a, K(2)))
            if a[0] == "mul" and is_k(a[2]) and a[1] == b:
                return simp(("mul", b, K(a[2][1] + 1)))
            if b[0] == "mul" and is_k(b[2]) and b[1] == a:
                return simp(("mul", a, K(b[2][1] + 1)))
            if a[0] == "mul" and b[0] == "mul" and is_k(a[2]) and is_k(b[2]) and a[1] == b[1]:
                return simp(("mul", a[1], K(a[2][1] + b[2][1])))
            if is_k(a) and not is_k(b):
                a, b = b, a
            if b[0] == "add" and is_k(b[2]) and not is_k(a):
                return simp(("add", simp(("add", a, b[1])), b[2]))
            if is_k(b) and b[1] == 0:
                return a
            if a[0] == "add" and is_k(a[2]) and is_k(b):
                return simp(("add", a[1], K(a[2][1] + b[1])))
            if a[0] == "sub" and is_k(a[2]) and is_k(b):
                return simp(("sub", a[1], K(a[2][1] - b[1])))
            if is_k(b) and b[1] < 0:
                return ("sub", a, K(-b[1]))
            return ("add", a, b)
        if op == "sub":
            if is_k(b) and b[1] == 0:
                return a
            if a[0] == "add" and a[2] == b:                  # (x + y) - y
                return a[1]
            if a[0] == "add" and a[1] == b:
                return a[2]
            if a[0] == "sub" and is_k(a[2]) and is_k(b):
                return simp(("sub", a[1], K(a[2][1] + b[1])))
            if a[0] == "add" and is_k(a[2]) and is_k(b):
                return simp(("add", a[1], K(a[2][1] - b[1])))
            if a == b:
                return K(0)
            return ("sub", a, b)
        if op == "and":
            if is_k(b) and b[1] == 0xFFFF:
                return a
            if a[0] == "neg" and a[1][0] == "b2i" and is_k(b):          # -(cond) & k  ==  cond ? k : 0
                return ("sel", a[1][1], b, K(0))
            if a[0] == "zx8" and is_k(b) and b[1] == 0xFF:
                return a
            return ("and", a, b)
        if op == "or":
            if is_k(b) and b[1] == 0:
                return a
            if a == b:
                return a
            return ("or", a, b)
        if op == "xor":
            if a == b:
                return K(0)
            return ("xor", a, b)
        if op == "mul":
            if a[0] == "mul" and is_k(a[2]) and is_k(b):
                return simp(("mul", a[1], K(a[2][1] * b[1])))
            return ("mul", a, b)
        if op == "shl":
            if is_k(b) and b[1] < 8:
                return simp(("mul", a, K(1 << b[1])))
            return ("shl", a, b)
        return (op, a, b)
    if op == "sx16":
        if is_k(e[1]):
            return K(0xFFFF if e[1][1] & 0x8000 else 0)
        return e
    if op == "neg":
        a = e[1]
        if is_k(a):
            return K(-a[1])
        return e
    if op == "not":
        a = e[1]
        if is_k(a):
            return K(~a[1])
        if a[0] == "scasb_cx":
            return ("add", ("strlen", a[1]), K(1))
        return e
    if op == "sel":
        c, a, b = e[1], e[2], e[3]
        if a == b:
            return a
        return e
    return e


def cmp_expr(cc, a, b):
    """boolean expression for `a cc b` (signed/unsigned compare, or sign tests of a result when b is 0)"""
    a, b = simp(a), simp(b)
    if cc in ("eq", "ne"):
        if is_k(b) and a[0] == "sub" and is_k(a[2]):         # (x - k1) == k2   ->   x == k1 + k2
            return (cc, a[1], K((a[2][1] + b[1]) & 0xFFFF))
        if is_k(b) and a[0] == "add" and is_k(a[2]):         # (x + k1) == k2   ->   x == k2 - k1
            return (cc, a[1], K((b[1] - a[2][1]) & 0xFFFF))
        if is_k(b) and b[1] == 0 and a[0] == "b2i":          # b2i(c) == 0  -> !c
            return ("lnot", a[1]) if cc == "eq" else a[1]
        if is_k(b) and b[1] == 0 and a[0] == "sel" and is_k(a[2]) and is_k(a[3]):
            if a[3][1] == 0 and a[2][1] != 0:
                return ("lnot", a[1]) if cc == "eq" else a[1]
    if cc in ("neg", "nneg"):
        return ("lt" if cc == "neg" else "ge", a, b)
    return (cc, a, b)


def negate_cond(c):
    if c[0] == "lnot":
        return c[1]
    if c[0] == "land":
        return ("lor", negate_cond(c[1]), negate_cond(c[2]))
    if c[0] == "lor":
        return ("land", negate_cond(c[1]), negate_cond(c[2]))
    if c[0] in NEGATE and len(c) == 3:
        return (NEGATE[c[0]], c[1], c[2])
    if c[0] == "and" and False:
        return c
    return ("lnot", c)


def walk(e):
    """all sub-expressions"""
    yield e
    if isinstance(e, tuple):
        for x in e[1:]:
            if isinstance(x, tuple):
                yield from walk(x)
            elif isinstance(x, list):
                for y in x:
                    if isinstance(y, tuple):
                        yield from walk(y)
