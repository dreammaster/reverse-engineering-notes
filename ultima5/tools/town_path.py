"""Key-script path inside a town: python tools/town_path.py TOWNE|DWELLING|CASTLE|KEEP INDEX LEVEL x0,y0 x1,y1 [walkable hex ids...]

Prints {up}{left}... keys (400 virtual ms apart) from (x0,y0) to (x1,y1) over the tiles of the given town map, found with BFS.
Default walkable tiles: 44 45 (brick floors).  Town index = map id - 1 within the file (8 towns per file, 2 levels each).
"""
import os
import sys
from collections import deque

GAME = os.environ.get("U5_GAME_DIR", "C:/games/ultima5")


def main():
    fn, idx, lvl = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    s = tuple(int(v) for v in sys.argv[4].split(","))
    g = tuple(int(v) for v in sys.argv[5].split(","))
    walk = {int(v, 16) for v in sys.argv[6:]} or {0x44, 0x45}
    d = open(os.path.join(GAME, fn + ".DAT"), "rb").read()
    off = (idx * 2 + lvl) * 0x400
    t = lambda x, y: d[off + y * 32 + x]
    q, prev = deque([s]), {s: None}
    while q:
        c = q.popleft()
        if c == g:
            break
        for dx, dy, k in ((0, -1, "{up}"), (0, 1, "{down}"), (-1, 0, "{left}"), (1, 0, "{right}")):
            n = (c[0] + dx, c[1] + dy)
            if 0 <= n[0] < 32 and 0 <= n[1] < 32 and n not in prev and t(*n) in walk:
                prev[n] = (c, k)
                q.append(n)
    if g not in prev:
        sys.exit("no path")
    p = []
    c = g
    while prev[c]:
        c, k = prev[c]
        p.append(k)
    print("".join(k + "{sleep:400}" for k in reversed(p)))


if __name__ == "__main__":
    main()
