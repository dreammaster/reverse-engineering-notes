#!/usr/bin/env python3
"""Translates the OnCreate()/OnInit() default-value callbacks of the data-record
classes (TTFont::OnCreate, TTScene::OnInit, ...) from an IDA .asm export into
C++. Almost all of them are straight runs of

    mov ecx, <event> ; mov edx, <value> ; mov esi, <field id> ; mov rdi, rbx
    call TVisionaireObject::SetValue(int, <type>, TSendEventEnum)

(a float's value is passed in xmm0 and the event in edx; a point/rect through a
stack slot; a string through a temporary wxString). This reads those off and
writes `object->SetValue(kField, value, event)` statements. A function that
does anything else (creates sub-objects, branches, ...) is written out as far as
it can be understood and marked with a FIXME comment listing what is missing, so
it can be finished by hand; the list is printed at the end.

Writes src/<game>/vstables/recordCallbacks.cpp.

Usage: python tools/gen_callbacks.py <GameName>_Linux.asm [game dir]
"""
import csv
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from asmdata import AsmData  # noqa: E402

ASM = sys.argv[1] if len(sys.argv) > 1 else "Deponia_Linux.asm"
GAME = sys.argv[2] if len(sys.argv) > 2 else "src/deponia1"


def const_names():
    used = set()
    by_id = {}
    with open("manifest/xml_names.tsv", encoding="utf-8") as f:
        for r in csv.DictReader(f, delimiter="\t"):
            ident = int(r["id"])
            if ident in by_id:
                continue
            name = r["name"]
            const = "k" + name[0].upper() + name[1:]
            if const in used:
                const = f"{const}_0x{ident:X}"
            used.add(const)
            by_id[ident] = const
    return by_id


CONSTS = const_names()


def field(ident):
    return CONSTS.get(ident, f"static_cast<eFieldId>({ident})")


def to_int(tok):
    tok = tok.strip()
    v = int(tok[:-1], 16) if tok.endswith("h") else int(tok)
    if v >= 0x80000000:
        v -= 1 << 32
    return v


def load_lines():
    with open(ASM, encoding="utf-8", errors="replace") as f:
        return [l.rstrip("\n") for l in f]


EVENTS = {0: "TSendEventEnum::kForce", 1: "TSendEventEnum::kSendEvent", 2: "TSendEventEnum::kNoEvent"}

# the SetValue overloads by mangled suffix: (kind, c++ conversion)
SETVALUE = re.compile(r"call\s+_ZN17TVisionaireObject(8SetValue|11SetPathList)Ei(\w+?)14TSendEventEnum")


def parse_function(lines, start, data):
    """Returns (statements, unhandled calls)."""
    stmts = []
    unhandled = []
    regs = {}
    xmm = None
    stack = {}
    label = None
    slot = 0
    i = start
    while i < len(lines):
        l = lines[i]
        if re.match(r"^_\w+ endp", l):
            break
        m = re.match(r"^\s+mov\s+(esi|edx|ecx|r8d|r9d), (-?[0-9A-F]+h?)\b", l)
        if m and "offset" not in l:
            regs[m.group(1)] = to_int(m.group(2))
        m = re.match(r"^\s+xor\s+(esi|edx|ecx|r8d|r9d), (esi|edx|ecx|r8d|r9d)\b", l)
        if m and m.group(1) == m.group(2):
            regs[m.group(1)] = 0
        if re.match(r"^\s+(pxor|xorps)\s+xmm0, xmm0", l):
            xmm = 0.0
        m = re.match(r"^\s+lea\s+rdx, \[rsp\+([0-9A-F]+)h\+var_([0-9A-F]+)\]", l)
        if m:
            slot = int(m.group(1), 16) - int(m.group(2), 16)
        m = re.match(r"^\s+movss\s+xmm0, (?:cs:)?(\w+)", l)
        if m:
            label = m.group(1)
            if label in data.labels:
                xmm = struct.unpack("<f", data.bytes_at(data.labels[label], 4))[0]
        m = re.match(r"^\s+mov\s+(?:dword ptr )?\[rsp\+([0-9A-F]+)h\+var_([0-9A-F]+)\], (-?[0-9A-F]+h?)\b", l)
        if m:
            stack[(int(m.group(1), 16) - int(m.group(2), 16))] = to_int(m.group(3))
        m = re.search(r"mov\s+esi, offset (a\w+)", l)
        if m and m.group(1) in data.labels:
            label = ("str", data.wstring(m.group(1)))
        m = SETVALUE.search(l)
        if m:
            suffix = m.group(2)
            fid = regs.get("esi")
            if suffix == "b":
                stmts.append(f"object->SetValue({field(fid)}, {'true' if regs.get('edx') else 'false'}, {EVENTS.get(regs.get('ecx'), regs.get('ecx'))});")
            elif suffix == "i":
                stmts.append(f"object->SetValue({field(fid)}, {regs.get('edx')}, {EVENTS.get(regs.get('ecx'), regs.get('ecx'))});")
            elif suffix == "f":
                v = ("%g" % xmm) if xmm is not None else "0"
                if "." not in v and "e" not in v:
                    v += ".0"
                stmts.append(f"object->SetValue({field(fid)}, {v}f, {EVENTS.get(regs.get('edx'), regs.get('edx'))});")
            elif suffix == "RK7wxPoint":
                stmts.append(f"object->SetValue({field(fid)}, wxPoint{{{stack.get(slot)}, {stack.get(slot + 4)}}}, {EVENTS.get(regs.get('ecx'), regs.get('ecx'))});")
            elif suffix == "RK8wxString":
                text = label[1] if isinstance(label, tuple) else "?"
                stmts.append(f'object->SetValue({field(fid)}, wxString(L"{text}"), {EVENTS.get(regs.get("ecx"), regs.get("ecx"))});')
            else:
                unhandled.append(f"SetValue overload {suffix}")
            regs = {}
            xmm = None
            stack = {}
        elif re.search(r"call\s+_ZN11TVisionaire12CreateObjectEiR10TVisObjRefi", l):
            stmts.append("{")
            stmts.append("	TVisObjRef parent(object);")
            stmts.append(f"	object->GetVisionaire()->CreateObject({regs.get('edx')}, parent, {field(regs.get('r8d'))});")
            stmts.append("}")
            regs = {}
        else:
            m = re.search(r"call\s+(\w+)", l)
            if m:
                name = m.group(1)
                if not re.match(r"_ZN17TVisionaireObject|_ZN10TVisObjRef(D2|C2)|_ZN8wxString|_ZNSb|__ZNSb|_Z5toUT|"
                                r"_ZN9__gnu|__ZN9__gnu|_ZNK17TVisionaireObject5GetId|__ZdlPv|_ZN11TCharHolder|"
                                r"yasm_value_delete|_ZSt|__ZSt|__Unwind_Resume|_ZNSt4pair|_ZN10TVisObjrefD2|_ZNK17TVisionaireObject13GetVisionaireEv", name):
                    unhandled.append(name)
        i += 1
    return stmts, unhandled


def main():
    lines = load_lines()
    addr = {}
    for i, l in enumerate(lines):
        m = re.match(r"^(_Z\w+) proc near", l)
        if m:
            addr[m.group(1)] = i
    data = AsmData(ASM, 3000000)

    groups = {}
    with open("manifest/schema_groups.tsv", encoding="utf-8") as f:
        for r in csv.DictReader(f, delimiter="\t"):
            groups[r["class"]] = r

    out = ["// Generated by tools/gen_callbacks.py from the binary's OnCreate()/OnInit() default-value",
           "// callbacks - do not edit the generated statements; the functions marked FIXME were not",
           "// fully translatable and are finished by hand in vstables/recordCallbacksManual.cpp.",
           "",
           '#include "vstables/records.h"',
           "",
           '#include "TSText.h"',
           '#include "TTAction.h"',
           '#include "TTButton.h"',
           '#include "TTScene.h"',
           '#include "TTText.h"',
           '#include "datastruct/visionaire.h"',
           '#include "datastruct/visionaireobject.h"',
           '#include "vstables/fieldIds.h"',
           ""]
    incomplete = []
    for cls in sorted(groups):
        for cb in ("OnCreate", "OnInit"):
            if cb not in groups[cls]["callbacks"].split(","):
                continue
            sym = f"_ZN{len(cls)}{cls}{len(cb)}{cb}EP17TVisionaireObject"
            if sym not in addr:
                incomplete.append((cls, cb, "symbol not found"))
                continue
            stmts, unhandled = parse_function(lines, addr[sym], data)
            if unhandled:
                incomplete.append((cls, cb, ", ".join(sorted(set(unhandled)))))
                continue
            out.append(f"void {cls}::{cb}(TVisionaireObject *object) {{")
            if stmts:
                for s in stmts:
                    out.append("\t" + s)
            out.append("}")
            out.append("")
    path = os.path.join(GAME, "vstables", "recordCallbacks.cpp")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out))
    print(f"{len(incomplete)} callbacks need hand work:")
    for cls, cb, why in incomplete:
        print(f"  {cls}::{cb}: {why}")


main()
