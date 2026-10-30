"""
Joins the room picture table (analysis/xanth_room_pics.tsv), the region files (XANTH_nn.RGN, same layout as Shannara's, rgn_read.py) and the object table into
analysis/xanth_scenes.tsv: for every room, its picture and the clickable regions (region id = object id) with shape and coordinates (picture-window pixels).

    python xanth_scenes.py ../../game
"""
import csv
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "shannara", "reversing", "scripts"))
import rgn_read  # noqa: E402

game = sys.argv[1]
A = os.path.join(HERE, "..", "analysis")
names = {int(r["id"]): r["name"] for r in csv.DictReader(open(os.path.join(A, "xanth_objects.tsv"), encoding="utf-8"), delimiter="\t")}
pics = list(csv.DictReader(open(os.path.join(A, "xanth_room_pics.tsv")), delimiter="\t"))
cache = {}
n = missing = mismatch = 0
with open(os.path.join(A, "xanth_scenes.tsv"), "w", encoding="utf-8") as f:
    f.write("room\troom_name\tpicNum\tregion_object\tobject_name\tshape\tcoords\n")
    for p in pics:
        fnum, ent = int(p["pic_file"][6:8]), int(p["entry"])
        path = os.path.join(game, "XANTH_%02d.RGN" % fnum)
        if not os.path.exists(path):
            missing += 1
            continue
        if path not in cache:
            cache[path] = rgn_read.read(path)
        e = cache[path][ent] if ent < len(cache[path]) else None
        if not e or not e[0]:
            missing += 1
            continue
        if e[1] != int(p["room"]):
            mismatch += 1
        for rid, shape, co in e[3]:
            f.write("%s\t%s\t%s\t%d\t%s\t%s\t%s\n" % (p["room"], p["room_name"], p["picNum"], rid, names.get(rid, ""), shape, " ".join(str(c) for c in co)))
            n += 1
print("regions", n, "rooms without regions", missing, "base != room", mismatch)
