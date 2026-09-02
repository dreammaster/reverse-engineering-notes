#!/usr/bin/env python3
"""
Match DOS SYSTEM.PASCAL procedures to Apple-source names, using (1) the text
each procedure references and (2) call-graph topology, propagated from a set
of hand-verified seeds.

    python tools/procmatch.py <SYSTEM.PASCAL> <ASCII.KRN> <Wiz1WizardryPascal.txt>

Reads seeds from docs/procmap.seed.tsv (conf "manual" - kept verbatim).
Writes docs/procmap.tsv:  dos_seg  dos_proc  apple_name  conf  evidence
"""
import re
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pcode_dis as P
import strpool

SEED = "docs/procmap.seed.tsv"
OUT = "docs/procmap.tsv"

# Apple segnum (2nd listing column) -> DOS segment name
APPLE_SEG = {1: "WIZARDRY", 7: "UTILITIE", 8: "SHOPS", 9: "SPECIALS",
             10: "COMBAT", 11: "CINIT", 12: "CUTIL", 14: "CASTASPE",
             15: "SWINGASW", 16: "CASTLE", 17: "ROLLER", 18: "CAMP",
             19: "REWARDS", 20: "RUNNER"}


def norm(s):
    s = re.sub(r"[^A-Z0-9]", "", s.upper())
    return s if len(s) >= 4 else ""


def add(st, s):
    if s:
        st.add(s)


# ---- Apple source ------------------------------------------------------
def parse_apple(path):
    """-> {segnum: [ {name, strings:set, calls:set} ... ]}, and name->seg"""
    raw = open(path, encoding="latin1").read().splitlines()
    rows = []
    for l in raw:
        m = re.match(r"\s*\d+\s+(\d+)\s+\d+:[A-Z0-9]\s+\d+ ?(.*)", l)
        if m:
            rows.append((int(m.group(1)), m.group(2)))
        else:
            rows.append((None, l))

    names = set()
    for _, t in rows:
        dm = re.search(r"\b(?:SEGMENT\s+)?(?:PROCEDURE|FUNCTION)\s+([A-Z0-9_]+)", t)
        if dm:
            names.add(dm.group(1))

    segs = {}
    cur = None
    curseg = None
    for seg, t in rows:
        dm = re.search(r"\b(?:SEGMENT\s+)?(?:PROCEDURE|FUNCTION)\s+([A-Z0-9_]+)", t)
        if dm and "FORWARD" not in t and "EXTERNAL" not in t:
            curseg = seg
            cur = {"name": dm.group(1), "strings": set(), "calls": set()}
            segs.setdefault(seg, []).append(cur)
            continue
        if cur is None:
            continue
        for lit in re.findall(r"'([^']{2,})'", t):
            add(cur["strings"], norm(lit))
        for tok in re.findall(r"\b([A-Z][A-Z0-9_]{2,})\b", t):
            if tok in names and tok != cur["name"]:
                cur["calls"].add(tok)
    # merge dup decls (forward + body)
    for seg in segs:
        byname = {}
        for p in segs[seg]:
            d = byname.setdefault(p["name"], {"name": p["name"], "strings": set(),
                                              "calls": set()})
            d["strings"] |= p["strings"]
            d["calls"] |= p["calls"]
        segs[seg] = list(byname.values())
    return segs


# ---- DOS side ---------------------------------------------------------
def dos_facts(seg, proc, sp):
    d = seg.data
    ip, end, anchor = proc.entry, proc.code_end, proc.anchor
    strings, calls = set(), set()
    consts = []
    while 0 <= ip < end:
        txt, kind, nxt = P.decode_one(d, ip, anchor, pretty=True)
        parts = txt.split()
        mn = parts[0]
        if mn in ("SLDC", "LDCI"):
            consts.append(int(parts[1]))
        elif mn == "LSA":
            q = txt.find('"')
            lit = txt[q + 1:txt.rfind('"')]
            if len(lit) >= 2:
                add(strings, norm(lit))
            consts.clear()
        elif mn in ("CXP", "CLP", "CGP", "CIP", "CBP"):
            calls.add((mn, parts[1] if len(parts) > 1 else ""))
            if mn == "CXP" and parts[1:2] == ["WIZARDRY,38"] and consts:
                b = sp.get(consts[-1])
                if b and len(b) >= 2:
                    add(strings, norm(b.decode("latin1")))
            consts.clear()
        elif mn not in ("SLDL", "SLDO", "SIND"):
            consts.clear()
        if nxt is None or nxt <= ip:
            break
        ip = nxt
    return strings, calls


def resolve_call(seg, mn, arg, segs_by_name):
    """(mn,arg) -> ('SEGNAME', procnum) or None"""
    if mn == "CXP":
        s, _, p = arg.partition(",")
        return (s, int(p)) if p else None
    if mn in ("CLP", "CGP", "CIP", "CBP"):
        return (seg.name, int(arg))
    return None


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        sys.exit(2)
    pas, krn, applesrc = argv[:3]
    sp = strpool.StringPool(krn)
    segs = P.load_segments(pas)
    segmap = {s.name: s for s in segs}
    apple = parse_apple(applesrc)
    apple_by_dosseg = {APPLE_SEG[k]: v for k, v in apple.items() if k in APPLE_SEG}

    # seeds
    name = {}            # (segname, procnum) -> (apple_name, conf, evidence)
    if os.path.exists(SEED):
        for l in open(SEED):
            if l.startswith("#") or "\t" not in l:
                continue
            f = l.rstrip("\n").split("\t")
            name[(f[0], int(f[1]))] = (f[2], f[3], f[4] if len(f) > 4 else "")

    # DOS facts
    facts = {}
    for s in segs:
        for pr in s.procs:
            if pr.entry < 0:
                continue
            facts[(s.name, pr.num)] = dos_facts(s, pr, sp)

    # iterative propagation, per matched segment
    for _ in range(6):
        for dosname, aprocs in apple_by_dosseg.items():
            s = segmap.get(dosname)
            if not s:
                continue
            taken = {v[0] for k, v in name.items() if k[0] == dosname}
            for pr in sorted(s.procs, key=lambda x: x.entry):
                key = (dosname, pr.num)
                if key in name:
                    continue
                dstr, dcalls = facts.get(key, (set(), set()))
                resolved = set()
                for mn, arg in dcalls:
                    r = resolve_call(s, mn, arg, segmap)
                    if r and r in name:
                        resolved.add(name[r][0])
                best = None
                for ap in aprocs:
                    if ap["name"] in taken:
                        continue
                    sc = 3 * len(dstr & ap["strings"]) + 2 * len(resolved & ap["calls"])
                    if sc and (best is None or sc > best[0]):
                        best = (sc, ap)
                    elif best and sc == best[0]:
                        best = (best[0], None)          # ambiguous
                if best and best[1] and best[0] >= 4:
                    ap = best[1]
                    ev = []
                    if dstr & ap["strings"]:
                        ev.append("str:" + ",".join(sorted(dstr & ap["strings"])[:3]))
                    if resolved & ap["calls"]:
                        ev.append("calls:" + ",".join(sorted(resolved & ap["calls"])[:3]))
                    name[key] = (ap["name"], f"auto:{best[0]}", " ".join(ev))
                    taken.add(ap["name"])

    with open(OUT, "w") as fh:
        fh.write("# dos_seg\tdos_proc\tapple_name\tconf\tevidence\n")
        for (sn, pn), (an, cf, ev) in sorted(name.items()):
            fh.write(f"{sn}\t{pn}\t{an}\t{cf}\t{ev}\n")

    man = sum(1 for v in name.values() if v[1] == "manual")
    auto = len(name) - man
    total = sum(1 for s in segs for pr in s.procs if pr.entry >= 0)
    print(f"{OUT}: {len(name)}/{total} procs named  ({man} manual, {auto} auto)")
    for (sn, pn), (an, cf, ev) in sorted(name.items()):
        if cf != "manual":
            print(f"  {sn:9s} {pn:3d}  {an:14s} {cf:9s} {ev}")


if __name__ == "__main__":
    main(sys.argv[1:])
