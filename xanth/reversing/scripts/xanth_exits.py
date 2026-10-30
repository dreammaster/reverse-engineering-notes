"""
Turns the exit dump (dump_exits.py) into analysis/xanth_exits.tsv using analysis/xanth_objects.tsv for the names.
Direction ids (word table in seg090 near the clock code): 1 north, 2 northeast, 3 east, 4 southeast, 5 south, 6 southwest, 7 west, 8 northwest, 9 up, 10 down
(9/10 inferred from the strings 'up'/'down' used by the handlers that move vertically; ids above 10 are listed as numbers).

    python xanth_exits.py exits.json
"""
import csv
import json
import os
import sys

here = os.path.dirname(os.path.abspath(__file__))
names = {r["id"]: r["name"] for r in csv.DictReader(open(os.path.join(here, "..", "analysis", "xanth_objects.tsv"), encoding="utf-8"), delimiter="\t")}
D = {1: "north", 2: "northeast", 3: "east", 4: "southeast", 5: "south", 6: "southwest", 7: "west", 8: "northwest", 9: "up", 10: "down"}
dump = json.load(open(sys.argv[1]))
rooms = dump["rooms"]
fns = {f["index"]: f for f in dump["exit_functions"]}
n = 0
with open(os.path.join(here, "..", "analysis", "xanth_exits.tsv"), "w", encoding="utf-8") as f:
    f.write("room\troom_name\tdirection\tkind\ttarget\ttarget_name\tdoor_object\n")
    for r in rooms:
        for e in r["exits"]:
            k = e["kind"]
            tgt = e["arg"] if k == 1 else e.get("door_target", "")
            door = e.get("door_obj", "")
            f.write("%d\t%s\t%s\t%d\t%s\t%s\t%s\n" % (r["room"], names.get(str(r["room"]), ""), D.get(e["dir"], e["dir"]), k, tgt if k in (1, 2) else "exit_fn_%02d" % e["arg"],
                                                     names.get(str(tgt), "") if k in (1, 2) else "", door))
            n += 1
print("exits", n)
