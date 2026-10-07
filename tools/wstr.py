#!/usr/bin/env python3
"""Decode wchar_t (UTF-32LE, as IDA lists them one `db` per byte) string
literals from an IDA .asm export by label.
Usage: wstr.py <file.asm> <label> [<label> ...]"""
import re
import sys

path = sys.argv[1]
labels = set(sys.argv[2:])
found = {}
cur = None
data = []
with open(path, encoding="utf-8", errors="replace") as f:
    for line in f:
        m = re.match(r"^(\w+)\s+db\s", line) or re.match(r"^(\w+)\s", line)
        if m and not line.startswith((" ", "\t")):
            if cur is not None:
                found[cur] = bytes(data)
                cur = None
            if m.group(1) in labels:
                cur, data = m.group(1), []
        if cur is not None:
            m = re.search(r"\bdb\s+([0-9A-F]+)h", line)
            if m:
                data.append(int(m.group(1), 16))
            else:
                m = re.search(r"\bdb\s+(\d+)\b", line)
                if m:
                    data.append(int(m.group(1)))
            if len(data) > 1200:
                found[cur] = bytes(data)
                cur = None
if cur is not None:
    found[cur] = bytes(data)
for label in sys.argv[2:]:
    b = found.get(label, b"")
    text = ""
    for i in range(0, len(b) - 3, 4):
        w = int.from_bytes(b[i:i + 4], "little")
        if w == 0:
            break
        text += chr(w)
    print(label, repr(text))
