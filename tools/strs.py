#!/usr/bin/env python3
"""Print the string literals (wide or narrow) referenced by `offset <label>` in a line range of
the IDA .asm export.
Usage: strs.py <file.asm> <first-line> <last-line>
Each result is `label: "text"`.

IDA lists a wide string as a run of `db 'x',0` fragments, each followed by an `align 4` (and
IDA gives each fragment its own auto name), so the text is followed across labels and
alignment directives until a zero wide character."""
import re
import struct
import sys
sys.stdout.reconfigure(encoding="utf-8", errors="replace")

path, first, last = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])

labels = []
seen = set()
with open(path, encoding="utf-8", errors="replace") as f:
    for n, line in enumerate(f, 1):
        if n < first:
            continue
        if n > last:
            break
        for m in re.finditer(r"offset (\w+)", line):
            lab = m.group(1)
            if lab not in seen and not lab.startswith(("_Z", "sub_", "loc_", "off_", "stru_", "j_")):
                seen.add(lab)
                labels.append(lab)

want = set(labels)
starts = {}
lines = []
with open(path, encoding="utf-8", errors="replace") as f:
    for n, line in enumerate(f, 1):
        if n < 2000000:
            continue
        lines.append(line)
        m = re.match(r"^(\w+)\s+(dd|db|dw|dq)\b", line)
        if m and m.group(1) in want:
            starts[m.group(1)] = len(lines) - 1


def parse_tokens(body, data):
    m = re.match(r"\s*(?:\w+\s+)?(dd|db|dw|dq)\s+(.*)$", body.rstrip())
    if not m:
        return False
    size = {"db": 1, "dw": 2, "dd": 4, "dq": 8}[m.group(1)]
    for tok in [t.strip() for t in m.group(2).split(",")]:
        if tok.startswith("'") and tok.endswith("'") and len(tok) >= 2:
            for ch in tok[1:-1]:
                data += ord(ch).to_bytes(size, "little")
        elif re.match(r"^[0-9A-Fa-f]+h$", tok):
            data += int(tok[:-1], 16).to_bytes(size, "little")
        elif re.match(r"^\d+$", tok):
            data += int(tok).to_bytes(size, "little")
        else:
            return False
    return True


def read_string(start):
    data = bytearray()
    i = start
    first_line = True
    while i < len(lines):
        line = lines[i]
        body = line.split(";")[0]
        # another string that something refers to starts here
        if not first_line and not line.startswith((" ", "	")) and "XREF" in line:
            break
        if first_line:
            body = re.sub(r"^\w+\s+", "", body, count=1)
            first_line = False
        elif not body.startswith((" ", "\t")):
            body = re.sub(r"^\w+\s+", "", body, count=1)
        stripped = body.strip()
        if stripped.startswith("align"):
            while len(data) % 4:
                data.append(0)
        elif stripped:
            if not parse_tokens(body, data):
                break
        # a whole zero wide character ends the string
        if len(data) % 4 == 0 and len(data) >= 4 and data[-4:] == b"\0\0\0\0":
            break
        if len(data) > 8000:
            break
        i += 1
    return bytes(data)


def decode(b):
    if len(b) >= 4 and b[1:4] == b"\0\0\0":
        out = []
        for k in range(0, len(b) - 3, 4):
            v = struct.unpack_from("<I", b, k)[0]
            if v == 0:
                break
            out.append(chr(v) if v < 0x110000 else "?")
        return 'L"%s"' % "".join(out).replace("\n", "\\n")
    s = b.split(b"\0")[0]
    try:
        return '"%s"' % s.decode("utf-8").replace("\n", "\\n")
    except UnicodeDecodeError:
        return repr(s)


for lab in labels:
    if lab in starts:
        print("%s: %s" % (lab, decode(read_string(starts[lab]))))
