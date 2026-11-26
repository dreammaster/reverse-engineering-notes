#!/usr/bin/env python3
"""Finds the routines of md/md_game.asm that load a string of GAME.EXE (segment 057 text or data segment) with the given text in it.

    md_findstr.py "Overcome by your wounds"
"""
import re
import sys

EXE = 'md/game/MARTIAN/GAME.EXE'
SEG057 = 0x2EF40
DSBASE = 0x3FAB0 - 0xCC00
import os
asm = open(os.environ.get('MD_ASM', 'md/md_game.asm'), encoding='utf-8', errors='replace').read().split('\n')
exe = open(EXE, 'rb').read()


def text(base, off):
    return exe[base + off:base + off + 200].split(b'\0')[0].decode('latin1')


pat = sys.argv[1].lower()
cur = None
for i, l in enumerate(asm):
    m = re.match(r'(sub_[0-9A-F]+)\s+proc', l)
    if m:
        cur = m.group(1)
    m = re.match(r'\s+mov\s+ax, ([0-9A-F]+)h$', l)
    if not m or i < 2:
        continue
    prev = asm[i - 1].strip()
    off = int(m.group(1), 16)
    if prev == 'push    ax' and asm[i - 2].strip().startswith('mov     ax, seg seg057'):
        s = text(SEG057, off)
    elif prev == 'push    ds' or prev.startswith('mov     ax, seg'):
        s = text(DSBASE, off)
    else:
        continue
    if pat in s.lower():
        print(cur, hex(off), repr(s[:80]))
