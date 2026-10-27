#!/usr/bin/env python3
"""mazepath.py <SCENARIO.DATA> <level> [type ...] : list the special squares of a maze level and the
key strings (W forward, A left, D right) that walk the party there from (0,0) facing north (or --from=x,y,dir).

Walls are passable unless type 1 (RUNNER:68); the squares are the SQRETYPE numbers RUNNER switches on
(1 stairs, 2 pit, 3 chute, 4 spinner, 5 dark, 6 teleport, 7 ouch, 8 elevator, 9 rock, 10 fizzle,
11 scnmsg, 12 encounter)."""
import sys, struct, collections
sys.path.insert(0, __import__('os').path.dirname(__file__))
import scenario

NAMES = {1: 'stairs', 2: 'pit', 3: 'chute', 4: 'spinner', 5: 'dark', 6: 'teleport', 7: 'ouch', 8: 'elevator',
         9: 'rock', 10: 'fizzle', 11: 'scnmsg', 12: 'encounter'}


def main():
    path, level = sys.argv[1], int(sys.argv[2])
    args = [a for a in sys.argv[3:] if not a.startswith('--from=')]
    frm = [a for a in sys.argv[3:] if a.startswith('--from=')]
    want = {int(a) for a in args}
    rec = scenario.Scenario(path).record(1, level - 1)
    w = lambda i: struct.unpack_from('<H', rec, i * 2)[0]

    def packed(base, rowwords, per, bits, x, y):
        return (w(base + x * rowwords + y // per) >> ((y % per) * bits)) & ((1 << bits) - 1)

    def wall(x, y, d):   # d 0 N, 1 E, 2 S, 3 W
        return packed({0: 180, 1: 120, 2: 60, 3: 0}[d], 3, 8, 2, x, y)

    def sqtype(x, y):
        idx = packed(280, 5, 4, 4, x, y)
        return (w(380 + idx // 4) >> ((idx % 4) * 4)) & 15, idx

    dx = {0: (0, 1), 1: (1, 0), 2: (0, -1), 3: (-1, 0)}
    start = tuple(int(v) for v in frm[0][7:].split(',')) if frm else (0, 0, 0)
    seen = {start: ''}
    q = collections.deque([start])
    while q:
        x, y, d = q.popleft()
        keys = seen[(x, y, d)]
        for k, nd in (('A', (d + 3) % 4), ('D', (d + 1) % 4)):
            if (x, y, nd) not in seen:
                seen[(x, y, nd)] = keys + k
                q.append((x, y, nd))
        if wall(x, y, d) != 1:
            nx, ny = (x + dx[d][0]) % 20, (y + dx[d][1]) % 20
            if (nx, ny, d) not in seen:
                seen[(nx, ny, d)] = keys + 'W'
                q.append((nx, ny, d))
    best = {}
    for (x, y, d), keys in seen.items():
        if (x, y) not in best or len(keys) < len(best[(x, y)]):
            best[(x, y)] = keys
    for y in range(20):
        for x in range(20):
            t, idx = sqtype(x, y)
            if t and (not want or t in want):
                a = [struct.unpack_from('<h', rec, (b + idx) * 2)[0] for b in (384, 400, 416)]
                print(f'({x:2d},{y:2d}) {NAMES.get(t, t):9s} idx {idx:2d} aux {a}  keys {best.get((x, y), "?")}')


main()
