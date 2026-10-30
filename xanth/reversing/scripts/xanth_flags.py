"""
Cross-references the Xanth save-region variables (the story/puzzle flags) with the handler digest: for every saved variable, the handlers that
write it (and the values written) and the handlers that test it.  Output: analysis/xanth_flags.tsv.

    python xanth_flags.py
"""
import collections
import csv
import os
import re

here = os.path.dirname(os.path.abspath(__file__))
A = os.path.join(here, "..", "analysis")
regions = list(csv.DictReader(open(os.path.join(A, "xanth_save_regions.tsv")), delimiter="\t"))
names = {r["name"]: r for r in regions if r["size"] in ("1", "2")}
byaddr = {r["address"].lower(): r for r in names.values()}


class Names(dict):
    """token -> region; tokens renamed to flag_<ADDR>_<where> resolve through their address"""
    def __contains__(self, k):
        return dict.__contains__(self, k) or bool(re.match(r"^flag_([0-9A-Fa-f]+)_", k) and re.match(r"^flag_([0-9A-Fa-f]+)_", k).group(1).lower() in byaddr)

    def __getitem__(self, k):
        if dict.__contains__(self, k):
            return dict.__getitem__(self, k)
        return byaddr[re.match(r"^flag_([0-9A-Fa-f]+)_", k).group(1).lower()]


orig = names
names = Names(orig)
digest = open(os.path.join(A, "xanth_handler_digest.txt"), encoding="utf-8").read().splitlines()
writes = collections.defaultdict(lambda: collections.defaultdict(set))
tests = collections.defaultdict(set)
cur, owner = None, ""
for l in digest:
    if l and not l.startswith(" "):
        m = re.match(r"^(\S+)(?:\s+# objects: (.*))?$", l)
        cur, owner = m.group(1), (m.group(2) or "").split(",")[0].strip()
        label = cur + ("(" + owner + ")" if owner else "")
        continue
    s = l.strip()
    m = re.match(r"^(\w+) (=|add|sub|or|and|inc|dec) ?(.*)$", s)
    if m and m.group(1) in names:
        writes[m.group(1)][label].add(m.group(3) if m.group(2) == "=" else m.group(2) + " " + m.group(3))
        continue
    m = re.match(r"^inc (\w+)$|^dec (\w+)$", s)
    if m and (m.group(1) or m.group(2)) in names:
        writes[m.group(1) or m.group(2)][label].add(s.split()[0])
        continue
    for nm in re.findall(r"\b(\w+)(?:==|!=|<=|>=|<|>)", s):
        if nm in names and s.startswith("if "):
            tests[nm].add(label)
with open(os.path.join(A, "xanth_flags.tsv"), "w", encoding="utf-8") as f:
    f.write("address\tname\tsize\twritten_by\ttested_by\n")
    for nm, r in orig.items():
        pre = "flag_" + r["address"].lower() + "_"
        keys = [nm] + [k for k in list(writes) + list(tests) if k.lower().startswith(pre)]
        wr = collections.defaultdict(set)
        for k in keys:
            for h, v in writes.get(k, {}).items():
                wr[h] |= v
        te = set().union(*[tests.get(k, set()) for k in keys])
        w = "; ".join("%s=%s" % (h, "/".join(sorted(v))) for h, v in sorted(wr.items()))
        f.write("%s\t%s\t%s\t%s\t%s\n" % (r["address"], nm, r["size"], w, "; ".join(sorted(te)[:12])))
print("variables", len(orig))
