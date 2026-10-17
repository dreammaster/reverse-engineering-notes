"""
Instruction coverage of the transpiled game layer from a SHAN_COVERAGE dump (matrix_diff.py writes one).
Reports per function class (object handlers objh_*, verb handlers, other) the fraction of instructions that executed, and lists handlers that
never ran at all - those are the ones the single-command matrix cannot reach (they need state set up by play: flags, moves, combat, dialogs).

    python port/tests/coverage_report.py game_out/cov7.txt [game_out/insns_all.json] [--list]
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main():
    cov = {int(l, 16) for l in open(sys.argv[1]) if l.strip()}
    jpath = next((a for a in sys.argv[2:] if a.endswith(".json")), os.path.join(ROOT, "game_out", "insns_all.json"))
    funcs = json.load(open(jpath))
    classes = {}
    never = []
    handler_eas = set()
    tsv = os.path.join(ROOT, "reversing", "analysis", "retail_object_handlers.tsv")
    for l in open(tsv).read().splitlines()[1:]:
        f = l.split("\t")
        if len(f) > 1 and f[1].startswith("0x"):
            handler_eas.add(int(f[1], 16))
    for name, v in funcs.items():
        eas = [r["ea"] for r in v["insns"]]
        if not eas:
            continue
        hit = sum(1 for e in eas if e in cov)
        cls = "object handlers" if v["ea"] in handler_eas else "verb handlers" if re.match(r"verb_|vh_", name) else "other"
        c = classes.setdefault(cls, [0, 0, 0, 0])
        c[0] += 1
        c[1] += hit > 0
        c[2] += hit
        c[3] += len(eas)
        if hit == 0:
            never.append((cls, name, v["ea"], len(eas)))
    for cls, (nf, nh, h, t) in sorted(classes.items()):
        print("%-16s functions %4d  reached %4d (%.0f%%)   instructions %6d / %6d (%.1f%%)" % (cls, nf, nh, 100.0 * nh / nf, h, t, 100.0 * h / t))
    if "--list" in sys.argv:
        for cls, name, ea, n in sorted(never):
            print("  never run: %-16s %-28s 0x%X (%d insns)" % (cls, name, ea, n))


main()
