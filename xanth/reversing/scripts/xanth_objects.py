"""
Decodes the object dump (dump_objects.py) with OBJECT.DAT into analysis/xanth_objects.tsv.

OBJECT.DAT = u16 poolSize + NUL-separated names; a record's +0 word is the 1-based offset of the object's name in that pool, +2 an alternative name/description
offset (0 = none).  Field offsets by object type (16-bit records), from the object_get_*/set_* accessors (Xanth 0x1B0xx-0x1B9xx):

  location (word)      type 2,3: +8     type 5: +6        (types 1 and 4 have none)
  slot (byte)          type 2,3: +0Ah   type 5: +8
  next sibling (word)  type 2,3: +0Ch   type 5: +0Ah
  handler index (word) type 1: +0Ah  2: +1Ah  3: +1Eh  4: +0Ch  5: +1Eh
  default verb (word)  type 2: +16h  3,5: +1Ah  4: +8
  points (word)        type 1: +8  2: +12h  3: +16h
  prop_a (word)        type 1: +0Ch  2: +14h  3,5: +18h

    python xanth_objects.py objects.json ../../game/OBJECT.DAT
"""
import json
import os
import struct
import sys

d = json.load(open(sys.argv[1]))
pool = open(sys.argv[2], "rb").read()
psize = struct.unpack_from("<H", pool, 0)[0]
pool = pool[2:2 + psize]


def name(off):
    if off <= 0 or off > len(pool):
        return ""
    e = pool.find(b"\0", off - 1)
    return pool[off - 1:e].decode("latin-1")


F = {
    "loc": {2: 8, 3: 8, 5: 6}, "slot": {2: 0xA, 3: 0xA, 5: 8}, "next": {2: 0xC, 3: 0xC, 5: 0xA},
    "handler": {1: 0xA, 2: 0x1A, 3: 0x1E, 4: 0xC, 5: 0x1E}, "verb": {2: 0x16, 3: 0x1A, 4: 8, 5: 0x1A},
    "points": {1: 8, 2: 0x12, 3: 0x16}, "propa": {1: 0xC, 2: 0x14, 3: 0x18, 5: 0x18},
}
here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, "..", "analysis", "xanth_objects.tsv"), "w", encoding="utf-8") as f:
    f.write("id\ttype\tname\talt\tlocation\tslot\tnext\thandler\tdefault_verb\tpoints\tprop_a\trecord_hex\n")
    for o in d["objects"]:
        r, t = bytes(o["data"]), o["type"]

        def g(k, wide=True):
            off = F[k].get(t)
            if off is None:
                return ""
            return str(struct.unpack_from("<H" if wide else "<B", r, off)[0])
        nm, alt = struct.unpack_from("<HH", r, 0)
        f.write("\t".join([str(o["id"]), str(t), name(nm), name(alt), g("loc"), g("slot", False), g("next"), g("handler"), g("verb"), g("points"), g("propa"), r.hex()]) + "\n")
print("objects", len(d["objects"]))
