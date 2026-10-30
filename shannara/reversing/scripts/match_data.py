"""
Pairs global variables between two builds from an existing function match (match_engines.py output): for every matched function pair whose
ordered lists of referenced data addresses have the same length, vote for position-wise pairing; a pair is kept when it has at least
two votes (or one vote from a function with >= 3 references) and no stronger competing vote.  Only Shannara names that are real names
(not dword_/word_/byte_/unk_/off_ auto names) are of interest downstream.

    python match_data.py sfeat.json xfeat.json matches.json out.json
"""
import collections
import json
import sys

S = {f["ea"]: f for f in json.load(open(sys.argv[1]))}
X = {f["ea"]: f for f in json.load(open(sys.argv[2]))}
M = json.load(open(sys.argv[3]))
votes = collections.defaultdict(collections.Counter)
for m in M:
    a, b = S[m["s"]], X[m["x"]]
    da, db = a["data"], b["data"]
    if len(da) == len(db) and da:
        for x, y in zip(da, db):
            w = 1 if len(da) < 3 else 2
            votes[x][y] += w
out = []
inv = collections.defaultdict(list)
for x, c in votes.items():
    y, n = c.most_common(1)[0]
    second = c.most_common(2)[1][1] if len(c) > 1 else 0
    if n >= 2 and n > second:
        inv[y].append((x, n))
for y, lst in inv.items():
    lst.sort(key=lambda t: -t[1])
    if len(lst) == 1 or lst[0][1] > lst[1][1]:
        out.append(dict(s=lst[0][0], x=y, votes=lst[0][1]))
json.dump(out, open(sys.argv[4], "w"))
print(len(out), "data pairs")
