#!/usr/bin/env python3
"""The original video module (MM3.CC member 8F99h) running in the emulator: oracle for the C text/window code.

    o = VdrvOracle("MM3.EXE", "MM3.CC")
    o.open_window(x, y, w, h, c1, c2, text)    # vdrv_1E_openWindow
    o.print_text(b"...")                       # vdrv_2D_printText
    o.close_windows(n)                         # vdrv_06_closeWindows
    o.screen()                                 # the 320x200 screen buffer (A000h), 64000 bytes

The module's own init (sub_62820) is skipped: the state it would have built (screen segment, font segment with FO.T, the row
offset table at 069Ch) is written directly; the temp file the module saves window backgrounds in is a virtual DOS file.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm3_cc  # noqa: E402
import mm3_emu  # noqa: E402

MOD_SEG = 0x6000
SCREEN_SEG = 0xA000
FONT_SEG = 0x7000
STR_SEG = 0x8000
API = {"closeWindows": 0x06, "openWindow": 0x1E, "printText": 0x2D, "drawSprite": 0x15}
# module data fields (offsets in the module segment)
W_SCREEN, W_SCREEN2, W_FONT, W_FILE = 0x9F8, 0x9FC, 0x9FE, 0x9CC


class VdrvOracle:
    def __init__(self, exe, cc_path):
        self.emu = e = mm3_emu.Emu(exe)
        d = open(cc_path, "rb").read()
        self.members = {i: mm3_cc.member(d, o, s)[0] for i, o, s in mm3_cc.read_toc(d)}
        self.module = self.members[0x8F99]
        e.uc.mem_write(MOD_SEG * 16, self.module)
        font = self.members[mm3_cc.name_id("FO.T")]
        e.uc.mem_write(FONT_SEG * 16, font)
        e.ww(W_SCREEN, SCREEN_SEG, MOD_SEG)
        e.ww(W_SCREEN2, SCREEN_SEG, MOD_SEG)
        e.ww(W_FONT, FONT_SEG, MOD_SEG)
        e.ww(W_FILE, 5, MOD_SEG)   # handle of the temp file
        e.ww(0xB22, 0x14BE, MOD_SEG)  # word_60B22: segment of _main (seg001); the module scans it for signature strings ("afts", "Iket"): copy protection
        for y in range(200):
            e.ww(0x69C + 2 * y, y * 320, MOD_SEG)
        self.pristine = bytes(e.uc.mem_read(MOD_SEG * 16, 0x10000))

    def reset(self):
        self.emu.uc.mem_write(MOD_SEG * 16, self.pristine)
        self.emu.dos_files.clear(); self.emu.dos_pos.clear()

    def set_screen(self, pixels):
        self.emu.uc.mem_write(SCREEN_SEG * 16, bytes(pixels))

    def screen(self):
        return bytes(self.emu.uc.mem_read(SCREEN_SEG * 16, 64000))

    def _call(self, off, args):
        return self.emu.call(MOD_SEG, off, args, ds=0x286F)

    def put_string(self, data, slot=0):
        off = 0x100 + slot * 0x400
        self.emu.uc.mem_write(STR_SEG * 16 + off, bytes(data) + b"\0")
        return off

    def print_text(self, text):
        off = self.put_string(text)
        self._call(API["printText"], [off, STR_SEG])

    def open_window(self, x, y, w, h, c1, c2, text=None, extra=(0, 0)):
        off = self.put_string(text, 1) if text is not None else 0
        seg = STR_SEG if text is not None else 0
        # ten words: six window parameters, then four more of which the last two are the text far pointer (docs: sub_63370)
        self._call(API["openWindow"], [x, y, w, h, c1, c2, extra[0], extra[1], off, seg])

    def close_windows(self, n):
        self._call(API["closeWindows"], [n])


if __name__ == "__main__":
    exe, cc = sys.argv[1], sys.argv[2]
    o = VdrvOracle(exe, cc)
    raw = o.members[mm3_cc.name_id("CREATE.RAW")]
    o.set_screen(raw)
    o.print_text(b"Hello, world")
    after = o.screen()
    print("pixels changed by plain print:", sum(1 for a, b in zip(raw, after) if a != b))
