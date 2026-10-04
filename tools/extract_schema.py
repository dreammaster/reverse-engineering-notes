#!/usr/bin/env python3
"""Extracts the game-data schema from an IDA .asm export.

Every data-record class TT<Name> has a static `InitType(int, int)` that builds
its TTypeGroup: `ClearTypes`, then one TTypeData constructor + `AddType` per
field, then `SetCreate`/`SetInit`/`SetNameInList`. The constructor arguments
are loaded into registers/stack slots by plain `mov`/`xor` instructions, so the
whole table can be read off mechanically. This writes two tab-separated files:

  manifest/schema_groups.tsv  one row per class: its TTypeGroup constructor
                              arguments (from the class's static initializer)
                              and its create/init/name callbacks.
  manifest/schema_fields.tsv  one row per field of every class (in definition
                              order): field id, XML name, value kind, link
                              table, version range and savegame info.

Needs manifest/xml_names.tsv (see extract_xml_names.py) for the field names.

Usage: python tools/extract_schema.py <GameName>_Linux.asm
"""
import csv
import os
import re
import sys

ASM = sys.argv[1] if len(sys.argv) > 1 else "Deponia_Linux.asm"


STACK_RE = re.compile(r"^\s+mov\s+(?:dword ptr )?\[rsp\+([0-9A-F]+)h\+var_([0-9A-F]+)\], (-?[0-9A-F]+h?)\b")


def to_int(tok):
    tok = tok.strip()
    v = int(tok[:-1], 16) if tok.endswith("h") else int(tok)
    if v >= 0x80000000:
        v -= 1 << 32
    return v


def load_names(path="manifest/xml_names.tsv"):
    names = {}
    with open(path, encoding="utf-8") as f:
        for r in csv.DictReader(f, delimiter="\t"):
            names.setdefault(int(r["id"]), r["name"])
    return names


def find_init_types(lines):
    """class name -> line index of its InitType(int, int)."""
    result = {}
    for i, l in enumerate(lines):
        m = re.match(r"^_ZN(\d+)(\w+)InitTypeEii proc near", l)
        if not m:
            continue
        n = int(m.group(1))
        # the mangled name is <len><Class><len>InitType...: m.group(2) is "<Class>8"
        cls = m.group(2)[:n]
        if cls.startswith("TT") or cls.startswith("TS"):
            result[cls] = i
    return result


def parse_fields(lines, cls, start, names):
    fields = []
    regs = {}
    stack = {}
    order = 0
    callbacks = {}
    for l in lines[start + 1:start + 12000]:
        if re.search(r"^_ZN\w+ endp", l) and "InitType" in l:
            break
        m = re.match(r"^\s+mov\s+(esi|edx|ecx|r8d|r9d), (-?[0-9A-F]+h?)\b", l)
        if m and "offset" not in l:
            try:
                regs[m.group(1)] = to_int(m.group(2))
            except ValueError:
                regs[m.group(1)] = None
        m = re.match(r"^\s+xor\s+(esi|edx|ecx|r8d|r9d), (esi|edx|ecx|r8d|r9d)\b", l)
        if m and m.group(1) == m.group(2):
            regs[m.group(1)] = 0
        m = re.match(r"^\s+mov\s+(esi|edx|ecx|r8d|r9d), (esi|edx|ecx|r8d|r9d)\b", l)
        if m:
            regs[m.group(1)] = regs.get(m.group(2))
        m = re.match(r"^\s+mov\s+(?:dword ptr )?\[rsp\+([0-9A-F]+)h\+var_([0-9A-F]+)\], (-?[0-9A-F]+h?)\b", l)
        if m:
            off = int(m.group(1), 16) - int(m.group(2), 16)
            if off in (0, 8):
                try:
                    stack[off] = to_int(m.group(3))
                except ValueError:
                    stack[off] = None
        if re.search(r"call\s+_ZN9TTypeDataC2", l):
            if "eSaveGame" in l:
                kind = "save"
            elif "eVisionaireTable" in l:
                kind = "link"
            else:
                kind = "plain"
            ident = regs.get("esi")
            row = {"class": cls, "order": order, "id": ident, "name": names.get(ident, "?"),
                   "type": regs.get("edx"), "kind": kind}
            if kind == "plain":
                row.update(link=-2, vin=regs.get("ecx"), vout=regs.get("r8d"), save="", sin=-1, sout=-1)
            elif kind == "link":
                row.update(link=regs.get("ecx"), vin=regs.get("r8d"), vout=regs.get("r9d"), save="", sin=-1, sout=-1)
            else:
                row.update(link=-2, vin=regs.get("r8d"), vout=regs.get("r9d"), save=regs.get("ecx"),
                           sin=stack.get(0), sout=stack.get(8))
            fields.append(row)
            order += 1
            regs = {}
            stack = {}
        if re.search(r"call\s+_ZN10TTypeGroup10ClearTypesEii", l):
            regs = {}
        m = re.search(r"mov\s+esi, offset (_ZN\w+?(OnCreate|OnInit|GetNameInList)\w*)", l)
        if m:
            callbacks[m.group(2)] = m.group(1)
    return fields, callbacks


def main():
    names = load_names()
    with open(ASM, encoding="utf-8", errors="replace") as f:
        lines = [l.rstrip("\n") for l in f]

    init_types = find_init_types(lines)
    fields = []
    groups = {}
    for cls, start in sorted(init_types.items(), key=lambda kv: kv[1]):
        rows, cb = parse_fields(lines, cls, start, names)
        fields.extend(rows)
        groups[cls] = {"callbacks": cb}

    # each class's static initializer builds its TTypeGroup: scan every
    # _GLOBAL__sub_I_* function for `mov edi, offset <Class>::typeGroup` followed
    # by the TTypeGroup constructor call
    i = 0
    n = len(lines)
    while i < n:
        l = lines[i]
        if not l.startswith("_GLOBAL__sub_I_") or " proc near" not in l:
            i += 1
            continue
        j = i
        regs = {}
        stack = {}
        cls = None
        while j < n and not lines[j].startswith("_GLOBAL__sub_I_" ) or j == i:
            l2 = lines[j]
            if re.match(r"^_\w+ endp", l2):
                break
            mm = re.match(r"^\s+mov\s+edi, offset _ZN(\d+)(\w+?)9typeGroupE", l2)
            if mm:
                cls = mm.group(2)[:int(mm.group(1))]
            mm = re.match(r"^\s+mov\s+(esi|edx|ecx|r8d|r9d), (-?[0-9A-F]+h?)\b", l2)
            if mm and "offset" not in l2:
                try:
                    regs[mm.group(1)] = to_int(mm.group(2))
                except ValueError:
                    pass
            mm = re.match(r"^\s+xor\s+(esi|edx|ecx|r8d|r9d), (esi|edx|ecx|r8d|r9d)\b", l2)
            if mm and mm.group(1) == mm.group(2):
                regs[mm.group(1)] = 0
            mm = re.match(r"^\s+mov\s+(esi|edx|ecx|r8d|r9d), (esi|edx|ecx|r8d|r9d)\b", l2)
            if mm:
                regs[mm.group(1)] = regs.get(mm.group(2))
            mm = STACK_RE.match(l2)
            if mm:
                off = int(mm.group(1), 16) - int(mm.group(2), 16)
                if off in (0, 8):
                    stack[off] = to_int(mm.group(3))
            mm = re.search(r"call\s+_ZN10TTypeGroupC2Ei16eVisionaireTable(\w*?)\s", l2)
            if mm and cls in groups:
                if "eSaveGame" in l2:
                    groups[cls].update(desc=regs.get("esi"), table=regs.get("edx"), save=regs.get("ecx"),
                                       vin=regs.get("r8d"), vout=regs.get("r9d"), sin=stack.get(0),
                                       sout=stack.get(8), form="save")
                else:
                    groups[cls].update(desc=regs.get("esi"), table=regs.get("edx"), vin=regs.get("ecx"),
                                       vout=regs.get("r8d"), save="", sin=-1, sout=-1, form="plain")
                regs = {}
                stack = {}
            j += 1
        i = j + 1

    os.makedirs("manifest", exist_ok=True)
    with open("manifest/schema_groups.tsv", "w", encoding="utf-8", newline="\n") as f:
        f.write("class\tdesc\tdesc_name\ttable\tversion_in\tversion_out\tform\tcallbacks\tsave_type\tsave_in\tsave_out\n")
        for cls, g in sorted(groups.items()):
            f.write("{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\n".format(
                cls, g.get("desc"), names.get(g.get("desc"), "?"), g.get("table"), g.get("vin"), g.get("vout"),
                g.get("form", ""), ",".join(sorted(g["callbacks"])), g.get("save", ""), g.get("sin", ""),
                g.get("sout", "")))
    with open("manifest/schema_fields.tsv", "w", encoding="utf-8", newline="\n") as f:
        f.write("class\torder\tid\tname\ttype\tkind\tlink\tversion_in\tversion_out\tsavegame\tsave_in\tsave_out\n")
        for r in fields:
            f.write("{class}\t{order}\t{id}\t{name}\t{type}\t{kind}\t{link}\t{vin}\t{vout}\t{save}\t{sin}\t{sout}\n".format(**r))
    print(f"{len(groups)} classes, {len(fields)} fields")


main()
