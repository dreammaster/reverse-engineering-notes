#!/usr/bin/env python3
"""Compares the event argument of SetValue (and the flag of SetLink / ClearLink) in the C++ with the one in the assembly of
the same function (found by its demangled symbol). Run from the project folder: python tools/eventcheck.py"""
import re, subprocess, sys, glob, os
EV = {0: "kForce", 1: "kSendEvent", 2: "kNoEvent"}
lines = open("Deponia_Linux.asm", encoding="utf-8", errors="replace").read().split("\n")
procs = []
for i, l in enumerate(lines):
    m = re.match(r"^(_Z\w+)\s+proc near", l)
    if m:
        procs.append((m.group(1), i + 1))
names = subprocess.run(["c++filt"], input="\n".join(p[0] for p in procs), capture_output=True, text=True).stdout.split("\n")
index = {}
for (sym, line), dem in zip(procs, names):
    base = dem.split("(")[0]
    index.setdefault(base, []).append(line)
def asm_pairs(start):
    end = start
    while end < len(lines) and " endp" not in lines[end]:
        end += 1
    r = subprocess.run([sys.executable, "tools/avc.py", "Deponia_Linux.asm", str(start), str(end)], capture_output=True, text=True)
    asm = {}
    ecx = None; edx = None; field = None
    for line in r.stdout.split("\n"):
        t = line.strip()
        mm = re.match(r"mov\s+ecx,\s*(\d+)$", t)
        if mm: ecx = int(mm.group(1))
        if re.match(r"xor\s+ecx,\s*ecx", t): ecx = 0
        mm = re.match(r"mov\s+esi,\s*\w+h?\s*;(k\w+)", t)
        if mm: field = mm.group(1)
        if re.search(r"CALL TVisObjRef::SetValue\(int,[^)]*,TSendEventEnum\)", t) and field and ecx is not None:
            asm.setdefault(field, set()).add(EV.get(ecx, "?"))
        mm = re.match(r"mov\s+edx,\s*(\d+)$", t)
        if mm: edx = int(mm.group(1))
        if re.match(r"xor\s+edx,\s*edx", t): edx = 0
        if re.search(r"CALL TVisObjRef::SetLink\(int,TVisObjRef const&,bool\)", t) and field and ecx is not None:
            asm.setdefault("L:" + field, set()).add(str(ecx))
        if re.search(r"CALL TVisObjRef::ClearLink\(int,bool\)", t) and field and edx is not None:
            asm.setdefault("C:" + field, set()).add(str(edx))
        if "CALL" in t and "SetValue" not in t: ecx = None
        if "CALL" in t: edx = None
    return asm
skip = ("graphicslib", "p2t", "WxStub", "SdlStub", "recordCallbacks", "records.cpp", "visionaireGameUpgrade", "playerCommands")
rows = []; nf = 0; npairs = 0
for fn in sorted(glob.glob("src/deponia1/**/*.cpp", recursive=True)):
    if any(x in fn for x in skip): continue
    s = open(fn, encoding="utf-8", errors="replace").read().replace("\r\n", "\n")
    for m in re.finditer(r"\n(?![/#\s])[^\n;{}]*?(\w+::~?\w+)\(", s):
        name = m.group(1)
        brace = s.find("{\n", m.end())
        semi = s.find(";", m.end())
        if brace < 0 or (semi >= 0 and semi < brace and s.find("{", m.end(), semi) < 0 and ":" not in s[m.end():semi].replace("::", "")):
            continue
        if name not in index: continue
        end = s.find("\n}\n", brace)
        body = s[brace:end]
        cpp = {}
        for cm in re.finditer(r"SetValue\(\s*(k[A-Za-z0-9_]+)\s*,[^;]*?TSendEventEnum::(kForce|kSendEvent|kNoEvent)\s*\)", body, re.S):
            cpp.setdefault(cm.group(1), set()).add(cm.group(2))
        for cm in re.finditer(r"SetLink\(\s*(k[A-Za-z0-9_]+)\s*,[^;]*?,\s*(true|false)\s*\)", body, re.S):
            cpp.setdefault("L:" + cm.group(1), set()).add("1" if cm.group(2) == "true" else "0")
        for cm in re.finditer(r"ClearLink\(\s*(k[A-Za-z0-9_]+)\s*,\s*(true|false)\s*\)", body, re.S):
            cpp.setdefault("C:" + cm.group(1), set()).add("1" if cm.group(2) == "true" else "0")
        if not cpp: continue
        nf += 1
        asm = {}
        for st in index[name]:
            for f, e in asm_pairs(st).items():
                asm.setdefault(f, set()).update(e)
        for field, evs in cpp.items():
            if field in asm:
                npairs += 1
                if not (evs & asm[field]):
                    rows.append((fn.replace("src/deponia1\\", ""), name, field, sorted(evs), sorted(asm[field])))
for r in rows:
    print("%s: %s\n    %s  C++ %s  asm %s" % r)
print(nf, "functions,", npairs, "pairs,", len(rows), "differences")
