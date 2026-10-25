#!/usr/bin/env python3
"""showgrid.py <a.grid> [b.grid ...] : print the composed screens (.grid dumps) as ASCII, rows 0-15.

Line-art glyphs: 9/10 = horizontal edges, 11/12 = verticals, 13/14 = diagonals, 1-8 = window border
pieces; printable ASCII is shown as is; anything else as its low hex digit."""
import sys

LINE = {9: '-', 10: '-', 11: '|', 12: '|', 13: chr(92), 14: '/', 15: '+', 16: '+', 17: '|', 18: '+', 19: '+',
        1: '#', 2: '#', 3: '#', 4: '#', 5: '#', 6: '#', 7: '#', 8: '#', 98: '.', 103: '*', 102: '/', 104: chr(92),
        105: chr(92), 106: '/'}


def glyphs(path):
    rows = open(path).read().split('\n')[:25]
    return [[int(x, 16) for x in r.split()] for r in rows]


def ch(v):
    if v == 0:
        return ' '
    if 0x20 <= v < 0x7f:
        return chr(v)
    return LINE.get(v, '%x' % (v % 16))


if __name__ == '__main__':
    rows = int(sys.argv[1]) if sys.argv[1].isdigit() else 16
    files = [a for a in sys.argv[1:] if not a.isdigit()]
    grids = [glyphs(p) for p in files]
    print('   '.join(p.split('/')[-1].ljust(40) for p in files))
    for y in range(rows):
        print('   '.join(''.join(ch(v) for v in g[y]) for g in grids))
