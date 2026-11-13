"""Structuring and C++ rendering for decomp.py."""
import re

from decomp import K, is_k, simp, negate_cond, walk, Unsupported
from decomp_gen import exprs_of, count_uses

BINOP = {"add": "+", "sub": "-", "and": "&", "or": "|", "xor": "^", "shl": "<<", "shr": ">>", "sar": ">>", "mul": "*"}
CMPOP = {"eq": "==", "ne": "!=", "lt": "<", "le": "<=", "gt": ">", "ge": ">=", "ult": "<", "ule": "<=", "ugt": ">", "uge": ">="}
SIGNED = ("lt", "le", "gt", "ge")
UNSIGNED = ("ult", "ule", "ugt", "uge")


class Names:
    """naming hooks (overridden by gen_logic.py); the defaults print the assembler names"""
    def glob(self, name, disp, size):
        return name if not disp else "%s_%+d" % (name, disp)

    def loc(self, func, name, disp, size):
        return name if not disp else "%s_%+d" % (name, disp)

    def func(self, name):
        if name.startswith("j_") and name != "j_":
            name = name[2:]
        return re.sub(r"[^A-Za-z0-9_]", "_", name.rstrip("?")) if name else name

    def ident(self, name):
        return re.sub(r"[^A-Za-z0-9_]", "_", name)

    def call(self, tgt, args):
        return "%s(%s)" % (self.func(tgt), ", ".join(args))

    def header(self, name, params):
        return "int %s(%s)" % (self.func(name), ", ".join("int " + p for p in params))

    def prolog(self):
        return []

    def ptr_sources(self):
        return []

    def dsoff(self, sym):
        return sym

    def min_params(self, name):
        return 0

    def returns_far(self, name):
        return False

    def const_cmp(self, lhs, value):
        return None

    def const_arg(self, fn, idx, value):
        return None

    def addr(self, sym):
        return "&" + sym

    def kstring(self, off):
        return "DSEG(0x%X)" % off

    def string(self, sym):
        t = self.ctx.strings.get(sym) if hasattr(self, "ctx") else None
        return '"%s"' % t if t is not None else '"?%s"' % sym

    def msg(self, block, index):
        return "MSG(%d, %d)" % (block, index)


def hexk(v):
    if v < 0:
        return "-%d" % -v
    return str(v) if v < 10 else "0x%X" % v


def is_atom(s):
    return re.match(r"^[\w.\[\]]+$", s) is not None or (s.startswith("(") and s.endswith(")") and balanced(s[1:-1]))


def balanced(s):
    d = 0
    for ch in s:
        if ch == "(":
            d += 1
        elif ch == ")":
            d -= 1
            if d < 0:
                return False
    return d == 0


class Renderer:
    def __init__(self, f, names, inline, temps):
        self.f, self.names, self.inline, self.temps = f, names, inline, temps
        self.calls = {}      # cid -> call statement

    def par(self, s):
        return s if is_atom(s) else "(" + s + ")"

    def call_text(self, cid):
        tgt, args = self.calls[cid]
        return self.call_expr(tgt, args)

    def call_expr(self, tgt, args):
        if isinstance(tgt, tuple):
            return "callIndirect(%s)" % ", ".join([self.R(tgt[1])] + [self.R(a) for a in args])
        out = []
        for i, a in enumerate(args):
            c = None
            if is_k(a):
                c = self.names.const_arg(tgt, i, a[1])
            out.append(c if c is not None else self.R(a))
        return self.names.call(tgt, out)

    def R(self, e, top=True):
        op = e[0]
        if op == "k":
            return hexk(e[1])
        if op == "loc":
            return self.names.loc(self.f.name, e[1], e[2], e[3])
        if op == "glob":
            return self.names.glob(e[1], e[2], e[3])
        if op == "tmp":
            return "t%d" % e[1]
        if op == "phi":
            return "p%d_%s" % (e[2], e[1])
        if op == "in":
            return "p%d_%s" % (e[2], e[1])
        if op == "res":
            cid = e[1]
            if e[2] == "dx":
                return "hi(%s)" % self.R(("res", cid, "ax"))
            if cid in self.inline:
                return self.call_text(cid)
            return "r%d_%d" % cid
        if op in BINOP:
            a, b = e[1], e[2]
            if op in ("shl", "shr", "sar") and False:
                pass
            return "%s %s %s" % (self.par(self.R(a, False)), BINOP[op], self.par(self.R(b, False))) if not top else "%s %s %s" % (self.par(self.R(a, False)), BINOP[op], self.par(self.R(b, False)))
        if op in CMPOP:
            a, b = e[1], e[2]
            if is_k(b):
                cs = self.names.const_cmp(a, b[1])
                if cs is not None:
                    return "%s %s %s" % (self.par(self.R(a, False)), CMPOP[op], cs)
            ra, rb = self.par(self.R(a, False)), self.par(self.R(b, False))
            if op in SIGNED:
                if not is_k(a):
                    ra = "(int16)" + ra
            elif op in UNSIGNED:
                if a[0] in BINOP:
                    ra = "(uint16)" + ra
                if b[0] in BINOP:
                    rb = "(uint16)" + rb
            return "%s %s %s" % (ra, CMPOP[op], rb)
        if op in ("land", "lor", "lnot"):
            return self.cond(e)
        if op == "b2i":
            return "(%s ? 1 : 0)" % self.R(e[1])
        if op == "sel":
            c, a, b = e[1], e[2], e[3]
            if c[0] == "lnot" or (c[0] == "eq" and is_k(c[2]) and c[2][1] == 0 and self.boolean(c[1])):
                c, a, b = negate_cond(c), b, a
            ra, rb = self.R(a), self.R(b)
            isp = lambda t: t.startswith(("ES(", '"', "g.print_obj", "g.ResourceText", "DSEG"))
            if isp(ra) and b == K(0):
                rb = "nullptr"
            elif isp(rb) and a == K(0):
                ra = "nullptr"

            return "(%s ? %s : %s)" % (self.cond(c), ra, rb)
        if op == "neg":
            if e[1][0] == "b2i":
                return "-(%s)" % self.R(e[1][1])
            return "-" + self.par(self.R(e[1], False))
        if op == "not":
            return "~" + self.par(self.R(e[1], False))
        if op == "zx8":
            return self.R(e[1], False)
        if op == "hi0":
            return "(%s & 0xFF)" % self.R(e[1], False)
        if op == "sx8":
            return "(int8)" + self.par(self.R(e[1], False))
        if op == "sx16":
            return "sext(%s)" % self.R(e[1])
        if op == "lo8":
            return "(%s & 0xFF)" % self.par(self.R(e[1], False))
        if op == "hi8":
            return "(%s >> 8)" % self.par(self.R(e[1], False))
        if op == "strlen":
            return "strlen(%s)" % self.R(e[1])
        if op == "addr":
            if getattr(self, "offmode", False):
                return self.names.dsoff(e[1])
            return self.names.addr(e[1])
        if op == "segof":
            return "SEG(%s)" % e[1]
        if op == "addrof":
            if e[1][0] == "glob" and e[1][1] in getattr(self.names, "overlay_syms", ()):
                return "0x" + e[1][1][4:]           # an overlay table of the original: passed as its address (overlays.cpp)
            return "(intptr_t)&" + self.R(e[1], False)
        if op == "dseg":
            return "DS"
        if op == "mem":
            sg, off = e[1], e[2]
            if sg[0] == "glob" and sg[1].startswith("seg_") and e[3] == 1 and off[0] == "add" and off[2] == K(0x66) and off[1][0] == "mul" and off[1][2] == K(6):
                return "objectType(%s)" % self.R(off[1][1])
            self.offmode = True
            try:
                return "mem%d(%s, %s)" % (e[3] * 8, self.R(e[1], False), self.R(e[2], False))
            finally:
                self.offmode = False
        if op == "msg":
            return self.names.msg(e[1], e[2])
        if op == "str":
            return self.names.string(e[1])
        if op == "fptr":
            if e[1] == ("dseg",):
                return self.ptr(e[2])
            return "FAR(%s, %s)" % (self.R(e[1]), self.R(e[2]))
        if op == "scasb_cx":
            return "(~strlen(%s) - 1)" % self.R(e[1])
        if op == "words":
            return "TextRef::fromWords(%s, %s)" % (self.R(e[1]), self.R(e[2]))
        if op in ("sdiv", "smod"):
            return "(int16)%s %s (int16)%s" % (self.par(self.R(e[1], False)), "/" if op == "sdiv" else "%", self.par(self.R(e[2], False)))
        if op in ("udiv", "umod"):
            return "(uint16)%s %s (uint16)%s" % (self.par(self.R(e[1], False)), "/" if op == "udiv" else "%", self.par(self.R(e[2], False)))
        if op == "lo8set":
            if e[1] == ("undef",):
                return self.R(e[2])
            return "((%s & 0xFF00) | (%s & 0xFF))" % (self.R(e[1], False), self.R(e[2], False))
        if op == "memptr":
            self.offmode = True
            try:
                return "g.exeText(%s, %s)" % (self.R(e[1]), self.R(e[2]))
            finally:
                self.offmode = False
        if op == "text":
            return "TextRef(%s)" % self.R(e[1])
        if op == "textvar":
            return self.R(e[1])
        if op == "null":
            return "nullptr"
        if op == "kstr":
            return self.names.kstring(e[1])
        if op == "far":
            if e[2] == ("dseg",):
                return self.ptr(e[1])
            return "FAR(%s, %s)" % (self.R(e[1]), self.R(e[2]))
        raise Unsupported("render " + op)

    def ptr(self, e):
        """an expression used as an offset into the data segment: constants are strings there"""
        if e[0] == "null":
            return "nullptr"
        if e[0] == "k":
            return self.names.kstring(e[1])
        if e[0] == "sel":
            c, a, b = e[1], e[2], e[3]
            return "(%s ? %s : %s)" % (self.cond(c), self.ptr(a), self.ptr(b))
        return self.R(e)

    PURE = re.compile(r"^(object_has_|object_get_|object_parent_is|event_pending|print_obj|_strlwr|strlen|number_to_words|object_is_|lock_combination)")

    def boolean(self, a):
        return a[0] in ("res", "lnot", "land", "lor") or a[0] in CMPOP

    def cond(self, c):
        """condition text: comparisons with zero of a boolean value become a plain truth test"""
        op = c[0]
        if op in ("land", "lor"):
            def side(x):
                t = self.cond(x)
                return "(" + t + ")" if x[0] in ("land", "lor") and x[0] != op else t
            return "%s %s %s" % (side(c[1]), "&&" if op == "land" else "||", side(c[2]))
        if op == "lnot":
            inner = c[1]
            if inner[0] in CMPOP or inner[0] in ("land", "lor"):
                return self.cond(negate_cond(inner))
            return "!" + self.par(self.R(inner, False))
        if op in ("eq", "ne") and is_k(c[2]) and c[2][1] == 0 and self.boolean(c[1]):
            a = c[1]
            s = self.cond(a) if a[0] in ("land", "lor", "lnot") or a[0] in CMPOP else self.R(a, False)
            return ("!" + self.par(s)) if op == "eq" else s
        return self.R(c)
