#!/usr/bin/env python3
"""
Global-variable census for DOS SYSTEM.PASCAL.

Globals live in one record based at ds:[600h]; the p-code reaches them with
LDO/SRO/LAO n and the short forms SLDO 1..16 (n = word index). This walks
every procedure and reports, per global word:

  - which procedures read (LDO/SLDO), write (SRO), or take the address of
    (LAO) it, with the proc name from docs/procmap.tsv where known
  - constants stored into it (SRO preceded by SLDC/LDCI)
  - whether it is used as an array base (LAO followed by IXA)

Usage:
    python tools/globals.py <SYSTEM.PASCAL>          [--min-procs 1]
    python tools/globals.py <SYSTEM.PASCAL> --g 24   # detail for one global
    python tools/globals.py gen <Wiz1WizardryPascal.txt>   # build docs/globals.tsv

The DOS global record is a reorganised superset of the Apple one: the DOS
build inserted ~289 words of new state (string tree, caches, kanji, extra
window pointers) after the scalars, but kept the layout from CHARACTR
onward identical -- so  DOS_word = Apple_word + 289  for word >= 363
(verified: CHARACTR 74->363, SCNTOC 698->987 with all 4 internal array
offsets matching, IOCACHE 950->1239). The scalars (Apple 3-27) were
renumbered and are hand-mapped in docs/globals.seed.tsv.
"""
import re
import sys
import os
import collections

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pcode_dis as P

SEED = "docs/globals.seed.tsv"
OUT = "docs/globals.tsv"
DELTA = 289          # DOS_word = Apple_word + DELTA for word >= 363


def apple_var_map(applesrc):
    """-> {apple_word: (name, decl)} from the program VAR section."""
    lines = open(applesrc, encoding="latin1").read().splitlines()
    out = {}
    grab = False
    for l in lines:
        m = re.match(r"\s*\d+\s+(\d+)\s+\d+:[A-Z0-9]\s+(\d+)\s+(.*)", l)
        if not m:
            continue
        seg, off, txt = m.group(1), int(m.group(2)), m.group(3).rstrip()
        if txt.strip() == "VAR" and seg == "1" and not grab:
            grab = True
            continue
        if not grab:
            continue
        if "FORWARD DECLARATIONS" in txt or re.match(r"(PROCEDURE|FUNCTION|BEGIN)\b", txt.strip()):
            break
        dm = re.match(r"([A-Z][A-Z0-9_]*)\s*:\s*(.*)", txt.strip())
        if dm:
            out[off] = (dm.group(1), dm.group(2))
    return out


def cmd_gen(argv):
    applesrc = argv[0]
    avm = apple_var_map(applesrc)

    rows = {}          # word -> (name, conf, note)
    for l in open(SEED):
        if l.startswith("#") or "\t" not in l:
            continue
        f = l.rstrip("\n").split("\t")
        rows[int(f[0])] = (f[1], f[2], f[3] if len(f) > 3 else "")

    # derive DOS names for the +289 region from the Apple scalar VAR names
    for aw, (nm, decl) in avm.items():
        dw = aw + DELTA
        if dw >= 363 and dw not in rows:
            rows[dw] = (nm, f"derived:+{DELTA}", decl[:48])

    with open(OUT, "w") as fh:
        fh.write("# word\tname\tconf\tnote\n")
        for w in sorted(rows):
            nm, cf, nt = rows[w]
            fh.write(f"{w}\t{nm}\t{cf}\t{nt}\n")
    print(f"{OUT}: {len(rows)} globals "
          f"({sum(1 for v in rows.values() if v[1] == 'manual')} manual, "
          f"{sum(1 for v in rows.values() if v[1].startswith('derived'))} derived)")

    # also dump the Apple reference
    ap = "docs/globals.apple.tsv"
    with open(ap, "w") as fh:
        fh.write("# apple_word\tname\tdecl\t(dos_word = +289 for word>=74)\n")
        for w in sorted(avm):
            nm, decl = avm[w]
            fh.write(f"{w}\t{nm}\t{decl}\n")
    print(f"{ap}: {len(avm)} Apple VARs")


def walk(seg, proc):
    """yield (op, glob, extra) for each global reference in the proc."""
    d = seg.data
    ip, end, anchor = proc.entry, proc.code_end, proc.anchor
    prev = None
    while 0 <= ip < end:
        txt, kind, nxt = P.decode_one(d, ip, anchor, pretty=True)
        parts = txt.split()
        mn = parts[0]
        arg = parts[1].rstrip(",").split(",")[0] if len(parts) > 1 else None
        if mn in ("LDO", "SRO", "LAO") and arg and arg.lstrip("-").isdigit():
            g = int(arg)
            extra = None
            if mn == "SRO" and prev and prev[0] in ("SLDC", "LDCI"):
                extra = ("const", prev[1])
            yield (mn, g, extra)
        elif mn == "SLDO":
            yield ("LDO", int(arg), None)
        elif mn == "IXA" and prev and prev[0] == "LAO":
            yield ("ARRB", int(prev[1]), None)
        prev = (mn, int(arg) if arg and arg.lstrip("-").isdigit() else None)
        if nxt is None or nxt <= ip:
            break
        ip = nxt


def main(argv):
    if len(argv) < 1:
        print(__doc__)
        sys.exit(2)
    if argv[0] == "gen":
        cmd_gen(argv[1:])
        return
    pas = argv[0]
    only = None
    minp = 1
    if "--g" in argv:
        only = int(argv[argv.index("--g") + 1])
    if "--min-procs" in argv:
        minp = int(argv[argv.index("--min-procs") + 1])

    P.load_procmap()
    segs = P.load_segments(pas)

    reads = collections.defaultdict(set)
    writes = collections.defaultdict(set)
    addrs = collections.defaultdict(set)
    arrbase = set()
    consts = collections.defaultdict(collections.Counter)

    for s in segs:
        for pr in s.procs:
            if pr.entry < 0:
                continue
            who = P.pname(s.name, pr.num) or f"{s.name}:{pr.num}"
            for op, g, extra in walk(s, pr):
                if op == "LDO":
                    reads[g].add(who)
                elif op == "SRO":
                    writes[g].add(who)
                    if extra and extra[0] == "const":
                        consts[g][extra[1]] += 1
                elif op == "LAO":
                    addrs[g].add(who)
                elif op == "ARRB":
                    arrbase.add(g)

    allg = sorted(set(reads) | set(writes) | set(addrs))
    if only is not None:
        allg = [only]

    for g in allg:
        nusers = len(reads[g] | writes[g] | addrs[g])
        if nusers < minp and only is None:
            continue
        tags = []
        if g in arrbase:
            tags.append("array")
        if consts[g]:
            tags.append("consts=" + ",".join(f"{v}x{k}" for k, v in consts[g].most_common(4)))
        flag = " ".join(tags)
        r, w, a = sorted(reads[g]), sorted(writes[g]), sorted(addrs[g])
        print(f"g{g:<5} rd={len(r):2d} wr={len(w):2d} ad={len(a):2d}  {flag}")
        if only is not None or nusers <= 8:
            if w:
                print(f"        write: {', '.join(w)}")
            if a:
                print(f"        addr : {', '.join(a)}")
            if r and (only is not None or len(r) <= 10):
                print(f"        read : {', '.join(r)}")


if __name__ == "__main__":
    main(sys.argv[1:])
