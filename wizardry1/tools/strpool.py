#!/usr/bin/env python3
"""
Decoder for the DOS Wizardry string pool (ASCII.KRN).

Recovered from SYSTEM.PASCAL: WIZARDRY proc 38 = GetStr (a binary search over
a range tree), WIZARDRY proc 82 = the string loader/decoder, KANJIREA procs
8/10 = the tree/offsets loader.

ASCII.KRN layout (512-byte blocks, little-endian; block numbers are relative
to the file):

    block 0   header:  u16 offBlk, u16 offLen, u16 treeBlk, u16 treeLen, ...
    blocks 1..           packed + enciphered string data
    block offBlk..       strOffsets[]  (u16 each) : key slot -> word offset
    block treeBlk..      strTree[]     : 5x u16 per node
                         {startIdx, endIdx, indexOffset, leftChild, rightChild}
                         node 0 is a header; its word[4] is the root node index

Lookup for key KN:
    node = root
    while node != 0:
        s,e,ioff,l,r = strTree[node]
        if   KN < s: node = l
        elif KN > e: node = r
        else:
            SVAL = strOffsets[ioff + KN - s]
            p    = 2 * SVAL                       # byte offset into ASCII.KRN
            n    = data[p]                        # length byte (not enciphered)
            raw  = data[p+1 : p+1+n]
            seed = 67 * (KN % 51)
            char_k = (raw[k-1] - seed - 23*k) & 0xFF     for k = 1..n

Usage:
    python tools/strpool.py info  <ASCII.KRN>
    python tools/strpool.py get   <ASCII.KRN> <key> [<key> ...]
    python tools/strpool.py dump  <ASCII.KRN> [lo] [hi]
"""

import sys
import struct

BLOCK = 512


class StringPool:
    def __init__(self, path):
        self.d = open(path, "rb").read()
        self.off_blk, self.off_len, self.tree_blk, self.tree_len = \
            struct.unpack_from("<4H", self.d, 0)
        o = self.off_blk * BLOCK
        self.offsets = list(struct.unpack_from(
            f"<{self.off_len // 2}H", self.d, o))
        t = self.tree_blk * BLOCK
        self.tree = [list(struct.unpack_from("<5H", self.d, t + 10 * i))
                     for i in range(self.tree_len // 10)]
        self.root = self.tree[0][4]

    def raw_slot(self, kn):
        node = self.root
        guard = 0
        while node != 0 and guard < 200:
            guard += 1
            s, e, ioff, l, r = self.tree[node]
            if kn < s:
                node = l
            elif kn > e:
                node = r
            else:
                return self.offsets[ioff + kn - s]
        return None

    def get(self, kn):
        sval = self.raw_slot(kn)
        if sval is None:
            return None
        p = 2 * sval
        n = self.d[p]
        raw = self.d[p + 1:p + 1 + n]
        seed = 67 * (kn % 51)
        return bytes((raw[k - 1] - seed - 23 * k) & 0xFF for k in range(1, n + 1))

    def key_range(self):
        lo = min(n[0] for n in self.tree[1:] if n != [0, 0, 0, 0, 0])
        hi = max(n[1] for n in self.tree[1:] if n != [0, 0, 0, 0, 0])
        return lo, hi


def _txt(b):
    if b is None:
        return "<no such key>"
    return "".join(chr(c) if 32 <= c < 127 else f"\\x{c:02x}" for c in b)


def cmd_info(a):
    sp = StringPool(a[0])
    print(f"{a[0]}  ({len(sp.d)} bytes, {len(sp.d)//BLOCK} blocks)")
    print(f"strOffsets : block {sp.off_blk}, {sp.off_len} bytes "
          f"({len(sp.offsets)} entries)")
    print(f"strTree    : block {sp.tree_blk}, {sp.tree_len} bytes "
          f"({len(sp.tree)} nodes), root node {sp.root}")
    lo, hi = sp.key_range()
    print(f"key range  : {lo}..{hi}")
    print("\nsample:")
    for kn in list(range(lo, min(lo + 12, hi + 1))):
        print(f"  [{kn:4d}] {_txt(sp.get(kn))!r}")


def cmd_get(a):
    sp = StringPool(a[0])
    for k in a[1:]:
        kn = int(k, 0)
        print(f"[{kn}] {_txt(sp.get(kn))!r}")


def cmd_dump(a):
    sp = StringPool(a[0])
    lo, hi = sp.key_range()
    if len(a) > 1:
        lo = int(a[1], 0)
    if len(a) > 2:
        hi = int(a[2], 0)
    for kn in range(lo, hi + 1):
        b = sp.get(kn)
        if b is not None:
            print(f"{kn:5d}  {_txt(b)}")


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        sys.exit(2)
    {"info": cmd_info, "get": cmd_get, "dump": cmd_dump}[argv[0]](argv[1:])


if __name__ == "__main__":
    main(sys.argv[1:])
