#!/usr/bin/env python3
"""asm2cpp: translate an IDA .asm export of one of the Chapter 1 programs into C++ for Shared::Machine16.

The output keeps the original's register file, flags, stack and segment:offset addressing (see shared/Machine16.h), one C++ member
function per routine; labels become C++ labels; table-driven jumps become switches over the labels that occur as code addresses
in the data segment. The routines the port provides natively (tile operations, the palette mailbox) are not translated.

    python asm2cpp.py <progN> <ClassName> <outBase> [--adapter name=expr ...]

reads  yendor1/<progN>.asm, <progN>.idc and game/<PROGN>.EXE, writes <outBase>.h and <outBase>.cpp.
"""
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
YENDOR1 = os.path.normpath(os.path.join(HERE, "..", "..", "yendor1"))

REG16 = ["ax", "bx", "cx", "dx", "si", "di", "bp", "sp"]
REG8 = ["al", "ah", "bl", "bh", "cl", "ch", "dl", "dh"]
SREG = ["es", "ds", "ss", "cs"]

CONDS = {
    "jz": "r.zf", "je": "r.zf", "jnz": "!r.zf", "jne": "!r.zf",
    "jl": "(r.sf != r.of)", "jnge": "(r.sf != r.of)", "jge": "(r.sf == r.of)", "jnl": "(r.sf == r.of)",
    "jle": "(r.zf || r.sf != r.of)", "jng": "(r.zf || r.sf != r.of)", "jg": "(!r.zf && r.sf == r.of)", "jnle": "(!r.zf && r.sf == r.of)",
    "jb": "r.cf", "jc": "r.cf", "jnae": "r.cf", "jnb": "!r.cf", "jnc": "!r.cf", "jae": "!r.cf",
    "jbe": "(r.cf || r.zf)", "jna": "(r.cf || r.zf)", "ja": "(!r.cf && !r.zf)", "jnbe": "(!r.cf && !r.zf)",
    "js": "r.sf", "jns": "!r.sf", "jo": "r.of", "jno": "!r.of", "jp": "r.pf", "jpe": "r.pf", "jnp": "!r.pf", "jpo": "!r.pf",
}


class Err(Exception):
    pass


def num(tok):
    tok = tok.strip()
    if tok.lower().endswith("h"):
        return int(tok[:-1], 16)
    if tok.startswith("-"):
        return int(tok)
    if re.fullmatch(r"[0-9]+", tok):
        return int(tok)
    raise ValueError(tok)


def is_num(tok):
    try:
        num(tok)
        return True
    except ValueError:
        return False


class Op:
    def __init__(self, kind, **kw):
        self.kind = kind
        self.__dict__.update(kw)


class Proc:
    def __init__(self, name, seg, far):
        self.name = name
        self.seg = seg
        self.far = far
        self.items = []          # ("label", name) | ("ins", mnem, ops(list of str), lineno)
        self.vars = {}
        self.var_sizes = {}


class Translator:
    def __init__(self, prog, cls, adapters):
        self.prog = prog
        self.cls = cls
        self.adapters = adapters
        self.asm = open(os.path.join(YENDOR1, prog + ".asm"), encoding="latin-1").read().split("\n")
        self.idc = open(os.path.join(YENDOR1, prog + ".idc"), encoding="latin-1").read()
        exe = open(os.path.join(YENDOR1, "game", prog.upper() + ".EXE"), "rb").read()
        hdr = struct.unpack_from("<H", exe, 8)[0] * 16
        img = exe[hdr:]
        i = img.find(bytes([0x8E, 0xD8]), 0, 400)
        self.ds_seg = struct.unpack_from("<H", img, i - 2)[0]
        self.ds_base = self.ds_seg * 16
        self.ds_image = img[self.ds_base:]
        segs = [(int(a, 16), int(b, 16)) for a, b, _ in re.findall(r"add_segm_ex\(0X([0-9A-F]+),0X([0-9A-F]+),0X([0-9A-F]+)", self.idc)]
        self.segs = segs
        self.ds_linear = 0x10000 + self.ds_base
        self.parm_base = 0
        self.parm_sizes = {}
        for line in self.asm:
            m = re.match(r"^(parm_[0-9A-F]+)\s+(db|dw|dd)", line)
            if m:
                self.parm_sizes[m.group(1)] = {"db": 8, "dw": 16, "dd": 32}[m.group(2)]
        self.procs = []
        self.proc_by_name = {}
        self.labels = {}         # name -> proc
        self.seg_start = {}      # seg name -> linear start
        self.seg_org = {}
        self.data_seg = None
        self.parse()
        st = self.proc_by_name.get("start")
        seen80 = False
        for it in st.items if st else []:
            if it[0] != "ins":
                continue
            if it[1] == "mov" and it[2] == ["si", "80h"]:
                seen80 = True
            elif seen80 and it[1] == "mov" and it[2][0] == "di" and is_num(it[2][1]):
                self.parm_base = num(it[2][1])
                break

    # ------------------------------------------------------------------ parsing
    def parse(self):
        seg_names = []
        cur_seg = None
        cur = None
        pending_labels = []
        seg_class = {}
        self.chunks = {}
        chunk_owner = None
        last_proc = None
        resumed = False
        for ln, line in enumerate(self.asm):
            m = re.match(r"^(seg\d+)\s+segment\s+\w+\s+(\w+)\s+'(\w+)'", line)
            if m:
                cur_seg = m.group(1)
                seg_names.append(cur_seg)
                seg_class[cur_seg] = m.group(3)
                continue
            m = re.match(r"^\s+;org ([0-9A-Fa-f]+)h?", line)
            if m and cur_seg:
                self.seg_org[cur_seg] = int(m.group(1), 16)
                continue
            if re.match(r"^seg\d+\s+ends", line):
                cur_seg = None
                continue
            if cur_seg is None or seg_class.get(cur_seg) != "CODE":
                continue
            m = re.match(r"^; START OF FUNCTION CHUNK FOR (\w+)", line)
            if m:
                chunk_owner = m.group(1)
                cur = Proc("<chunk %s>" % chunk_owner, cur_seg, False)
                self.chunks.setdefault(chunk_owner, []).append(cur)
                continue
            if line.startswith("; END OF FUNCTION CHUNK"):
                cur = None
                chunk_owner = None
                continue
            m = re.match(r"^(\w+)\s+proc\s+(near|far)", line)
            if m:
                cur = Proc(m.group(1), cur_seg, m.group(2) == "far")
                self.procs.append(cur)
                resumed = False
                self.proc_by_name[cur.name] = cur
                self.labels[cur.name] = cur
                continue
            m = re.match(r"^(\w+)\s+endp", line)
            if m:
                if cur is not None and not cur.name.startswith("<chunk"):
                    last_proc = cur
                cur = None
                continue
            if cur is None and last_proc is not None and last_proc.seg == cur_seg and (re.match(r"^(\w+):", line) or re.match(r"^\s{12,}[a-z]", line)):
                # code IDA left outside any function (after a function it ended early: "endp ; sp-analysis failed") belongs to that function
                cur = last_proc
                resumed = True
            m = re.match(r"^(\w+):", line)
            if m and cur is not None:
                cur.items.append(("label", m.group(1)))
                self.labels[m.group(1)] = cur
                continue
            if cur is None:
                continue
            m = re.match(r"^\s*(var_\w+|arg_\w+)\s*=\s*\w+ ptr\s+(-?[0-9A-Fa-f]+h?)", line)
            if m:
                cur.vars[m.group(1)] = num(m.group(2)) if is_num(m.group(2)) else int(m.group(2), 16)
                cur.var_sizes[m.group(1)] = {"byte": 8, "word": 16, "dword": 32}[re.search(r"=\s*(\w+) ptr", line).group(1)]
                continue
            m = re.match(r"^\s{12,}(\w+)(?:\s+([^;]*?))?\s*(?:;.*)?$", line)
            if m and not line.lstrip().startswith(";"):
                mnem = m.group(1).lower()
                if mnem in ("assume", "align", "public", "extrn", "end", "segment", "ends"):
                    continue
                if resumed and mnem in ("db", "dw", "dd"):
                    sys.stderr.write("%s.asm:%d: undecoded bytes outside any function skipped\n" % (self.prog, ln + 1))
                    continue
                opsraw = (m.group(2) or "").strip()
                ops = self.split_ops(opsraw)
                cur.items.append(("ins", mnem, ops, ln + 1))
        # merge function chunks into their owners
        for owner, chunks in self.chunks.items():
            op = self.proc_by_name.get(owner)
            if op is None:
                raise Err("chunk of unknown function " + owner)
            for ch in chunks:
                op.items.extend(ch.items)
                op.vars.update(ch.vars)
                op.var_sizes.update(ch.var_sizes)
                for it in ch.items:
                    if it[0] == "label":
                        self.labels[it[1]] = op
        # segment starts from the idc, in order of appearance
        for name, (s, e) in zip(seg_names, self.segs):
            self.seg_start[name] = s
        self.seg_class = seg_class

    @staticmethod
    def split_ops(s):
        if not s:
            return []
        out, depth, cur = [], 0, ""
        for ch in s:
            if ch == "[":
                depth += 1
            if ch == "]":
                depth -= 1
            if ch == "," and depth == 0:
                out.append(cur.strip())
                cur = ""
            else:
                cur += ch
        out.append(cur.strip())
        return out

    # ------------------------------------------------------------------ addresses
    def label_addr(self, name):
        m = re.match(r"^\w+?_([0-9A-F]{4,6})(?:_\d+)?$", name)
        if not m:
            return None
        return int(m.group(1), 16)

    def data_offset(self, name):
        m = re.match(r"^parm_([0-9A-F]{2})$", name)
        if m:
            return self.parm_base + int(m.group(1), 16)
        a = self.label_addr(name)
        if a is None:
            raise Err("no address in " + name)
        off = a - self.ds_linear
        if not (0 <= off < 0x10000):
            raise Err("%s is not in the data segment" % name)
        return off

    def code_offset(self, name):
        proc = self.labels.get(name)
        if proc is None:
            raise Err("unknown code label " + name)
        a = self.label_addr(name)
        if a is None:
            return None
        return a - self.seg_start[proc.seg] + self.seg_org.get(proc.seg, 0)

    # ------------------------------------------------------------------ operands
    def parse_op(self, text, proc, size_hint=None):
        t = text.strip()
        size = None
        m = re.match(r"^(byte|word|dword) ptr (.*)$", t)
        if m:
            size = {"byte": 8, "word": 16, "dword": 32}[m.group(1)]
            t = m.group(2).strip()
        if t in REG16:
            return Op("reg", name=t, size=16)
        if t in REG8:
            return Op("reg", name=t, size=8)
        if t in SREG:
            return Op("sreg", name=t, size=16)
        while True:
            m = re.match(r"^(short|near ptr|far ptr)\s+(.*)$", t)
            if not m:
                break
            t = m.group(2).strip()
        m = re.match(r"^offset (\w+)(?:\+([0-9A-Fa-f]+h?))?$", t)
        if m:
            nm = m.group(1)
            extra = num(m.group(2)) if m.group(2) else 0
            if nm in self.labels:
                return Op("imm", value=(self.code_offset(nm) + extra) & 0xFFFF, size=16)
            return Op("imm", value=(self.data_offset(nm) + extra) & 0xFFFF, size=16)
        m = re.match(r"^seg (\w+)$", t)
        if m:
            nm = m.group(1)
            if self.seg_class.get(nm) == "CODE":
                return Op("imm", value=0x1000, size=16, text="Machine16::kSegData")
            if nm == self.data_seg_name():
                return Op("imm", value=0, size=16, text="Machine16::kSegData")
            return Op("imm", value=0, size=16, text="Machine16::kSegStack")
        if is_num(t) and not t.startswith("["):
            return Op("imm", value=num(t) & 0xFFFF, size=size)
        seg = None
        m = re.match(r"^(es|ds|cs|ss):(.*)$", t)
        if m:
            seg = m.group(1)
            t = m.group(2).strip()
        # label (code) as jump/call target
        if re.match(r"^(loc|sub|locret|shared|start|waitInput|\w+)", t) and t in self.labels and seg is None and size is None:
            return Op("code", name=t)
        if t in self.proc_by_name and seg is None:
            return Op("code", name=t)
        base_regs = []
        disp = 0
        dispnames = []
        if t.startswith("["):
            inner = t[1:t.index("]")]
            for sign, tok in re.findall(r"([+-]?)\s*([\w]+)", inner):
                tok = tok.strip()
                sgn = -1 if sign == "-" else 1
                if tok in ("bx", "si", "di", "bp"):
                    base_regs.append(tok)
                elif tok in proc.vars:
                    disp += sgn * proc.vars[tok]
                    if size is None:
                        size = proc.var_sizes.get(tok)
                elif is_num(tok):
                    disp += sgn * num(tok)
                else:
                    disp += sgn * self.data_offset(tok)
            rest = t[t.index("]") + 1:]
            if rest.startswith("+"):
                disp += num(rest[1:])
        else:
            m = re.match(r"^(\w+)(?:\+([0-9A-Fa-f]+h?))?$", t)
            if not m:
                raise Err("cannot parse operand %r" % text)
            tok = m.group(1)
            extra = num(m.group(2)) if m.group(2) else 0
            if is_num(tok):
                disp = num(tok) + extra
            else:
                disp = self.data_offset(tok) + extra
                if size is None:
                    p = tok.split("_")[0]
                    size = {"byte": 8, "word": 16, "dword": 32}.get(p)
                    if size is None and tok in self.parm_sizes:
                        size = self.parm_sizes[tok]
        if seg is None:
            seg = "ss" if "bp" in base_regs else "ds"
        return Op("mem", seg=seg, regs=base_regs, disp=disp & 0xFFFF, size=size or size_hint)

    def data_seg_name(self):
        if self.data_seg is None:
            for n, c in self.seg_class.items():
                if c != "CODE" and self.seg_start.get(n) is not None and abs(self.seg_start[n] - self.ds_linear) < 0x10:
                    self.data_seg = n
            if self.data_seg is None:
                self.data_seg = ""
        return self.data_seg

    # ------------------------------------------------------------------ C++ expression helpers
    def segx(self, seg):
        return {"es": "r.es", "ds": "r.ds", "ss": "r.ss", "cs": "Machine16::kSegData"}[seg]

    def addr(self, op):
        parts = ["r." + x for x in op.regs]
        if op.disp or not parts:
            parts.append("0x%X" % op.disp)
        return "(uint16_t)(%s)" % " + ".join(parts)

    def ctype(self, size):
        return "uint8_t" if size == 8 else "uint16_t"

    def rd(self, op, size=None):
        size = op.size or size
        if op.kind == "reg":
            return "r." + op.name
        if op.kind == "sreg":
            return "r.es" if op.name == "es" else ("r.ds" if op.name == "ds" else ("r.ss" if op.name == "ss" else "Machine16::kSegData"))
        if op.kind == "imm":
            return getattr(op, "text", None) or "0x%X" % op.value
        if op.kind == "mem":
            if size is None:
                raise Err("operand size unknown")
            if size == 8:
                return "m.b(%s, %s)" % (self.segx(op.seg), self.addr(op))
            return "m.rw(%s, %s)" % (self.segx(op.seg), self.addr(op))
        raise Err("cannot read " + op.kind)

    def wr(self, op, val, size=None):
        size = op.size or size
        if op.kind == "reg":
            return "r.%s = %s;" % (op.name, ("(uint8_t)(%s)" if op.size == 8 else "(uint16_t)(%s)") % val)
        if op.kind == "sreg":
            return "r.%s = (uint16_t)(%s);" % (op.name, val)
        if op.kind == "mem":
            if size is None:
                raise Err("operand size unknown")
            if size == 8:
                return "m.b(%s, %s) = (uint8_t)(%s);" % (self.segx(op.seg), self.addr(op), val)
            return "m.ww(%s, %s, (uint16_t)(%s));" % (self.segx(op.seg), self.addr(op), val)
        raise Err("cannot write " + op.kind)

    def opsize(self, a, b=None):
        if a.kind in ("reg", "sreg"):
            return a.size
        if a.size:
            return a.size
        if b is not None and b.kind in ("reg", "sreg"):
            return b.size
        if b is not None and b.size:
            return b.size
        raise Err("unknown size")

    # ------------------------------------------------------------------ instruction translation
    def jump_target(self, op, proc):
        name = op.name
        tp = self.labels.get(name)
        if tp is None:
            raise Err("unknown jump target " + name)
        return tp, name

    def goto(self, op, proc, cond=None):
        tp, name = self.jump_target(op, proc)
        if tp is proc:
            # a jump back: the platform gets a chance to stop a loop that never ends (Machine16::loopTick)
            stmt = ("{ m.loopTick(); goto %s; }" if name in self._seen else "goto %s;") % name
        else:
            self.entries.setdefault(tp.name, set()).add(name)
            if name == tp.name:
                stmt = "return %s();" % tp.name
            else:
                stmt = "return %s(0x%X);" % (tp.name, self.code_offset(name))
        return ("if (%s) %s" % (cond, stmt)) if cond else stmt

    def translate_proc(self, proc):
        out = ["%s:;" % proc.name]
        self._seen = set([proc.name])
        for idx, it in enumerate(proc.items):
            if it[0] == "label":
                out.append("%s:;" % it[1])
                self._seen.add(it[1])
                continue
            _, mnem, ops, ln = it
            self._prev_push_cs = idx > 0 and proc.items[idx - 1][0] == "ins" and proc.items[idx - 1][1] == "push" and proc.items[idx - 1][2] == ["cs"]
            try:
                nxt = None
                for k in range(idx + 1, len(proc.items)):
                    if proc.items[k][0] == "ins":
                        nxt = proc.items[k]
                        break
                lines = self.ins(proc, mnem, ops, nxt)
            except Err as e:
                lines = ['m.unsupported("%s:%d %s %s: %s");' % (self.prog, ln, mnem, ", ".join(ops).replace('"', "'"), str(e).replace('"', "'"))]
                sys.stderr.write("%s.asm:%d: %s %s: %s\n" % (self.prog, ln, mnem, ", ".join(ops), e))
            out.extend("    " + l for l in lines)
        return out

    def alu2(self, fn, proc, ops, store=True, logic=None):
        a = self.parse_op(ops[0], proc)
        b = self.parse_op(ops[1], proc, a.size)
        sz = self.opsize(a, b)
        t = self.ctype(sz)
        av = self.rd(a, sz)
        bv = self.rd(b, sz)
        if logic:
            expr = "alu::logic<%s>(r, (%s)(%s %s %s))" % (t, t, av, logic, bv)
        else:
            expr = "alu::%s<%s>(r, %s, (%s)(%s)%s)" % (fn[0], t, av, t, bv, fn[1])
        if not store:
            return ["(void)%s;" % expr]
        return [self.wr(a, expr, sz)]

    def ins(self, proc, mnem, ops, nxt):
        P = lambda i, h=None: self.parse_op(ops[i], proc, h)
        if mnem == "mov":
            a = P(0)
            b = P(1, a.size)
            sz = self.opsize(a, b)
            return [self.wr(a, self.rd(b, sz), sz)]
        if mnem == "lea":
            a = P(0)
            b = P(1)
            if b.kind != "mem":
                raise Err("lea of non-memory")
            return [self.wr(a, self.addr(b))]
        if mnem in ("add", "sub", "cmp", "and", "or", "xor", "test", "adc", "sbb"):
            if mnem == "add":
                return self.alu2(("add", ""), proc, ops)
            if mnem == "sub":
                return self.alu2(("sub", ""), proc, ops)
            if mnem == "cmp":
                return self.alu2(("sub", ""), proc, ops, store=False)
            if mnem == "adc":
                return self.alu2(("add", ", r.cf ? 1u : 0u"), proc, ops)
            if mnem == "sbb":
                return self.alu2(("sub", ", r.cf ? 1u : 0u"), proc, ops)
            if mnem == "and":
                return self.alu2(None, proc, ops, logic="&")
            if mnem == "or":
                return self.alu2(None, proc, ops, logic="|")
            if mnem == "xor":
                return self.alu2(None, proc, ops, logic="^")
            if mnem == "test":
                return self.alu2(None, proc, ops, store=False, logic="&")
        if mnem in ("inc", "dec", "neg", "not"):
            a = P(0)
            sz = self.opsize(a)
            t = self.ctype(sz)
            v = self.rd(a, sz)
            if mnem == "not":
                return [self.wr(a, "(%s)~%s" % (t, v), sz)]
            return [self.wr(a, "alu::%s<%s>(r, %s)" % (mnem, t, v), sz)]
        if mnem in ("shl", "sal", "shr", "sar", "rol", "ror"):
            a = P(0)
            b = P(1)
            sz = self.opsize(a)
            t = self.ctype(sz)
            fn = {"sal": "shl"}.get(mnem, mnem)
            return [self.wr(a, "alu::%s<%s>(r, %s, %s)" % (fn, t, self.rd(a, sz), self.rd(b, 8)), sz)]
        if mnem in ("mul", "imul", "div", "idiv"):
            a = P(0)
            sz = self.opsize(a)
            return ["alu::%s%d(r, %s);" % (mnem, sz, self.rd(a, sz))]
        if mnem == "push":
            a = P(0)
            return ["m.push((uint16_t)(%s));" % self.rd(a, 16)]
        if mnem == "pop":
            a = P(0)
            return [self.wr(a, "m.pop()", 16)]
        if mnem == "xchg":
            a = P(0)
            b = P(1, a.size)
            sz = self.opsize(a, b)
            t = self.ctype(sz)
            return ["{ %s _t = %s; %s %s }" % (t, self.rd(a, sz), self.wr(a, self.rd(b, sz), sz), self.wr(b, "_t", sz))]
        if mnem == "cbw":
            return ["r.ax = (uint16_t)(int16_t)(int8_t)r.al;"]
        if mnem == "cwd":
            return ["r.dx = (r.ax & 0x8000) ? 0xFFFF : 0;"]
        if mnem in ("cld", "std", "clc", "stc", "cmc", "cli", "sti", "nop", "hlt"):
            return {"cld": ["r.df = false;"], "std": ["r.df = true;"], "clc": ["r.cf = false;"], "stc": ["r.cf = true;"], "cmc": ["r.cf = !r.cf;"],
                    "cli": [], "sti": [], "nop": [], "hlt": []}[mnem]
        if mnem == "daa":
            return ["r.al = alu::daa(r, r.al);"]
        if mnem == "pushf":
            return ["m.push(alu::getFlags(r));"]
        if mnem == "popf":
            return ["alu::setFlags(r, m.pop());"]
        if mnem == "in":
            port = ops[1]
            p = "r.dx" if port == "dx" else "0x%X" % num(port)
            if ops[0] == "al":
                return ["r.al = m.in8(%s);" % p]
            return ["r.ax = m.in8(%s);" % p]
        if mnem == "out":
            port = ops[0]
            p = "r.dx" if port == "dx" else "0x%X" % num(port)
            return ["m.out8(%s, r.al);" % p]
        if mnem == "int":
            return ["m.intr(0x%X);" % num(ops[0])]
        if mnem == "loop":
            a = P(0)
            return ["r.cx--;", self.goto(a, proc, "r.cx != 0")]
        if mnem in ("jcxz",):
            return [self.goto(P(0), proc, "r.cx == 0")]
        if mnem == "jmp":
            a = ops[0].strip()
            if a in REG16:
                return self.indirect_jump(proc, a)
            if "$+2" in a:
                return []
            op = P(0)
            if op.kind != "code":
                raise Err("jmp to " + ops[0])
            return [self.goto(op, proc)]
        if mnem in CONDS:
            if "$+2" in ops[0]:
                return []
            op = P(0)
            if op.kind != "code":
                raise Err("jcc to " + ops[0])
            return [self.goto(op, proc, CONDS[mnem])]
        if mnem == "call":
            a = ops[0].strip()
            if a in REG16:
                return ["m.push(0);", "callIndirect(r.%s);" % a]
            if re.match(r"^dword ptr", a):
                return ["/* call %s: the sound driver hook is absent */" % a]
            op = P(0)
            if op.kind != "code":
                raise Err("call " + ops[0])
            name = op.name
            if name in self.adapters:
                return ["%s;" % self.adapters[name]]
            tp = self.labels[name]
            if tp.name in self.skipped:
                return ["/* %s is native */" % name]
            if tp.name != name:
                raise Err("call into the middle of " + tp.name)
            # a far routine returns with retf (pops ip and cs): a far call pushes both, `push cs; call near ptr far_proc` pushes
            # the code segment itself first (the translated `push cs` pushed it already)
            far = tp.far and not self._prev_push_cs
            return ["m.push(0);"] + (["m.push(0);"] if far else []) + ["%s();" % name]
        if mnem == "retn":
            n = num(ops[0]) if ops else 0
            return ["r.sp = (uint16_t)(r.sp + %d);" % (2 + n), "return;"]
        if mnem == "retf":
            n = num(ops[0]) if ops else 0
            return ["r.sp = (uint16_t)(r.sp + %d);" % (4 + n), "return;"]
        if mnem == "iret":
            return ["return;"]
        if mnem in ("rep", "repe", "repz", "repne", "repnz"):
            s = ops[0] if ops else ""
            return self.string_op(mnem, s)
        if mnem in ("movsb", "movsw", "stosb", "stosw", "lodsb", "lodsw", "scasb", "scasw", "cmpsb", "cmpsw"):
            return self.string_op("", mnem)
        raise Err("unsupported instruction")

    def string_op(self, prefix, s):
        if prefix == "rep" and " " in s:       # "rep movsb" arrives as mnemonic rep with operand "movsb"
            pass
        s = s.strip()
        base, sz = s[:-1], (1 if s.endswith("b") else 2)
        stmt = {"movs": "m.movs(%d);", "stos": "m.stos(%d);", "lods": "r.%s = (%s)m.lods(%d);", "scas": "m.scas(%d);", "cmps": "m.cmps(%d);"}[base]
        if base == "lods":
            stmt = "r.%s = (%s)m.lods(%d);" % ("al" if sz == 1 else "ax", "uint8_t" if sz == 1 else "uint16_t", sz)
        else:
            stmt = stmt % sz
        if prefix == "":
            return [stmt]
        cond = ""
        if prefix in ("repne", "repnz"):
            cond = " if (r.zf) break;"
        elif prefix in ("repe", "repz"):
            cond = " if (!r.zf) break;"
        return ["while (r.cx != 0) { %s r.cx--;%s }" % (stmt, cond)]

    def indirect_jump(self, proc, reg):
        targets = self.indirect_targets.get(proc.seg, [])
        cands = [n for n in targets if self.labels[n] is proc]
        others = [n for n in targets if self.labels[n] is not proc]
        lines = ["switch (r.%s) {" % reg]
        for n in cands:
            lines.append("case 0x%X: goto %s;" % (self.code_offset(n), n))
        for n in others:
            tp = self.labels[n]
            self.entries.setdefault(tp.name, set()).add(n)
            lines.append("case 0x%X: return %s(0x%X);" % (self.code_offset(n), tp.name, self.code_offset(n)))
        lines.append("default: return;")
        lines.append("}")
        return lines

    # ------------------------------------------------------------------ driver
    def find_indirect_targets(self):
        # code offsets (within their own segment) that occur as words in the data segment image or as `offset` immediates
        offs = {}
        for name, proc in self.labels.items():
            o = self.code_offset(name)
            if o is not None:
                offs.setdefault(proc.seg, {}).setdefault(o, name)
        words = set()
        img = self.ds_image
        for i in range(0, len(img) - 1):
            words.add(img[i] | (img[i + 1] << 8))
        imm = set()
        for line in self.asm:
            m = re.search(r"offset ((?:loc|sub)_[0-9A-F]+)", line)
            if m and m.group(1) in self.labels:
                imm.add(m.group(1))
        self.indirect_targets = {}
        for seg, table in offs.items():
            names = set(table[o] for o in words if o in table)
            names.update(n for n in imm if self.labels[n].seg == seg)
            self.indirect_targets[seg] = sorted(names, key=self.code_offset)

    def generate(self, outbase):
        self.skipped = set(self.adapters_skip)
        self.entries = {}
        self.find_indirect_targets()
        bodies = {}
        for p in self.procs:
            if p.name in self.skipped or p.name in self.adapters:
                continue
            bodies[p.name] = self.translate_proc(p)
        # fall-through at the end of a proc into the next one
        order = [p for p in self.procs if p.name in bodies]
        for i, p in enumerate(order):
            last = None
            for it in reversed(p.items):
                if it[0] == "ins":
                    last = it
                    break
            if last is None:
                continue
            if last[1] in ("retn", "retf", "iret", "jmp") or (last[1] == "int" and num(last[2][0]) == 0x21):
                continue
            nxt_all = self.procs[self.procs.index(p) + 1] if self.procs.index(p) + 1 < len(self.procs) else None
            if nxt_all is not None and nxt_all.name in bodies:
                bodies[p.name].append("    return %s();" % nxt_all.name)
        name = self.cls
        # header
        h = []
        h.append("/* Generated by tools/asm2cpp.py from %s.asm: do not edit. */" % self.prog)
        h.append("#ifndef YENDOR1_GEN_%s_H" % name.upper())
        h.append("#define YENDOR1_GEN_%s_H" % name.upper())
        h.append('#include "shared/Machine16.h"')
        h.append("namespace Yendor1 {")
        h.append("namespace Gen {")
        h.append("class %s {" % name)
        h.append("public:")
        h.append("\texplicit %s(Shared::Machine16 &machine) : m(machine), r(machine.r) {}" % name)
        h.append("\tstatic const uint32_t kDataSegmentImageOffset = 0x%X;" % self.ds_base)
        h.append("\t/** Runs the program from its entry point; returns the DOS exit code. */")
        h.append("\tint run();")
        h.append("\tShared::Machine16 &m;")
        h.append("private:")
        h.append("\tShared::Regs &r;")
        h.append("\tvoid callIndirect(uint16_t address);")
        for p in order:
            if p.name in self.entries:
                h.append("\tvoid %s(uint16_t entry = 0);" % p.name)
            else:
                h.append("\tvoid %s();" % p.name)
        h.append("};")
        h.append("} // namespace Gen")
        h.append("} // namespace Yendor1")
        h.append("#endif")
        open(outbase + ".h", "w", newline="\n").write("\n".join(h) + "\n")
        # source
        c = []
        c.append("/* Generated by tools/asm2cpp.py from %s.asm: do not edit. */" % self.prog)
        c.append('#include "%s.h"' % os.path.basename(outbase))
        c.append("namespace Yendor1 {")
        c.append("namespace Gen {")
        c.append("using namespace Shared;")
        c.append("int %s::run() {" % name)
        c.append("\ttry {")
        c.append("\t\tstart();")
        c.append("\t} catch (const ProgramExit &e) {")
        c.append("\t\treturn e.code;")
        c.append("\t}")
        c.append("\treturn 0;")
        c.append("}")
        # indirect calls: every proc of the caller's segment
        c.append("void %s::callIndirect(uint16_t address) {" % name)
        c.append("\tswitch (address) {")
        seen = set()
        for p in order:
            o = self.code_offset(p.name)
            if o is None or o in seen:
                continue
            if p.name.startswith("sub_") or p.name == "start":
                seen.add(o)
                c.append("\tcase 0x%X: %s(); break;" % (o, p.name))
        c.append("\tdefault: break;")
        c.append("\t}")
        c.append("}")
        for p in order:
            sig = "void %s::%s(uint16_t entry)" % (name, p.name) if p.name in self.entries else "void %s::%s()" % (name, p.name)
            c.append(sig + " {")
            if p.name in self.entries:
                c.append("    switch (entry) {")
                c.append("    case 0: break;")
                for lab in sorted(self.entries[p.name], key=lambda x: self.code_offset(x) or 0):
                    c.append("    case 0x%X: goto %s;" % (self.code_offset(lab), lab))
                c.append("    default: break;")
                c.append("    }")
            c.extend(bodies[p.name])
            c.append("}")
        c.append("} // namespace Gen")
        c.append("} // namespace Yendor1")
        open(outbase + ".cpp", "w", newline="\n").write("\n".join(c) + "\n")


def main():
    args = sys.argv[1:]
    prog, cls, outbase = args[0], args[1], args[2]
    adapters = {}
    skip = []
    for a in args[3:]:
        if a.startswith("--adapter"):
            continue
        if "=" in a:
            k, v = a.split("=", 1)
            adapters[k] = v
        elif a.startswith("skip:"):
            skip.append(a[5:])
    t = Translator(prog, cls, adapters)
    t.adapters_skip = skip
    t.generate(outbase)
    sys.stderr.write("wrote %s.h/.cpp (%d routines)\n" % (outbase, len(t.procs)))


main()
