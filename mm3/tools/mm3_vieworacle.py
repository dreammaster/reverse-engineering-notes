#!/usr/bin/env python3
"""Draw lists of the original indoor renderer, produced by running its code in the emulator (tools/mm3_emu.py).

usage: mm3_vieworacle.py MM3.EXE MM3.CUR MAP X Y FACING
Prints the records the renderer hands to the text printer (docs/view.md "The draw list").
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm3_cc  # noqa: E402
import mm3_emu  # noqa: E402

# DGROUP offsets (IDA linear - 286F0h)
PARTY_FACING, PARTY_X, PARTY_Y, PARTY_MAP = 0xE8F4, 0xE8F5, 0xE8F6, 0xE8F7
MAZE_CUR_SLOT = 0xC53E
SLOT_BASE, SLOT_STRIDE = 0xC554, 0x340
MAZE_SLOT_IDS = 0x274E
ENGINE_MODE = 0xBF20
WRAP_MODE = 0x015B

LINEAR = {  # IDA linear addresses of routines (names/mm3.tsv); all far
    "prepareIndoorView": 0x1C195, "renderIndoorView": 0x1E407,
    "moveMonsters": 0x1B358, "playSoundEffect": 0x1B16B, "vdrv_0F_fade": 0x24FBF, "freeSpriteSlot": 0x26685,
    "vdrv_2D_printText": 0x24FB5, "_sprintf": 0x11E1D,
}


def lin_to_dg(linear):
    return linear - 0x286F0


# Far pointers to sprite sets (offset word, segment word) in DGROUP.  The emulator has no graphics loaded, so each is given a
# marker segment (MARKER_BASE + address / 2) and records "FFFF off seg" can be traced back to the variable they came from.
# Sources: the SPRITESET records of the scene writers (tools/mm3_scenelist.py), docs/view.md.
MARKER_BASE = 0x4000
POINTER_VARS = {  # DGROUP offset of the (offset, segment) pair -> meaning
    0xC4AA: "wl1", 0xC4AE: "wl2", 0xC4B2: "wl3", 0xC4B6: "wl4",  # wall sheets by number (word_34B9A = C4AA ...)
    0xA742: "sheet32E32",           # word_32E34/word_32E32 (used by drawWallFaces)
    0xECDC: "effects",              # word_373CE/word_373CC
    0xECC4: "background",           # word_373B6:373B4, copied into the scene header word_35D62:35D60 by renderIndoorView
    0xA77A: "hud32E6A", 0xABDA: "hud332CA", 0xAC18: "hud33308", 0xC536: "hud34C26", 0xE8C6: "hud36FB6",
}
for i in range(5):
    POINTER_VARS[0xC540 + 4 * i] = "object%d" % i      # [bx-3AC0h]
for i in range(6):
    POINTER_VARS[0xA750 + 4 * i] = "monster%d" % i     # [bx-58B0h]


def marker(addr):
    return MARKER_BASE + (addr >> 1)


def segof(name):
    for a, n in POINTER_VARS.items():
        if n == name:
            return marker(a)
    return int(name[3:], 16)  # "segXXXX"


name_to_seg = segof


def marker_name(seg):
    for a, n in POINTER_VARS.items():
        if marker(a) == seg:
            return n
    return "seg%04X" % seg


class ViewOracle:
    def __init__(self, exe, cur):
        self.emu = mm3_emu.Emu(exe)
        d = open(cur, "rb").read()
        self.members = {i: mm3_cc.member(d, o, s)[0] for i, o, s in mm3_cc.read_toc(d)}
        self.lists = []
        self.printed = []
        e = self.emu
        e.hook(LINEAR["moveMonsters"], lambda emu: 0, "far")
        e.hook(LINEAR["playSoundEffect"], lambda emu: 0, "far")
        e.hook(LINEAR["vdrv_0F_fade"], lambda emu: 0, "far")
        e.hook(LINEAR["freeSpriteSlot"], lambda emu: 0, "far")
        e.hook(LINEAR["vdrv_2D_printText"], self._print_text, "far")
        e.hook(LINEAR["_sprintf"], self._sprintf, "far")

    # ---- hooks
    def _sprintf(self, emu):
        buf, fmt = emu.arg(0, True), emu.arg(1, True)
        f = emu.cstr(fmt).decode("latin1")
        out, ai, i = "", 2, 0
        while i < len(f):
            c = f[i]
            if c != "%":
                out += c
                i += 1
                continue
            j = i + 1
            while f[j] in "0123456789-": j += 1
            spec, width = f[j], f[i + 1:j]
            v = emu.arg(ai, True); ai += 1
            if spec == "p": out += "%04X" % v
            elif spec in "ud": out += (("%" + width + "d") % (v if spec == "u" or v < 32768 else v - 65536))
            elif spec == "s": out += emu.cstr(v).decode("latin1")
            else: out += "?"
            i = j + 1
        emu.wbytes(buf, out.encode("latin1") + b"\0")
        return len(out)

    def _print_text(self, emu):
        off, seg = emu.arg(0, True), emu.arg(1, True)
        s = emu.cstr(off, seg)
        self.printed.append(s)
        i = 0
        while i < len(s):
            if s[i] == 5:
                addr = int(s[i + 1:i + 5].decode(), 16)
                self.lists.append(self._read_list(addr))
                i += 5
            else:
                i += 1

    def _read_list(self, addr):
        e = self.emu
        recs, cur, p = [], None, addr
        for _ in range(4000):
            w = e.rw(p)
            if w == 0xFFFF:
                off, seg = e.rw(p + 2), e.rw(p + 4)
                p += 6
                if seg == 0:
                    break
                cur = (seg, off)
                recs.append(("set", marker_name(seg)))
            else:
                x, y, fl, fr = struct.unpack("<hhHH", e.rbytes(p, 8))
                recs.append(("rec", x, y, fl, fr))
                p += 8
        return recs

    # ---- state
    def setup(self, map_id, x, y, facing):
        e = self.emu
        e.reset_dgroup()
        dat = self.members[mm3_cc.name_id("MAZE%02d.DAT" % map_id)]
        e.wbytes(SLOT_BASE, dat[:SLOT_STRIDE])
        e.wb(MAZE_CUR_SLOT, 0)
        e.wbytes(MAZE_SLOT_IDS, bytes([map_id, 0xFF, 0xFF, 0xFF]))
        e.wb(PARTY_X, x); e.wb(PARTY_Y, y); e.wb(PARTY_FACING, facing); e.wb(PARTY_MAP, map_id)
        e.wb(ENGINE_MODE, 1)
        e.wb(WRAP_MODE, 0)
        for a in POINTER_VARS:
            e.ww(a, 0)
            e.ww(a + 2, marker(a))

    def render(self):
        self.lists, self.printed = [], []
        self.emu.call_linear(LINEAR["prepareIndoorView"])
        self.emu.call_linear(LINEAR["renderIndoorView"])
        return self.lists


def main():
    exe, cur, map_id, x, y, f = sys.argv[1], sys.argv[2], *map(int, sys.argv[3:7])
    o = ViewOracle(exe, cur)
    o.setup(map_id, x, y, f)
    lists = o.render()
    print("instructions:", o.emu.insn_count, "lists:", len(lists), "printed:", o.printed)
    for n, l in enumerate(lists):
        print("list", n)
        for r in l:
            print("  ", r)


if __name__ == "__main__":
    main()
