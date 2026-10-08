#!/usr/bin/env python3
"""Translate straight-line routines of mm3.asm (IDA export) into goto-style C.

This is a *mechanical* translation of a small 8086 subset (mov/add/sub/cmp/inc/dec/shifts/imul/idiv/jcc/jmp/call/push/pop ...)
used by the 3D renderer's list writers and view scanners; it keeps the original's registers, flags and stack frames so the
result can be checked bit-for-bit against the original code running in the emulator (tools/mm3_emu.py).

usage: asm2c.py mm3.asm mm3.idc OUT.c FUNC [FUNC ...]
Calls to functions that are not in the FUNC list become `host_<name>(c)` (to be written by hand in src/).
The generated code uses the runtime in src/recomp.h (CPU state, DGROUP array DG[], stack array STK[]).
"""
import os
import re
import sys

REG16 = ["ax", "bx", "cx", "dx", "si", "di", "bp", "sp"]
REG8 = {"al": ("ax", 0), "ah": ("ax", 8), "bl": ("bx", 0), "bh": ("bx", 8), "cl": ("cx", 0), "ch": ("cx", 8),
        "dl": ("dx", 0), "dh": ("dx", 8)}
DGROUP_LINEAR = 0x286F0


class Translator:
    def __init__(self, asm_path, idc_path):
        self.asm_path = asm_path
        self.lines = open(asm_path, encoding="latin1").read().split("\n")
        self.names = {}
        for m in re.finditer(r'set_name\s*\(0[Xx]([0-9A-Fa-f]+),\s*"([^"]+)"', open(idc_path, encoding="latin1").read()):
            self.names[m.group(2)] = int(m.group(1), 16)
        self.procs = {}   # name -> (start, end, kind)
        for i, l in enumerate(self.lines):
            m = re.match(r"^(\w+)\s+proc (far|near)", l)
            if m:
                self.procs[m.group(1)] = [i, None, m.group(2)]
            m = re.match(r"^(\w+)\s+endp", l)
            if m and m.group(1) in self.procs:
                self.procs[m.group(1)][1] = i
        self.segments = []  # (start, end, selector) from mm3.idc add_segm_ex
        for m in re.finditer(r"add_segm_ex\(0[Xx]([0-9A-Fa-f]+),0[Xx]([0-9A-Fa-f]+),0[Xx]([0-9A-Fa-f]+)", open(idc_path, encoding="latin1").read()):
            self.segments.append((int(m.group(1), 16), int(m.group(2), 16), int(m.group(3), 16)))
        self.datasize = {}
        for l in self.lines:
            m = re.match(r"^(\w+)\s+(db|dw|dd)\b", l)
            if m:
                self.datasize[m.group(1)] = {"db": 8, "dw": 16, "dd": 32}[m.group(2)]
        self.jpt = {}
        for i, l in enumerate(self.lines):
            m = re.match(r"^(jpt_\w+)\s+dw (.*)$", l)
            if not m:
                continue
            tab = re.findall(r"offset (\w+)", m.group(2).split(";")[0])
            j = i + 1
            while True:
                t = self.lines[j]
                if re.match(r"^\s+;", t):      # comment-only line inside the table
                    j += 1
                    continue
                mm = re.match(r"^\s+dw (.*)$", t)
                if not mm or "offset" not in mm.group(1):
                    break
                tab.extend(re.findall(r"offset (\w+)", mm.group(1).split(";")[0]))
                j += 1
            self.jpt[m.group(1)] = tab

    def func_linear(self, fname):
        if fname in self.names:
            return self.names[fname]
        m = re.match(r"^\w+?_([0-9A-F]{5})$", fname)
        if m:
            return int(m.group(1), 16)
        return None

    def func_segment(self, fname):
        a = self.func_linear(fname)
        if a is None:
            return None
        for start, end, sel in self.segments:
            if start <= a < end:
                return sel
        return None

    # ---- symbols
    def sym_addr(self, name):
        m = re.match(r"^(?:byte|word|dword|unk|off|stru|asc|a)_?([0-9A-F]{4,5})$", name)
        if m and name.split("_")[0] in ("byte", "word", "dword", "unk", "off", "stru"):
            return int(m.group(1), 16) - DGROUP_LINEAR
        if name in self.names:
            return self.names[name] - DGROUP_LINEAR
        m = re.match(r"^\w+_([0-9A-F]{5})$", name)
        if m:
            return int(m.group(1), 16) - DGROUP_LINEAR
        raise KeyError("unknown symbol " + name)

    def sym_addr_linear(self, name):
        m = re.match(r"^(?:byte|word|dword|unk|off|stru)_([0-9A-F]{4,5})$", name)
        if m:
            return int(m.group(1), 16)
        if name in self.names:
            return self.names[name]
        raise KeyError("unknown code symbol " + name)

    def sym_size(self, name):
        if name in self.datasize:
            return self.datasize[name]
        p = name.split("_")[0]
        return {"byte": 8, "word": 16, "dword": 32}.get(p, 16)

    # ---- operands
    def parse_num(self, t):
        t = t.strip()
        neg = t.startswith("-")
        t = t.lstrip("+-")
        if re.match(r"^[0-9A-Fa-f]+h$", t):
            v = int(t[:-1], 16)
        elif re.match(r"^\d+$", t):
            v = int(t)
        else:
            raise ValueError(t)
        return -v if neg else v

    def operand(self, text, locals_, hint_size=None):
        """-> dict(kind, size, expr info)"""
        t = text.strip().replace("ffblk.ff_name", "ffblk+1Eh")  # Borland struct ffblk: the name is at +1Eh
        size = None
        m = re.match(r"^(byte|word|dword) ptr\s+(.*)$", t)
        if m:
            size = {"byte": 8, "word": 16, "dword": 32}[m.group(1)]
            t = m.group(2).strip()
        if t in REG8:
            return {"kind": "reg", "name": t, "size": 8}
        if t in REG16:
            return {"kind": "reg", "name": t, "size": 16}
        if t in ("cs", "ds", "es", "ss"):
            return {"kind": "seg", "name": t, "size": 16}
        m = re.match(r"^\(?offset (\w+)(?:\s*([+-])\s*(\d+|[0-9A-Fa-f]+h))?\)?$", t)
        if m:
            v = self.sym_addr(m.group(1))
            if m.group(2):
                v += (-1 if m.group(2) == "-" else 1) * self.parse_num(m.group(3))
            return {"kind": "imm", "size": 16, "value": str(v & 0xFFFF)}
        if re.match(r"^-?[0-9][0-9A-Fa-f]*h?$", t):
            try:
                return {"kind": "imm", "size": size or hint_size or 16, "value": str(self.parse_num(t))}
            except ValueError:
                pass
        # memory: [..] or symbol[+off]
        m = re.match(r"^(?:cs:|ds:|ss:|es:)?\[(.*)\]$", t)
        seg = "DS"
        es_prefix = t.startswith("es:")
        cs_prefix = t.startswith("cs:")
        if m:
            parts = re.findall(r"([+-]?)\s*([A-Za-z_]\w*|[0-9][0-9A-Fa-f]*h?)", m.group(1))
            regs, disp, ss = [], 0, False
            local_size = None
            for sign, tok in parts:
                if tok in REG16:
                    regs.append(tok)
                    if tok == "bp":
                        ss = True
                elif tok in locals_:
                    disp += (-1 if sign == "-" else 1) * locals_[tok]
                    local_size = getattr(self, "local_sizes", {}).get(tok)
                else:
                    disp += (-1 if sign == "-" else 1) * self.parse_num(tok if tok[0].isdigit() else "0")
            addr = " + ".join(["c->%s" % r for r in regs] + [str(disp)]) if regs or disp else "0"
            if local_size and local_size > 16:  # a dword local: accessed as words through `word ptr [bp+var]`, never sized implicitly
                local_size = None
            return {"kind": "mem", "size": size or local_size or hint_size, "addr": "(uint16_t)(%s)" % addr, "seg": "CS" if cs_prefix else ("ES" if es_prefix else ("ST" if ss else "DG"))}
        m = re.match(r"^(?:cs:)?([A-Za-z_]\w*)(?:\s*\+\s*([0-9A-Fa-f]+h?|\d+))?(?:\s*-\s*([0-9A-Fa-f]+h?|\d+))?$", t)
        if m:
            name = m.group(1)
            if cs_prefix:  # a variable in the code segment: its address is the linear address minus the segment base
                a = self.sym_addr_linear(name)
                if m.group(2):
                    a += self.parse_num(m.group(2))
                return {"kind": "mem", "size": size or self.sym_size(name), "addr": "CSOFF(%d)" % a, "seg": "CS", "linear": a, "sym": name}
            if name in locals_:
                ls = getattr(self, "local_sizes", {}).get(name)
                return {"kind": "mem", "size": size or (ls if ls and ls <= 16 else None) or hint_size or 16, "addr": "(uint16_t)(c->bp + %d)" % locals_[name], "seg": "ST"}
            a = self.sym_addr(name)
            if m.group(2):
                a += self.parse_num(m.group(2))
            if m.group(3):
                a -= self.parse_num(m.group(3))
            return {"kind": "mem", "size": size or self.sym_size(name), "addr": str(a & 0xFFFF), "seg": "DG", "sym": name}
        raise ValueError("operand? " + text)

    def cur_seg_expr(self, o):
        if "linear" in o:  # a named variable: its own segment
            for start, end, sel in self.segments:
                if start <= o["linear"] < end:
                    return "0x%X" % sel
        return "0x%X" % self.cur_seg

    def cs_addr(self, o):
        if "linear" in o:
            for start, end, sel in self.segments:
                if start <= o["linear"] < end:
                    return str(o["linear"] - sel * 16)
        return o["addr"]

    # ---- C emission helpers
    def rd(self, o, size=None):
        if o["kind"] == "imm":
            return "(%s)" % o["value"]
        if o["kind"] == "reg":
            if o["size"] == 16:
                return "c->%s" % o["name"]
            r, sh = REG8[o["name"]]
            return "((c->%s >> %d) & 0xFF)" % (r, sh)
        sz = o["size"] or size or 16
        if o["kind"] == "mem":
            if o["seg"] == "ES":
                return "ES%d(c, %s)" % (sz, o["addr"])
            if o["seg"] == "CS":
                return "CS%d(%s, %s)" % (sz, self.cur_seg_expr(o), self.cs_addr(o))
            return "%s%d(%s)" % (o["seg"], sz, o["addr"])
        raise ValueError(o)

    def wr(self, o, val):
        if o["kind"] == "reg":
            if o["size"] == 16:
                return "c->%s = (uint16_t)(%s);" % (o["name"], val)
            r, sh = REG8[o["name"]]
            mask = 0xFF << sh
            return "c->%s = (uint16_t)((c->%s & 0x%04X) | (((%s) & 0xFF) << %d));" % (r, r, 0xFFFF ^ mask, val, sh)
        if o["kind"] == "mem":
            if o["seg"] == "ES":
                return "ES%d_SET(c, %s, %s);" % (o["size"] or 16, o["addr"], val)
            if o["seg"] == "CS":
                return "wr%d(SEGP(%s), %s, %s);" % (o["size"] or 16, self.cur_seg_expr(o), self.cs_addr(o), val)
            return "%s%d_SET(%s, %s);" % (o["seg"], o["size"] or 16, o["addr"], val)
        raise ValueError(o)

    def opsize(self, a, b=None):
        for o in (a, b):
            if o and o["kind"] == "reg":
                return o["size"]
        for o in (a, b):
            if o and o["kind"] == "mem" and o["size"]:
                return o["size"]
        return 16

    # ---- function translation
    def translate(self, fname, translated):
        s, e, kind = self.procs[fname]
        locals_ = {}
        self.local_sizes = {}
        body = []
        for i in range(s + 1, e):
            raw = self.lines[i]
            l = raw.split(";")[0].rstrip()
            if not l.strip():
                continue
            m = re.match(r"^(\w+)\s*=\s*(byte|word|dword|ffblk|\w+) ptr\s+(-?[0-9A-Fa-f]+h?)", l)
            if m:
                locals_[m.group(1)] = self.parse_num(m.group(3))
                self.local_sizes[m.group(1)] = {"byte": 8, "word": 16, "dword": 32}.get(m.group(2))
                continue
            body.append((i, l))
        self.cur_seg = self.func_segment(fname) or 0
        self.extend_tail(fname, e, body)
        self.cur_labels = {}
        for i, l in body:
            m = re.match(r"^(\w+):\s*$", l)
            if m:
                a = self.func_linear(m.group(1))
                if a is not None:
                    self.cur_labels[a - self.cur_seg * 16] = m.group(1)
        self.cur_fname = fname
        self.synth = {}
        self.add_table_labels(fname, body)
        out = ["static void fn_%s(Cpu *c) {" % fname, "\tuint32_t t_; (void)t_;", "\tFNTRACE(\"%s\");" % fname]
        for i, l in body:
            if i in self.synth:
                out.append("%s: RTRACE(c, \"%s\");" % (self.synth[i], self.synth[i]))
            m = re.match(r"^(\w+):\s*$", l)
            if m:
                out.append("%s: RTRACE(c, \"%s\");" % (m.group(1), m.group(1)))
                continue
            m = re.match(r"^\s+(\w+)(?:\s+(.*))?$", l)
            if not m:
                if re.match(r"^\s*(align|nop|assume)", l):
                    continue
                raise ValueError("line? %r" % l)
            mn, rest = m.group(1), (m.group(2) or "").strip()
            try:
                out.extend("\t" + x for x in self.insn(mn, rest, locals_, kind, translated, i))
            except Exception as ex:  # noqa: BLE001
                raise RuntimeError("%s: line %d %r: %s" % (fname, i + 1, l.strip(), ex))
        out.append("}")
        return "\n".join(out)

    def extend_tail(self, fname, end, body):
        """IDA sometimes ends a procedure at its computed `jmp cs:[bx+K]` and lists the handlers after `endp` as loose code
        (up to the routine's last retf, followed by a separator line and data).  Append those lines to the body."""
        if not any(re.search(r"jmp\s+(?:word ptr )?cs:\[bx\+", l) for _, l in body):
            return
        if not body or not re.search(r"jmp\s+(?:word ptr )?cs:\[bx\+", body[-1][1]):
            return
        j = end + 1
        tail = []
        while j < len(self.lines):
            raw = self.lines[j]
            l = raw.split(";")[0].rstrip()
            if re.match(r"^\w+\s+proc\b", l) or re.match(r"^\w+\s+(db|dw|dd)\b", l):
                break
            if l.strip():
                tail.append((j, l))
                if re.match(r"^\s+(retf|retn|ret)\b", l):
                    # a separator right after the return ends the routine; anything else is more handler code
                    k = j + 1
                    while k < len(self.lines) and not self.lines[k].strip():
                        k += 1
                    if re.match(r"^; -{20,}", self.lines[k]):
                        k += 1
                        while k < len(self.lines) and not self.lines[k].strip():
                            k += 1
                        if re.match(r"^\s+align\b|^\s+(db|dw|dd)\b", self.lines[k]) or not re.match(r"^\s+\w", self.lines[k]) and not re.match(r"^\w+:", self.lines[k]):
                            break
            j += 1
        body.extend(tail)

    def add_table_labels(self, fname, body):
        """A computed `jmp cs:[bx+K]` dispatches through a table of handler offsets.  IDA leaves some handlers
        unlabelled, so read the real table from the image, work out every instruction's address with objdump and
        add a label (self.synth: body line -> name) at each handler that has none."""
        jl = [i for i, l in body if re.search(r"jmp\s+(?:word ptr )?cs:\[bx\+", l)]
        if not jl:
            return
        import subprocess
        import tempfile
        img = open(os.path.join(os.path.dirname(os.path.abspath(self.asm_path)), "data", "IMAGE.BIN"), "rb").read()
        start = self.func_linear(fname)
        base = self.cur_seg * 16
        targets = set()
        for i in jl:
            line = dict(body)[i]
            k = self.parse_num(re.search(r"\[bx\+(\w+)\]", line).group(1))
            n = tab = None
            for j, l2 in reversed([x for x in body if x[0] < i]):
                m = re.match(r"^\s+mov\s+cx,\s*(\w+)", l2)
                if m and n is None:
                    n = self.parse_num(m.group(1))
                m = re.match(r"^\s+mov\s+bx,\s*(\w+)", l2)
                if m and tab is None:
                    tab = self.parse_num(m.group(1))
                if n is not None and tab is not None:
                    break
            if n is None or tab is None:
                continue
            for t in range(n):
                at = base + tab + k + 2 * t - 0x10000
                targets.add(int.from_bytes(img[at:at + 2], "little"))
        missing = {t for t in targets if t not in self.cur_labels}
        if not missing:
            return
        end = base + max(list(self.cur_labels) + list(missing)) + 0x200
        with tempfile.NamedTemporaryFile(suffix=".bin") as f:
            f.write(img[start - 0x10000:end - 0x10000])
            f.flush()
            dis = subprocess.run(["objdump", "-D", "-b", "binary", "-m", "i8086", "-M", "intel", "--adjust-vma=0x%x" % start, f.name],
                                 capture_output=True, text=True).stdout
        # objdump wraps long instructions onto continuation lines (no mnemonic); an instruction starts at a line with one
        starts = []
        for m in re.finditer(r"^\s*([0-9a-f]+):\t([0-9a-f ]+?)\s*(\t.*)?$", dis, re.M):
            if m.group(3) and m.group(3).strip():
                starts.append(int(m.group(1), 16))
        nxt = {a: b for a, b in zip(starts, starts[1:])}
        cur = start
        for i, l in body:
            m = re.match(r"^(\w+):\s*$", l)
            if m:
                a = self.func_linear(m.group(1))
                if a is not None:
                    cur = a
                continue
            if re.match(r"^\s*(align|assume)", l):
                continue
            off = cur - base
            if off in missing and off not in self.cur_labels:
                self.synth[i] = "ul_%05X" % cur
                self.cur_labels[off] = self.synth[i]
            if cur not in nxt:
                break
            cur = nxt[cur]

    def split_ops(self, rest):
        depth, cur, ops = 0, "", []
        for ch in rest:
            if ch == "[":
                depth += 1
            elif ch == "]":
                depth -= 1
            if ch == "," and depth == 0:
                ops.append(cur)
                cur = ""
            else:
                cur += ch
        if cur.strip():
            ops.append(cur)
        return [o.strip() for o in ops]

    def insn(self, mn, rest, L, kind, translated, lineno):
        ops = self.split_ops(rest)
        P = lambda t, h=None: self.operand(t, L, h)
        if mn in ("nop", "align", "cld", "assume"):
            return []
        if mn == "db" and rest.strip().upper() in ("90H", "0X90"):  # padding nop
            return []
        if mn in ("loop", "loope", "loopne"):
            target = re.sub(r"^(short )", "", ops[0])
            cond = {"loop": "", "loope": " && c->zf", "loopne": " && !c->zf"}[mn]
            return ["c->cx = (uint16_t)(c->cx - 1); if (c->cx != 0%s) goto %s;" % (cond, target)]
        if mn == "les" or mn == "lds":
            d = P(ops[0]); s = P(ops[1], 32)
            seg = "es" if mn == "les" else "ds"
            return [self.wr(d, "%s16(%s)" % (s["seg"], s["addr"])), "c->%s = %s16((uint16_t)(%s + 2));" % (seg, s["seg"], s["addr"])]
        if mn == "mov":
            d = P(ops[0]); sz = self.opsize(d)
            s = P(ops[1], sz)
            if d["kind"] == "seg" or s["kind"] == "seg":
                if d["kind"] == "seg" and d["name"] in ("es", "ds"):
                    return ["c->%s = (uint16_t)(%s);" % (d["name"], self.rd(s, 16))]
                if s["kind"] == "seg" and s["name"] in ("es", "ds", "ss", "cs"):
                    val = {"es": "c->es", "ds": "c->ds", "ss": "STACK_SEG", "cs": "0"}[s["name"]]
                    return [self.wr(d, val)]
                return ["/* segment move %s */" % rest]
            if d["kind"] == "mem" and not d["size"]:
                d["size"] = self.opsize(d, s)  # `mov [si+3], al`: the register decides the width
            return [self.wr(d, self.rd(s, self.opsize(d, s)))]
        if mn in ("add", "sub", "and", "or", "xor", "cmp", "adc", "sbb"):
            d = P(ops[0]); sz = self.opsize(d)
            s = P(ops[1], sz)
            sz = self.opsize(d, s)
            if d["kind"] == "mem" and not d["size"]:
                d["size"] = sz
            fn = {"add": "alu_add", "sub": "alu_sub", "cmp": "alu_sub", "and": "alu_and", "or": "alu_or", "xor": "alu_xor",
                  "adc": "alu_adc", "sbb": "alu_sbb"}[mn]
            expr = "%s(c, %s, %s, %d)" % (fn, self.rd(d, sz), self.rd(s, sz), sz)
            return ["t_ = %s;" % expr] + ([] if mn == "cmp" else [self.wr(d, "t_")])
        if mn == "test":
            d = P(ops[0]); sz = self.opsize(d)
            s = P(ops[1], sz)
            sz = self.opsize(d, s)
            return ["t_ = alu_and(c, %s, %s, %d); (void)t_;" % (self.rd(d, sz), self.rd(s, sz), sz)]
        if mn in ("inc", "dec"):
            d = P(ops[0]); sz = self.opsize(d)
            return ["t_ = alu_%s(c, %s, %d);" % (mn, self.rd(d, sz), sz), self.wr(d, "t_")]
        if mn == "neg":
            d = P(ops[0]); sz = self.opsize(d)
            return ["t_ = alu_sub(c, 0, %s, %d);" % (self.rd(d, sz), sz), self.wr(d, "t_")]
        if mn in ("rcl", "rcr", "rol", "ror"):
            d = P(ops[0]); sz = self.opsize(d)
            cnt = "c->cx & 0xFF" if ops[1] == "cl" else str(self.parse_num(ops[1]))
            return ["t_ = alu_%s(c, %s, %s, %d);" % (mn, self.rd(d, sz), cnt, sz), self.wr(d, "t_")]
        if mn in ("shl", "sal", "shr", "sar"):
            d = P(ops[0]); sz = self.opsize(d)
            cnt = "c->cx & 0xFF" if ops[1] == "cl" else str(self.parse_num(ops[1]))
            fn = {"shl": "alu_shl", "sal": "alu_shl", "shr": "alu_shr", "sar": "alu_sar"}[mn]
            return ["t_ = %s(c, %s, %s, %d);" % (fn, self.rd(d, sz), cnt, sz), self.wr(d, "t_")]
        if mn == "xchg":
            a = P(ops[0]); b = P(ops[1]); sz = self.opsize(a, b)
            return ["{ uint32_t x_ = %s, y_ = %s; %s %s }" % (self.rd(a, sz), self.rd(b, sz), self.wr(a, "y_"), self.wr(b, "x_"))]
        if mn == "cbw":
            return ["c->ax = (uint16_t)(int16_t)(int8_t)(c->ax & 0xFF);"]
        if mn == "cwd":
            return ["c->dx = (c->ax & 0x8000) ? 0xFFFF : 0;"]
        if mn == "imul":
            s = P(ops[0]); sz = self.opsize(s)
            if sz != 16:
                raise ValueError("8-bit imul")
            return ["{ int32_t r_ = (int32_t)(int16_t)c->ax * (int32_t)(int16_t)%s; c->ax = (uint16_t)r_; c->dx = (uint16_t)((uint32_t)r_ >> 16); "
                    "c->cf = c->of = (r_ != (int16_t)r_); }" % self.rd(s, 16)]
        if mn == "mul":
            s = P(ops[0]); sz = self.opsize(s)
            if sz == 16:
                return ["{ uint32_t r_ = (uint32_t)c->ax * (uint32_t)%s; c->ax = (uint16_t)r_; c->dx = (uint16_t)(r_ >> 16); c->cf = c->of = (c->dx != 0); }" % self.rd(s, 16)]
            return ["{ uint16_t r_ = (uint16_t)((c->ax & 0xFF) * %s); c->ax = r_; c->cf = c->of = ((r_ >> 8) != 0); }" % self.rd(s, 8)]
        if mn == "idiv":
            s = P(ops[0]); sz = self.opsize(s)
            if sz != 16:
                raise ValueError("8-bit idiv")
            return ["{ int32_t n_ = (int32_t)(((uint32_t)c->dx << 16) | c->ax); int16_t d_ = (int16_t)%s; "
                    "c->ax = (uint16_t)(n_ / d_); c->dx = (uint16_t)(n_ %% d_); }" % self.rd(s, 16)]
        if mn == "div":
            s = P(ops[0]); sz = self.opsize(s)
            if sz != 16:
                raise ValueError("8-bit div")
            return ["{ uint32_t n_ = ((uint32_t)c->dx << 16) | c->ax; uint16_t d_ = %s; c->ax = (uint16_t)(n_ / d_); c->dx = (uint16_t)(n_ %% d_); }" % self.rd(s, 16)]
        if mn == "push":
            if ops[0] in ("cs", "ds", "es", "ss"):
                val = {"cs": "0", "ds": "c->ds", "es": "c->es", "ss": "STACK_SEG"}[ops[0]]
                return ["PUSH(c, %s); /* push %s */" % (val, ops[0])]
            s = P(ops[0], 16)
            return ["PUSH(c, %s);" % self.rd(s, 16)]
        if mn == "pop":
            d = P(ops[0])
            if d["kind"] == "seg":
                if d["name"] in ("es", "ds"):
                    return ["c->%s = POP(c);" % d["name"]]
                return ["POP(c); /* pop %s */" % ops[0]]
            return [self.wr(d, "POP(c)")]
        if mn == "lea":
            d = P(ops[0]); s = P(ops[1])
            return [self.wr(d, s["addr"])]
        if mn in ("retf", "retn", "ret"):
            return ["c->sp += %d; return;" % (4 if mn == "retf" else 2)]
        if mn == "jmp":
            m2 = re.match(r"^(?:word ptr )?cs:\[(.*)\]$", ops[0])
            if m2 and not ops[0].startswith("cs:jpt"):
                o = P(ops[0])
                cases = "".join("case 0x%X: goto %s;" % (off, lab) for off, lab in sorted(self.cur_labels.items()))
                return ["{ uint16_t jt_ = CS16(0x%X, %s); switch (jt_) { %s default: recomp_bad_jump(\"%s\", 0x%X, jt_); } }" % (self.cur_seg, o["addr"], cases, self.cur_fname, self.cur_seg)]
            m = re.match(r"^cs:(jpt_\w+)\[bx\]$", ops[0])
            if m:
                tab = self.jpt[m.group(1)]
                cases = "".join("case %d: goto %s;" % (i, t) for i, t in enumerate(tab))
                return ["switch (c->bx >> 1) { %s default: abort(); }" % cases]
            target = re.sub(r"^(short |near ptr |far ptr )", "", ops[0])
            if target == "$+2":  # short jump to the next instruction: a no-op
                return []
            if target in self.procs:
                return ["fn_%s(c); return;" % target] if target in translated else ["host_%s(c); c->sp += %d; return;" % (target, 4 if kind == "far" else 2)]
            return ["goto %s;" % target]
        cond = {"jz": "c->zf", "je": "c->zf", "jnz": "!c->zf", "jne": "!c->zf", "jl": "(c->sf != c->of)", "jnge": "(c->sf != c->of)",
                "jge": "(c->sf == c->of)", "jnl": "(c->sf == c->of)", "jle": "(c->zf || c->sf != c->of)", "jng": "(c->zf || c->sf != c->of)",
                "jg": "(!c->zf && c->sf == c->of)", "jnle": "(!c->zf && c->sf == c->of)", "jb": "c->cf", "jc": "c->cf", "jnae": "c->cf",
                "jae": "!c->cf", "jnb": "!c->cf", "jnc": "!c->cf", "jbe": "(c->cf || c->zf)", "jna": "(c->cf || c->zf)",
                "ja": "(!c->cf && !c->zf)", "jnbe": "(!c->cf && !c->zf)", "js": "c->sf", "jns": "!c->sf"}.get(mn)
        if cond:
            target = re.sub(r"^(short )", "", ops[0])
            return ["if (%s) goto %s;" % (cond, target)]
        if mn == "call":
            target = re.sub(r"^(near ptr |far ptr )", "", ops[0])
            if target.startswith("j_") and target[2:] in translated:  # overlay thunk of a routine we translate
                target = target[2:]
            if target not in self.procs and not re.match(r"^[\w@]+$", target):
                raise ValueError("indirect call")
            target_c = target.replace("@", "_AT")
            far_call = "far ptr" in ops[0] or (("near ptr" not in ops[0]) and self.procs.get(target, [0, 0, "near"])[2] == "far")
            if target not in translated:
                return ["host_%s(c); /* no return address is pushed for host routines: args start at sp */" % target_c]
            pushes = ["PUSH(c, 0); PUSH(c, 0);"] if far_call else ["PUSH(c, 0);"]
            return pushes + ["fn_%s(c);" % target]
        raise ValueError("unsupported instruction " + mn)


def main():
    asm, idc, out = sys.argv[1:4]
    funcs = sys.argv[4:]
    tr = Translator(asm, idc)
    for f in funcs:
        if f not in tr.procs:
            raise SystemExit("no such function: " + f)
    ordered = list(funcs)
    code = [f"/* Generated by tools/asm2c.py from mm3.asm -- do not edit.  Functions: {', '.join(funcs)} */",
            '#include "../recomp.h"', ""]
    # prototypes (host functions are declared in recomp.h or the host file)
    for f in ordered:
        code.append("static void fn_%s(Cpu *c);" % f)
    code.append("")
    body = []
    hosts = set()
    for f in ordered:
        text = tr.translate(f, set(ordered))
        hosts.update(re.findall(r"host_(\w+)\(c\)", text))
        body.append(text)
        body.append("")
    weak = os.environ.get("ASM2C_WEAK_HOSTS")
    for h in sorted(hosts):
        if weak:  # unimplemented hosts report themselves instead of failing to link
            code.append('void __attribute__((weak)) host_%s(Cpu *c) { recomp_unimplemented("%s", c); }' % (h, h))
        else:
            code.append("void host_%s(Cpu *c);" % h)
    code.append("")
    code.extend(body)
    for f in ordered:
        code.append("%svoid call_%s(Cpu *c) { fn_%s(c); }" % ("static " if os.environ.get("ASM2C_STATIC_CALLS") else "", f, f))
    code.append("")
    code.append("/* name -> entry point, for test drivers */")
    code.append("const RecompEntry recomp_entries_%s[] = {" % re.sub(r"\W", "_", out.split("/")[-1].rsplit(".", 1)[0]))
    for f in ordered:
        code.append('\t{"%s", fn_%s},' % (f, f))
    code.append("\t{0, 0}")
    code.append("};")
    open(out, "w").write("\n".join(code) + "\n")
    print("wrote %s (%d functions, host calls: %s)" % (out, len(ordered), ", ".join(sorted(hosts)) or "none"))


if __name__ == "__main__":
    main()
