"""
Reader for MM2's data files (formats: mm2/docs/file-formats.md).

    python mm2_data.py info              # summary of every parsed file
    python mm2_data.py unpack OUTDIR     # write decompressed/decoded copies

Library use:  from mm2_data import Game;  g = Game();  g.map(0), g.events('I', 3), g.strings(), ...
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_lzw import lzw_decode  # noqa: E402
from mm2_layout import DEFAULT_GAME_DIR  # noqa: E402


class Game:
    def __init__(self, game_dir=DEFAULT_GAME_DIR):
        self.dir = game_dir

    def read(self, name):
        with open(os.path.join(self.dir, name), "rb") as f:
            return f.read()

    # -- LZW files with a 4-byte length header --------------------------------
    @staticmethod
    def lzw_blob(blob):
        size = struct.unpack_from("<I", blob, 0)[0]
        out = lzw_decode(blob[4:])
        assert len(out) == size, (len(out), size)
        return out

    def attrib(self):
        return self.lzw_blob(self.read("ATTRIB.DAT"))     # 3840 bytes

    def monsters(self):
        return self.lzw_blob(self.read("MONSTERS.DAT"))   # 6656 bytes = 416 x 16

    def strings(self):
        """STR.DAT: LZW, then every byte + 1Ch (mod 256); 1Dh is the line break."""
        raw = self.lzw_blob(self.read("STR.DAT"))
        return bytes((b + 0x1C) & 0xFF for b in raw)

    # -- MAP.DAT: 60 word offsets, each a LZW chunk of 512 bytes -------------------
    def map(self, n):
        d = self.read("MAP.DAT")
        offs = list(struct.unpack_from("<60H", d, 0)) + [len(d)]
        return self.lzw_blob(d[offs[n]:offs[n + 1]])

    # -- EVENTS{I,O}.DAT: dword offset table, LZW chunks ---------------------------
    def events(self, kind, n):
        """kind 'I' or 'O'; chunk n (0..70).  Returns the decompressed chunk or None."""
        d = self.read("EVENTS%s.DAT" % kind)
        first = struct.unpack_from("<I", d, 0)[0] or struct.unpack_from("<I", d, 20)[0]
        offs = [struct.unpack_from("<I", d, 4 * i)[0] for i in range(first // 4)]
        if n >= len(offs) or not offs[n]:
            return None
        nxt = min([o for o in offs if o > offs[n]] + [len(d)])
        return self.lzw_blob(d[offs[n]:nxt])

    @staticmethod
    def parse_events(chunk):
        """-> (triggers[(cell, script, facing_mask)], scripts[bytes], messages[bytes]) for map chunks."""
        p = 0
        triggers = []
        while chunk[p:p + 3] != b"\0\0\0":
            triggers.append(tuple(chunk[p:p + 3]))
            p += 3
        p += 3
        size = chunk[p] | (chunk[p + 1] << 8)
        body = chunk[p + 2:p + size]
        return triggers, body, chunk[p + size:]


# Total command size in bytes (opcode included), copied from DGROUP:15E6.  Entry 37 (pay gems)
# says 2 although the handler reads a word (3 bytes); the opcode is never used in the shipped data.
EVENT_OPCODE_LEN = [0, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 3, 3, 2, 2, 1, 2, 2, 13, 11, 1, 4, 3, 3, 5, 5, 3, 2, 2, 2,
                    2, 7, 7, 4, 3, 3, 3, 2, 1, 1, 3, 1, 15, 2, 2, 3, 3, 1, 11, 4, 2]


def split_scripts(body):
    """Split the script area into individual scripts (each ends with 0FFh); yields lists of (opcode, args)."""
    q, cur, out = 0, [], []
    while q < len(body):
        op = body[q]
        if op == 0xFF:
            out.append(cur)
            cur = []
            q += 1
            continue
        n = EVENT_OPCODE_LEN[op]
        cur.append((op, bytes(body[q + 1:q + n])))
        q += n
    return out


def main():
    g = Game()
    if len(sys.argv) < 2 or sys.argv[1] == "info":
        print("ATTRIB", len(g.attrib()), "MONSTERS", len(g.monsters()), "STR", len(g.strings()))
        for kind in "IO":
            for n in range(71):
                c = g.events(kind, n)
                if c:
                    print("EVENTS%s chunk %2d: %d bytes" % (kind, n, len(c)))
        return
    out = sys.argv[2]
    os.makedirs(out, exist_ok=True)
    open(os.path.join(out, "attrib.bin"), "wb").write(g.attrib())
    open(os.path.join(out, "monsters.bin"), "wb").write(g.monsters())
    open(os.path.join(out, "str.txt"), "wb").write(g.strings().replace(b"\x1d", b"\n"))
    for n in range(60):
        open(os.path.join(out, "map%02d.bin" % n), "wb").write(g.map(n))
    for kind in "IO":
        for n in range(71):
            c = g.events(kind, n)
            if c:
                open(os.path.join(out, "events%s_%02d.bin" % (kind, n)), "wb").write(c)


if __name__ == "__main__":
    main()
