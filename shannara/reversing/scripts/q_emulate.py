"""
Runs the demo's real Q-block decoder (sub_1652E0, called by q_expand) under the unicorn x86 emulator so the
Q codec can be studied and a reimplementation can be differentially tested WITHOUT any .Q data files.

Setup (once):  mkdir qdump && cd qdump && idat.exe -A -S"<repo>/reversing/scripts/dump_demo_segments.py" <copy of Shannara Demo.idb>
               pip install unicorn
Usage:         python q_emulate.py <qdump dir>          (runs a smoke test)
As a module:   from q_emulate import QEmu;  e = QEmu(qdump_dir, W, H);  out = e.decode_frame(stream, qmap=None, video=None)

The harness reproduces what q_open/q_load set up for a full-frame (type 2) chunk:
  _video_buffer, _qblock_addr_table (init_addr_tbl), _qloop1/_qloop2, the 7 _Q_RAW_LABEL jump-table slots and
  _q_qmap_table, then calls sub_1652E0(eax = block stream, edx = address of the first block).
The video buffer has the engine's 640-byte pitch; blocks are 4x4 pixels (0xA00 bytes = 4 rows per block row).
"""
import json
import os
import struct
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_MEM_UNMAPPED, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EBX, \
    UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP

PITCH = 0x280
VIDEO = 0x400000
VIDEO_SIZE = 0x200000
STREAM = 0x700000
STACK = 0x900000
COMP = 0xB00000
COMP_SIZE = 0x100000
COMP_TBL = 0xBF0000
RETADDR = 0x800000


class QEmu:
    def __init__(self, dump_dir, blocks_wide, blocks_high):
        meta = json.load(open(os.path.join(dump_dir, "segs.json")))
        self.names = meta["names"]
        self.W, self.H = blocks_wide, blocks_high
        self.dump_dir = dump_dir
        self.segs = meta["segs"]
        self.reset_memory()

    def reset_memory(self):
        self.start_idx = 0
        self.uc = uc = Uc(UC_ARCH_X86, UC_MODE_32)
        for name, start, end in self.segs:
            size = (end - start + 0xFFF) & ~0xFFF
            uc.mem_map(start, size + 0x1000)
            data = open(os.path.join(self.dump_dir, "seg_%X.bin" % start), "rb").read()
            uc.mem_write(start, data)
        uc.mem_map(VIDEO, VIDEO_SIZE)
        uc.mem_map(STREAM, 0x100000)
        uc.mem_map(COMP, COMP_SIZE)
        uc.mem_map(STACK - 0x20000, 0x40000)
        uc.mem_map(RETADDR, 0x1000)
        uc.mem_write(RETADDR, b"\xf4")  # hlt = return marker
        self.init_q_state()

    def w32(self, addr, v):
        self.uc.mem_write(addr, struct.pack("<I", v & 0xFFFFFFFF))

    def init_q_state(self):
        n = self.names
        W, H = self.W, self.H
        self.w32(n["_video_buffer"], VIDEO)
        # init_addr_tbl: block (r,c) -> VIDEO + r*0xA00 + c*4
        tbl = n["_qblock_addr_table"]
        for r in range(H):
            for c in range(W):
                self.w32(tbl + 4 * (r * W + c), VIDEO + r * 0xA00 + c * 4)
        # composite (type-6 snapshot) buffer + its address table, same 640-byte pitch
        self.w32(n["_composite_addr_tbl"], COMP_TBL)
        for r in range(H):
            for c in range(W):
                self.w32(COMP_TBL + 4 * (r * W + c), COMP + r * 0xA00 + c * 4)
        # _qloop1 / _qloop2 exactly as q_open computes them (min = 0, max = W-1 / H)
        max_col, max_row = W - 1, H
        ecx = ((((max_col << 8) + 0xFF) << 16) + W - max_col)
        self.w32(n["_qloop1"], ecx - 1)
        self.w32(n["_qloop2"], (max_row + (max_col << 8)) << 16)
        # the seven RAW slots as written by q_open
        for lab, tgt in (("_Q_RAW_LABEL1", 0x165790), ("_Q_RAW_LABEL2", 0x166850), ("_Q_RAW_LABEL7", 0x166A70),
                         ("_Q_RAW_LABEL6", 0x1665C0), ("_Q_RAW_LABEL5", 0x165E10), ("_Q_RAW_LABEL4", 0x165AD0),
                         ("_Q_RAW_LABEL3", 0x166F00)):
            self.w32(n[lab], tgt)

    def set_composite(self, data):
        self.uc.mem_write(COMP, bytes(data))

    def set_window(self, min_col, min_row, max_col, max_row):
        """what q_load does for a type 3/4 chunk (qinfo trailer)"""
        n, W = self.names, self.W
        ecx = ((((max_col << 8) + 0xFF) << 16) + W - max_col) - 1
        self.w32(n["_qloop1"], ecx)
        self.w32(n["_qloop2"], ((max_row + (max_col << 8)) << 16) + W * min_row + min_col)
        self.start_idx = W * min_row + min_col   # q_expand passes the address of this block as the start address

    def set_qmap(self, data256):
        assert len(data256) <= 0x100
        self.uc.mem_write(self.names["_q_qmap_table"], bytes(data256))

    def set_raw_mode(self):
        """what a type-7 chunk does: every RAW slot -> loc_166FE0"""
        n = self.names
        for i in range(1, 8):
            self.w32(n["_Q_RAW_LABEL%d" % i], 0x166FE0)

    def decode_frame(self, stream, video=None, max_insns=2_000_000, trace=False):
        uc = self.uc
        if video is not None:
            uc.mem_write(VIDEO, bytes(video))
        uc.mem_write(STREAM + 0x40, bytes(stream) + b"\x00" * 64)  # lead-in: the real stream follows audio bytes
        uc.reg_write(UC_X86_REG_EAX, STREAM + 0x40)
        si = getattr(self, "start_idx", 0)
        uc.reg_write(UC_X86_REG_EDX, VIDEO + (si // self.W) * 0xA00 + (si % self.W) * 4)
        sp = STACK
        sp -= 4
        uc.mem_write(sp, struct.pack("<I", RETADDR))
        uc.reg_write(UC_X86_REG_ESP, sp)
        self.trail = []
        if trace:
            uc.hook_add(UC_HOOK_CODE, lambda u, a, sz, ud: (self.trail.append(a), self.trail.__delitem__(0) if len(self.trail) > 40 else None))
        try:
            uc.emu_start(self.names["sub_1652E0"], RETADDR, count=max_insns)
        except Exception as e:  # unmapped etc.
            return None, str(e)
        used = uc.reg_read(UC_X86_REG_EDX) - (STREAM + 0x40)  # stream bytes consumed (edx walks the stream)
        end = uc.reg_read(UC_X86_REG_EAX) - VIDEO
        pix = bytes(uc.mem_read(VIDEO, PITCH * 4 * self.H))
        return pix, (used, end)

    def block(self, pix, r, c):
        return [list(pix[(r * 4 + y) * PITCH + c * 4:(r * 4 + y) * PITCH + c * 4 + 4]) for y in range(4)]


if __name__ == "__main__":
    d = sys.argv[1]
    e = QEmu(d, 2, 1)
    e.set_qmap(bytes(range(128)) * 2)
    for op in (0x00, 0x10, 0xF8, 0xF9, 0xFB, 0xFE, 0xFF):
        e.reset_memory()
        e.set_qmap(bytes(range(128)) * 2)
        pix, info = e.decode_frame(bytes([op, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77] * 4), video=bytes([0xAA]) * (PITCH * 8))
        print(hex(op), info, None if pix is None else e.block(pix, 0, 0))
