#!/usr/bin/env python3
"""Run routines of the unpacked MM3 load image in a 16-bit CPU emulator (Unicorn).

Used to get the *exact* output of the original 3D renderer (draw lists) for any party position: the view code
(`prepareIndoorView`, `renderIndoorView`, `drawWallFaces`, ...) lives in the root image, calls few helpers, and reads
the maze through the page slots in DGROUP, so it can run natively on data loaded into DGROUP.

    emu = Emu("MM3.EXE")             # packed or unpacked game executable
    emu.call_far(0x1B66, 0x24DD, [])  # seg:off, cdecl args (list of words, first = first parameter)

Addresses are IDA linear addresses (load segment 1000h, as in the IDA database).  DGROUP is segment 286Fh.
"""
import os
import struct
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UcError
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
                               UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS,
                               UC_X86_REG_IP, UC_X86_REG_FLAGS)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import unpack_mm3  # noqa: E402

LOAD_SEG = 0x1000
DSEG = 0x286F
IMAGE_LINEAR = LOAD_SEG * 16
STACK_SEG = 0x9000
SENTINEL_SEG = 0x9F00  # return address of the top-level call: execution stops when it is reached
MEM_SIZE = 0x100000


class Emu:
    def __init__(self, exe_path):
        f = open(exe_path, "rb").read()
        hdr, img, _, _, _, _ = unpack_mm3.unpack_outer(f)
        image, relocs, _, _ = unpack_mm3.unpack_exepack(hdr + img)
        image = bytearray(image)
        for seg, off in relocs:  # add the load segment to every relocated word
            p = seg * 16 + off
            struct.pack_into("<H", image, p, (struct.unpack_from("<H", image, p)[0] + LOAD_SEG) & 0xFFFF)
        self.uc = Uc(UC_ARCH_X86, UC_MODE_16)
        self.uc.mem_map(0, MEM_SIZE)
        self.uc.mem_write(IMAGE_LINEAR, bytes(image))
        self.pristine_dgroup = bytes(self.uc.mem_read(DSEG * 16, 0x10000 if DSEG * 16 + 0x10000 <= MEM_SIZE else 0))
        self.hooks = {}       # linear address -> (python handler, 'near'|'far')
        self.watches = {}     # linear address -> handler(emu), called without changing the execution
        self.insn_count = 0
        self.stop_at = None
        self.uc.hook_add(UC_HOOK_CODE, self._code_hook)

    # ---- memory helpers (DGROUP offsets unless noted)
    def rb(self, off, seg=DSEG): return self.uc.mem_read(seg * 16 + off, 1)[0]
    def rw(self, off, seg=DSEG): return struct.unpack("<H", self.uc.mem_read(seg * 16 + off, 2))[0]
    def wb(self, off, v, seg=DSEG): self.uc.mem_write(seg * 16 + off, bytes([v & 255]))
    def ww(self, off, v, seg=DSEG): self.uc.mem_write(seg * 16 + off, struct.pack("<H", v & 0xFFFF))
    def rbytes(self, off, n, seg=DSEG): return bytes(self.uc.mem_read(seg * 16 + off, n))
    def wbytes(self, off, data, seg=DSEG): self.uc.mem_write(seg * 16 + off, bytes(data))
    def cstr(self, off, seg=DSEG):
        out = bytearray()
        while True:
            b = self.rb(off, seg)
            if not b:
                return bytes(out)
            out.append(b)
            off += 1

    def reset_dgroup(self):
        self.uc.mem_write(DSEG * 16, self.pristine_dgroup)

    # ---- calls
    def hook(self, linear, handler, kind="near"):
        """Intercept a routine: handler(emu) is run instead; its return value goes in AX; the routine returns to its caller."""
        self.hooks[linear] = (handler, kind)

    def _code_hook(self, uc, address, size, user):
        self.insn_count += 1
        if address == SENTINEL_SEG * 16:
            uc.emu_stop()
            return
        w = self.watches.get(address)
        if w:
            w(self)
        h = self.hooks.get(address)
        if h:
            handler, kind = h
            sp = uc.reg_read(UC_X86_REG_SP)
            ss = uc.reg_read(UC_X86_REG_SS)
            ax = handler(self)
            if ax is not None:
                uc.reg_write(UC_X86_REG_AX, ax & 0xFFFF)
            base = ss * 16
            if kind == "near":
                ip = struct.unpack("<H", uc.mem_read(base + sp, 2))[0]
                uc.reg_write(UC_X86_REG_SP, sp + 2)
                uc.reg_write(UC_X86_REG_IP, ip)
            else:
                ip, cs = struct.unpack("<HH", uc.mem_read(base + sp, 4))
                uc.reg_write(UC_X86_REG_SP, sp + 4)
                uc.reg_write(UC_X86_REG_CS, cs)
                uc.reg_write(UC_X86_REG_IP, ip)
            # continue at the new CS:IP: restart the emulator there
            self.stop_at = "redirect"
            uc.emu_stop()

    def watch(self, linear, handler):
        """Call handler(emu) whenever execution reaches the address (the routine still runs)."""
        self.watches[linear] = handler

    def arg(self, n, far=False):
        """n-th cdecl word argument (0-based) of a hooked routine, read from the stack at call time."""
        sp = self.uc.reg_read(UC_X86_REG_SP)
        ss = self.uc.reg_read(UC_X86_REG_SS)
        skip = 4 if far else 2
        return struct.unpack("<H", self.uc.mem_read(ss * 16 + sp + skip + 2 * n, 2))[0]

    def call(self, seg, off, args=(), far=True, max_insns=20_000_000, ds=DSEG, **regs):
        uc = self.uc
        uc.reg_write(UC_X86_REG_SS, STACK_SEG)
        sp = 0xFF00
        words = list(args)[::-1]
        for w in words:
            sp -= 2
            uc.mem_write(STACK_SEG * 16 + sp, struct.pack("<H", w & 0xFFFF))
        # return address: far -> sentinel seg:0, near -> same segment (sentinel must be in caller's segment): use far only
        sp -= 4
        uc.mem_write(STACK_SEG * 16 + sp, struct.pack("<HH", 0, SENTINEL_SEG))
        uc.reg_write(UC_X86_REG_SP, sp)
        uc.reg_write(UC_X86_REG_BP, 0)
        for name, v in (("DS", ds), ("ES", ds)):
            uc.reg_write({"DS": UC_X86_REG_DS, "ES": UC_X86_REG_ES}[name], v)
        for r, v in regs.items():
            uc.reg_write({"ax": UC_X86_REG_AX, "bx": UC_X86_REG_BX, "cx": UC_X86_REG_CX, "dx": UC_X86_REG_DX,
                          "si": UC_X86_REG_SI, "di": UC_X86_REG_DI}[r], v)
        uc.reg_write(UC_X86_REG_CS, seg)
        uc.reg_write(UC_X86_REG_IP, off)
        self.insn_count = 0
        cs, ip = seg, off
        while True:
            self.stop_at = None
            try:
                uc.emu_start(cs * 16 + ip, 0, count=max_insns)
            except UcError as e:
                raise RuntimeError("emulation error %s at %04X:%04X after %d instructions" % (
                    e, uc.reg_read(UC_X86_REG_CS), uc.reg_read(UC_X86_REG_IP), self.insn_count))
            if self.stop_at != "redirect":
                break
            cs, ip = uc.reg_read(UC_X86_REG_CS), uc.reg_read(UC_X86_REG_IP)
        return uc.reg_read(UC_X86_REG_AX)

    def call_linear(self, linear, args=(), **kw):
        """Far-call the routine at an IDA linear address (segment = the containing IDA segment's selector)."""
        seg = segment_of(linear)
        return self.call(seg, linear - seg * 16, args, **kw)


# IDA segment selectors of the root code segments (mm3.idc)
CODE_SEGMENTS = [0x1000, 0x14BE, 0x1523, 0x1903, 0x1B66, 0x1E40, 0x203F, 0x224F, 0x2511]


def segment_of(linear):
    best = None
    for s in CODE_SEGMENTS:
        if s * 16 <= linear and (best is None or s > best):
            best = s
    return best


if __name__ == "__main__":
    emu = Emu(sys.argv[1])
    # smoke test: isBitSet (far, cdecl: (ptr, bit)) on DGROUP data
    emu.wbytes(0x100, b"\x00\x20")
    for bit in (9, 10, 0):
        print("isBitSet(100h, %d) = %d" % (bit, emu.call_linear(0x2BA0 if False else int(sys.argv[2], 16), [0x100, bit])))
