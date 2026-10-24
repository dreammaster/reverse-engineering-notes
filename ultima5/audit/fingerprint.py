"""
Semantic fingerprint audit: compare, per function, what the original machine code touches (callees, data-segment
addresses) with what the decompiled C touches.  We cannot recompile with MSC 5.1 here, so this stands in for an
instruction-level match.  A function whose binary references something the C never mentions (or vice versa) has
missing/incorrect logic or at least needs a human look.

Outputs audit/fingerprint.json and audit/fingerprint.md
"""
import bisect
import json
import os
import re

import audit_functions as a

OVL_FIRST = 0x7300  # EXE code ends here; overlay load areas start above
CRT_LIMIT = 0x878  # ULTIMA offsets below this are the C runtime (long math helpers, startup)

ID_RE = re.compile(r"\bD_([0-9a-f]{4})\b|\bD_([0-9a-f]{4})_\w+")
HEXLIT_RE = re.compile(r"\b0x([0-9a-fA-F]{3,4})\b")
FIELD_RE = re.compile(r"\b_([0-9a-f]{4})_\w+")


def var_starts():
    starts = set()
    for fn in ("vars.h", "vars.c"):
        for line in open(os.path.join(a.SRC, fn), encoding="utf-8", errors="replace"):
            for m in re.finditer(r"\bD_([0-9a-f]{4})", line):
                starts.add(int(m.group(1), 16))
    return sorted(starts)


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def c_function_text():
    """{(mod, off): text} for the offset-named C definitions (same scanner as audit_functions)."""
    out = {}
    for fn in sorted(os.listdir(a.SRC)):
        if not fn.endswith(".c"):
            continue
        lines = open(os.path.join(a.SRC, fn), encoding="utf-8", errors="replace").read().splitlines()
        n = len(lines)
        i = 0
        while i < n:
            line = lines[i]
            nm = a.NAME_RE.search(line) if line and line[0] not in " \t/#{}" else None
            j = i
            if nm and not line.rstrip().endswith(";"):
                while j < n and j - i < 6 and "{" not in lines[j] and ";" not in lines[j]:
                    j += 1
            if nm and j < n and "{" in lines[j] and (j == i or ";" not in lines[j - 1]) and "=" not in line.split("(")[0]:
                k = j
                if not lines[j].rstrip().endswith("}"):
                    k = j + 1
                    while k < n and not lines[k].startswith("}"):
                        k += 1
                out[(nm.group(1), int(nm.group(2), 16))] = (fn, i + 1, "\n".join(lines[i:k + 1]))
                i = k
            i += 1
    return out


def main():
    starts = var_starts()
    raw = c_function_text()
    import constants  # lazy: constants imports this module

    ppf = {}
    for fn, text in constants.preprocess_all().items():
        for key, bodies in constants.functions_from_text(text).items():
            ppf.setdefault(key, []).extend(bodies)
    # preprocessed body (matching DOS16 configuration, macros expanded) with the raw source's file:line for reporting
    ctext = {k: (v[0], v[1], chr(10).join(ppf.get(k, [v[2]]))) for k, v in raw.items()}

    def container(addr):
        i = bisect.bisect_right(starts, addr) - 1
        return starts[i] if i >= 0 else None

    def next_start(s):
        i = bisect.bisect_right(starts, s)
        return starts[i] if i < len(starts) else 0x10000

    bases = json.load(open(os.path.join(a.OUT, "ovl_bases.json")))
    exe = json.load(open(os.path.join(a.OUT, "ULTIMA.json")))
    exe_starts = set(f["start"] - 0x10000 for f in exe["functions"])
    ovl_starts = {m2: set(f["start"] for f in json.load(open(os.path.join(a.OUT, m2 + ".json")))["functions"]) for m2 in a.OVERLAYS}
    stubs = json.load(open(os.path.join(a.OUT, "stubs.json")))["stubs"]
    results = {}
    for mod in a.OVERLAYS + ["ULTIMA"]:
        js = json.load(open(os.path.join(a.OUT, mod + ".json")))
        base = 0x10000 if mod == "ULTIMA" else 0
        modsize = js["segments"][0]["end"] - base if mod != "ULTIMA" else 0
        rows = []
        for f in js["functions"]:
            off = f["start"] - base
            key = (mod, off)
            if key not in ctext:
                continue
            fn, line, text = ctext[key]
            body = strip_comments(text)

            # ---- callees ------------------------------------------------------------------------------
            names = [(m.group(1), int(m.group(2), 16)) for m in a.NAME_RE.finditer(body)]
            c_calls = set(names)
            if names.count(key) <= 1:  # the definition itself; more than one occurrence means real recursion
                c_calls.discard(key)
            b_only = []
            matched = set()
            for t in f["calls"]:
                if t < 0:
                    continue
                cands = []
                if mod != "ULTIMA" and 0 <= t < modsize:
                    cands = [(mod, t)]
                else:
                    F = t if mod == "ULTIMA" else (t + bases[mod]) & 0xFFFF
                    if str(F) in stubs:
                        cands = [tuple(stubs[str(F)])]
                    else:
                        cands = [("ULTIMA", F)]
                hit = [c for c in cands if c in c_calls]
                if hit:
                    matched.add(hit[0])
                elif not (cands and cands[0][0] == "ULTIMA" and cands[0][1] < CRT_LIMIT):
                    b_only.append(cands[0] if len(cands) == 1 else (("|".join(c[0] for c in cands) or "?"), cands[0][1] if cands else t))
            b_only = sorted(set(b_only))
            c_only = sorted(x for x in c_calls if x not in matched)

            # ---- data refs ----------------------------------------------------------------------------
            c_nums = set(v for v in constants.c_constants(body) if 0 < v < 0x10000)
            c_ids = set(int(m.group(1) or m.group(2), 16) for m in ID_RE.finditer(body))
            c_fields = set(int(m.group(1), 16) for m in FIELD_RE.finditer(body))
            c_lits = set(int(m.group(1), 16) for m in HEXLIT_RE.finditer(body))
            bin_addrs = sorted(set(x[1] for x in f["mems"]))
            used_addrs = sorted(set(bin_addrs) | set(x[1] for x in f["imms"]))
            # a C id covers [id - 0x20, next var start) : folded constant displacements can fall slightly below the base
            cover = [(i - 0x60, next_start(i)) for i in c_ids]
            unexplained = []
            for ad in bin_addrs:
                if ad < 0x100:
                    continue
                if any(lo <= ad < hi for lo, hi in cover) or ad in c_fields or ad in c_lits:
                    continue
                if any(abs(ad - x) <= 0x60 for x in c_fields | c_lits):
                    continue
                # base + literal offset folded into one constant (e.g. &D_b21e[0x229] -> b447)
                if any(lo <= ad - k < hi for k in c_nums for lo, hi in cover):
                    continue
                unexplained.append(ad)
            c_unused = sorted(i for i in c_ids if not any(i - 0x60 <= x < next_start(i) or (x - i) in c_nums for x in used_addrs))

            rows.append(dict(off=off, size=f["size"], insns=f["insns"], jcc=f["jcc"], clines=text.count("\n") + 1,
                             file=fn, line=line, b_only_calls=b_only, c_only_calls=c_only,
                             unexplained=unexplained, c_unused=c_unused))
        results[mod] = rows

    json.dump(results, open(os.path.join(a.ROOT, "audit", "fingerprint.json"), "w"), indent=0)

    md = ["# Semantic fingerprint audit\n\n",
          "For each C-defined function, the original binary's callees and data-segment references are compared to the "
          "decompiled C. *Binary-only* items are things the machine code does that the C never mentions (possible "
          "missing logic). *C-only* items are things the C does that the machine code never does (possible wrong "
          "variable/call). CRT long-math helper calls are ignored. Expect some noise: macros, constant folding, "
          "variables accessed through casts/struct fields.\n\n"]
    tot = dict(funcs=0, clean=0)
    for mod, rows in results.items():
        bad = [r for r in rows if r["b_only_calls"] or r["c_only_calls"] or r["unexplained"] or r["c_unused"]]
        tot["funcs"] += len(rows)
        tot["clean"] += len(rows) - len(bad)
        md.append("## %s: %d functions compared, %d with differences\n\n" % (mod, len(rows), len(bad)))
        if bad:
            md.append("| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |\n|---|---|---|---|---|---|\n")
            for r in bad:
                md.append("| %04x | %s:%d | %s | %s | %s | %s |\n" % (
                    r["off"], r["file"], r["line"],
                    " ".join("%s:%04x" % x for x in r["b_only_calls"]), " ".join("%s:%04x" % x for x in r["c_only_calls"]),
                    " ".join("%04x" % x for x in r["unexplained"]), " ".join("%04x" % x for x in r["c_unused"])))
        md.append("\n")
    md.insert(1, "**Overall:** %d of %d compared functions have no fingerprint differences.\n\n" % (tot["clean"], tot["funcs"]))
    open(os.path.join(a.ROOT, "audit", "fingerprint.md"), "w").write("".join(md))
    for mod, rows in results.items():
        bad = [r for r in rows if r["b_only_calls"] or r["c_only_calls"] or r["unexplained"] or r["c_unused"]]
        print("%-9s %3d/%3d differ  (call:%d cvars:%d bdata:%d)" % (
            mod, len(bad), len(rows), sum(1 for r in rows if r["b_only_calls"] or r["c_only_calls"]),
            sum(1 for r in rows if r["c_unused"]), sum(1 for r in rows if r["unexplained"])))
    print(tot)


if __name__ == "__main__":
    main()
