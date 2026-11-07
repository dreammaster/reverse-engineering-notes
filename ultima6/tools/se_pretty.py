#!/usr/bin/env python3
"""Shortens the disassembly dumps of se/se_use_subs.txt: the idioms of the object arrays become expressions.

    se_pretty.py se/se_use_subs.txt 34D88 [34EDF ...]

  mov bx, R / shl bx, 1 / mov ax, [bx-6306h] / and ax, 3FFh      ->  ax = TYPE(R)
  ... mov cl, 0Ah / shr ax, cl                                  ->  ax = FRAME(R)
  mov ax, R / mov dx, 3 / imul dx / mov bx, ax / mov ax, [bx+78E4h] / and ax, 3FFh  ->  ax = X(R)  (78E5: Y, 78E6: Z)
"""
import re
import sys


def load(path, name):
    out = []
    on = False
    for line in open(path, encoding='utf-8', errors='replace'):
        line = line.rstrip('\n')
        if line.startswith('=== '):
            on = line.startswith('=== sub_%s ' % name)
            if on:
                out.append(line)
            continue
        if on:
            out.append(line)
    return out


def ops(lines):
    res = []
    for l in lines:
        m = re.match(r'([0-9A-F]{5}) (.*)', l)
        res.append((m.group(1), m.group(2)) if m else ('', l))
    return res


def pretty(lines):
    L = ops(lines)
    out = []
    i = 0

    def t(k):
        return L[i + k][1] if i + k < len(L) else ''

    while i < len(L):
        a, s = L[i]
        # TYPE / FRAME
        m = re.match(r'mov\s+bx, (\w+|\[bp\+\w+\]|word_\w+)$', s)
        if m and re.match(r'shl\s+bx, 1$', t(1)) and re.match(r'mov\s+(ax|dx), \[bx-6306h\]$', t(2)):
            reg = re.match(r'mov\s+(ax|dx), ', t(2)).group(1)
            if re.match(r'and\s+%s, 3FFh$' % reg, t(3)):
                out.append('%s %s = TYPE(%s)' % (a, reg, m.group(1)))
                i += 4
                continue
            if re.match(r'mov\s+cl, 0Ah$', t(3)) and re.match(r'shr\s+%s, cl$' % reg, t(4)):
                out.append('%s %s = FRAME(%s)' % (a, reg, m.group(1)))
                i += 5
                continue
            out.append('%s %s = WORD(%s)   ; type | frame << 10' % (a, reg, m.group(1)))
            i += 3
            continue
        # X / Y / Z
        m = re.match(r'mov\s+ax, (\w+|\[bp\+\w+\]|word_\w+)$', s)
        if m and re.match(r'mov\s+dx, 3$', t(1)) and re.match(r'imul\s+dx$', t(2)) and re.match(r'mov\s+bx, ax$', t(3)):
            n = t(4)
            r = m.group(1)
            if re.match(r'mov\s+ax, \[bx\+78E4h\]$', n) and re.match(r'and\s+ax, 3FFh$', t(5)):
                out.append('%s ax = X(%s)' % (a, r))
                i += 6
                continue
            if re.match(r'mov\s+ax, \[bx\+78E5h\]$', n) and re.match(r'sar\s+ax, 1$', t(5)) and re.match(r'sar\s+ax, 1$', t(6)) and re.match(r'and\s+ax, 3FFh$', t(7)):
                out.append('%s ax = Y(%s)' % (a, r))
                i += 8
                continue
            if re.match(r'mov\s+al, \[bx\+78E6h\]$', n) and re.match(r'mov\s+ah, 0$', t(5)) and re.match(r'mov\s+cl, 4$', t(6)) and re.match(r'sar\s+ax, cl$', t(7)):
                out.append('%s ax = Z(%s)' % (a, r))
                i += 8
                continue
        # the acting member's actor: byte_42252 -> roster
        if re.match(r'mov\s+al, byte_42252$', s) and re.match(r'mov\s+ah, 0$', t(1)) and re.match(r'mov\s+bx, ax$', t(2)) and re.match(r'mov\s+al, \[bx\+4EDAh\]$', t(3)) and re.match(r'mov\s+ah, 0$', t(4)):
            out.append('%s ax = ACTOR   ; roster[byte_42252]' % a)
            i += 5
            continue
        out.append('%s %s' % (a, s))
        i += 1
    return out


def main():
    path = sys.argv[1]
    for name in sys.argv[2:]:
        lines = load(path, name)
        body = pretty(lines)
        for n, l in enumerate(body):
            if re.match(r'[0-9A-F]{5} (push\s+(bp|si|di)|mov\s+bp, sp)$', l) and n < 8:
                continue
            if re.match(r'[0-9A-F]{5} (pop\s+(bp|si|di)|mov\s+sp, bp)$', l) and n > len(body) - 8:
                continue
            print(l[:130])


main()
