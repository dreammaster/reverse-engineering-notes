"""
Turns the JSON from dump_conversations.py into
  * analysis/xanth_conversations.tsv   structure only (no game text): kind, id, flag, next, option message ids, callbacks - safe to commit
  * <outdir>/conversations.txt         the same with the XANTHSTR.DAT text resolved (local only: xanth/game_out is git-ignored)

    python xanth_conversations.py conv.json ../../game/XANTHSTR.DAT ../../game_out
"""
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "shannara", "reversing", "scripts"))
from shanstr import ShanStr  # noqa: E402

conv = json.load(open(sys.argv[1]))
ss = ShanStr(open(sys.argv[2], "rb").read())
outdir = sys.argv[3]
os.makedirs(outdir, exist_ok=True)
here = os.path.dirname(os.path.abspath(__file__))


def txt(o):
    if "msg" in o:
        try:
            return ss.string(o["msg"][0], o["msg"][1]).rstrip(b"\0").decode("latin-1").replace("\n", " / ")
        except Exception as e:  # noqa: BLE001
            return "<%s>" % e
    return "<func %s>" % o.get("name", o.get("func"))


def nxt(n):
    return "end" if n == 0xFFFF else ("menu %d" % (n & 0x7FFF) if n >= 0x8000 else "line %d" % n)


def mid(o):
    return "%d:%d" % tuple(o["msg"]) if "msg" in o else "fn:" + o["func"]


with open(os.path.join(here, "..", "analysis", "xanth_conversations.tsv"), "w") as t, open(os.path.join(outdir, "conversations.txt"), "w", encoding="utf-8") as x:
    t.write("kind\tid\tflag\tnext\tmsg\tcallback\n")
    for s in conv["simple"]:
        t.write("line\t%d\t%d\t%s\t%s\t\n" % (s["id"], s["flag"], nxt(s["next"]), mid(s)))
        x.write("line %d [%s] -> %s\n    %s\n" % (s["id"], mid(s), nxt(s["next"]), txt(s)))
    for m in conv["menus"]:
        cb = m.get("callback", {}).get("name", "")
        x.write("\nmenu %d (flag %d, callback %s)\n" % (m["id"] & 0x7FFF, m["flag"], cb or "-"))
        for j, o in enumerate(m["options"]):
            t.write("option\t%d.%d\t%d\t%s\t%s\t%s\n" % (m["id"] & 0x7FFF, j, o["flag"], nxt(o["next"]), mid(o), cb))
            x.write("  %d. [%s] -> %s%s\n      %s\n" % (j, mid(o), nxt(o["next"]), "" if o["flag"] else " (disabled)", txt(o)))
        if not m["options"]:
            t.write("menu\t%d\t%d\t\t\t%s\n" % (m["id"] & 0x7FFF, m["flag"], cb))
print("simple", len(conv["simple"]), "menus", len(conv["menus"]))
