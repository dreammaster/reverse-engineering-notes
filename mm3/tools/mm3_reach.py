#!/usr/bin/env python3
"""Work out which routines of mm3.asm get translated into C, starting from some roots.

usage: mm3_reach.py mm3.asm ROOT[,ROOT...] HOSTFILE > funcs.txt
Everything reachable through calls and jumps is included except the routines listed in HOSTFILE (one name or `prefix*` per line,
`#` comments): those are written by hand in src/ (C runtime, DOS, video module, input, sound, resource files ...).  Routines that
cannot be translated yet are reported on stderr."""
import fnmatch
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import asm2c  # noqa: E402


def main():
    asm, roots, hostfile = sys.argv[1], sys.argv[2].split(","), sys.argv[3]
    idc = os.path.join(os.path.dirname(os.path.abspath(asm)), "mm3.idc")
    hostpats = [l.split("#")[0].strip() for l in open(hostfile) if l.split("#")[0].strip()]
    is_host = lambda n: any(fnmatch.fnmatch(n, p) for p in hostpats)
    tr = asm2c.Translator(asm, idc)
    procs = tr.procs
    calls = {}
    for n, (s, e, k) in procs.items():
        cs = set()
        for l in tr.lines[s:e]:
            t = l.split(";")[0]
            m = re.search(r"\bcall\s+(?:near ptr |far ptr )?([\w@]+)", t)
            if m:
                cs.add(m.group(1))
            m = re.search(r"\bjmp\s+(?:far ptr |near ptr |short )?(\w+)\s*$", t)
            if m and m.group(1) in procs:
                cs.add(m.group(1))
        calls[n] = cs
    seen, queue, order = set(), list(roots), []
    while queue:
        n = queue.pop(0)
        if n.startswith("j_") and n[2:] in procs:
            n = n[2:]
        if n in seen or n not in procs or is_host(n):
            continue
        seen.add(n)
        order.append(n)
        queue.extend(sorted(calls.get(n, ())))
    bad = []
    for n in order:
        try:
            tr.translate(n, set(order))
        except Exception as ex:  # noqa: BLE001
            bad.append((n, str(ex)[:140]))
    for n, m in bad:
        print("UNTRANSLATABLE:", m, file=sys.stderr)
    bad_names = {n for n, _ in bad}
    print("# %d routines" % (len(order) - len(bad_names)), file=sys.stderr)
    for n in order:
        if n not in bad_names:
            print(n)


if __name__ == "__main__":
    main()
