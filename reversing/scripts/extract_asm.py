#!/usr/bin/env python3
"""
Parse rob_blanc_1.asm (IDA-exported disassembly listing) and extract:
  - functions (named + sub_XXXXXXXX) with start address
  - string literals from .data/.rdata with address + decoded text
  - named (non-auto) global data labels

Output: reversing/analysis/functions.json, strings.json, globals.json

Re-run this whenever rob_blanc_1.asm is re-exported from IDA (e.g. after
new names are applied) to refresh the analysis snapshot.
"""
import re
import json
import sys
from pathlib import Path

ASM_PATH = Path(__file__).resolve().parent.parent.parent / "rob_blanc_1.asm"
OUT_DIR = Path(__file__).resolve().parent.parent / "analysis"

FUNC_RE = re.compile(r"^(\S+)\s+proc near(?:\s|$)")
LABEL_DB_RE = re.compile(r"^(\S+)\s+db\s+(.*)$")
CONT_DB_RE = re.compile(r"^\s+db\s+(.*)$")
ADDR_COMMENT_RE = re.compile(r"; DATA XREF: \.(?:rdata|data):(?:seg\d+:)?([0-9A-F]{8})")
AUTO_NAME_RE = re.compile(r"^(unk_|byte_|word_|dword_|qword_|asc_|off_|loc_|sub_|stru_|flt_|dbl_)[0-9A-Fa-f]+$")
# IDA's other auto-generated placeholder for a FLIRT partial/ambiguous library match
# (no real name recovered) -- e.g. "unknown_libname_1", "unknown_libname_2". Without this,
# these get miscounted as "named" even though they're just as unidentified as sub_XXXXXXXX.
AUTO_LIBNAME_RE = re.compile(r"^unknown_libname_\d+$")

STRPART_RE = re.compile(r"'((?:[^'\\]|'')*)'")


def decode_db_operands(operand_str):
    """Given the operand text after 'db', return decoded chars where possible."""
    chars = []
    # split on commas that are not inside quotes
    parts = []
    cur = ""
    in_q = False
    i = 0
    while i < len(operand_str):
        c = operand_str[i]
        if c == "'":
            in_q = not in_q
            cur += c
        elif c == ";" and not in_q:
            break
        elif c == "," and not in_q:
            parts.append(cur)
            cur = ""
        else:
            cur += c
        i += 1
    if cur.strip():
        parts.append(cur)

    for p in parts:
        p = p.strip()
        m = STRPART_RE.match(p)
        if m:
            chars.append(m.group(1).replace("''", "'"))
        else:
            # numeric byte, e.g. 0, 0Ah, 5, 0FFh
            try:
                val = int(p.rstrip("h") or "0", 16 if p.lower().endswith("h") else 10)
            except ValueError:
                continue
            if val == 0:
                chars.append("\0")
            elif 32 <= val < 127:
                chars.append(chr(val))
            else:
                chars.append(f"\\x{val:02x}")
    return "".join(chars)


def main():
    print(f"Reading {ASM_PATH} ...", file=sys.stderr)
    lines = ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines()
    print(f"{len(lines)} lines", file=sys.stderr)

    functions = []  # {name, is_named}
    strings = []    # {label, text}
    globals_ = []   # {label} non-auto data labels not already captured as strings

    current_label = None
    current_parts = []

    def flush_string():
        nonlocal current_label, current_parts
        if current_label is not None and current_parts:
            text = "".join(current_parts)
            # trim at first embedded NUL (typical C-string terminator)
            if "\0" in text:
                text = text.split("\0")[0]
            if text:
                strings.append({"label": current_label, "text": text})
        current_label = None
        current_parts = []

    seg = None
    for line in lines:
        if line.startswith("_text") and "segment para" in line:
            seg = "text"
        elif line.startswith("_rdata") and "segment para" in line:
            seg = "rdata"
        elif line.startswith("_data") and "segment para" in line:
            seg = "data"

        fm = FUNC_RE.match(line)
        if fm:
            name = fm.group(1)
            functions.append({
                "name": name,
                "addr_hint": None,
                "is_named": not (AUTO_NAME_RE.match(name) or AUTO_LIBNAME_RE.match(name)),
            })
            continue

        if seg in ("rdata", "data"):
            lm = LABEL_DB_RE.match(line)
            if lm:
                flush_string()
                label, rest = lm.group(1), lm.group(2)
                if not AUTO_NAME_RE.match(label):
                    current_label = label
                    current_parts = [decode_db_operands(rest)]
                continue
            cm = CONT_DB_RE.match(line)
            if cm and current_label is not None:
                current_parts.append(decode_db_operands(cm.group(1)))
                continue
            else:
                flush_string()
    flush_string()

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "functions.json").write_text(json.dumps(functions, indent=1), encoding="utf-8")
    (OUT_DIR / "strings.json").write_text(json.dumps(strings, indent=1), encoding="utf-8")

    named = [f for f in functions if f["is_named"]]
    unnamed = [f for f in functions if not f["is_named"]]
    print(f"Functions: {len(functions)} total, {len(named)} named, {len(unnamed)} unnamed (sub_*)", file=sys.stderr)
    print(f"Strings extracted: {len(strings)}", file=sys.stderr)


if __name__ == "__main__":
    main()
