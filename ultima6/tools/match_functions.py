#!/usr/bin/env python3
"""
Matches functions between two databases using the signatures written by
ida_scripts/export_signatures.py.

    match_functions.py <sig dir> <a> <b> [--min-ins N] [--propagate] [--csv out.csv]

A pair is reported when the signature is unique in both databases (so the
match is unambiguous) and the function has at least --min-ins instructions
(tiny functions look alike everywhere).  --propagate then walks the call
graph from the matches: an unmatched callee that is the only one left at the
same call position in matched callers is matched as well.

Names are only printed for functions that are already named in <a> (not
sub_XXXX), which is how names are carried from one executable to another.
"""

import argparse
import collections
import json
import os


def load(path):
    with open(path) as fh:
        return json.load(fh)


def unique_by_hash(funcs, min_ins):
    groups = collections.defaultdict(list)
    for f in funcs:
        groups[f["hash"]].append(f)
    return {h: g[0] for h, g in groups.items() if len(g) == 1 and g[0]["nins"] >= min_ins}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--min-ins", type=int, default=6)
    ap.add_argument("--propagate", action="store_true")
    ap.add_argument("--csv")
    args = ap.parse_args()

    fa = load(os.path.join(args.dir, args.a + "_sigs.json"))
    fb = load(os.path.join(args.dir, args.b + "_sigs.json"))
    ua, ub = unique_by_hash(fa, args.min_ins), unique_by_hash(fb, args.min_ins)
    match = {ua[h]["ea"]: ub[h]["ea"] for h in ua if h in ub}

    by_a = {f["ea"]: f for f in fa}
    by_b = {f["ea"]: f for f in fb}

    if args.propagate:
        changed = True
        while changed:
            changed = False
            used_b = set(match.values())
            for ea, eb in list(match.items()):
                ca = [c for c in by_a[ea]["callees"]]
                cb = [c for c in by_b[eb]["callees"]]
                if len(ca) != len(cb):
                    continue
                for x, y in zip(ca, cb):
                    if x not in match and y not in used_b and by_a[x]["nins"] == by_b[y]["nins"]:
                        match[x] = y
                        used_b.add(y)
                        changed = True

    named = sum(1 for ea in match if not by_a[ea]["name"].startswith("sub_"))
    print("%s (%d funcs) vs %s (%d funcs): %d matches, %d of them named in %s" % (
        args.a, len(fa), args.b, len(fb), len(match), named, args.a))
    if args.csv:
        with open(args.csv, "w") as fh:
            fh.write("a_ea,a_name,b_ea,b_name,nins\n")
            for ea, eb in sorted(match.items()):
                fh.write("%X,%s,%X,%s,%d\n" % (ea, by_a[ea]["name"], eb, by_b[eb]["name"], by_a[ea]["nins"]))


if __name__ == "__main__":
    main()
