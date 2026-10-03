"""Tiny 16-bit x86 subset interpreter for IDA listings of simple leaf routines.

Supports what RollCharacterAttributes / ComputeDerivedCharacterStats use. The record
is a bytearray addressed through [si+X]. Calls are dispatched to python callbacks.
"""
import re

M = 0xFFFF


def parse_proc(path, name):
    lines = open(path, encoding='latin-1').read().split('\n')
    out = []
    on = False
    for l in lines:
        if re.match(r'^' + name + r'\s+proc', l):
            on = True
        if on:
            code = l.split(';')[0].rstrip()
            out.append(code)
        if on and re.match(r'^' + name + r'\s+endp', l):
            break
    return out


def num(tok):
    tok = tok.strip()
    if tok.endswith('h'):
        return int(tok[:-1], 16)
    return int(tok)


class Emu:
    def __init__(self, listing, record, calls):
        self.rec = record
        self.calls = calls
        self.regs = {'ax': 0, 'bx': 0, 'cx': 0, 'dx': 0}
        self.zf = False
        self.sf = False
        self.of = False
        self.cf = False
        # build instruction list and label map
        self.ins = []
        self.labels = {}
        for l in listing:
            s = l.strip()
            if not s:
                continue
            m = re.match(r'^(\w+):$', s)
            if m:
                self.labels[m.group(1)] = len(self.ins)
                continue
            if re.match(r'^(\w+)\s+(proc|endp)', s):
                continue
            if s.startswith(';'):
                continue
            # label with instruction on same line? (not in IDA)
            self.ins.append(s)

    def rd(self, off):
        return self.rec[off] | (self.rec[off + 1] << 8)

    def wr(self, off, v):
        v &= M
        self.rec[off] = v & 0xFF
        self.rec[off + 1] = v >> 8

    def operand(self, s):
        s = s.strip()
        m = re.match(r'^(?:word ptr )?\[si\+([0-9A-Fa-f]+h)\]$', s)
        if m:
            return ('mem', num(m.group(1)))
        if s in self.regs:
            return ('reg', s)
        return ('imm', num(s) & M)

    def get(self, op):
        k, v = op
        if k == 'mem':
            return self.rd(v)
        if k == 'reg':
            return self.regs[v]
        return v

    def put(self, op, val):
        k, v = op
        val &= M
        if k == 'mem':
            self.wr(v, val)
        else:
            self.regs[v] = val

    def setflags_sub(self, a, b):
        r = (a - b) & M
        self.zf = r == 0
        self.sf = bool(r & 0x8000)
        sa, sb = bool(a & 0x8000), bool(b & 0x8000)
        self.of = (sa != sb) and (self.sf != sa)
        self.cf = a < b

    def run(self):
        pc = 0
        steps = 0
        while pc < len(self.ins):
            steps += 1
            assert steps < 10000
            s = self.ins[pc]
            pc += 1
            parts = s.split(None, 1)
            op = parts[0]
            args = [a.strip() for a in parts[1].split(',')] if len(parts) > 1 else []
            if op == 'mov':
                self.put(self.operand(args[0]), self.get(self.operand(args[1])))
            elif op == 'add':
                d = self.operand(args[0])
                self.put(d, self.get(d) + self.get(self.operand(args[1])))
            elif op == 'cmp':
                self.setflags_sub(self.get(self.operand(args[0])), self.get(self.operand(args[1])))
            elif op == 'jz':
                if self.zf:
                    pc = self.labels[args[0].replace('short ', '')]
            elif op == 'jnz':
                if not self.zf:
                    pc = self.labels[args[0].replace('short ', '')]
            elif op == 'jmp':
                pc = self.labels[args[0].replace('short ', '')]
            elif op == 'jge':
                if self.sf == self.of:
                    pc = self.labels[args[0].replace('short ', '')]
            elif op == 'jl':
                if self.sf != self.of:
                    pc = self.labels[args[0].replace('short ', '')]
            elif op == 'jg':
                if (not self.zf) and self.sf == self.of:
                    pc = self.labels[args[0].replace('short ', '')]
            elif op == 'jle':
                if self.zf or self.sf != self.of:
                    pc = self.labels[args[0].replace('short ', '')]
            elif op == 'dec':
                d = self.operand(args[0])
                self.put(d, self.get(d) - 1)
            elif op == 'shr':
                d = self.operand(args[0])
                self.put(d, self.get(d) >> 1)
            elif op == 'mul':
                v = self.get(self.operand(args[0].replace('word ptr ', '')))
                r = self.regs['ax'] * v
                self.regs['ax'] = r & M
                self.regs['dx'] = (r >> 16) & M
            elif op == 'test':
                self.zf = (self.get(self.operand(args[0])) & self.get(self.operand(args[1]))) == 0
            elif op == 'push':
                self.stack = getattr(self, 'stack', [])
                self.stack.append(self.get(self.operand(args[0])))
            elif op == 'pop':
                self.put(self.operand(args[0]), self.stack.pop())
            elif op == 'call':
                self.calls[args[0]](self)
            elif op == 'retn' or op == 'retf':
                return
            else:
                raise Exception('unsupported: ' + s)


def scale_pct(emu):
    ax, bx = emu.regs['ax'], emu.regs['bx']
    r = (ax * bx) & M
    r = (r + 50) & M
    emu.regs['ax'] = r // 100
    emu.regs['dx'] = 0
