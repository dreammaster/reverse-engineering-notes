"""Compares the fields that every OnCreate/OnInit of src/deponia1/vstables/recordCallbacks.cpp sets with the ones in the disassembly.
The generator (gen_callbacks.py) lost the last statement of the functions that end in a tail call (a jmp to SetValue); this finds them.
(The name kVisionaireAdventure in the output is the annotation of the number 100: noise.)
Usage: python tools/check_callbacks.py"""
import re, subprocess, sys
from collections import Counter
src = open("src/deponia1/vstables/recordCallbacks.cpp", encoding="utf-8").read().replace("\r\n", "\n")
funcs = {}
for m in re.finditer(r"^void (\w+)::(OnCreate|OnInit)\(TVisionaireObject \*\w*\) \{\n(.*?)^\}\n", src, re.M | re.S):
    funcs[(m.group(1), m.group(2))] = m.group(3)
lines = open("Deponia_Linux.asm", encoding="utf-8", errors="replace").read().split("\n")
starts = {}
for i, l in enumerate(lines):
    m = re.match(r"^_ZN\d+(\w+?)(8OnCreate|6OnInit|6OnInit)EP17TVisionaireObject proc near", l)
    m2 = re.match(r"^_ZN(\d+)([A-Za-z0-9_]+?)(8OnCreate|6OnInit)EP17TVisionaireObject proc near", l)
    if m2:
        n = int(m2.group(1))
        rest = m2.group(2) + m2.group(3)
        cls = rest[:n]
        kind = "OnCreate" if m2.group(3) == "8OnCreate" else "OnInit"
        starts[(cls, kind)] = i + 1
mismatch = 0
for key, body in sorted(funcs.items()):
    if key not in starts:
        continue
    first = starts[key]
    end = first
    while end < len(lines) and not re.match(r"^\S+ endp", lines[end]):
        end += 1
    r = subprocess.run([sys.executable, "tools/avc.py", "Deponia_Linux.asm", str(first), str(end + 1)], capture_output=True, text=True)
    afields = Counter(re.findall(r";(k[A-Z][A-Za-z0-9]+)", r.stdout))
    cfields = Counter(re.findall(r"\b(k[A-Z][A-Za-z0-9]+)\b", body))
    for k in ("kNoEvent", "kSendEvent", "kForce"):
        cfields.pop(k, None); afields.pop(k, None)
    if afields != cfields:
        mismatch += 1
        print("%s::%s asm-only: %s | C++-only: %s" % (key[0], key[1], dict(afields - cfields), dict(cfields - afields)))
print("checked", len(funcs), "mismatches", mismatch)
