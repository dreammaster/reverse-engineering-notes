#!/usr/bin/env python3
"""
Cross-reference string literals extracted from rob_blanc_1.asm against the
AGS 3.2.1.1115 reference source tree (Common/, Engine/) to find exact or
near matches. This bootstraps function/file identification: a disassembly
string used inside function sub_XXXXXXXX that also appears in a specific
source .cpp file is strong evidence that function (or a sibling in the same
file) originated from that file.

Output: reversing/analysis/string_matches.json
  Each entry: { label, text, matches: [ {file, line, exact} ] }
Also prints a summary to stderr.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
OUT_DIR = Path(__file__).resolve().parent.parent / "analysis"
SRC_DIRS = [ROOT / "Common", ROOT / "Engine"]
SRC_EXTS = {".cpp", ".c", ".h", ".CPP", ".C", ".H"}

MIN_LEN = 5  # ignore very short/common strings (too many false positives)


def load_source_files():
    files = []
    for d in SRC_DIRS:
        if not d.exists():
            continue
        for p in d.rglob("*"):
            if p.is_file() and p.suffix in SRC_EXTS:
                try:
                    text = p.read_text(encoding="utf-8", errors="replace")
                except Exception:
                    continue
                files.append((p, text, text.splitlines()))
    return files


def find_string_literal_occurrences(text_lines, needle):
    """Return list of 1-based line numbers where a C string literal containing
    `needle` (exact substring) appears, scanning for the needle inside quotes."""
    hits = []
    for i, line in enumerate(text_lines, start=1):
        if needle in line and '"' in line:
            hits.append(i)
    return hits


def main():
    strings = json.loads((OUT_DIR / "strings.json").read_text(encoding="utf-8"))
    src_files = load_source_files()
    print(f"Loaded {len(src_files)} source files", file=sys.stderr)

    results = []
    matched = 0
    for s in strings:
        text = s["text"]
        if len(text) < MIN_LEN:
            continue
        matches = []
        for path, full_text, lines in src_files:
            if text in full_text:
                for ln in find_string_literal_occurrences(lines, text):
                    rel = str(path.relative_to(ROOT)).replace("\\", "/")
                    matches.append({"file": rel, "line": ln})
        if matches:
            matched += 1
            results.append({"label": s["label"], "text": text, "matches": matches})

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "string_matches.json").write_text(json.dumps(results, indent=1), encoding="utf-8")
    print(f"{matched} / {len(strings)} strings matched >=1 source location", file=sys.stderr)


if __name__ == "__main__":
    import sys
    main()
