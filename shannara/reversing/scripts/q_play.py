"""
Decodes a real Shannara .Q video with the verified reference decoder (q_decode_ref.py) and writes selected frames as PNG.

    python q_play.py <file.q> <outdir> [frame numbers...]       e.g. q_play.py Logosvga.q out 0 10 40

Container rules (analysis/q_codec_spec.md): 0x1A-byte header, then {u16 type; u32 size} prefixes; types 6 and 7 carry no payload;
frame chunks (2/3/4) occupy size + audio bytes (+8 byte window trailer for 3/4).
"""
import os
import struct
import sys
import zlib

from q_decode_ref import QDecoder, load_tables, ROW


def write_png(path, w, h, rgb):
    raw = b"".join(b"\x00" + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b"")
    open(path, "wb").write(png)


class QFile:
    def __init__(self, data):
        self.d = data
        assert struct.unpack_from("<H", data, 0)[0] == 0x6839, "not a Q file"
        self.W = struct.unpack_from("<H", data, 4)[0]
        self.H = struct.unpack_from("<H", data, 6)[0]
        self.nframes = struct.unpack_from("<H", data, 0xA)[0]
        self.loop = struct.unpack_from("<I", data, 0xC)[0]
        self.fps = data[0x10] or 5
        self.rate = struct.unpack_from("<I", data, 0x16)[0]
        self.audio = self.rate // self.fps

    def frames(self, tables):
        """yields (frame index, bytearray video 640 pitch, palette bytes) for every video frame in the first pass"""
        d, W, H = self.d, self.W, self.H
        video = bytearray(ROW * H + 0x10000)
        comp = None
        qmap = bytes(0x100)
        raw = False
        win = (0, 0, W - 1, H)
        pal = bytes(768)
        pos = 0x1A
        f = 0
        while pos + 6 <= len(d):
            t, size = struct.unpack_from("<HI", d, pos)
            body = pos + 6
            if t == 0xFFFF:
                return
            if t == 6:
                comp = bytearray(video[:W * 4 * H * 4]) + bytearray(0x10000)
                pos = body
                continue
            if t == 7:
                raw = True
                pos = body
                continue
            if t in (2, 3, 4):
                size += self.audio
            payload = d[body: body + size]
            pos = body + size
            if t == 1:
                pal = bytes(payload)
            elif t == 5:
                qmap = bytes(payload) + bytes(0x100 - len(payload))
            elif t in (2, 3, 4):
                if t in (3, 4):
                    win = struct.unpack_from("<4H", d, pos)
                    pos += 8
                dec = QDecoder(tables, W, H, qmap, video, composite=comp)
                dec.raw_mode = raw
                used = dec.decode(bytes(payload[self.audio:]) + bytes(64), *win)
                yield f, video, pal, used, len(payload) - self.audio, t
                f += 1


def to_rgb(video, pal, W, H):
    out = bytearray(W * 4 * H * 4 * 3)
    scale = [min(255, (v & 63) * 4 + ((v & 63) >> 4)) for v in range(256)]
    p = [scale[b] for b in pal]
    for y in range(H * 4):
        row = video[y * 640: y * 640 + W * 4]
        o = y * W * 4 * 3
        for x, c in enumerate(row):
            out[o + 3 * x] = p[c * 3]
            out[o + 3 * x + 1] = p[c * 3 + 1]
            out[o + 3 * x + 2] = p[c * 3 + 2]
    return bytes(out)


if __name__ == "__main__":
    path, outdir = sys.argv[1], sys.argv[2]
    want = set(int(a) for a in sys.argv[3:])
    os.makedirs(outdir, exist_ok=True)
    q = QFile(open(path, "rb").read())
    tables = load_tables()
    print("W x H blocks:", q.W, q.H, "header frame count:", q.nframes, "fps:", q.fps, "audio bytes/frame:", q.audio)
    n = 0
    bad = 0
    for f, video, pal, used, size, t in q.frames(tables):
        if used != size and bad < 5:
            print("frame", f, "type", t, "stream bytes", size, "consumed", used)
            bad += 1
        if f in want:
            write_png(os.path.join(outdir, "%s_%04d.png" % (os.path.splitext(os.path.basename(path))[0], f)), q.W * 4, q.H * 4, to_rgb(video, pal, q.W, q.H))
        n += 1
    print("decoded frames:", n, "frames where consumed != stream size:", bad)
