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
    for nm, r in names.items():
        w = "; ".join("%s=%s" % (h, "/".join(sorted(v))) for h, v in sorted(writes[nm].items()))
        f.write("%s\t%s\t%s\t%s\t%s\n" % (r["address"], nm, r["size"], w, "; ".join(sorted(tests[nm])[:12])))
print("variables", len(names), "with writers", sum(1 for n in names if writes[n]), "with tests", sum(1 for n in names if tests[n]))
