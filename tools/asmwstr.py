#!/usr/bin/env python3
"""Prints the wide (UTF-32) strings at the given data labels of the disassembly:  tools/asmwstr.py Deponia_Linux.asm unk_D686F0 ..."""
import re
import sys

lines = open(sys.argv[1], encoding="utf-8", errors="replace").read().split("\n")
index = {}
for number, line in enumerate(lines):
    match = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s+d[bdw]\s", line)
    if match:
        index.setdefault(match.group(1), number)
for name in sys.argv[2:]:
    start = index.get(name)
    if start is None:
        print(name, "?")
        continue
    data = bytearray()
    for line in lines[start:start + 2000]:
        if re.match(r"^\s*;", line):
            continue
        match = re.match(r"^(?:[A-Za-z_][A-Za-z0-9_]*\s+)?\s*(db|dw|dd)\s+([^;]*)", line)
        if not match:
            break
        size = {"db": 1, "dw": 2, "dd": 4}[match.group(1)]
        for token in match.group(2).split(","):
            token = token.strip()
            if not token:
                continue
            value = int(token[:-1], 16) if token.endswith("h") else int(token, 0) if re.match(r"^-?\d+$|^0x", token) else None
            if value is None:
                continue
            data += (value & ((1 << (8 * size)) - 1)).to_bytes(size, "little")
        if len(data) >= 4 and data[-4:] == b"\0\0\0\0" and len(data) % 4 == 0:
            break
    text = ""
    for k in range(0, len(data) - 3, 4):
        code = int.from_bytes(data[k:k + 4], "little")
        if code == 0:
            break
        text += chr(code) if code < 0x110000 else "?"
    print(name, repr(text))
