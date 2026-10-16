"""
Prints conversations from the retail dialog tables joined with SHANSTR.DAT (needs the real game data; output is game text, keep it local).

    python dialog_view.py <game dir> <first dialog id> [<first dialog id> ...]       e.g. dialog_view.py ../../game 0xC7A

Pipeline (analysis/retail_game_layer.md section 27): dialog id -> 6-byte descriptor {w0 = portrait<<8 | style, w1, w2 = next index}
(retail_dialog_table.tsv) -> w1 indexes the u32 message-id table at dword_B66F0 (retail_dialog_text_ids.tsv) -> 0xF0000000 | block << 16 | index
-> SHANSTR.DAT string.  w2 = 0xFFFF / 0x8000 ends the conversation.
"""
import csv
import os
import sys

from shanstr import ShanStr

here = os.path.dirname(os.path.abspath(__file__))
ana = os.path.join(here, "..", "analysis")
ss = ShanStr(open(os.path.join(sys.argv[1], "SHANSTR.DAT"), "rb").read())
rows = {int(r[0]): [int(x, 16) for x in r[1:4]] for r in list(csv.reader(open(os.path.join(ana, "retail_dialog_table.tsv")), delimiter="\t"))[1:]}
ids = {int(r[0]): int(r[1], 16) for r in list(csv.reader(open(os.path.join(ana, "retail_dialog_text_ids.tsv")), delimiter="\t"))[1:]}


def text(mid):
    try:
        return ss.string((mid >> 16) & 0xFF, mid & 0xFFFF).decode("latin-1").rstrip("\0")
    except Exception:
        return "<unavailable: %08X>" % mid


for a in sys.argv[2:]:
    i = int(a, 0)
    for _ in range(200):
        w0, w1, w2 = rows[i]
        print("[%d] portrait %d style %d: %s" % (i, w0 >> 8, w0 & 255, text(ids[w1]) if w1 in ids else "<function text %04X>" % w1))
        if w2 in (0xFFFF, 0x8000):
            break
        i = w2
    print()
