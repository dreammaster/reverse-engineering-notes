"""Where to stand to talk to a merchant: python tools/find_stand.py TOWNE|DWELLING|CASTLE|KEEP INDEX LEVEL mx,my [walkable hex ids]

Lists reachable tiles within 2 squares of the merchant in a straight line (talk works across a counter), nearest to the
entrance (15,30) first, with the key to press for 'Talk' direction.
"""
import os
import sys
from collections import deque

GAME = os.environ.get("U5_GAME_DIR", "C:/games/ultima5")


def main():
    fn, idx, lvl = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    mx, my = (int(v) for v in sys.argv[4].split(","))
    walk = {int(v, 16) for v in sys.argv[5:]} or {0x44, 0x45, 0xAA, 0x05, 0xBB, 0xF8}
    d = open(os.path.join(GAME, fn + ".DAT"), "rb").read()
    off = (idx * 2 + lvl) * 0x400
    t = lambda x, y: d[off + y * 32 + x]
    start = (15, 30)
    dist = {start: 0}
    q = deque([start])
    while q:
        c = q.popleft()
        for dx, dy in ((0, -1), (0, 1), (-1, 0), (1, 0)):
            n = (c[0] + dx, c[1] + dy)
            if 0 <= n[0] < 32 and 0 <= n[1] < 32 and n not in dist and t(*n) in walk:
                dist[n] = dist[c] + 1
                q.append(n)
    out = []
    for dx, dy, key in ((0, 1, "up"), (0, -1, "down"), (1, 0, "left"), (-1, 0, "right")):
        for k in (1, 2):
            s = (mx + dx * k, my + dy * k)
            if s in dist:
                out.append((dist[s], s, key, k))
    for dd, s, key, k in sorted(out):
        print("stand %d,%d  talk {%s}  (distance %d, %d steps from the entrance)" % (s[0], s[1], key, k, dd))
    if not out:
        print("no reachable tile; tile at merchant = %02x" % t(mx, my))


if __name__ == "__main__":
    main()
