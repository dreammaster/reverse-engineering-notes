"""
Differential fuzz of the C++ Q block decoder against the verified Python reference (q_decode_ref.py), using the same
random valid-stream generator as reversing/scripts/q_fuzz.py (which fuzzed the Python code against the real machine code).

    python port/tests/fuzz_q.py [iterations] [seed]
"""
import os
import random
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "reversing", "scripts"))
TOOL = os.path.join(ROOT, "port", "build", "shan_tool.exe")

from q_decode_ref import QDecoder, load_tables, ROW, PITCH, T0   # noqa: E402
from q_fuzz import Gen                                           # noqa: E402

VIDEO_SIZE = 0x200000
M = (1 << 64) - 1


def fnv(data, h=1469598103934665603):
    for b in data:
        h ^= b
        h = (h * 1099511628211) & M
    return h


def make(iters, seed):
    tables = load_tables()
    rng = random.Random(seed)
    blob = bytearray()
    expect = []
    for it in range(iters):
        W, H = rng.choice([(10, 5), (3, 2), (16, 4), (40, 3)])
        window = rng.random() < 0.4
        raw = rng.random() < 0.25
        qmap = bytes(rng.randrange(256) for _ in range(256))
        video0 = bytes(rng.randrange(256) for _ in range(H * ROW + 4 * PITCH))
        comp = bytes(rng.randrange(256) for _ in range(H * ROW + 4 * PITCH))
        v = bytearray(video0 + bytes(VIDEO_SIZE - len(video0)))
        dec = QDecoder(tables, W, H, qmap, v, composite=bytearray(comp + bytes(0x10000)))
        stream = bytearray()
        dec.s = stream
        dec.p = 0
        min_col, min_row, max_col, max_row = 0, 0, W - 1, H
        if window:
            min_col = rng.randrange(0, W)
            max_col = rng.randrange(0, W - min_col)
            min_row = rng.randrange(0, H)
            max_row = rng.randrange(1, H - min_row + 1)
        dec.idx = min_row * W + min_col
        dec.cols = max_col
        dec.rows = max_row
        dec.start = dec.dst = dec.rowstart = dec.baddr(dec.idx)
        dec.row_i = 0
        dec.max_col = max_col
        dec.min_col = min_col
        dec.raw_mode = raw
        dec.state = T0
        dec.done = False
        gen = Gen(dec, rng)
        guard = 0
        while not dec.done and guard < 5000:
            guard += 1
            stream += gen.choose()
            dec.step()
        stream_b = bytes(stream)
        blob += struct.pack("<6I", W, H, min_col, min_row, max_col, max_row) + bytes([1 if raw else 0, 0, 0, 0]) + qmap + video0 + comp
        blob += struct.pack("<I", len(stream_b)) + stream_b
        expect.append("%d consumed %d hash=%016x" % (it, dec.p, fnv(bytes(dec.v[:H * ROW]))))
    return bytes(blob), expect


if __name__ == "__main__":
    iters = int(sys.argv[1]) if len(sys.argv) > 1 else 300
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    blob, expect = make(iters, seed)
    path = os.path.join(ROOT, "port", "build", "qvec.bin")
    open(path, "wb").write(blob)
    out = subprocess.run([TOOL, "qvec", path], capture_output=True, text=True, check=True).stdout.splitlines()
    bad = [(a, b) for a, b in zip(out, expect) if a != b]
    print("vectors:", len(expect), "C++ lines:", len(out), "mismatches:", len(bad) + abs(len(out) - len(expect)))
    for a, b in bad[:5]:
        print("  C++:", a, "\n  Py :", b)
    sys.exit(1 if bad or len(out) != len(expect) else 0)
