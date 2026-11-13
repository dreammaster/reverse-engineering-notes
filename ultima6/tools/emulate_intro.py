#!/usr/bin/env python3
"""Runs the introduction routine of MARTIAN.EXE (sub_19266 of md/md_martian.asm) on a small interpreter of its 16-bit code and records the calls it makes to the drawing driver.

    emulate_intro.py md/md_martian.asm md/game/MARTIAN/MARTIAN.EXE md/intro_ops.json

The routine is a straight script: it loads pictures (sub_15177: file name offset, item), draws shapes (dword_2813A), writes captions (sub_152B5), waits (sub_15610) and fades. The interpreter knows only
the few instructions it uses; the calls are not run but recorded with their arguments (handles of loaded pictures and the numbers of the caption lines are given as tagged values).
"""
import json
import re
import sys

ASM, EXE, OUT = sys.argv[1:4]
FUNC = sys.argv[4] if len(sys.argv) > 4 else 'sub_19266'
PRESET = dict(a.split('=') for a in sys.argv[5:])  # globals to set before the run, e.g. word_29B98=0x157
RET = {'sub_11AB3': 1, 'sub_11947': 1}  # file calls: a read gets its item, an open a handle
exe = open(EXE, 'rb').read()
DS = 0x186A0

lines = open(ASM, encoding='utf-8', errors='replace').read().split('\n')
start = next(i for i, l in enumerate(lines) if re.match(FUNC + r'\s+proc', l))
end = next(i for i in range(start, len(lines)) if re.match(FUNC + r'\s+endp', lines[i]))

varoff = {}
instrs = []
labels = {}
for l in lines[start:end]:
    code = l.split(';')[0].rstrip()
    if not code.strip():
        continue
    m = re.match(r'\s*(var_[0-9A-F]+)\s*=\s*(?:word|byte|dword) ptr\s+(-?[0-9A-F]+)h?$', code)
    if m:
        v = int(m.group(2).replace('-', ''), 16)
        varoff[m.group(1)] = -v if '-' in m.group(2) else v
        continue
    m = re.match(r'\s*(arg_[0-9A-F]+)\s*=\s*word ptr\s+([0-9A-F]+)h?$', code)
    if m:
        varoff[m.group(1)] = int(m.group(2), 16)
        continue
    m = re.match(r'(loc_[0-9A-F]+):', code.strip())
    if m:
        labels[m.group(1)] = len(instrs)
        continue
    if re.match(r'\s*' + FUNC + r'\s+proc', code):
        continue
    s = code.strip()
    if not s:
        continue
    op, _, rest = s.partition(' ')
    instrs.append((op, rest.strip()))

mem = bytearray(0x10000)  # the stack segment
bss = {}
regs = {'ax': 0, 'bx': 0, 'cx': 0, 'dx': 0, 'si': 0, 'di': 0, 'bp': 0xF000, 'sp': 0xF000}
flags = {'z': 0, 's': 0, 'c': 0, 'o': 0}
BASE = 0xF000


def num(t):
    t = t.strip()
    m = re.match(r'^(-?)([0-9A-F]+)h$', t)
    if m:
        v = int(m.group(2), 16)
        return -v if m.group(1) else v
    if re.match(r'^-?[0-9]+$', t):
        return int(t)
    return None


def s16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


def ea(operand):
    """The address and size of a memory operand."""
    size = 2
    t = operand
    m = re.match(r'(byte|word) ptr (.*)', t)
    if m:
        size = 1 if m.group(1) == 'byte' else 2
        t = m.group(2)
    m = re.match(r'\[(.*)\]$', t)
    if m:
        parts = m.group(1).split('+')
        addr = 0
        for p in parts:
            p = p.strip()
            if p in regs:
                addr += regs[p]
            elif p in varoff:
                addr += varoff[p]
            elif num(p) is not None:
                addr += num(p)
            else:
                raise Exception('operand ' + operand)
        return ('bp' if 'bp' in m.group(1) else 'x', addr & 0xFFFF if not ('bp' in m.group(1)) else addr, size)
    m = re.match(r'(byte|word|dword)_([0-9A-F]+)$', t)
    if m:
        return ('g', t, 1 if m.group(1) == 'byte' else 2)
    raise Exception('operand ' + operand)


def rd_mem(kind, addr, size):
    if kind == 'g':
        return bss.get(addr, 0)
    if kind == 'bp' or (kind == 'x' and addr >= 0xE000):
        a = addr if kind == 'bp' else addr
        if kind == 'bp':
            a = addr
        return mem[a] if size == 1 else mem[a] | (mem[a + 1] << 8)
    base = DS + addr
    return exe[base] if size == 1 else exe[base] | (exe[base + 1] << 8)


def wr_mem(kind, addr, size, v):
    if kind == 'g':
        bss[addr] = v & (0xFF if size == 1 else 0xFFFF)
        return
    a = addr
    mem[a] = v & 0xFF
    if size == 2:
        mem[a + 1] = (v >> 8) & 0xFF


def getreg(r):
    if r in regs:
        return regs[r]
    if r == 'al':
        return regs['ax'] & 0xFF
    if r == 'ah':
        return regs['ax'] >> 8
    if r == 'bl':
        return regs['bx'] & 0xFF
    if r == 'dl':
        return regs['dx'] & 0xFF
    if r == 'cl':
        return regs['cx'] & 0xFF
    raise Exception('reg ' + r)


def setreg(r, v):
    if r in regs:
        regs[r] = v & 0xFFFF
    elif r == 'al':
        regs['ax'] = (regs['ax'] & 0xFF00) | (v & 0xFF)
    elif r == 'ah':
        regs['ax'] = (regs['ax'] & 0xFF) | ((v & 0xFF) << 8)
    elif r == 'dl':
        regs['dx'] = (regs['dx'] & 0xFF00) | (v & 0xFF)
    elif r == 'bl':
        regs['bx'] = (regs['bx'] & 0xFF00) | (v & 0xFF)
    elif r == 'cl':
        regs['cx'] = (regs['cx'] & 0xFF00) | (v & 0xFF)
    else:
        raise Exception('reg ' + r)


def get(operand):
    operand = operand.strip()
    if operand in regs or operand in ('al', 'ah', 'bl', 'dl', 'cl'):
        return getreg(operand)
    n = num(operand)
    if n is not None:
        return n & 0xFFFF
    k, a, sz = ea(operand)
    return rd_mem(k, a, sz)


def put(operand, v):
    operand = operand.strip()
    if operand in regs or operand in ('al', 'ah', 'bl', 'dl', 'cl'):
        setreg(operand, v)
        return
    k, a, sz = ea(operand)
    wr_mem(k, a, sz, v)


def push(v):
    regs['sp'] -= 2
    mem[regs['sp']] = v & 0xFF
    mem[regs['sp'] + 1] = (v >> 8) & 0xFF


def pop():
    v = mem[regs['sp']] | (mem[regs['sp'] + 1] << 8)
    regs['sp'] += 2
    return v


def setf(res, a=None, b=None, sub=False, width=16):
    r = res & 0xFFFF
    flags['z'] = int(r == 0)
    flags['s'] = int(r >> 15)
    if a is not None:
        if sub:
            flags['c'] = int((a & 0xFFFF) < (b & 0xFFFF))
            flags['o'] = int(((a ^ b) & (a ^ r)) >> 15 & 1)
        else:
            flags['c'] = int(res > 0xFFFF)
            flags['o'] = 0


randn = [0]
ARITY = {'sub_1E59F': 2, 'sub_151F3': 3, 'sub_15177': 2, 'sub_15610': 1, 'dword_2814E': 2, 'dword_2813A': 6, 'dword_2814A': 2, 'sub_152B5': 7, 'sub_234C5': 2, 'sub_155DB': 0, 'sub_155A5': 0,
         'sub_218FC': 1, 'sub_21EB8': 2, 'dword_2817A': 9, 'dword_28172': 5, 'sub_20951': 6, 'sub_20D32': 3, 'sub_132A6': 0, 'sub_1313C': 0}
NAMES = {}
for l in lines:
    pass

ops = []
handles = {}
texts = {}
tagcount = [0]
last_push_cs = [False]


def record(name, args):
    ops.append({'call': name, 'args': args})


def handle_arg(v_off):
    return handles.get(v_off, ('?', v_off))


for k, v in PRESET.items():
    bss[k] = int(v, 0)
ip = 0
steps = 0
while ip < len(instrs) and steps < 2000000:
    steps += 1
    op, rest = instrs[ip]
    ip += 1
    if op == 'retf':
        break
    args = [a.strip() for a in re.split(r',(?![^\[]*\])', rest)] if rest else []
    if op == 'push':
        if args[0] == 'cs':
            last_push_cs[0] = True
            push(0xC5C5)
        elif args[0] in ('ds', 'ss', 'es'):
            push(0)
        else:
            push(get(args[0]))
    elif op == 'pop':
        put(args[0], pop())
    elif op == 'mov':
        if args[0].startswith('word ptr') or args[0].startswith('byte ptr') or args[0].startswith('['):
            put(args[0], get(args[1]))
        else:
            v = get(args[1])
            if args[0] in ('al', 'ah', 'bl', 'dl', 'cl'):
                v &= 0xFF
            put(args[0], v)
    elif op == 'xor':
        a, b = get(args[0]), get(args[1])
        put(args[0], a ^ b)
        setf(a ^ b)
        flags['c'] = flags['o'] = 0
    elif op == 'or':
        a, b = get(args[0]), get(args[1])
        put(args[0], a | b)
        setf(a | b)
        flags['c'] = flags['o'] = 0
    elif op == 'and':
        a, b = get(args[0]), get(args[1])
        put(args[0], a & b)
        setf(a & b)
    elif op == 'add':
        if args[0] == 'sp':
            regs['sp'] += get(args[1])
        else:
            a, b = get(args[0]), get(args[1])
            put(args[0], a + b)
            setf(a + b, a, b)
    elif op == 'sub':
        if args[0] == 'sp':
            regs['sp'] -= get(args[1])
        else:
            a, b = get(args[0]), get(args[1])
            put(args[0], a - b)
            setf(a - b, a, b, sub=True)
    elif op == 'inc':
        if args[0] == 'sp':
            regs['sp'] += 1
        else:
            a = get(args[0])
            put(args[0], a + 1)
            setf(a + 1)
    elif op == 'dec':
        a = get(args[0])
        put(args[0], a - 1)
        setf(a - 1)
    elif op == 'cmp':
        a, b = get(args[0]), get(args[1])
        if args[0].startswith('byte') or args[0] in ('al', 'ah'):
            a, b = a & 0xFF, b & 0xFF
        setf(a - b, a, b, sub=True)
    elif op == 'shl':
        a = get(args[0])
        n = get(args[1]) if args[1] != '1' else 1
        put(args[0], a << n)
    elif op == 'neg':
        a = get(args[0])
        put(args[0], -a)
        setf(-a)
        flags['c'] = int(a != 0)
    elif op == 'sbb':
        a, b = get(args[0]), get(args[1])
        r = a - b - flags['c']
        put(args[0], r)
        setf(r, a, b, sub=True)
    elif op == 'sar':
        a = s16(get(args[0]))
        put(args[0], a >> (get(args[1]) if args[1] != '1' else 1))
    elif op == 'shr':
        a = get(args[0])
        put(args[0], a >> (get(args[1]) if args[1] != '1' else 1))
    elif op == 'cwd':
        regs['dx'] = 0xFFFF if regs['ax'] & 0x8000 else 0
    elif op == 'imul':
        r = s16(regs['ax']) * s16(get(args[0]))
        regs['ax'] = r & 0xFFFF
        regs['dx'] = (r >> 16) & 0xFFFF
    elif op == 'idiv':
        n = (s16(regs['dx']) << 16) | regs['ax']
        d = s16(get(args[0]))
        q = int(n / d)
        regs['ax'] = q & 0xFFFF
        regs['dx'] = (n - q * d) & 0xFFFF
    elif op == 'lea':
        k, a, sz = ea(args[1])
        put(args[0], a)
    elif op == 'jmp':
        ip = labels[args[0].replace('short ', '')]
    elif op in ('jz', 'jnz', 'jl', 'jle', 'jg', 'jge', 'jb', 'jbe', 'ja', 'jnb', 'jae', 'js', 'jns'):
        t = args[0].replace('short ', '')
        z, s, c, o = flags['z'], flags['s'], flags['c'], flags['o']
        cond = {'jz': z, 'jnz': not z, 'jl': s != o, 'jle': z or s != o, 'jg': (not z) and s == o, 'jge': s == o, 'jb': c, 'jbe': c or z, 'ja': (not c) and (not z), 'jnb': not c, 'jae': not c,
                'js': s, 'jns': not s}[op]
        if cond:
            ip = labels[t]
    elif op == 'call':
        target = re.sub(r'^(near ptr|far ptr)\s+', '', rest)
        if last_push_cs[0] and target.startswith('sub_'):
            pop()  # the segment the callee returns through
        last_push_cs[0] = False
        n = ARITY.get(target)
        if n is None:
            # the arguments are the words the caller takes off after the call (add sp, N or pairs of inc sp)
            n = 0
            k = ip
            if k < len(instrs) and instrs[k][0] == 'add' and instrs[k][1].startswith('sp,'):
                n = int(instrs[k][1].split(',')[1].strip().rstrip('h') or '0', 16 if instrs[k][1].strip().endswith('h') else 10) // 2
            else:
                while k + 1 < len(instrs) and instrs[k][0] == 'inc' and instrs[k][1] == 'sp' and instrs[k + 1][0] == 'inc' and instrs[k + 1][1] == 'sp':
                    n += 1
                    k += 2
            raw = []
            for q in range(n):
                a = regs['sp'] + 2 * q
                raw.append(mem[a] | (mem[a + 1] << 8))
            raw.reverse()
            ops.append({'call': 'other', 'fn': target, 'args': [s16(v) for v in raw]})
            regs['ax'] = RET.get(target, 0)
            regs['dx'] = 0
            continue
        # the arguments in the order they were pushed
        raw = []
        for k in range(n):
            a = regs['sp'] + 2 * k
            raw.append(mem[a] | (mem[a + 1] << 8))
        raw.reverse()  # first pushed first
        if target == 'sub_15177':
            tagcount[0] += 1
            handles[0x4000 + tagcount[0]] = {'item': raw[0], 'file': raw[1]}
            ops.append({'call': 'load', 'item': raw[0], 'file': raw[1], 'tag': 0x4000 + tagcount[0]})
            regs['ax'] = 0x4000 + tagcount[0]
            regs['dx'] = 0x5000
        elif target == 'sub_151F3':
            # a text file's lines are put in the array whose address is the first pushed argument
            buf = raw[0]
            item, name = raw[1], raw[2]
            ops.append({'call': 'text', 'item': item, 'file': name})
            for i in range(0, 64):
                tag = 0x2000 + item * 0x100 + i
                a = buf + 4 * i
                mem[a] = tag & 0xFF
                mem[a + 1] = tag >> 8
                mem[a + 2] = 0x00
                mem[a + 3] = 0x30
            regs['ax'] = buf
            regs['dx'] = 0x5100
        elif target == 'sub_15610':
            ops.append({'call': 'wait', 'ms': raw[0]})
            regs['ax'] = 0
        elif target == 'dword_2813A':
            # pushed: frame, seg, off, y, x, dest   (the driver takes dest first)
            ops.append({'call': 'draw', 'frame': s16(raw[0]), 'handle': raw[2], 'y': s16(raw[3]), 'x': s16(raw[4])})
        elif target == 'sub_152B5':
            # pushed: colour, font, seg, off, y, x, dest
            ops.append({'call': 'caption', 'text': raw[3], 'y': s16(raw[4]), 'x': s16(raw[5])})
        elif target == 'dword_2814E':
            ops.append({'call': 'clear'})
        elif target == 'dword_2814A':
            ops.append({'call': 'present'})
        elif target == 'sub_234C5':
            ops.append({'call': 'free', 'handle': raw[1]})
        elif target == 'sub_155DB':
            ops.append({'call': 'fadein'})
        elif target == 'sub_155A5':
            ops.append({'call': 'fadeout'})
        elif target in ('sub_218FC', 'sub_21EB8'):
            ops.append({'call': 'music', 'fn': target, 'args': [s16(v) for v in raw]})
        elif target in ('dword_2817A', 'dword_28172', 'sub_20951', 'sub_20D32'):
            ops.append({'call': target, 'args': [s16(v) for v in raw]})
        elif target == 'sub_1E59F':
            # rand(a, b) (the routine takes its two arguments off the stack itself); a fixed series stands for the random numbers
            lo, hi = sorted((raw[0], raw[1]))
            randn[0] += 1
            regs['ax'] = lo + (randn[0] * 7919 + 13) % (hi - lo + 1)
            regs['sp'] += 4
        elif target in ('sub_132A6', 'sub_1313C'):
            regs['ax'] = 0
        else:
            raise Exception(target)
    elif op in ('nop',):
        pass
    else:
        raise Exception('instruction %s %s' % (op, rest))

json.dump({'ops': ops, 'handles': {str(k): v for k, v in handles.items()}}, open(OUT, 'w'), indent=0)
print(len(ops), 'ops,', steps, 'steps')
