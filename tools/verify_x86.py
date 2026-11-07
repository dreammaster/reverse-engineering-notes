#!/usr/bin/env python3
"""Run the native routines of ROUTINEn.TAD (timer table = offset in the data table at +0x2C) in Unicorn (original code, memory snapshot of the running game)
and in tools/x86.py with the same random start state; compare data segment, registers and the trace of service calls (far calls to 1000:6019, which are
answered with a deterministic function).   python3 tools/verify_x86.py LEVEL [SCENARIOS]"""
import sys, os, struct, random
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emu_call import *
import x86

level = int(sys.argv[1]) if len(sys.argv) > 1 else 1
nscen = int(sys.argv[2]) if len(sys.argv) > 2 else 20
e = boot(level)
uc = e.uc
full = bytes(uc.mem_read(0, 0xA0000))
regs0 = {r: e.r(r) for r in REGS}
ds_seg = 0x1F17
tad_seg = struct.unpack('<H', full[DS + 0x78A:DS + 0x78C])[0]
tad_len = 0x1000
table = struct.unpack('<5H', full[tad_seg * 16 + 0x2C:tad_seg * 16 + 0x36])
nent = 0
ents = []
p = table[3]
while True:
    w = struct.unpack('<H', full[tad_seg * 16 + p:tad_seg * 16 + p + 2])[0]
    if w == 0 or w > 0x1000:
        break
    ents.append(w)
    p += 2
# behaviour routines: 1000:601E takes the word at 0x2C + 4*k and adds 2 (k = 9.. in level 1: 0x81, 0x15A, 0x194, 0x1A1)
for k in range(9, 16):
    w = struct.unpack('<H', full[tad_seg * 16 + 0x2C + 4 * k:tad_seg * 16 + 0x2E + 4 * k])[0]
    if 0x40 < w < 0x1000:
        ents.append(w + 2)
print('level', level, 'tad seg %04x' % tad_seg, 'timer entries', [hex(x) for x in ents])
SENT = 0x0001                       # return address inside the TAD segment's zero header
SVC = (0x1000, 0x6019)


def svc_result(ax, si, bx, cx, dx, di, bp, s792):
    return ((ax * 31 + s792 + bx * 7 + cx * 3 + dx) & 0xFFFF)


rng = random.Random(level)
bad = ok = 0
ds_lo, ds_hi = 0x0, 0x8000
for ent in ents:
    for sc in range(nscen):
        mem = bytearray(full) + bytearray(0x100000 - len(full))
        # random state: perturb the globals region ds:5A0..720 and some record bytes
        for _ in range(rng.randrange(0, 40)):
            a = rng.randrange(0x590, 0x720)
            mem[DS + a] = rng.choice((0, 1, 2, 4, 8, 0xFF, rng.randrange(256)))
        r = dict(ax=rng.randrange(0, 12), bx=rng.randrange(0x10000), cx=rng.randrange(0x10000), dx=rng.randrange(0x10000),
                 si=rng.choice((0x3db2, 0x3dda, 0x3dea, rng.randrange(0x8000))), di=rng.choice((0x3dda, 0x3dea, rng.randrange(0x8000))),
                 bp=rng.choice((0x3dda, 0x3dea, 0x3dfa)))
        sp = regs0['esp'] & 0xFFFF
        ss = regs0['ss']
        # ---- Python CPU
        cpu = x86.CPU(mem=bytearray(mem))
        cpu.r.update(r)
        cpu.r['sp'] = sp - 2
        cpu.s.update(ds=ds_seg, es=ds_seg, ss=ss, cs=tad_seg)
        cpu.ww(ss, sp - 2, SENT)
        cpu.ip = ent
        trace_py = []

        def hook(c):
            s792 = c.rw(ds_seg, 0x792)
            trace_py.append((s792, c.r['ax'], c.r['bx'], c.r['cx'], c.r['dx'], c.r['si'], c.r['di'], c.r['bp']))
            c.r['ax'] = svc_result(c.r['ax'], c.r['si'], c.r['bx'], c.r['cx'], c.r['dx'], c.r['di'], c.r['bp'], s792)
            c.ip = c.pop()
            c.s['cs'] = c.pop()
        cpu.hooks[SVC] = hook
        err = None
        try:
            cpu.run(stop=(tad_seg, SENT), max_steps=100000)
        except x86.X86Error as ex:
            err = str(ex)
        # ---- Unicorn
        uc.mem_write(0, bytes(mem[:0xA0000]))
        for k, v in regs0.items():
            e.w(k, v)
        for k, v in r.items():
            e.w(k, v)
        e.w('cs', tad_seg); e.w('ds', ds_seg); e.w('es', ds_seg)
        e.w('sp', sp - 2)
        uc.mem_write(ss * 16 + sp - 2, struct.pack('<H', SENT))
        trace_u = []

        def uhook(u, addr, size, user):
            s792 = struct.unpack('<H', bytes(u.mem_read(DS + 0x792, 2)))[0]
            g = lambda n: e.r(n) & 0xFFFF
            trace_u.append((s792, g('eax'), g('ebx'), g('ecx'), g('edx'), g('esi'), g('edi'), g('ebp')))
            e.w('eax', svc_result(g('eax'), g('esi'), g('ebx'), g('ecx'), g('edx'), g('edi'), g('ebp'), s792))
            spv = e.r('sp')
            ip, cs = struct.unpack('<HH', bytes(u.mem_read(e.r('ss') * 16 + spv, 4)))
            e.w('sp', spv + 4)
            e.w('cs', cs); e.w('eip', ip)
        h = uc.hook_add(UC_HOOK_CODE, uhook, None, e.lin(*SVC), e.lin(*SVC))
        try: uc.ctl_flush_tb()
        except Exception: pass
        try:
            uc.emu_start(e.lin(tad_seg, ent), e.lin(tad_seg, SENT), count=100000)
            uerr = None
        except Exception as ex:
            uerr = str(ex)
        uc.hook_del(h)
        dsu = bytes(uc.mem_read(DS, 0x10000))
        dsp = bytes(cpu.mem[DS:DS + 0x10000])
        diffs = [a for a in range(0x10000) if dsu[a] != dsp[a]]
        regs_u = dict(ax=e.r('eax') & 0xFFFF, bx=e.r('ebx') & 0xFFFF, cx=e.r('ecx') & 0xFFFF, dx=e.r('edx') & 0xFFFF, si=e.r('esi') & 0xFFFF,
                      di=e.r('edi') & 0xFFFF, bp=e.r('ebp') & 0xFFFF, sp=e.r('sp'))
        regs_p = {k: cpu.r[k] for k in regs_u}
        if err or uerr or diffs or regs_u != regs_p or trace_u != trace_py:
            bad += 1
            if bad <= 6:
                print('MISMATCH entry %#x scenario %d: py err=%s uc err=%s, %d ds diffs %s, regs %s, trace %d/%d' % (
                    ent, sc, err, uerr, len(diffs), [hex(a) for a in diffs[:6]], 'same' if regs_u == regs_p else (regs_u, regs_p), len(trace_u), len(trace_py)))
        else:
            ok += 1
print('level %d: %d scenarios identical, %d differ' % (level, ok, bad))
