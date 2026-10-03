#!/usr/bin/env python3
"""Extracts the id -> XML name table from TVisionaireGame::InitXMLNames() (and
TXMLNames::InitXMLNamesIntern()) in an IDA .asm export.

Both functions are long runs of
    mov esi, offset <wide string label>      ; the name
    ... call std::wstring::basic_string(wchar_t const*, ...)
    mov edx, <bool>
    mov esi, <id>
    call TXMLNames::AddXMLName(wxString const&, int, bool)
so the table can be read off mechanically. Names are decoded from the data
labels by asmdata.AsmData (UTF-32LE wide strings).

Usage: python tools/extract_xml_names.py <GameName>_Linux.asm [out.tsv]
Output columns: id (decimal), id (hex), name, label, bool flag, function.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from asmdata import AsmData  # noqa: E402

ASM = sys.argv[1] if len(sys.argv) > 1 else "Deponia_Linux.asm"
OUT = sys.argv[2] if len(sys.argv) > 2 else "manifest/xml_names.tsv"

FUNCS = ["_ZN9TXMLNames18InitXMLNamesInternEv", "_ZN15TVisionaireGame12InitXMLNamesEv"]


def find_function_ranges(lines):
    ranges = {}
    for i, l in enumerate(lines):
        for f in FUNCS:
            if l.startswith(f + " proc near"):
                ranges[f] = [i, None]
            if l.startswith(f + " endp") and f in ranges:
                ranges[f][1] = i
    return ranges


def main():
    with open(ASM, encoding="utf-8", errors="replace") as f:
        lines = [l.rstrip("\n") for l in f]
    ranges = find_function_ranges(lines)
    data = AsmData(ASM)
    rows = []
    for func, (a, b) in ranges.items():
        label = None
        ident = None
        flag = None
        for l in lines[a:b]:
            m = re.search(r"mov\s+esi, offset (\w+)", l)
            if m:
                label = m.group(1)
            m = re.search(r"mov\s+(?:esi|edx), (\d+|[0-9A-F]+h)\b", l)
            if m and "offset" not in l:
                v = m.group(1)
                val = int(v[:-1], 16) if v.endswith("h") else int(v)
                if re.search(r"\bmov\s+edx,", l):
                    flag = val
                else:
                    ident = val
            if re.search(r"call\s+_ZN9TXMLNames10AddXMLName", l):
                name = data.wstring(label) if label in data.labels else None
                rows.append((ident, name, label, flag, func))
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("id\thex\tname\tlabel\tflag\tfunction\n")
        for ident, name, label, flag, func in rows:
            f.write(f"{ident}\t0x{ident:X}\t{name}\t{label}\t{flag}\t{func.split('N')[-1]}\n")
    print(f"{len(rows)} names -> {OUT}")


main()
