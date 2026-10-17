"""
Renders a retail room the way the engine builds it: the room background picture (`get_room_pic_id`, retail_rooms.tsv) with the
handler's overlay sprites (retail_overlays.tsv) XORed on top. Used to check the room/overlay data model against real graphics.

    python compose_room.py ROOM_OBJECT_ID out.png [--all] [--bg ENTRY]   (--all also draws entries whose container/attr/cond would hide them)

An overlay sprite with its own palette (flag 0x10) switches the active palette, exactly as `load_palette` does.

Needs the git-ignored game_out/pics/SHANnnn.PIC files (PICS.UHA extracted).
"""
import csv
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import pic_scan                      # noqa: E402
from q_play import write_png         # noqa: E402

ANALYSIS = os.path.join(ROOT, "reversing", "analysis")


def tsv(name):
    with open(os.path.join(ANALYSIS, name), encoding="utf-8") as f:
        return list(csv.DictReader(f, delimiter="\t"))


def pic_file(num):
    return open(os.path.join(ROOT, "game_out", "pics", "SHAN%03d.PIC" % num), "rb").read()


def scale6(v):
    v &= 63
    return min(255, v * 4 + (v >> 4))


def compose(room, include_hidden=False, bg_entry=None):
    rooms = {int(r["room(obj id)"]): r for r in tsv("retail_rooms.tsv")}
    objs = {int(o["id"]): o for o in tsv("retail_objects.tsv")}
    r = rooms[room]
    pf, pe = int(r["picFile"]), int(r["picEntry"])
    if bg_entry is not None:
        pe = bg_entry      # e.g. get_room_pic_id swaps room 88 to entry 3 once game flag 22 is set
    d = pic_file(pf)
    es = pic_scan.entries(d)
    bg_e = es[pe]
    pal, _, px = pic_scan.load_entry(d, bg_e)
    W, H = bg_e["w"], bg_e["h"]
    img = bytearray(px)
    handler = "objh_%03d" % int(objs[room]["handler"])
    drawn = []
    seen = set()
    for o in tsv("retail_overlays.tsv"):
        if o["handler"] != handler or o["called via"] != "setup_pic_regions":
            continue
        key = (o["table"], o["index"])
        if key in seen:
            continue
        seen.add(key)
        hidden = int(o["container"]) not in (0, room) or int(o["requiredAttr"]) != 0 or int(o["conditionFn"], 16) != 0
        if hidden and not include_hidden:
            continue
        sf, se = int(o["picFile"]), int(o["picEntry"])
        sd = pic_file(sf) if sf != pf else d
        e = pic_scan.entries(sd)[se]
        spal, xy, spx = pic_scan.load_entry(sd, e)
        if spal is not None and (e["flags"] & 0x40) and not hidden:
            pal = spal          # an overlay that carries a palette replaces the active one (its indices are designed for it)
        x, y = xy
        if e["flags"] & 0x40 and 0 <= x and x + e["w"] <= W and 0 <= y and y + e["h"] <= H:
            img = pic_scan.composite_xor(img, W, spx, x, y, e["w"], e["h"])
        drawn.append((int(o["obj"]), se, x, y, e["w"], e["h"], hidden))
    rgb = bytearray()
    sp = [scale6(b) for b in pal]
    for c in img:
        rgb += bytes((sp[c * 3], sp[c * 3 + 1], sp[c * 3 + 2]))
    return W, H, bytes(rgb), drawn


if __name__ == "__main__":
    room = int(sys.argv[1])
    bg = int(sys.argv[sys.argv.index("--bg") + 1]) if "--bg" in sys.argv else None
    W, H, rgb, drawn = compose(room, "--all" in sys.argv, bg)
    write_png(sys.argv[2], W, H, rgb)
    print("room", room, "size", W, "x", H, "overlays drawn:", drawn)
