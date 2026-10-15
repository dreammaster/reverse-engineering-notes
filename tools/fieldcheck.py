#!/usr/bin/env python3
"""Lists, for every function of src/deponia1 whose original is found by its demangled symbol, the field constants (kXxx)
that the C++ mentions and the assembly of that function does not (noise: enum values, inlined helpers).
Run from the project folder: python tools/fieldcheck.py"""
import re, subprocess, sys, glob
lines = open("Deponia_Linux.asm", encoding="utf-8", errors="replace").read().split("\n")
procs = []
for i, l in enumerate(lines):
    m = re.match(r"^(_Z\w+)\s+proc near", l)
    if m:
        procs.append((m.group(1), i + 1))
names = subprocess.run(["c++filt"], input="\n".join(p[0] for p in procs), capture_output=True, text=True).stdout.split("\n")
index = {}
for (sym, line), dem in zip(procs, names):
    index.setdefault(dem.split("(")[0], []).append(line)
cache = {}
def asm_fields(start):
    if start in cache: return cache[start]
    end = start
    while end < len(lines) and " endp" not in lines[end]:
        end += 1
    r = subprocess.run([sys.executable, "tools/avc.py", "Deponia_Linux.asm", str(start), str(end)], capture_output=True, text=True)
    f = set(re.findall(r";(k[A-Z][A-Za-z0-9]+)", r.stdout))
    cache[start] = f
    return f
skip = ("graphicslib", "p2t", "WxStub", "SdlStub", "recordCallbacks", "records.cpp", "visionaireGameUpgrade", "playerCommands", "TGActionCommands")
for fn in sorted(glob.glob("src/deponia1/**/*.cpp", recursive=True)):
    if any(x in fn for x in skip): continue
    s = open(fn, encoding="utf-8", errors="replace").read().replace("\r\n", "\n")
    for m in re.finditer(r"\n(?![/#\s])[^\n;{}]*?(\w+::~?\w+)\(", s):
        name = m.group(1)
        if name not in index: continue
        brace = s.find("{\n", m.end())
        if brace < 0: continue
        semi = s.find(";", m.end())
        if semi >= 0 and semi < brace and s.find("{", m.end(), semi) < 0 and ":" not in s[m.end():semi].replace("::", ""):
            continue
        end = s.find("\n}\n", brace)
        body = s[brace:end]
        if body.count("\n") > 80: continue
        cf = set(re.findall(r"\b(k[A-Z][A-Za-z0-9]+)\b", body))
        cf = {c for c in cf if c.startswith(("kCharacter","kGame","kScene","kObject","kButton","kInterface","kAction","kAnimation","kText","kOutfit","kCondition","kValue","kDialog","kWay","kPoint","kCursor","kFont","kSprite","kCommentSet","kLoading","kParticle","kScript"))}
        if not cf: continue
        af = set()
        for st in index[name]:
            af |= asm_fields(st)
        cppOnly = sorted(cf - af)
        if cppOnly and af:
            print("%s: %s\n    C++ only: %s" % (fn.replace("src/deponia1\\", ""), name, cppOnly))
