#!/usr/bin/env python3
"""Reference for test_x86_sound.c: the same driver calls under Unicorn.  usage: x86_sound_ref.py DRV SONG TICKS [fx ...]"""
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INSN, UC_HOOK_CODE
from unicorn.x86_const import *

drv, song, ticks = sys.argv[1], sys.argv[2], int(sys.argv[3])
fx = [int(a) for a in sys.argv[4:]]
mu = Uc(UC_ARCH_X86, UC_MODE_16)
mu.mem_map(0, 0x110000)
mu.mem_write(0x20000, open(drv, 'rb').read())
mu.mem_write(0x40000, open(song, 'rb').read())
mu.mem_write(0x20, bytes([0x20, 0xFF, 0x00, 0xF0]))
mu.mem_write(0xFFF20, b'\xCF')
out = []
state = {'reg': 0, 'status': 0}

def hook_out(uc, port, size, value, user):
    if port == 0x388:
        state['reg'] = value & 0xFF
    elif port == 0x389:
        out.append('R %02X %02X' % (state['reg'], value & 0xFF))
        if state['reg'] == 4:
            if value & 0x80: state['status'] = 0
            elif value & 1: state['status'] = 0xC0
    else:
        out.append('P %04X %02X' % (port, value & 0xFF))

def hook_in(uc, port, size, user):
    return state['status'] if port == 0x388 else 0

from unicorn.x86_const import UC_X86_INS_OUT, UC_X86_INS_IN
mu.hook_add(UC_HOOK_INSN, hook_out, None, 1, 0, UC_X86_INS_OUT)
mu.hook_add(UC_HOOK_INSN, hook_in, None, 1, 0, UC_X86_INS_IN)

def setregs():
    mu.reg_write(UC_X86_REG_SS, 0x3000); mu.reg_write(UC_X86_REG_SP, 0xFFF0)
    mu.reg_write(UC_X86_REG_DS, 0x1000); mu.reg_write(UC_X86_REG_ES, 0x1000)

def push(v):
    sp = (mu.reg_read(UC_X86_REG_SP) - 2) & 0xFFFF
    mu.mem_write(0x30000 + sp, bytes([v & 0xFF, v >> 8]))
    mu.reg_write(UC_X86_REG_SP, sp)

def run(cs, ip, stop=(0xF000, 0xFF00)):
    mu.reg_write(UC_X86_REG_CS, cs); mu.reg_write(UC_X86_REG_IP, ip)
    mu.emu_start((cs << 4) + ip, (stop[0] << 4) + stop[1], count=5000000)

def call_far(seg, off, args):
    for a in reversed(args): push(a)
    push(0xF000); push(0xFF00)
    run(seg, off)
    mu.reg_write(UC_X86_REG_SP, (mu.reg_read(UC_X86_REG_SP) + 2 * len(args)) & 0xFFFF)

setregs()
out.append('# init')
call_far(0x2000, 0, [0x388])
out.append('# music')
call_far(0x2000, 6, [0, 0x4000])
iv = mu.mem_read(0x20, 4)
off, seg = iv[0] | iv[1] << 8, iv[2] | iv[3] << 8
out.append('# ivt08 %04X:%04X' % (seg, off))
n = ticks
for phase in range(len(fx) + 1):
    if phase > 0:
        out.append('# fx %d' % fx[phase - 1])
        call_far(0x2000, 9, [fx[phase - 1]])
        n = 100
    for t in range(n):
        saved = {r: mu.reg_read(r) for r in (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS, UC_X86_REG_IP, UC_X86_REG_EFLAGS)}
        fl = mu.reg_read(UC_X86_REG_EFLAGS) & 0xFFFF
        push(fl); push(0xF000); push(0xFF00)
        out.append('# tick %d' % t)
        run(seg, off)
        for r, v in saved.items(): mu.reg_write(r, v)
print('\n'.join(out))
