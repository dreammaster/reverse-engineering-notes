#!/usr/bin/env python3
"""
Differential oracle for the ACL opcodes: run one opcode handler of CAD.EXE (table CS:7819) in the emulator on a snapshot and report the
effect on the *script-visible state*: the 16-byte position records of all things (RAM layout), the script variables and a few globals, the
event queue.   Used to develop and verify tools/acl_vm.py.
"""
import struct, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emu_call import *

CODE = None
GLOBALS = [0x605, 0x23a, 0x23b, 0x24c, 0x24d, 0x24e, 0x24f, 0x248, 0x249, 0x24a, 0x24b, 0x232, 0x233, 0x592, 0x593, 0x594, 0x595, 0x596, 0x597, 0x5ff, 0x704]
VARS = range(0x611, 0x650)


class Oracle:
    def __init__(self, level=1):
        global CODE
        CODE = open(ROOT + '/unpacked/cadaver1/CAD.EXE', 'rb').read()[0xB0:]
        self.e = boot(level)
        e = self.e
        self.full = bytes(e.uc.mem_read(0, 0xA0000))
        self.regs = {r: e.r(r) for r in REGS}
        self.optab = struct.unpack_from('<100H', CODE, 0x7819)
        self.condtab = struct.unpack_from('<30H', CODE, 0x76fc)
        self.recptr = {}
        for t in range(1000):
            o = call(e, 0x407f, edx=t, eax=0, ds=0x1F17)
            if not (o['eflags'] >> 6) & 1:
                self.recptr[t] = o['edi'] & 0xFFFF
        self.restore()

    def restore(self):
        self.e.uc.mem_write(0, self.full)
        for r, v in self.regs.items():
            self.e.w(r, v)

    def state(self):
        ds = bytes(self.e.uc.mem_read(DS, 0x10000))
        s = {'rec': {t: ds[p:p + 16] for t, p in self.recptr.items()},
             'glob': {a: ds[a] for a in list(GLOBALS) + list(VARS)}}
        q = ds[0x84] | ds[0x85] << 8
        base = ds[0x5a] | ds[0x5b] << 8
        s['events'] = [struct.unpack_from('<3H', ds, a) for a in range(base, q, 6)] if q > base else []
        return s

    def run_op(self, op, data, cur=None, poke=None):
        """-> (state before, state after, bytes consumed) ; cur = thing id of the current object (default: first thing)"""
        self.restore()
        e = self.e
        curp = self.recptr[cur] if cur is not None else list(self.recptr.values())[0]
        e.wr(0x1F17, 0xF000, bytes(data) + b'\x17\x17\x17')
        e.wr(0x1F17, 0x9c, struct.pack('<H', curp))
        for (a, v) in (poke or []):
            e.wr(0x1F17, a, bytes([v]))
        before = self.state()
        try:
            o = call(e, self.optab[op], esi=0xF000, edi=curp, ds=0x1F17)
        except Exception:
            return before, None, None
        return before, self.state(), (o['esi'] & 0xFFFF) - 0xF000


def diff(a, b):
    d = {}
    for t, r in b['rec'].items():
        if a['rec'][t] != r:
            d[('rec', t)] = [(i, a['rec'][t][i], r[i]) for i in range(16) if a['rec'][t][i] != r[i]]
    for k, v in b['glob'].items():
        if a['glob'][k] != v:
            d[('g', hex(k))] = (a['glob'][k], v)
    if b['events'] != a['events']:
        d['events+'] = b['events'][len(a['events']):]
    return d
