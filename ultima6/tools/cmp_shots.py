#!/usr/bin/env python3
"""Compares two 320 x 200 screenshots (ours from `--shot` at scale 1, the original's from DOSBox): prints the number of differing pixels and their bounding box; with a third argument
writes a PNG-less ASCII map of the differing 8 x 8 blocks."""
import sys
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from pngread import read_png

a = read_png(sys.argv[1])
b = read_png(sys.argv[2])
assert a[0] == b[0] and a[1] == b[1], (a[:2], b[:2])
w, h = a[0], a[1]
diff = [(x, y) for y in range(h) for x in range(w) if a[2][y][x] != b[2][y][x]]
print('differing pixels:', len(diff))
if diff:
    xs = [d[0] for d in diff]
    ys = [d[1] for d in diff]
    print('bbox', min(xs), min(ys), max(xs), max(ys))
    if len(sys.argv) > 3:
        blocks = {}
        for x, y in diff:
            blocks[(x // 8, y // 8)] = blocks.get((x // 8, y // 8), 0) + 1
        for by in range(h // 8):
            print(''.join('#' if (bx, by) in blocks else '.' for bx in range(w // 8)))
