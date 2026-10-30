"""
Turns a Shannara<->Xanth function/data match (shannara/reversing/scripts/match_engines.py + match_data.py) into the files the IDB scripts apply:

  analysis/function_matches.tsv   every accepted pair with tier and evidence (committed; the durable result of the matching)
  analysis/data_matches.tsv       matched global variables
  analysis/xanth_names.tsv        what to write into the Xanth IDB: address, new name or '-', comment
  analysis/shannara_hints.tsv     Xanth's existing human names for still-unnamed Shannara functions (comments for the Shannara IDB)

Tiers: A = shared unique string; B = callee/sole-caller propagation with similar size; C = order/size alignment (both functions >= 24 bytes,
similar size); D (dropped) = anything weaker.  Names transferred from Shannara get a trailing '?'.  An existing Xanth name is never overwritten:
those functions only receive a comment.

    python make_xanth_names.py sfeat.json xfeat.json matches.json data_matches.json
(run from xanth/reversing/scripts; the feature files are produced by shannara/reversing/scripts/extract_func_features.py)
"""
import json
import os
import re
import sys

S = {f["ea"]: f for f in json.load(open(sys.argv[1]))}
X = {f["ea"]: f for f in json.load(open(sys.argv[2]))}
M = json.load(open(sys.argv[3]))
D = json.load(open(sys.argv[4]))
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "analysis")

AUTO = ("sub_", "nullsub_", "loc_", "j_")
AUTO_DATA = ("dword_", "word_", "byte_", "unk_", "off_", "stru_", "asc_", "flt_", "qword_", "seg_", "jpt_", "byte", "word")


def tier(m):
    via = m["via"].split(":")[0]
    r = (m["s_size"] + 6) / (m["x_size"] + 6)
    if via == "string":
        return "A"
    if via in ("callee-of", "sole-caller-of") and 0.6 <= r <= 1.7:
        return "B"
    if via in ("order-gap", "callee-order") and 0.6 <= r <= 1.7 and m["s_size"] >= 24 and m["x_size"] >= 24:
        return "C"
    return "D"


def xname(sname):
    if sname.startswith(("objh_",)):
        return None
    if re.match(r"verbdef_\d+_", sname):
        sname = "verbdef_" + sname.split("_", 2)[2]
    if sname.startswith(AUTO):
        return None
    if sname.endswith("?"):
        return sname
    return sname + "?"


rows, names, hints = [], [], []
for m in M:
    t = tier(m)
    rows.append((m["s"], m["s_name"], m["x"], m["x_name"], t, m["via"], m["s_size"], m["x_size"]))
    if t == "D":
        continue
    nm = xname(m["s_name"])
    x_auto = m["x_name"].startswith(AUTO)
    ev = "Shannara retail %x %s (tier %s, %s, sizes %d/%d)" % (m["s"], m["s_name"], t, m["via"], m["s_size"], m["x_size"])
    if nm and x_auto:
        names.append((m["x"], nm, ev))
    elif not m["s_name"].startswith(AUTO):
        names.append((m["x"], "-", ev))
    if m["s_name"].startswith(AUTO) and not x_auto:
        hints.append((m["s"], m["x_name"], "Xanth %x %s (tier %s, %s)" % (m["x"], m["x_name"], t, m["via"])))

with open(os.path.join(OUT, "function_matches.tsv"), "w") as f:
    f.write("shannara_ea\tshannara_name\txanth_ea\txanth_name\ttier\tvia\tshannara_size\txanth_size\n")
    for r in rows:
        f.write("%x\t%s\t%x\t%s\t%s\t%s\t%d\t%d\n" % r)
sn = {}
xn = {}
for f in S.values():
    for a, n in zip(f["data"], f["data_names"]):
        sn[a] = n
for f in X.values():
    for a, n in zip(f["data"], f["data_names"]):
        xn[a] = n
dn = 0
with open(os.path.join(OUT, "data_matches.tsv"), "w") as f:
    f.write("shannara_ea\tshannara_name\txanth_ea\txanth_name\tvotes\n")
    for p in D:
        f.write("%x\t%s\t%x\t%s\t%d\n" % (p["s"], sn.get(p["s"], ""), p["x"], xn.get(p["x"], ""), p["votes"]))
        s_real = sn.get(p["s"], "") and not sn[p["s"]].startswith(AUTO_DATA)
        x_auto = not xn.get(p["x"], "") or xn[p["x"]].startswith(AUTO_DATA)
        if s_real and x_auto:
            nm = sn[p["s"]]
            names.append((p["x"], nm if nm.endswith("?") else nm + "?", "data: Shannara retail %x %s (%d votes)" % (p["s"], nm, p["votes"])))
            dn += 1
with open(os.path.join(OUT, "xanth_names.tsv"), "w") as f:
    f.write("xanth_ea\tname\tcomment\n")
    for a, n, c in names:
        f.write("%x\t%s\t%s\n" % (a, n, c))
with open(os.path.join(OUT, "shannara_hints.tsv"), "w") as f:
    f.write("shannara_ea\txanth_name\tcomment\n")
    for a, n, c in hints:
        f.write("%x\t%s\t%s\n" % (a, n, c))
tiers = {}
for r in rows:
    tiers[r[4]] = tiers.get(r[4], 0) + 1
print("pairs by tier", tiers, "| xanth renames/comments", len(names), "of which data", dn, "| shannara hints", len(hints))
