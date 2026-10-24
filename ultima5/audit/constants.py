"""
Constant-level fingerprint audit.

Preprocesses every decompiled C file in its original-matching configuration (-DMSDOS -DM_I86 => TARGET_DOS16 /
MATCHING_BUILD, so macros and enums become literal numbers, and _TEXT(addr,"str") becomes the string's DS address),
then compares, per function, the integer immediates of the original machine code against the integer literals of the C.

  * binary-only constants  => the original does something with a value the C never uses (missing/wrong logic)
  * C-only constants       => the C uses a value the original never loads (wrong constant, or invented code)

Constant folding, strength reduction (x*4 -> shl 2), off-by-one comparison rewrites (<= n  vs  < n+1) and sign
extension are tolerated.  Writes audit/constants.json and audit/constants.md.
"""
import json
import os
import re
import subprocess
import sys
import tempfile

import audit_functions as a
import fingerprint as fp

TEXT_RE = re.compile(r'\b_(?:STATIC_)?TEXT\(\s*(0x[0-9a-fA-F]+)\s*,\s*(?:"(?:[^"\\]|\\.)*"\s*)+\)')
NUM_RE = re.compile(r"(?<![\w.])(0[xX][0-9a-fA-F]+|\d+)(?:[uUlL]*)(?![\w.])")
CHR_RE = re.compile(r"'(\\x[0-9a-fA-F]+|\\.|[^\\'])'")
STR_RE = re.compile(r'"(?:[^"\\]|\\.)*"')

SMALL = 8
BS = chr(92)
ESC = {"n": 10, "r": 13, "t": 9, "0": 0, BS: 92, "'": 39}


def preprocess_all():
    out = {}
    tmp = tempfile.mkdtemp(prefix="u5pp_", dir=os.environ.get("U5_SCRATCH"))
    for fn in sorted(os.listdir(a.SRC)):
        if not fn.endswith(".c"):
            continue
        src = open(os.path.join(a.SRC, fn), encoding="utf-8", errors="replace").read()
        src = TEXT_RE.sub(lambda m: "((char*)%s)" % m.group(1), src)
        tp = os.path.join(tmp, fn)
        open(tp, "w").write(src)
        r = subprocess.run(["gcc", "-E", "-P", "-I" + a.SRC, "-D_COMMON_PLATFORM_H", "-DOS_DOS16=1", "-DTARGET_DOS16=1", "-DCOMPILER_GCC=1", tp, "-o", tp + ".i"],
                           capture_output=True, text=True, cwd=a.SRC)
        if r.returncode != 0:
            print("preprocess failed", fn, r.stderr[-300:], file=sys.stderr)
            continue
        out[fn] = open(tp + ".i", encoding="utf-8", errors="replace").read()
    return out


def functions_from_text(text):
    lines = text.splitlines()
    n = len(lines)
    res = {}
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
            res.setdefault((nm.group(1), int(nm.group(2), 16)), []).append("\n".join(lines[i:k + 1]))
            i = k
        i += 1
    return res


def c_constants(body):
    body = STR_RE.sub('""', body)
    vals = set()
    for m in CHR_RE.finditer(body):
        s = m.group(1)
        vals.add(int(s[2:], 16) if s.startswith(BS + "x") else ord(s) if len(s) == 1 else ESC.get(s[1], 0))
    body = CHR_RE.sub(" ", body)
    for m in NUM_RE.finditer(body):
        t = m.group(1)
        vals.add(int(t, 16) if t.lower().startswith("0x") else int(t))
    return vals


def near(v, pool):
    for d in (0, 1, -1):
        w = (v + d) & 0xFFFF
        if w in pool:
            return True
        if ((-w) & 0xFFFF) in pool:
            return True
    return False


def main():
    pp = preprocess_all()
    ctext = {}
    for fn, text in pp.items():
        for key, bodies in functions_from_text(text).items():
            ctext.setdefault(key, []).extend(bodies)
    starts = fp.var_starts()
    import bisect

    def nxt(s0):
        i = bisect.bisect_right(starts, s0)
        return starts[i] if i < len(starts) else 0x10000

    results = {}
    for mod in a.OVERLAYS + ["ULTIMA"]:
        js = json.load(open(os.path.join(a.OUT, mod + ".json")))
        base = 0x10000 if mod == "ULTIMA" else 0
        rows = []
        for f in js["functions"]:
            key = (mod, f["start"] - base)
            if key not in ctext:
                continue
            cvals = set()
            for b in ctext[key]:
                cvals |= {v & 0xFFFF for v in c_constants(b)}
                # negative literals
            # also allow folded sums/products of two C literals (e.g. 0x20 * 0x10)
            pool = set(cvals)
            small = [v for v in cvals if v <= 0x400]
            for x in small:
                for y in small:
                    pool.add((x + y) & 0xFFFF)
                    pool.add((x * y) & 0xFFFF)
                    pool.add((x - y) & 0xFFFF)
                    pool.add((x << (y & 7)) & 0xFFFF) if y <= 8 else None
            bvals = {v for _, v in f["imms"]}
            ids = set()
            for b in ctext[key]:
                ids |= set(int(m.group(1) or m.group(2), 16) for m in fp.ID_RE.finditer(b))
                ids |= set(int(m.group(1), 16) for m in fp.FIELD_RE.finditer(b))
            cover = [(i - 0x20, nxt(i)) for i in ids]
            mem = {v for _, v in f["mems"]}
            b_only = sorted(v for v in bvals if v > 0x40 and v not in mem and not near(v, pool) and v < 0xFF00
                            and not any(lo <= v < hi for lo, hi in cover))
            # inline displacement constants (e.g. [bx+8]) are in mems/ops not captured; C-only is checked against imms+mems
            bpool = set(bvals) | mem
            for v in list(bvals):
                bpool.add((-v) & 0xFFFF)
                bpool.add((v - 1) & 0xFFFF)
                bpool.add((v + 1) & 0xFFFF)
            c_only = sorted(v for v in cvals if v > 0x40 and v not in bpool and (v & 0xFFFF) not in bpool
                            and not any(v == (x >> s) or v == x << s for x in bvals for s in range(1, 5)))
            rows.append(dict(off=key[1], size=f["size"], b_only=b_only, c_only=c_only))
        results[mod] = rows
    json.dump(results, open(os.path.join(a.ROOT, "audit", "constants.json"), "w"))
    md = ["# Constant-level fingerprint audit\n\n",
          "Integer immediates of the original machine code vs integer literals of the DOS16-configured C. "
          "*bin-only* = loaded by the original but absent from the C; *C-only* = in the C but never loaded by the original. "
          "Overlay string addresses (b21e-relative pools), jump-table case labels and small struct strides are known noise.\n\n"]
    for mod, rows in results.items():
        d = [r for r in rows if r["b_only"] or r["c_only"]]
        md.append("## %s: %d of %d functions differ\n\n" % (mod, len(d), len(rows)))
        if d:
            md.append("| fn | size | bin-only | C-only |\n|---|---|---|---|\n")
            for r in d:
                md.append("| %04x | %d | %s | %s |\n" % (r["off"], r["size"], " ".join("%x" % v for v in r["b_only"]),
                                                          " ".join("%x" % v for v in r["c_only"])))
            md.append("\n")
    open(os.path.join(a.ROOT, "audit", "constants.md"), "w").write("".join(md))
    tot = bad = 0
    for mod, rows in results.items():
        d = [r for r in rows if r["b_only"] or r["c_only"]]
        tot += len(rows)
        bad += len(d)
        print("%-9s %3d/%3d differ  b-only=%d c-only=%d" % (mod, len(d), len(rows), sum(1 for r in rows if r["b_only"]), sum(1 for r in rows if r["c_only"])))
    print(tot, bad)


if __name__ == "__main__":
    main()
