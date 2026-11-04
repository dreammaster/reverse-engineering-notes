import sys,struct
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from dosemu import Emu
from unicorn import *
from unicorn import x86_const as X
import os
ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DS=0x1F170
def boot():
    e=Emu(ROOT+'/unpacked/cadaver1/CAD.EXE',ROOT+'/cadaver1')
    e.pending_keys=[0x0231,0x011b]
    while e.ticks<900: e.run(max_instr=20000)
    return e
REGS=['eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','es','ds','ss','cs','eflags']
def call(e,ip,**regs):
    saved={r:e.r(r) for r in REGS}
    sp=e.r('sp')-2
    ret=0xFFF0
    e.wr(e.r('ss'),sp,struct.pack('<H',ret)); e.w('sp',sp)
    for k,v in regs.items(): e.w(k,v)
    e.uc.emu_start(e.lin(0x1000,ip),e.lin(0x1000,ret),count=200000)
    out={r:e.r(r) for r in REGS}
    for r,v in saved.items(): e.w(r,v)
    return out
