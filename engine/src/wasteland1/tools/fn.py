#!/usr/bin/env python3
"""fn.py ADDR... : prints the disassembly of the given functions (offsets inside CS, hex) from ida_scripts/dump_wasteland.txt
with data segment offsets for the IDA names (word_21A2F -> ds:3C0F) and the conditions of cmp/test + jcc pairs spelled out."""
import os
import re
import sys

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "..", "ida_scripts")
dump = open(os.path.join(root, "dump_wl_ext.txt"), errors="replace").read().split(";==== ")
funcs = {}
owner = {}      # address of any instruction -> function start
for chunk in dump[1:]:
    head = chunk.split("\n", 1)[0]
    m = re.search(r"@ ([0-9A-F]+)", head)
    if m:
        start = int(m.group(1), 16)
        funcs[start] = ";==== " + chunk.rstrip()
        for ln in chunk.split("\n")[1:]:
            mm = re.match(r"([0-9A-F]{5})  ", ln)
            if mm:
                owner[int(mm.group(1), 16)] = start
names = {}
for line in open(os.path.join(root, "wl_dsnames.txt")):
    p = line.split()
    if len(p) >= 2:
        names[p[1]] = int(p[0], 16)

COND = {"jz": "==", "jnz": "!=", "jb": "<", "jnb": ">=", "jbe": "<=", "ja": ">", "jl": "< (signed)", "jge": ">= (signed)",
        "jle": "<= (signed)", "jg": "> (signed)", "jnae": "<", "jae": ">=", "je": "==", "jne": "!=", "jc": "carry set",
        "jnc": "carry clear", "js": "sign set", "jns": "sign clear", "jnl": ">= (signed)", "jng": "<= (signed)", "jna": "<=",
        "jnbe": ">"}


def annotate(text):
    def rep(m):
        n = m.group(0)
        mm = re.match(r"(?:byte|word|dword|unk|off)_([0-9A-F]+)$", n)
        if mm:
            a = int(mm.group(1), 16) - 0x1CE20
            if 0 <= a < 0x10000:
                return "%s{ds:%04X}" % (n, a)
        if n in names:
            return "%s{ds:%04X}" % (n, names[n])
        return n
    return re.sub(r"\b[A-Za-z_][A-Za-z_0-9?.]*", rep, text)


def fold(text):
    out = []
    prev = None
    for ln in text.split("\n"):
        m = re.match(r"([0-9A-F]+)\s+(\w+)\s*(.*)", ln)
        note = ""
        if m and m.group(2) in COND and prev:
            op, a = prev
            tgt = m.group(3).split(";")[0].strip().replace("short ", "")
            c = COND[m.group(2)]
            if op == "cmp" and "," in a:
                x, y = [t.strip() for t in a.split(",", 1)]
                note = "   ; if (%s %s %s) goto %s" % (x, c, y, tgt)
            elif op == "test" and "," in a:
                x, y = [t.strip() for t in a.split(",", 1)]
                note = "   ; if ((%s & %s) %s 0) goto %s" % (x, y, c, tgt)
            elif op in ("dec", "inc", "sub", "add", "or", "and", "xor", "neg", "shl", "shr"):
                note = "   ; if (flags of '%s %s' %s) goto %s" % (op, a, c, tgt)
        out.append(ln + note)
        if m and m.group(2) not in COND:
            prev = (m.group(2), m.group(3).split(";")[0].strip())
    return "\n".join(out)


for arg in sys.argv[1:]:
    a = int(arg, 16)
    if a < 0x10000:
        a += 0x10000
    if a not in funcs:
        if a in owner:
            print("; %X is inside the function at %X" % (a, owner[a]))
            a = owner[a]
        else:
            print("no function at %X" % a)
            continue
    print(fold(annotate(funcs[a])))
    print()
