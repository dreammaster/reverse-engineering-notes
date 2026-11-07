#!/usr/bin/env python3
"""Prints routines of md/md_game.asm in a short form for porting Martian Dreams' recipe actions.

    md_pretty.py 5C8D9 59480 ...

  the idioms of the object arrays become expressions (TYPE(R), FRAME(R), X(R), Y(R), STATUS(R), QTY(R), QUAL(R)),
  `mov ax, seg seg057 / push / mov ax, N / push / call sub_35778` becomes PRINT "text" (the string at file 0x2EF40 + N).
"""
import re
import sys

EXE = 'md/game/MARTIAN/GAME.EXE'
SEG057 = 0x2EF40
asm = open('md/md_game.asm', encoding='utf-8', errors='replace').read().split('\n')
exe = open(EXE, 'rb').read()


def cstr(off):
    s = exe[SEG057 + off:SEG057 + off + 200].split(b'\0')[0]
    return s.decode('latin1').replace('\n', '\\n')


DSBASE = 0x3FAB0 - 0xCC00


def dstr(off):
    s = exe[DSBASE + off:DSBASE + off + 200].split(bytes([0]))[0]
    return s.decode("latin1").replace(chr(10), chr(92) + "n")


def load(name):
    out = []
    on = False
    for l in asm:
        if re.match(r'sub_%s\s+proc' % name, l):
            on = True
        if on:
            out.append(l)
            if re.match(r'sub_%s\s+endp' % name, l):
                break
    return out


def clean(lines):
    res = []
    for l in lines:
        if l.startswith(';') or not l.strip():
            continue
        l = l.split(';')[0].rstrip()
        if not l.strip():
            continue
        if re.match(r'\s+(push\s+cs|pop\s+cx)\s*$', l):
            continue
        res.append(l.strip())
    return res


def pretty(lines):
    L = clean(lines)
    out = []
    i = 0

    def t(k):
        return L[i + k] if i + k < len(L) else ''

    while i < len(L):
        s = L[i]
        m = re.match(r'mov\s+bx, (\S+)$', s)
        if m and t(1) == 'shl     bx, 1' and re.match(r'mov\s+(ax|dx), \[bx-633Ch\]$', t(2)):
            reg = re.match(r'mov\s+(ax|dx), ', t(2)).group(1)
            if re.match(r'and\s+%s, 3FFh$' % reg, t(3)):
                out.append('%s = TYPE(%s)' % (reg, m.group(1)))
                i += 4
                continue
            if t(3) == 'mov     cl, 0Ah' and re.match(r'shr\s+%s, cl$' % reg, t(4)):
                out.append('%s = FRAME(%s)' % (reg, m.group(1)))
                i += 5
                continue
            out.append('%s = WORD(%s)' % (reg, m.group(1)))
            i += 3
            continue
        m = re.match(r'mov\s+bx, (\S+)$', s)
        if m and t(1) == 'shl     bx, 1':
            n = t(2)
            mm = re.match(r'(test|cmp|mov)\s+(byte ptr )?(.*)\[bx\+54BBh\](.*)$', n)
            if re.search(r'\[bx\+54BBh\]', n):
                out.append('; QUAL(%s): %s' % (m.group(1), n))
                i += 3
                continue
            if re.search(r'\[bx\+54BAh\]', n):
                out.append('; QTY(%s): %s' % (m.group(1), n))
                i += 3
                continue
        m = re.match(r'mov\s+ax, (\S+)$', s)
        if m and t(1) == 'mov     dx, 3' and t(2) == 'imul    dx' and t(3) == 'mov     bx, ax':
            n = t(4)
            r = m.group(1)
            if re.match(r'mov\s+ax, \[bx\+78AEh\]$', n) and re.match(r'and\s+ax, 3FFh$', t(5)):
                out.append('ax = X(%s)' % r)
                i += 6
                continue
            if re.match(r'mov\s+ax, \[bx\+78AFh\]$', n) and t(5) == 'sar     ax, 1' and t(6) == 'sar     ax, 1' and re.match(r'and\s+ax, 3FFh$', t(7)):
                out.append('ax = Y(%s)' % r)
                i += 8
                continue
        # PRINT "..."
        if s == 'mov     ax, seg seg057' and t(1) == 'push    ax':
            m2 = re.match(r'mov\s+ax, ([0-9A-F]+)h$', t(2))
            if m2 and t(3) == 'push    ax' and re.match(r'call\s+sub_35778', t(4)):
                out.append('PRINT "%s"' % cstr(int(m2.group(1), 16)))
                i += 5
                continue
            if m2:
                out.append('; seg057:%s = "%s"' % (m2.group(1), cstr(int(m2.group(1), 16))))
                i += 3
                continue
        if s == 'push    ds':
            m2 = re.match(r'mov\s+ax, ([0-9A-F]+)h$', t(1))
            if m2 and t(2) == 'push    ax':
                txt = dstr(int(m2.group(1), 16))
                if re.match(r'call\s+sub_35778', t(3)):
                    out.append('PRINT "%s"' % txt)
                    i += 4
                    continue
                out.append('; ds:%s = "%s"' % (m2.group(1), txt))
                i += 3
                continue
        out.append(s)
        i += 1
    return out


def main():
    for name in sys.argv[1:]:
        print('=== sub_' + name)
        for l in pretty(load(name)):
            print(l[:130])


main()
