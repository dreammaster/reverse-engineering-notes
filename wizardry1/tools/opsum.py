"""condense an .ops file: merge PutC runs into strings, drop the L0 echo of L2 ops"""
import sys, re
lines = open(sys.argv[1]).read().split('\n')
out = []
buf = []; bufw = None
def flush():
    global buf, bufw
    if buf:
        out.append(f'PutC*({bufw}, {"".join(buf)!r})')
    buf = []; bufw = None
for l in lines:
    m = re.match(r'PutC\((W\w+), (\d+)\)', l)
    if m:
        if bufw not in (None, m.group(1)): flush()
        bufw = m.group(1); buf.append(chr(int(m.group(2)))); continue
    if l.startswith('  L0 putc') or l.startswith('  L0 puts') and False: continue
    flush()
    if l.strip(): out.append(l)
flush()
print('\n'.join(out))
