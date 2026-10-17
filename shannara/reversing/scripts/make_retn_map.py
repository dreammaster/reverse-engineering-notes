"""
Writes `name N` lines: how many bytes of stack arguments each function of a Shannara.asm export pops with `retn N` (callee-cleaned).
Stubs and shims must pop the same amount. Input: the text export (Shannara.asm); output: e.g. game_out/retn_map.txt.

    python make_retn_map.py ../../Shannara.asm ../../game_out/retn_map.txt
"""
import re
import sys

m = {}
cur = None
for line in open(sys.argv[1], encoding="latin-1"):
    mm = re.match(r"^(\S+)\s+proc", line)
    if mm:
        cur = mm.group(1)
        continue
    if cur and cur not in m:
        r = re.match(r"\s+retn\s+([0-9A-Fa-f]+)h?\s*(;.*)?$", line)
        if r:
            v = r.group(1)
            m[cur] = int(v, 16) if re.search("[A-Fa-f]", v) or line.split(";")[0].rstrip().endswith("h") else int(v)
with open(sys.argv[2], "w") as f:
    f.write("".join("%s %d\n" % kv for kv in m.items()))
print(len(m), "functions pop stack arguments")
