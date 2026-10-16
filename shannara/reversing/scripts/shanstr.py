"""
Reader for SHANSTR.DAT (retail Shannara message text), written from init_string_file? / load_string_ / decode_string?
(analysis/retail_game_layer.md sections 16 and 20).

    python shanstr.py SHANSTR.DAT                    # statistics + a few strings
    python shanstr.py SHANSTR.DAT dump out.txt       # every string as "block:index<TAB>text"
    python shanstr.py SHANSTR.DAT 13 6               # one string
"""
import struct
import sys


class ShanStr:
    def __init__(self, data):
        self.d = data
        p = 0
        self.nblocks, = struct.unpack_from("<H", data, p)
        p += 2
        self.blocks = []
        for i in range(self.nblocks):
            cnt, nbytes = struct.unpack_from("<HI", data, p)
            self.blocks.append((cnt, nbytes))
            p += 6
        self.len_base = p
        total = sum(c for c, _ in self.blocks)
        p += 2 * total
        n1, = struct.unpack_from("<H", data, p)
        p += 2
        self.tree = struct.unpack_from("<%dH" % n1, data, p)
        self.tree = [v - 0x10000 if v >= 0x8000 else v for v in self.tree]
        p += 2 * n1
        k, = struct.unpack_from("<H", data, p)
        p += 2
        self.phrases = []
        if k:
            offs = struct.unpack_from("<%dH" % k, data, p)
            p += 2 * k
            pool_size, = struct.unpack_from("<H", data, p)
            p += 2
            pool = data[p:p + pool_size]
            p += pool_size
            for o in offs:
                e = pool.index(b"\0", o) if b"\0" in pool[o:] else len(pool)
                self.phrases.append(bytes(pool[o:e]))
        self.data_base = p
        self.lengths = []
        q = self.len_base
        for cnt, _ in self.blocks:
            self.lengths.append(struct.unpack_from("<%dH" % cnt, data, q))
            q += 2 * cnt
        self.block_off = []
        o = self.data_base
        for _, nbytes in self.blocks:
            self.block_off.append(o)
            o += nbytes
        self.end = o

    def string(self, block, index):
        cnt, _ = self.blocks[block]
        off = self.block_off[block] + sum(self.lengths[block][:index])
        n = self.lengths[block][index]
        src = self.d[off: off + n]
        return self.decode(src)

    def decode(self, src):
        tree = self.tree
        root = len(tree) - 2
        out = bytearray()
        node = root
        last = -1
        bit_i = 0
        nbits = len(src) * 8
        while True:
            if (bit_i + 7) // 8 >= len(src) and last == 0:   # engine: all bytes loaded and the last symbol was the NUL
                break
            if bit_i >= nbits:
                break
            bit = (src[bit_i >> 3] >> (bit_i & 7)) & 1
            bit_i += 1
            node = tree[node | bit]
            if node < 0:
                sym = -node - 1
                last = sym
                if sym >= 0x80 and sym - 0x80 < len(self.phrases):
                    out += self.phrases[sym - 0x80]
                else:
                    out.append(sym)
                node = root
        return bytes(out)


if __name__ == "__main__":
    s = ShanStr(open(sys.argv[1], "rb").read())
    print("blocks:", s.nblocks, "strings:", sum(c for c, _ in s.blocks), "tree words:", len(s.tree), "phrases:", len(s.phrases),
          "data base:", s.data_base, "computed end:", s.end, "file size:", len(s.d))
    if len(sys.argv) >= 4 and sys.argv[2] == "dump":
        with open(sys.argv[3], "w", encoding="latin-1") as f:
            for b, (cnt, _) in enumerate(s.blocks):
                for i in range(cnt):
                    f.write("%d:%d\t%s\n" % (b, i, s.string(b, i).decode("latin-1").replace("\n", "\\n").replace("\r", "\\r").replace("\x0b", "\\v")))
    elif len(sys.argv) == 4:
        print(s.string(int(sys.argv[2]), int(sys.argv[3])))
    else:
        for b in range(min(3, s.nblocks)):
            for i in range(min(3, s.blocks[b][0])):
                print(b, i, s.string(b, i))
