"""
Reader for Companions of Xanth .PIC files.  Same 12-byte directory as Shannara's (analysis in shannara/reversing/analysis/retail_game_layer.md
section 22: {u32 offset, u8 hasOffset, u8 flags, u16 w, u16 h, u8, u8}; optional {i16 x, i16 y}; optional 768-byte 6-bit palette when
flags & 0x10) but the pixel data is **PKWARE DCL "implode" compressed** (flags 0x23/0x33/0x43/0x53; XANTH.EXE contains the PKWARE library):
`dcl_explode.explode(data)` gives exactly w*h bytes for every entry tested.

    python xanth_pic.py <XANTH_nn.PIC> <entry> <out.png>
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "shannara", "reversing", "scripts"))
from dcl_explode import explode          # noqa: E402
import pic_scan                          # noqa: E402
from q_play import write_png             # noqa: E402


def load(d, e):
    base = e["off"]
    xy = (0, 0)
    if e["count"]:
        xy = struct.unpack_from("<hh", d, base)
        base += 4 * e["count"]
    pal = None
    if e["flags"] & 0x10:
        pal = d[base:base + 768]
        base += 768
    px, _ = explode(d[base:e["end"]])
    return pal, xy, px


if __name__ == "__main__":
    d = open(sys.argv[1], "rb").read()
    es = pic_scan.entries(d)
    e = es[int(sys.argv[2])]
    pal, xy, px = load(d, e)
    print("entry", e["i"], "%dx%d" % (e["w"], e["h"]), "offset", xy, "palette" if pal else "no palette", len(px), "bytes")
    if pal and len(sys.argv) > 3:
        sc = lambda v: min(255, (v & 63) * 4 + ((v & 63) >> 4))
        rgb = bytearray()
        for c in px:
            rgb += bytes((sc(pal[c * 3]), sc(pal[c * 3 + 1]), sc(pal[c * 3 + 2])))
        write_png(sys.argv[3], e["w"], e["h"], bytes(rgb))
