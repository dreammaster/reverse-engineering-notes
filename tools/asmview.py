#!/usr/bin/env python3
"""Print a range of an IDA .asm export without the noise: keep labels (minus
their XREF comments), drop pure comment/blank/align lines, shorten calls.
Usage: asmview.py [-t] <file.asm> <first-line> <last-line>
(-t: also drop prologue/epilogue and wstring-refcount noise)"""
import re
import sys

sys.stdout.reconfigure(encoding="utf-8", errors="replace")

args = [a for a in sys.argv[1:] if a != "-t"]
terse = "-t" in sys.argv[1:]
path, first, last = args[0], int(args[1]), int(args[2])
NOISE = re.compile(r"exchange_and_add|_M_destroy|S_empty_rep|Unwind_Resume|~pair|^\s*(push|pop)\s+r[a-z0-9]+$|"
                   r"sub\s+r[a-z0-9]+, 18h|cmp\s+r[a-z0-9]+, offset _ZNSbIw|lea\s+rdi, \[r[a-z0-9]+\+10h\]|"
                   r"^\s*nop|mov\s+r[a-z0-9]+, \[rsp\+.*(var|arg)_(10|8|18|20|28|30)\]")
skip = 0
with open(path, encoding="utf-8", errors="replace") as f:
    for n, line in enumerate(f, 1):
        if n < first:
            continue
        if n > last:
            break
        s = line.rstrip()
        if terse and "exchange_and_add" in s:
            skip = 6
            continue
        if skip and terse:
            skip -= 1
            if "_M_destroy" in s or re.match(r"^[A-Za-z_]", s):
                skip = 0
            if not re.match(r"^[A-Za-z_]", s):
                continue
        if not s.strip() or s.lstrip().startswith(";") or "align" in s.split(";")[0]:
            continue
        if re.match(r"^[A-Za-z_][\w@$.?]*:", s):
            print(s.split(";")[0].rstrip())
            continue
        if s.lstrip().startswith(("public ", "var_", "arg_")) or "endp" in s:
            if "endp" in s:
                print(s.strip())
            continue
        if terse and NOISE.search(s.strip()):
            continue
        print("%d\t%s" % (n, s.strip()[:130]))
