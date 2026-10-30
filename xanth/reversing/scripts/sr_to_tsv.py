"""json from dump_save_regions.py -> analysis/xanth_save_regions.tsv:  python sr_to_tsv.py sr.json"""
import json
import os
import sys

r = json.load(open(sys.argv[1]))
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "analysis", "xanth_save_regions.tsv")
with open(out, "w", newline="") as f:
    f.write("index\taddress\tsize\tname\n")
    for x in r:
        f.write("%d\t%s\t%d\t%s\n" % (x["index"], x["addr"], x["size"], x["name"]))
print("regions", len(r))
