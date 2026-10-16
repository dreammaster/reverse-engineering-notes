"""
Builds LOCAL-ONLY annotations that join the exe-derived tables with the real game text, so the IDB is easier to navigate:

  game_out/object_names.tsv   object id, type, name (from OBJECT.DAT via the 1-based name offsets of retail_objects.tsv)
  game_out/handler_notes.tsv  function name -> comment: owning object names + the first few message texts the handler prints
                              (message ids come from retail_handler_digest.txt, text from SHANSTR.DAT)

Both outputs contain game text and are git-ignored.  apply_handler_notes.py (an IDAPython script) then writes them into the IDB.

    python make_handler_notes.py <game dir> <out dir>      (run from reversing/scripts)
"""
import csv
import re
import struct
import sys

from shanstr import ShanStr

game, out = sys.argv[1], sys.argv[2]
d = open(game + "/OBJECT.DAT", "rb").read()
n, = struct.unpack_from("<H", d, 0)
pool = d[2:2 + n]


def name_at(off):
    if off <= 0:
        return ""
    e = pool.index(b"\0", off - 1)
    return pool[off - 1:e].decode("latin-1")


objs = {}
with open(out + "/object_names.tsv", "w", encoding="utf-8") as f:
    for o in csv.DictReader(open("../analysis/retail_objects.tsv", encoding="utf-8"), delimiter="\t"):
        nm = name_at(int(o["nameOff"] or 0))
        objs[int(o["id"])] = (int(o["type"]), nm)
        f.write("%s\t%s\t%s\n" % (o["id"], o["type"], nm))

ss = ShanStr(open(game + "/SHANSTR.DAT", "rb").read())


def text(block, idx):
    try:
        return ss.string(block, idx).decode("latin-1").rstrip("\0").replace("\n", " ").replace("\x0b", " ")
    except Exception:
        return "?"


owners = {}
for r in csv.reader(open("../analysis/retail_object_handlers.tsv", encoding="utf-8"), delimiter="\t"):
    if r[0].isdigit():
        owners["objh_%03d" % int(r[0])] = r[3]

digest = open("../analysis/retail_handler_digest.txt", encoding="utf-8").read().split("== ")[1:]
with open(out + "/handler_notes.tsv", "w", encoding="utf-8") as f:
    for blk in digest:
        name, _, body = blk.partition("\n")
        msgs = []
        for m in re.finditer(r"msg\((\d+):(\d+)\)", body):
            t = (int(m.group(1)), int(m.group(2)))
            if t not in msgs:
                msgs.append(t)
        who = ""
        if name in owners:
            parts = []
            for tok in owners[name].split():
                oid = int(tok.split(":")[0])
                if oid in objs:
                    parts.append("%d '%s' (type %d)" % (oid, objs[oid][1], objs[oid][0]))
            who = "Objects: " + ", ".join(parts)
        lines = [who] if who else []
        lits = []
        for m in re.finditer(r'"([^"]{6,})"', body):
            if m.group(1) not in lits:
                lits.append(m.group(1))
        for t in lits[:2]:
            lines.append('text: ' + t[:110])
        for b, i in msgs[:4]:
            lines.append("msg %d:%d: %s" % (b, i, text(b, i)[:110]))
        f.write("%s\t%s\n" % (name, " | ".join(lines).replace("\t", " ")))
print("done", len(objs), "objects,", len(digest), "handlers")
