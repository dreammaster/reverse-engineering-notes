#!/usr/bin/env python3
"""Lists the strings (wide or narrow) that a function of an IDA listing refers to, in the order of the references.
Usage: tools/asm_strings.py <listing.asm> <mangled name of the function>"""
import re
import sys

PROC = re.compile(r"^(\S+)\s+proc near")


def load(path):
    return open(path, encoding="utf8", errors="replace").read().split("\n")


def function_lines(lines, name):
    start = None
    for i, l in enumerate(lines):
        if l.startswith(name + " proc near"):
            start = i
        elif start is not None and l.startswith(name + " endp"):
            return lines[start:i]
    return []


def data_bytes(lines, label, index):
    at = index.get(label)
    if at is None:
        return b""
    out = bytearray()
    for l in lines[at:at + 400]:
        code = l.split(";")[0]
        m = re.match(r"^\S+\s+(db|dw|dd|dq)\s+(.*)$", code) if re.match(r"^\S", code) else re.match(r"^\s+(db|dw|dd|dq)\s+(.*)$", code)
        if not m:
            if out and code.strip() and not code.strip().startswith(("align", "public")):
                break
            continue
        kind, rest = m.groups()
        size = {"db": 1, "dw": 2, "dd": 4, "dq": 8}[kind]
        single = re.match(r"^'(.)',\s*0$", rest.strip())
        if single and kind == "db":
            # IDA shows a wide string as `db 'c',0` and `align 4`: a character of 4 bytes
            out += single.group(1).encode("latin1") + bytes(3)
            continue
        for part in rest.split(","):
            part = part.strip()
            rep = re.match(r"(\d+) dup\((\?|[0-9A-Fa-f]+h?)\)", part)
            if rep:
                out += bytes(int(rep.group(1)) * size)
                continue
            if part.startswith("'") and kind == "db":
                out += part.strip("'").encode("latin1")
                continue
            try:
                v = int(part[:-1], 16) if part.endswith("h") else int(part)
            except ValueError:
                continue
            out += v.to_bytes(size, "little", signed=False) if v >= 0 else b""
        if len(out) >= 4 and bytes(out[-4:]) == b"\0\0\0\0" and len(out) % 4 == 0:
            break
    return bytes(out)


def main():
    path, name = sys.argv[1], sys.argv[2]
    lines = load(path)
    index = {}
    for i, l in enumerate(lines):
        m = re.match(r"^((?:qword|dword|word|byte|unk|a|src|off|asc|stru)_?[A-Za-z0-9_]*)\s", l)
        if m:
            index.setdefault(m.group(1), i)
    seen = []
    for l in function_lines(lines, name):
        for m in re.finditer(r"offset ((?:qword|dword|word|byte|unk|a)[A-Za-z0-9_]*)", l):
            if m.group(1) not in seen:
                seen.append(m.group(1))
    for label in seen:
        raw = data_bytes(lines, label, index)
        if not raw:
            continue
        wide = raw
        text = ""
        if len(raw) >= 8 and raw[1:4] == b"\0\0\0":
            for i in range(0, len(raw) - 3, 4):
                c = int.from_bytes(raw[i:i + 4], "little")
                if c == 0:
                    break
                text += chr(c) if c < 0x110000 else "?"
        else:
            text = raw.split(b"\0")[0].decode("latin1")
        print("%-24s %r" % (label, text))


main()
