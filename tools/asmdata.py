#!/usr/bin/env python3
"""Reads the data sections of an IDA .asm export into an addressable byte image.

IDA's listing shows data as `label db/dw/dd/dq ...` lines plus `align N`
padding, with the *address* only embedded in auto-generated label names
(`unk_D68CA0`, `dword_D6FF3C`, ...). Named strings (`aTop`, `aT_263`) carry no
address, so to decode them this module walks the data lines in order,
re-synchronising whenever it meets a label with a hex suffix, and records
every label's address and every byte it can reconstruct (values that are
`offset X` pointers are skipped, their bytes left as zero).

    data = AsmData("Deponia_Linux.asm")
    data.wstring("aT_263")      # UTF-32LE wide string at that label
    data.cstring("aKeypad")     # NUL-terminated narrow string
"""
import re
import sys

LABEL_RE = re.compile(r"^([A-Za-z_][\w$@?.]*)\s+(db|dw|dd|dq)\b")
ANON_RE = re.compile(r"^\s+(db|dw|dd|dq)\b")
SUFFIX_RE = re.compile(r"_([0-9A-F]{6,7})$")


class AsmData:
    def __init__(self, path, first_line=1, last_line=None):
        self.labels = {}
        self.image = {}  # address -> byte
        self._load(path, first_line, last_line)

    def _load(self, path, first_line, last_line):
        addr = None
        with open(path, encoding="utf-8", errors="replace") as f:
            for n, line in enumerate(f, 1):
                if n < first_line:
                    continue
                if last_line and n > last_line:
                    break
                t = line.strip()
                if not t or t.startswith(";"):
                    continue
                ma = re.match(r"^align\s+(\w+)$", t)
                if ma and addr is not None:
                    v = ma.group(1)
                    a = int(v[:-1], 16) if v.endswith("h") else int(v)
                    while addr % a:
                        self.image[addr] = 0
                        addr += 1
                    continue
                ml = LABEL_RE.match(t)
                body = None
                if ml:
                    label, kind = ml.group(1), ml.group(2)
                    ms = SUFFIX_RE.search(label)
                    if ms:
                        addr = int(ms.group(1), 16)
                    if addr is not None:
                        self.labels[label] = addr
                    body = t[ml.end(2):]
                else:
                    ma2 = ANON_RE.match(line)
                    if not ma2 or addr is None:
                        continue
                    kind = ma2.group(1)
                    body = t[len(kind):]
                if addr is None:
                    continue
                body = body.split(";")[0].strip()
                size = {"db": 1, "dw": 2, "dd": 4, "dq": 8}[kind]
                md = re.match(r"^(\d+)\s+dup\s*\(", body)
                if md:
                    addr += int(md.group(1)) * size
                    continue
                for v in re.findall(r"'(?:[^']|'')*'|[^,]+", body):
                    v = v.strip()
                    if not v:
                        continue
                    if v.startswith("'"):
                        for ch in v[1:-1].encode():
                            self.image[addr] = ch
                            addr += 1
                        continue
                    if v.startswith("offset") or v == "?":
                        addr += size
                        continue
                    tok = v.split()[0]
                    try:
                        val = int(tok[:-1], 16) if tok.endswith("h") else int(tok)
                    except ValueError:
                        addr += size
                        continue
                    for k in range(size):
                        self.image[addr + k] = (val >> (8 * k)) & 0xFF
                    addr += size

    def bytes_at(self, address, count):
        return bytes(self.image.get(address + i, 0) for i in range(count))

    def cstring(self, label):
        a = self.labels[label]
        out = bytearray()
        while a in self.image and self.image[a] != 0:
            out.append(self.image[a])
            a += 1
        return out.decode("utf-8", "replace")

    def wstring(self, label):
        a = self.labels[label]
        out = ""
        while a in self.image:
            c = int.from_bytes(self.bytes_at(a, 4), "little")
            if c == 0 or c > 0x10FFFF:
                break
            out += chr(c)
            a += 4
        return out


if __name__ == "__main__":
    d = AsmData(sys.argv[1])
    for label in sys.argv[2:]:
        print(label, repr(d.wstring(label)), repr(d.cstring(label)))
