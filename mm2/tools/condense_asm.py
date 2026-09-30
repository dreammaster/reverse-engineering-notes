"""Condense an IDA .asm export for reading: strip xref comments, blank lines, prologue noise."""
import re, sys
src, dst = sys.argv[1], sys.argv[2]
out = []
for l in open(src, errors="replace"):
    s = l.rstrip()
    if not s.strip() or (s.strip().startswith(";") and "S U B" not in s and "proc" not in s): continue
    s = re.sub(r"\s+; (CODE|DATA) XREF.*", "", s)
    if s.startswith("                                        ;"): continue
    s = re.sub(r"\s+", " ", s)
    if re.match(r"^ (push bp|mov bp, sp|pop bp|mov sp, bp|add sp, \d+|pop si|pop di|push si|push di)$", s) or s.startswith(" align"): continue
    out.append(s)
open(dst, "w").write("\n".join(out))
print(len(out))
