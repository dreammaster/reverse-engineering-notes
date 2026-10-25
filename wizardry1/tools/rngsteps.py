#!/usr/bin/env python3
"""rngsteps.py <from> <to> : how many RANDOM calls take state <from> to <to> (hex s0,s1,s2,s3)."""
import sys
def nxt(s):
    s0 = (s[0] * 0x6A2D + 0x3619) & 0xFFFF
    s1 = (s[1] * 0xFFF1 + 0xFF8B) & 0xFFFF
    s2 = (s[2] * 0xFFAF + 0x0183) & 0xFFFF
    s3 = (s[3] * 0xFFD9 + 0x7FC9) & 0xFFFF
    return [s0, s1, s2, s3]
def parse(t): return [int(x, 16) for x in t.split(',')]
a, b = parse(sys.argv[1]), parse(sys.argv[2])
s = a
for k in range(0, 70000):
    if s == b:
        print(k); break
    s = nxt(s)
else:
    print('not reachable')
