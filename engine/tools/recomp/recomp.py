#!/usr/bin/env python3
"""
Static recompiler for the 16-bit WL.EXE code: reads the unpacked program image and a list of function entry points and
writes C++ (one function per entry, labels and gotos for the control flow) that runs against wl::Machine / wl::rc::Regs
(see include/wl/recomp.h).

    recomp.py <image.bin> <entries.txt> <out.cpp> [--interp ADDR,...] [--hook ADDR,...]

image.bin   linear 0x10000.. of the program (CS = 1000h), e.g. the unpacked WL.EXE with WLA.BIN overlaid
entries.txt one hex function entry (offset inside CS) per line; '#' comments
--interp    entries that are executed by the 8086 interpreter instead (self modifying code)
"""
import re
import sys

BASE = 0x10000          # linear address of image byte 0 / CS:0000
CS = 0x1000

CHECK = "--check" in sys.argv
REG8 = ["al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"]
REG16 = ["ax", "cx", "dx", "bx", "sp", "bp", "si", "di"]
SREG = ["es", "cs", "ss", "ds"]


class Op:
    """Operand: kind in reg8/reg16/sreg/imm/mem"""
    def __init__(self, kind, **kw):
        self.kind = kind
        self.__dict__.update(kw)


class Insn:
    def __init__(self, addr):
        self.addr = addr
        self.size = 0
        self.op = ""
        self.a = None
        self.b = None
        self.rep = None
        self.target = None      # branch target (offset inside CS)
        self.width = 0          # operand size in bits (string ops etc.)
        self.extra = None


class Decoder:
    def __init__(self, image):
        self.image = image

    def u8(self, off):
        i = off
        return self.image[i] if 0 <= i < len(self.image) else 0

    def u16(self, off):
        return self.u8(off) | (self.u8(off + 1) << 8)

    def decode(self, off):
        ins = Insn(off)
        p = off
        seg = None
        rep = None
        while True:
            b = self.u8(p)
            if b in (0x26, 0x2E, 0x36, 0x3E):
                seg = {0x26: "es", 0x2E: "cs", 0x36: "ss", 0x3E: "ds"}[b]
                p += 1
            elif b in (0xF2, 0xF3):
                rep = "repne" if b == 0xF2 else "rep"
                p += 1
            elif b == 0xF0:
                p += 1
            else:
                break
        ins.rep = rep
        op = self.u8(p)
        p += 1

        def modrm(wide_reg=None):
            nonlocal p
            m = self.u8(p)
            p += 1
            mod, reg, rm = m >> 6, (m >> 3) & 7, m & 7
            if mod == 3:
                return reg, ("reg", rm)
            base = [("bx", "si"), ("bx", "di"), ("bp", "si"), ("bp", "di"), ("si",), ("di",), ("bp",), ("bx",)][rm]
            disp = 0
            direct = False
            if mod == 0 and rm == 6:
                base = ()
                disp = self.u16(p)
                p += 2
                direct = True
            elif mod == 1:
                d = self.u8(p)
                p += 1
                disp = d - 256 if d >= 128 else d
            elif mod == 2:
                disp = self.u16(p)
                p += 2
            dseg = "ss" if (base and base[0] == "bp") else "ds"
            return reg, ("mem", base, disp, seg or dseg)

        def mk(rmres, width):
            if rmres[0] == "reg":
                return Op("reg8" if width == 8 else "reg16", n=rmres[1])
            return Op("mem", base=rmres[1], disp=rmres[2], seg=rmres[3], width=width)

        def imm(width):
            nonlocal p
            if width == 8:
                v = self.u8(p)
                p += 1
            else:
                v = self.u16(p)
                p += 2
            return Op("imm", v=v, width=width)

        alu = ["add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"]
        if op < 0x40 and (op & 7) < 6:
            ins.op = alu[op >> 3]
            k = op & 7
            if k in (0, 1, 2, 3):
                wide = 16 if (k & 1) else 8
                reg, rmv = modrm()
                regop = Op("reg%d" % wide, n=reg)
                rmop = mk(rmv, wide)
                if k < 2:
                    ins.a, ins.b = rmop, regop
                else:
                    ins.a, ins.b = regop, rmop
                ins.width = wide
            elif k == 4:
                ins.a, ins.b, ins.width = Op("reg8", n=0), imm(8), 8
            else:
                ins.a, ins.b, ins.width = Op("reg16", n=0), imm(16), 16
        elif op in (0x06, 0x0E, 0x16, 0x1E):
            ins.op, ins.a = "push", Op("sreg", n={0x06: 0, 0x0E: 1, 0x16: 2, 0x1E: 3}[op])
        elif op in (0x07, 0x17, 0x1F):
            ins.op, ins.a = "pop", Op("sreg", n={0x07: 0, 0x17: 2, 0x1F: 3}[op])
        elif 0x40 <= op <= 0x47:
            ins.op, ins.a, ins.width = "inc", Op("reg16", n=op & 7), 16
        elif 0x48 <= op <= 0x4F:
            ins.op, ins.a, ins.width = "dec", Op("reg16", n=op & 7), 16
        elif 0x50 <= op <= 0x57:
            ins.op, ins.a = "push", Op("reg16", n=op & 7)
        elif 0x58 <= op <= 0x5F:
            ins.op, ins.a = "pop", Op("reg16", n=op & 7)
        elif 0x70 <= op <= 0x7F:
            ins.op = ["jo", "jno", "jb", "jnb", "jz", "jnz", "jbe", "ja", "js", "jns", "jp", "jnp", "jl", "jge", "jle", "jg"][op & 15]
            d = self.u8(p)
            p += 1
            ins.target = (p + (d - 256 if d >= 128 else d)) & 0xFFFF
        elif op in (0x80, 0x81, 0x82, 0x83):
            wide = 16 if (op & 1) else 8
            reg, rmv = modrm()
            ins.op = alu[reg]
            ins.a = mk(rmv, wide)
            if op == 0x81:
                ins.b = imm(16)
            elif op == 0x83:
                v = self.u8(p)
                p += 1
                ins.b = Op("imm", v=(v - 256 if v >= 128 else v) & 0xFFFF, width=16)
            else:
                ins.b = imm(8)
            ins.width = wide
        elif op in (0x84, 0x85):
            wide = 16 if (op & 1) else 8
            reg, rmv = modrm()
            ins.op, ins.a, ins.b, ins.width = "test", mk(rmv, wide), Op("reg%d" % wide, n=reg), wide
        elif op in (0x86, 0x87):
            wide = 16 if (op & 1) else 8
            reg, rmv = modrm()
            ins.op, ins.a, ins.b, ins.width = "xchg", mk(rmv, wide), Op("reg%d" % wide, n=reg), wide
        elif op in (0x88, 0x89):
            wide = 16 if (op & 1) else 8
            reg, rmv = modrm()
            ins.op, ins.a, ins.b, ins.width = "mov", mk(rmv, wide), Op("reg%d" % wide, n=reg), wide
        elif op in (0x8A, 0x8B):
            wide = 16 if (op & 1) else 8
            reg, rmv = modrm()
            ins.op, ins.a, ins.b, ins.width = "mov", Op("reg%d" % wide, n=reg), mk(rmv, wide), wide
        elif op == 0x8C:
            reg, rmv = modrm()
            ins.op, ins.a, ins.b, ins.width = "mov", mk(rmv, 16), Op("sreg", n=reg & 3), 16
        elif op == 0x8D:
            reg, rmv = modrm()
            ins.op, ins.a, ins.b = "lea", Op("reg16", n=reg), mk(rmv, 16)
        elif op == 0x8E:
            reg, rmv = modrm()
            ins.op, ins.a, ins.b, ins.width = "mov", Op("sreg", n=reg & 3), mk(rmv, 16), 16
        elif op == 0x8F:
            reg, rmv = modrm()
            ins.op, ins.a = "pop", mk(rmv, 16)
        elif op == 0x90:
            ins.op = "nop"
        elif 0x91 <= op <= 0x97:
            ins.op, ins.a, ins.b, ins.width = "xchg", Op("reg16", n=0), Op("reg16", n=op & 7), 16
        elif op == 0x98:
            ins.op = "cbw"
        elif op == 0x99:
            ins.op = "cwd"
        elif op == 0x9A:
            o = self.u16(p)
            s = self.u16(p + 2)
            p += 4
            ins.op, ins.extra = "callf", (s, o)
        elif op == 0x9B:
            ins.op = "nop"
        elif op == 0x9C:
            ins.op = "pushf"
        elif op == 0x9D:
            ins.op = "popf"
        elif op == 0x9E:
            ins.op = "sahf"
        elif op == 0x9F:
            ins.op = "lahf"
        elif op in (0xA0, 0xA1, 0xA2, 0xA3):
            wide = 16 if (op & 1) else 8
            o = self.u16(p)
            p += 2
            mem = Op("mem", base=(), disp=o, seg=seg or "ds", width=wide)
            acc = Op("reg%d" % wide, n=0)
            ins.op, ins.width = "mov", wide
            if op < 0xA2:
                ins.a, ins.b = acc, mem
            else:
                ins.a, ins.b = mem, acc
        elif op in (0xA4, 0xA5, 0xA6, 0xA7, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF):
            ins.op = {0xA4: "movs", 0xA6: "cmps", 0xAA: "stos", 0xAC: "lods", 0xAE: "scas"}[op & 0xFE]
            ins.width = 16 if (op & 1) else 8
            ins.extra = seg
        elif op == 0xA8:
            ins.op, ins.a, ins.b, ins.width = "test", Op("reg8", n=0), imm(8), 8
        elif op == 0xA9:
            ins.op, ins.a, ins.b, ins.width = "test", Op("reg16", n=0), imm(16), 16
        elif 0xB0 <= op <= 0xB7:
            ins.op, ins.a, ins.b, ins.width = "mov", Op("reg8", n=op & 7), imm(8), 8
        elif 0xB8 <= op <= 0xBF:
            ins.op, ins.a, ins.b, ins.width = "mov", Op("reg16", n=op & 7), imm(16), 16
        elif op == 0xC2:
            ins.op, ins.extra = "retn", self.u16(p)
            p += 2
        elif op == 0xC3:
            ins.op, ins.extra = "retn", 0
        elif op in (0xC4, 0xC5):
            reg, rmv = modrm()
            ins.op, ins.a, ins.b = ("les" if op == 0xC4 else "lds"), Op("reg16", n=reg), mk(rmv, 16)
        elif op in (0xC6, 0xC7):
            wide = 16 if (op & 1) else 8
            reg, rmv = modrm()
            ins.op, ins.a, ins.width = "mov", mk(rmv, wide), wide
            ins.b = imm(wide)
        elif op == 0xCA:
            ins.op, ins.extra = "retf", self.u16(p)
            p += 2
        elif op == 0xCB:
            ins.op, ins.extra = "retf", 0
        elif op == 0xCC:
            ins.op, ins.extra = "int", 3
        elif op == 0xCD:
            ins.op, ins.extra = "int", self.u8(p)
            p += 1
        elif op == 0xCF:
            ins.op = "iret"
        elif op in (0xD0, 0xD1, 0xD2, 0xD3):
            wide = 16 if (op & 1) else 8
            reg, rmv = modrm()
            ins.op = ["rol", "ror", "rcl", "rcr", "shl", "shr", "shl", "sar"][reg]
            ins.a, ins.width = mk(rmv, wide), wide
            ins.b = Op("imm", v=1, width=8) if op < 0xD2 else Op("reg8", n=1)   # CL
        elif op == 0xD7:
            ins.op = "xlat"
            ins.extra = seg
        elif op in (0xE0, 0xE1, 0xE2, 0xE3):
            ins.op = {0xE0: "loopne", 0xE1: "loope", 0xE2: "loop", 0xE3: "jcxz"}[op]
            d = self.u8(p)
            p += 1
            ins.target = (p + (d - 256 if d >= 128 else d)) & 0xFFFF
        elif op in (0xE4, 0xE5):
            ins.op, ins.width = "in", 16 if (op & 1) else 8
            ins.a = Op("imm", v=self.u8(p), width=8)
            p += 1
        elif op in (0xE6, 0xE7):
            ins.op, ins.width = "out", 16 if (op & 1) else 8
            ins.a = Op("imm", v=self.u8(p), width=8)
            p += 1
        elif op == 0xE8:
            d = self.u16(p)
            p += 2
            ins.op, ins.target = "call", (p + d) & 0xFFFF
        elif op == 0xE9:
            d = self.u16(p)
            p += 2
            ins.op, ins.target = "jmp", (p + d) & 0xFFFF
        elif op == 0xEA:
            o = self.u16(p)
            s = self.u16(p + 2)
            p += 4
            ins.op, ins.extra = "jmpf", (s, o)
        elif op == 0xEB:
            d = self.u8(p)
            p += 1
            ins.op, ins.target = "jmp", (p + (d - 256 if d >= 128 else d)) & 0xFFFF
        elif op in (0xEC, 0xED):
            ins.op, ins.width = "in", 16 if (op & 1) else 8
            ins.a = None
        elif op in (0xEE, 0xEF):
            ins.op, ins.width = "out", 16 if (op & 1) else 8
            ins.a = None
        elif op == 0xF4:
            ins.op = "hlt"
        elif op == 0xF5:
            ins.op = "cmc"
        elif op in (0xF6, 0xF7):
            wide = 16 if (op & 1) else 8
            reg, rmv = modrm()
            ins.a, ins.width = mk(rmv, wide), wide
            ins.op = ["test", "test", "not", "neg", "mul", "imul", "div", "idiv"][reg]
            if reg < 2:
                ins.b = imm(wide)
        elif op in (0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD):
            ins.op = {0xF8: "clc", 0xF9: "stc", 0xFA: "cli", 0xFB: "sti", 0xFC: "cld", 0xFD: "std"}[op]
        elif op in (0xFE, 0xFF):
            wide = 16 if (op & 1) else 8
            reg, rmv = modrm()
            if reg == 0:
                ins.op, ins.a, ins.width = "inc", mk(rmv, wide), wide
            elif reg == 1:
                ins.op, ins.a, ins.width = "dec", mk(rmv, wide), wide
            elif op == 0xFF and reg == 2:
                ins.op, ins.a = "calli", mk(rmv, 16)
            elif op == 0xFF and reg == 3:
                ins.op, ins.a = "callfi", mk(rmv, 16)
            elif op == 0xFF and reg == 4:
                ins.op, ins.a = "jmpi", mk(rmv, 16)
            elif op == 0xFF and reg == 5:
                ins.op, ins.a = "jmpfi", mk(rmv, 16)
            elif op == 0xFF and reg == 6:
                ins.op, ins.a = "push", mk(rmv, 16)
            else:
                ins.op = "bad"
        else:
            ins.op = "bad"
            ins.extra = op
        ins.size = p - off
        return ins


# ---------------------------------------------------------------------------------------------------------------------
class Emitter:
    def __init__(self, image, entries, interp):
        self.dec = Decoder(image)
        self.entries = set(entries)
        self.interp = set(interp)
        self.out = []
        self.todo = sorted(self.entries)
        self.done = set()
        self.cache = {}

    def insn(self, a):
        if a not in self.cache:
            self.cache[a] = self.dec.decode(a)
        return self.cache[a]

    # --- operand expressions ---
    @staticmethod
    def reg(op):
        if op.kind == "reg8":
            n = REG8[op.n]
            return "r.%s.b.%s" % ("acdb"[op.n & 3], "l" if op.n < 4 else "h")
        if op.kind == "reg16":
            return ["r.a.x", "r.c.x", "r.d.x", "r.b.x", "r.sp", "r.bp", "r.si", "r.di"][op.n]
        if op.kind == "sreg":
            return "r." + SREG[op.n]
        raise ValueError(op.kind)

    @staticmethod
    def ea(op):
        parts = []
        for b in op.base:
            parts.append({"bx": "r.b.x", "bp": "r.bp", "si": "r.si", "di": "r.di"}[b])
        if op.disp or not parts:
            parts.append("0x%X" % (op.disp & 0xFFFF))
        return "(uint16_t)(%s)" % " + ".join(parts)

    def rd(self, op, width=None):
        if op.kind in ("reg8", "reg16", "sreg"):
            return self.reg(op)
        if op.kind == "imm":
            return "%d" % op.v if op.width == 8 else "0x%X" % op.v
        if op.kind == "mem":
            w = width or op.width
            return "%s(m, r.%s, %s)" % ("rb" if w == 8 else "rw", op.seg, self.ea(op))
        raise ValueError(op.kind)

    def wr(self, op, val, width=None):
        if op.kind in ("reg8", "reg16", "sreg"):
            return "%s = %s;" % (self.reg(op), val)
        if op.kind == "mem":
            w = width or op.width
            return "%s(m, r.%s, %s, %s);" % ("wb" if w == 8 else "ww", op.seg, self.ea(op), val)
        raise ValueError(op.kind)

    def T(self, width):
        return "uint8_t" if width == 8 else "uint16_t"

    COND = {
        "jo": "r.of", "jno": "!r.of", "jb": "r.cf", "jnb": "!r.cf", "jz": "r.zf", "jnz": "!r.zf", "jbe": "(r.cf || r.zf)",
        "ja": "(!r.cf && !r.zf)", "js": "r.sf", "jns": "!r.sf", "jp": "r.pf", "jnp": "!r.pf", "jl": "(r.sf != r.of)",
        "jge": "(r.sf == r.of)", "jle": "(r.zf || r.sf != r.of)", "jg": "(!r.zf && r.sf == r.of)",
    }

    def stmt(self, ins, nextaddr):
        """C++ statements for one instruction (control flow handled by the caller)"""
        op = ins.op
        w = ins.width
        T = self.T(w) if w else None
        a, b = ins.a, ins.b
        rd, wr = self.rd, self.wr
        if op == "mov":
            return [wr(a, rd(b, w), w)]
        if op in ("add", "sub", "and", "or", "xor", "adc", "sbb", "cmp"):
            if op == "cmp":
                return ["sub_<%s>(r, %s, %s);" % (T, rd(a, w), rd(b, w))]
            if op == "add":
                e = "add_<%s>(r, %s, %s)" % (T, rd(a, w), rd(b, w))
            elif op == "sub":
                e = "sub_<%s>(r, %s, %s)" % (T, rd(a, w), rd(b, w))
            elif op == "adc":
                e = "add_<%s>(r, %s, %s, r.cf)" % (T, rd(a, w), rd(b, w))
            elif op == "sbb":
                e = "sub_<%s>(r, %s, %s, r.cf)" % (T, rd(a, w), rd(b, w))
            else:
                e = "%s_<%s>(r, %s, %s)" % (op, T, rd(a, w), rd(b, w))
            return [wr(a, e, w)]
        if op == "test":
            return ["and_<%s>(r, %s, %s);" % (T, rd(a, w), rd(b, w))]
        if op in ("inc", "dec"):
            return [wr(a, "%s_<%s>(r, %s)" % (op, T, rd(a, w)), w)]
        if op == "neg":
            return [wr(a, "neg_<%s>(r, %s)" % (T, rd(a, w)), w)]
        if op == "not":
            return [wr(a, "(%s)~%s" % (T, rd(a, w)), w)]
        if op in ("shl", "shr", "sar", "rol", "ror", "rcl", "rcr"):
            cnt = rd(b, 8)
            return [wr(a, "%s_<%s>(r, %s, %s)" % (op, T, rd(a, w), cnt if b.kind != "reg8" else "(unsigned)" + cnt), w)]
        if op == "push":
            return ["push(m, r, %s);" % (rd(a, 16) if a.kind != "reg16" or a.n != 4 else "(uint16_t)(r.sp - 2)")]
        if op == "pop":
            if a.kind == "reg16" and a.n == 4:
                return ["r.sp = pop(m, r);"]
            if a.kind == "mem":
                # pop to memory: the stack pointer is incremented before the address is computed
                return ["{ const uint16_t v = pop(m, r); %s }" % wr(a, "v", 16)]
            return [wr(a, "pop(m, r)", 16)]
        if op == "xchg":
            return ["{ const %s t = %s; %s %s }" % (T, rd(a, w), wr(a, rd(b, w), w), wr(b, "t", w))]
        if op == "lea":
            return [wr(a, self.ea(b), 16)]
        if op in ("les", "lds"):
            seg = "es" if op == "les" else "ds"
            return ["{ const uint16_t o = rw(m, r.%s, %s), s = rw(m, r.%s, (uint16_t)(%s + 2)); %s r.%s = s; }" % (b.seg, self.ea(b), b.seg, self.ea(b), wr(a, "o", 16), seg)]
        if op == "cbw":
            return ["r.a.x = (uint16_t)(int16_t)(int8_t)r.a.b.l;"]
        if op == "cwd":
            return ["r.d.x = (r.a.x & 0x8000) ? 0xFFFF : 0;"]
        if op == "pushf":
            return ["push(m, r, r.flags());"]
        if op == "popf":
            return ["r.setFlags(pop(m, r));"]
        if op == "sahf":
            return ["{ const uint8_t f = r.a.b.h; r.cf = f & 1; r.pf = f & 4; r.af = f & 0x10; r.zf = f & 0x40; r.sf = f & 0x80; }"]
        if op == "lahf":
            return ["r.a.b.h = (uint8_t)(r.flags() & 0xFF);"]
        if op in ("clc", "stc", "cmc", "cld", "std", "cli", "sti"):
            return {"clc": ["r.cf = false;"], "stc": ["r.cf = true;"], "cmc": ["r.cf = !r.cf;"], "cld": ["r.df = false;"], "std": ["r.df = true;"],
                    "cli": ["r.ifl = false;"], "sti": ["r.ifl = true;"]}[op]
        if op == "nop":
            return []
        if op == "mul":
            return ["mul%d(r, %s);" % (w, rd(a, w))]
        if op == "imul":
            return ["imul%d(r, %s);" % (w, rd(a, w))]
        if op == "div":
            return ["div%d(r, %s);" % (w, rd(a, w))]
        if op == "idiv":
            return ["idiv%d(r, %s);" % (w, rd(a, w))]
        if op == "xlat":
            seg = ins.extra or "ds"
            return ["r.a.b.l = rb(m, r.%s, (uint16_t)(r.b.x + r.a.b.l));" % seg]
        if op == "in":
            port = ("%d" % a.v) if a is not None else "r.d.x"
            return ["%s = (%s)portIn(m, r, %s, %s);" % ("r.a.b.l" if w == 8 else "r.a.x", self.T(w), port, "false" if w == 8 else "true")]
        if op == "out":
            port = ("%d" % a.v) if a is not None else "r.d.x"
            return ["portOut(m, r, %s, %s, %s);" % (port, "r.a.b.l" if w == 8 else "r.a.x", "false" if w == 8 else "true")]
        if op == "int":
            return ["intr(m, r, %d);" % ins.extra]
        if op in ("movs", "cmps", "stos", "lods", "scas"):
            return self.string_op(ins)
        return ["fatal(\"unsupported %s at %04X\");" % (op, ins.addr)]

    def string_op(self, ins):
        w = ins.width
        sz = w // 8
        T = self.T(w)
        src = ins.extra or "ds"
        acc = "r.a.b.l" if w == 8 else "r.a.x"
        rdm = "rb" if w == 8 else "rw"
        wrm = "wb" if w == 8 else "ww"
        body = {
            "movs": "%s(m, r.es, r.di, %s(m, r.%s, r.si)); r.si += step(r, %d); r.di += step(r, %d);" % (wrm, rdm, src, sz, sz),
            "stos": "%s(m, r.es, r.di, %s); r.di += step(r, %d);" % (wrm, acc, sz),
            "lods": "%s = %s(m, r.%s, r.si); r.si += step(r, %d);" % (acc, rdm, src, sz),
            "cmps": "sub_<%s>(r, %s(m, r.%s, r.si), %s(m, r.es, r.di)); r.si += step(r, %d); r.di += step(r, %d);" % (T, rdm, src, rdm, sz, sz),
            "scas": "sub_<%s>(r, (%s)%s, %s(m, r.es, r.di)); r.di += step(r, %d);" % (T, T, acc, rdm, sz),
        }[ins.op]
        if not ins.rep:
            return ["{ %s }" % body]
        if ins.op in ("movs", "stos", "lods"):
            return ["while (r.c.x != 0) { %s --r.c.x; }" % body]
        cond = "r.zf" if ins.rep == "rep" else "!r.zf"
        return ["while (r.c.x != 0) { %s --r.c.x; if (!(%s)) break; }" % (body, cond)]

    # --- function emission ---
    def emit_function(self, entry):
        name = "f_%04X" % entry
        if entry in self.interp:
            self.out.append("void %s(Machine &m, Regs &r) { interpret(m, r, 0x%04X); }\n" % (name, entry))
            return
        # collect reachable instructions
        insns = {}
        leaders = {entry}
        work = [entry]
        tails = set()
        fall = {}          # address of an instruction -> entry it falls through into
        while work:
            a = work.pop()
            while True:
                if a in insns:
                    break
                ins = self.insn(a)
                insns[a] = ins
                nxt = a + ins.size
                op = ins.op
                if op in ("retn", "retf", "iret", "hlt", "bad", "jmpi", "jmpf", "jmpfi"):
                    break
                if ins.target is not None and op != "call":
                    t = ins.target
                    if op == "jmp" and t in self.entries and t != entry:
                        tails.add(t)
                    elif op not in ("jmp",) and t in self.entries and t != entry:
                        tails.add(t)
                    else:
                        leaders.add(t)
                        work.append(t)
                    if op == "jmp":
                        break
                if op == "call" and ins.target is not None and ins.target not in self.entries:
                    self.entries.add(ins.target)
                    self.todo.append(ins.target)
                a = nxt
                if a in self.entries and a != entry:
                    # fall through into another function: tail call
                    tails.add(a)
                    fall[ins.addr] = a
                    break
        lines = ["void %s(Machine &m, Regs &r) {" % (("b_%04X" % entry) if CHECK else name)]
        if min(insns) != entry:
            lines.append("    goto L_%04X;" % entry)
        for a in sorted(insns):
            ins = insns[a]
            if a in leaders:
                lines.append("L_%04X:;" % a)
            nxt = a + ins.size
            op = ins.op
            c = "    // %04X" % a
            if op == "call":
                if ins.target in self.interp:
                    lines.append("    push(m, r, 0x%04X); f_%04X(m, r);%s" % (nxt, ins.target, c))
                else:
                    lines.append("    push(m, r, 0x%04X); f_%04X(m, r);%s" % (nxt, ins.target, c))
            elif op == "calli":
                lines.append("    { const uint16_t t = %s; push(m, r, 0x%04X); dispatch(m, r, t); }%s" % (self.rd(ins.a, 16), nxt, c))
            elif op == "callf":
                s, o = ins.extra
                lt = ((s << 4) + o - 0x10000) & 0xFFFFF
                if s == CS:
                    lt = o
                lines.append("    push(m, r, r.cs); push(m, r, 0x%04X); %s%s" % (nxt, "f_%04X(m, r);" % lt if lt in self.entries else 'fatal("far call");', c))
            elif op == "callfi":
                lines.append("    fatal(\"indirect far call at %04X\");" % a)
            elif op == "retn":
                lines.append("    r.sp = (uint16_t)(r.sp + 2 + %d); return;%s" % (ins.extra, c))
            elif op == "retf":
                lines.append("    r.sp = (uint16_t)(r.sp + 4 + %d); return;%s" % (ins.extra, c))
            elif op == "iret":
                lines.append("    pop(m, r); pop(m, r); r.setFlags(pop(m, r)); return;%s" % c)
            elif op == "hlt":
                lines.append("    fatal(\"hlt\"); return;")
            elif op == "jmp":
                t = ins.target
                be = ("backedge(m, r, 0x%04X); " % a) if t <= a else ""
                if t in leaders:
                    lines.append("    { %sgoto L_%04X; }%s" % (be, t, c))
                else:
                    lines.append("    { %sf_%04X(m, r); return; }%s" % (be, t, c))
            elif op == "jmpi":
                lines.append("    dispatch(m, r, %s); return;%s" % (self.rd(ins.a, 16), c))
            elif op == "jmpfi":
                lines.append("    { const uint16_t o = %s, s = rw(m, r.%s, (uint16_t)(%s + 2)); farJump(m, r, s, o); return; }%s" % (self.rd(ins.a, 16), ins.a.seg, self.ea(ins.a), c))
            elif op == "jmpf":
                lines.append("    fatal(\"far jump at %04X\"); return;" % a)
            elif op in self.COND:
                t = ins.target
                cond = self.COND[op]
                be = ("backedge(m, r, 0x%04X); " % a) if t <= a else ""
                if t in leaders:
                    lines.append("    if (%s) { %sgoto L_%04X; }%s" % (cond, be, t, c))
                else:
                    lines.append("    if (%s) { %sf_%04X(m, r); return; }%s" % (cond, be, t, c))
            elif op in ("loop", "loope", "loopne", "jcxz"):
                t = ins.target
                be = ("backedge(m, r, 0x%04X); " % a) if t <= a else ""
                dest = ("{ %sgoto L_%04X; }" % (be, t)) if t in leaders else ("{ %sf_%04X(m, r); return; }" % (be, t))
                if op == "loop":
                    lines.append("    if (--r.c.x != 0) %s%s" % (dest, c))
                elif op == "loope":
                    lines.append("    if (--r.c.x != 0 && r.zf) %s%s" % (dest, c))
                elif op == "loopne":
                    lines.append("    if (--r.c.x != 0 && !r.zf) %s%s" % (dest, c))
                else:
                    lines.append("    if (r.c.x == 0) %s%s" % (dest, c))
            elif op == "bad":
                lines.append("    fatal(\"bad opcode %02X at %04X\"); return;" % (ins.extra if ins.extra is not None else 0, a))
            else:
                for s in self.stmt(ins, nxt):
                    lines.append("    " + s + c)
                    c = ""
            # fall through into a following tail target
            if a in fall:
                lines.append("    f_%04X(m, r); return;" % fall[a])
        lines.append("}\n")
        if CHECK:
            lines.append("void %s(Machine &m, Regs &r) { const uint16_t sp0 = r.sp; b_%04X(m, r); stackCheck(0x%04X, sp0, r.sp); }\n" % (name, entry, entry))
        self.out.extend(lines)
        for t in tails:
            if t not in self.entries:
                self.entries.add(t)
                self.todo.append(t)

    def run(self):
        while self.todo:
            e = self.todo.pop()
            if e in self.done:
                continue
            self.done.add(e)
            self.emit_function(e)


def main():
    args = sys.argv[1:]
    if len(args) < 3:
        print(__doc__)
        return 1
    image = open(args[0], "rb").read()
    entries = []
    for line in open(args[1]):
        line = line.split("#")[0].strip()
        if line:
            entries.append(int(line, 16))
    interp = set()
    for i, a in enumerate(args):
        if a == "--interp":
            interp = set(int(x, 16) for x in args[i + 1].split(",") if x)
    em = Emitter(image, entries, interp)
    em.run()
    allf = sorted(em.done)
    with open(args[2], "w") as f:
        f.write("// Generated by tools/recomp/recomp.py from the WL.EXE image - do not edit.\n")
        f.write('#include "wl/recomp_rt.h"\n\nnamespace wl {\nnamespace rc {\n\n')
        for e in allf:
            f.write("void f_%04X(Machine &m, Regs &r);\n" % e)
        f.write("\n")
        f.write("".join(l + "\n" for l in em.out))
        f.write("void dispatch(Machine &m, Regs &r, uint16_t target) {\n    switch (target) {\n")
        for e in allf:
            f.write("    case 0x%04X: f_%04X(m, r); return;\n" % (e, e))
        f.write('    default: unknownTarget(m, r, target); return;\n    }\n}\n\n')
        f.write("bool isEntry(uint16_t a) {\n    switch (a) {\n")
        for e in allf:
            f.write("    case 0x%04X:\n" % e)
        f.write("        return true;\n    default: return false;\n    }\n}\n\n} // namespace rc\n} // namespace wl\n")
    print("functions:", len(allf))
    return 0


if __name__ == "__main__":
    sys.exit(main())
