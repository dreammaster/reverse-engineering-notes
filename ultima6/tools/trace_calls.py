#!/usr/bin/env python3
"""Turns the disassembly of one routine (md/md_martian.asm) into a trace of its calls with the arguments that were pushed.

    trace_calls.py md/md_martian.asm sub_19266 > md/intro_trace.txt

Pushes of `mov ax, N / push ax` and `push [bp+var]` are collected until the next call; `add sp, N` ends the group. Labels and jumps are printed so that the loops stay visible.
"""
import re
import sys

asm = open(sys.argv[1], encoding='utf-8', errors='replace').read().split('\n')
name = sys.argv[2]
start = next(i for i, l in enumerate(asm) if re.match(r'%s\s+proc' % name, l))
end = next(i for i in range(start, len(asm)) if re.match(r'%s\s+endp' % name, l := asm[i]))
stack = []
ax = None
al = None
for l in asm[start:end]:
    l = l.split(';')[0].rstrip()
    if not l.strip():
        continue
    s = l.strip()
    m = re.match(r'(loc_[0-9A-F]+):$', s)
    if m:
        print(s)
        continue
    m = re.match(r'mov\s+ax, ([0-9A-Fa-f]+h?|0)$', s)
    if m:
        ax = m.group(1)
        continue
    m = re.match(r'mov\s+ax, (\S+)$', s)
    if m:
        ax = m.group(1)
        continue
    m = re.match(r'mov\s+al, (\S+)$', s)
    if m:
        ax = m.group(1)
        continue
    if re.match(r'xor\s+ax, ax$', s):
        ax = '0'
        continue
    m = re.match(r'push\s+(\S+)$', s)
    if m:
        v = m.group(1)
        if v == 'ax':
            v = ax if ax is not None else 'ax'
        if v in ('cs', 'ds', 'ss', 'es'):
            continue
        stack.append(v)
        continue
    m = re.match(r'call\s+(?:near ptr |far ptr )?(\S+)$', s)
    if m:
        print('    %s(%s)' % (m.group(1), ', '.join(stack)))
        stack = []
        ax = None
        continue
    if re.match(r'add\s+sp, ', s):
        continue
    if re.match(r'(jmp|jz|jnz|jl|jle|jg|jge|jb|jbe|ja|jae|jnb|jns|js|loop)\b', s):
        print('  ' + s)
        stack = []
        continue
    if re.match(r'(cmp|test|or|and|inc|dec|mov|lea|sub|add|shl|shr|imul|cwd|idiv|div|neg|xchg|les|lds)\b', s):
        if re.match(r'(cmp|test)\b', s) or re.match(r'mov\s+(\[|byte ptr|word ptr)', s) or re.match(r'(inc|dec)\b', s):
            print('  ' + s)
        continue
