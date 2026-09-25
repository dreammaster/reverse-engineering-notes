#!/usr/bin/env python3
"""Recover the original source tree layout from x_assert(bool, expr, file,
line) call sites.

x_assert's call sites embed the original __FILE__ (as a full build-machine
path, e.g. "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/control/
gameController.cpp") and the stringified expression. Cross-referencing which
function each call site falls inside (via manifest/proprietary_functions.tsv)
tells us which of Visionnaire's classes/functions lived in which original
source file - useful for laying out src/<game>/ to mirror the real project
structure as we reimplement each class.

Usage:
    python tools/extract_source_layout.py <path-to-.asm> [--out manifest/source_layout.tsv]

Calling convention recovered from manual inspection (x_assert(bool cond,
const char* expr, const char* file, int line)):
    edi/dil = cond, esi = expr string, edx = file string, ecx = line number
These can appear in any order/instruction mix before the call (register
allocation varies per call site), so this scans backward from each `call
x_assert` for the most recent `mov esi/edx, offset <label>` and `mov ecx,
<imm>` and takes whichever it finds within the window - it does not try to
prove those operands weren't clobbered/reused in between, so treat unclear
rows as approximate.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

# `jmp` (not just `call`) too: a tail-call assert (the assert is the last
# thing the function does) compiles to a plain jmp, e.g.
# TMasterControl::Signal's default case - missing these undercounted call
# sites and silently dropped some classes from the recovered layout.
CALL_RE = re.compile(r"^\s*(?:call|jmp)\s+_Z8x_assertbPKcS0_i\b")
MOV_REG_OFFSET_RE = re.compile(r"^\s*mov\s+(esi|edx)\s*,\s*offset\s+(\S+)")
MOV_ECX_IMM_RE = re.compile(r"^\s*mov\s+ecx\s*,\s*([0-9A-Fa-f]+h|\d+)\b")
PROC_RE = re.compile(r"^(\S+)\s+proc (?:near|far)\b")
LABEL_STRING_START_RE = re.compile(r"^(\S+)\s+db\s+'((?:[^'])*)'(.*)$")
STRING_CONT_RE = re.compile(r"^\s+db\s+'((?:[^'])*)'(.*)$")

WINDOW = 25  # lines to scan backward for each operand


def parse_imm(s: str) -> int:
    if s.endswith("h") or s.endswith("H"):
        return int(s[:-1], 16)
    return int(s)


def load_string_literals(asm_path: Path, needed_labels: set) -> dict:
    """Second pass: resolve label -> the narrow C string starting at that
    label. IDA sometimes splits one logical string across several `db
    'fragment'` lines (each ending in ',0' only on the final fragment), with
    comment-only lines (DATA XREF notes, `align`) interspersed - those are
    skipped rather than treated as the end of the string; only an explicit
    ',0' terminator (or a line that plainly starts something new) ends it."""
    result = {}
    pending = set(needed_labels)
    if not pending:
        return result
    current_label = None
    current_chars = []
    lines_since_progress = 0

    def finalize():
        nonlocal current_label, current_chars
        if current_label is not None:
            result[current_label] = "".join(current_chars)
            pending.discard(current_label)
        current_label = None
        current_chars = []

    with asm_path.open(encoding="utf-8", errors="replace") as f:
        for line in f:
            m = LABEL_STRING_START_RE.match(line)
            if m:
                label, ch, rest = m.group(1), m.group(2), m.group(3)
                finalize()
                if label in pending:
                    current_label = label
                    current_chars = [ch]
                    lines_since_progress = 0
                    if re.match(r"^\s*,\s*0\b", rest):
                        finalize()
                if not pending:
                    break
                continue
            if current_label is not None:
                m2 = STRING_CONT_RE.match(line)
                if m2:
                    ch, rest = m2.group(1), m2.group(2)
                    current_chars.append(ch)
                    lines_since_progress = 0
                    if re.match(r"^\s*,\s*0\b", rest):
                        finalize()
                        if not pending:
                            break
                    continue
                # Comment-only / align / blank lines between fragments are
                # expected; anything else after too many such lines means
                # this wasn't a continuation after all.
                lines_since_progress += 1
                if lines_since_progress > 5:
                    finalize()
    finalize()
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("asm_path", type=Path)
    ap.add_argument("--out", type=Path, default=Path("manifest/source_layout.tsv"))
    ap.add_argument("--functions-tsv", type=Path, default=Path("manifest/proprietary_functions.tsv"))
    args = ap.parse_args()

    print(f"Pass 1: scanning {args.asm_path} for x_assert call sites and proc boundaries...", file=sys.stderr)
    lines = []  # we need random backward access; read the whole file once
    proc_at_line = []  # (line_no, raw_symbol) sorted by line_no

    with args.asm_path.open(encoding="utf-8", errors="replace") as f:
        for line_no, line in enumerate(f, start=1):
            lines.append(line)
            m = PROC_RE.match(line)
            if m:
                proc_at_line.append((line_no, m.group(1)))

    print(f"  {len(lines)} lines, {len(proc_at_line)} functions.", file=sys.stderr)

    # Demangle proc symbols so we can report human-readable owners.
    raw_names = [name for _, name in proc_at_line]
    demangled = subprocess.run(
        ["c++filt"], input="\n".join(raw_names), capture_output=True, text=True, check=True
    ).stdout.splitlines()
    proc_demangled = {line_no: dem for (line_no, _), dem in zip(proc_at_line, demangled)}
    proc_lines_sorted = [ln for ln, _ in proc_at_line]

    import bisect

    def enclosing_function(line_no):
        i = bisect.bisect_right(proc_lines_sorted, line_no) - 1
        if i < 0:
            return None
        return proc_demangled[proc_lines_sorted[i]]

    print("Pass 2: extracting x_assert call operands...", file=sys.stderr)
    calls = []  # (line_no, enclosing_func, expr_label, file_label, assert_line_no)
    needed_labels = set()
    for i, line in enumerate(lines):
        if not CALL_RE.match(line):
            continue
        line_no = i + 1
        expr_label = None
        file_label = None
        line_num = None
        start = max(0, i - WINDOW)
        for back in lines[start:i]:
            m = MOV_REG_OFFSET_RE.match(back)
            if m:
                reg, label = m.group(1), m.group(2)
                if reg == "esi":
                    expr_label = label
                elif reg == "edx":
                    file_label = label
            m2 = MOV_ECX_IMM_RE.match(back)
            if m2:
                line_num = parse_imm(m2.group(1))
        calls.append((line_no, enclosing_function(line_no), expr_label, file_label, line_num))
        if expr_label:
            needed_labels.add(expr_label)
        if file_label:
            needed_labels.add(file_label)

    print(f"  {len(calls)} call sites, resolving {len(needed_labels)} string labels...", file=sys.stderr)
    strings = load_string_literals(args.asm_path, needed_labels)

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", encoding="utf-8") as out:
        out.write("assert_line\toriginal_file\toriginal_source_line\texpression\tenclosing_function\n")
        for line_no, func, expr_label, file_label, orig_line in calls:
            file_text = strings.get(file_label, f"<unresolved:{file_label}>") if file_label else ""
            expr_text = strings.get(expr_label, f"<unresolved:{expr_label}>") if expr_label else ""
            out.write(f"{line_no}\t{file_text}\t{orig_line if orig_line is not None else ''}\t{expr_text}\t{func or ''}\n")
    print(f"Wrote {args.out}", file=sys.stderr)

    # Summary: distinct original files recovered, with a sample owner.
    file_to_funcs = {}
    for _, func, _, file_label, _ in calls:
        if not file_label:
            continue
        text = strings.get(file_label)
        if not text:
            continue
        file_to_funcs.setdefault(text, set()).add(func)

    summary_path = args.out.with_name("source_layout_summary.tsv")
    with summary_path.open("w", encoding="utf-8") as out:
        out.write("original_file\tassert_count\tdistinct_functions\tsample_functions\n")
        counts = {}
        for _, _, _, file_label, _ in calls:
            if file_label:
                text = strings.get(file_label)
                if text:
                    counts[text] = counts.get(text, 0) + 1
        for text, funcs in sorted(file_to_funcs.items()):
            samples = " | ".join(sorted(f for f in funcs if f)[:5])
            out.write(f"{text}\t{counts.get(text, 0)}\t{len(funcs)}\t{samples}\n")
    print(f"Wrote {summary_path} ({len(file_to_funcs)} distinct original files recovered)", file=sys.stderr)


if __name__ == "__main__":
    main()
