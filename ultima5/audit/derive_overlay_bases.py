"""
Overlay address shifts.  Overlays are near-called into the EXE with the overlay's own segment as CS, so a call from
overlay M to EXE function F is encoded as target t = F - b_M (mod 64K).  We recover b_M by finding the shift that maps
the most external call targets onto known EXE function starts.  The result groups the overlays into four load areas
(overlays in the same area are never resident together).

Writes ida/out/ovl_bases.json.
"""
import collections
import json
import os

import audit_functions as a

exe = json.load(open(os.path.join(a.OUT, "ULTIMA.json")))
starts = set(f["start"] - 0x10000 for f in exe["functions"])
res = {}
for m in a.OVERLAYS:
    d = json.load(open(os.path.join(a.OUT, m + ".json")))
    size = d["segments"][0]["end"]
    ext = set(t for f in d["functions"] for t in f["calls"] if t >= 0 and not (0 <= t < size))
    if not ext:
        res[m] = None
        continue
    best = max(range(0x10000), key=lambda b: sum(1 for t in ext if ((t + b) & 0xFFFF) in starts))
    hits = sum(1 for t in ext if ((t + best) & 0xFFFF) in starts)
    res[m] = best
    print("%-9s shift=%04x  %d/%d external targets land on EXE functions" % (m, best, hits, len(ext)))
# FLAMES makes no external calls; it is in the first area with TOWN/MAINOUT/DUNGEON/INTRO
res["FLAMES"] = res["TOWN"]
json.dump(res, open(os.path.join(a.OUT, "ovl_bases.json"), "w"), indent=1)
