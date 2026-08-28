#!/usr/bin/env python3
"""
Parse the IDA-exported `Shannara Demo.asm` text listing into structured data.

We work from the .asm export rather than the live .idb/.id0/.id1 because the
database is normally open in the IDA Pro GUI during analysis sessions, and
scripting against it headlessly (idat64 -S...) while it's open risks
corrupting/conflicting with that session. Re-run this whenever a fresh .asm
export is produced (Produce file > Create ASM file in IDA).

Outputs reversing/analysis/function_inventory.json - one record per
subroutine (name, address range, size, xref count, whether it already has a
real applied signature/name vs. an auto-generated sub_XXXXXX, and a rough
"library vs engine vs unexamined" bucket by name-prefix heuristics).

Usage:
    python parse_asm.py [path/to/Shannara Demo.asm]
"""
import json
import re
import sys
from pathlib import Path

DEFAULT_ASM = Path(__file__).resolve().parents[2] / "Shannara Demo.asm"
OUT_DIR = Path(__file__).resolve().parents[1] / "analysis"

PROC_RE = re.compile(r'^(\S+)\s+proc\s+(near|far)\b')
ENDP_RE = re.compile(r'^(\S+)\s+endp\b')
XREF_RE = re.compile(r'CODE XREF:')
DOTS_RE = re.compile(r'\.\.\.$')

LIB_PREFIXES = [
    ("_AIL_API_", "miles_ail_internal"),
    ("_AIL_", "miles_ail_public"),
    ("_SS_", "miles_ail_dig"),
    ("_XMI_", "miles_ail_xmidi"),
    ("_Gr", "ms_graphics"),
    ("_Init", "ms_graphics"),
    ("_Calc", "ms_graphics"),
    ("_clearscreen", "ms_graphics"),
    ("_L2clearscreen", "ms_graphics"),
    ("_TxtClear", "ms_graphics"),
    ("__", "watcom_runtime"),  # double-underscore Watcom CRT internals
]
GX_PREFIX = "gx"


def classify(name: str) -> str:
    if name.startswith("sub_") or name.startswith("loc_") or name.startswith("nullsub_"):
        return "unnamed"
    for prefix, bucket in LIB_PREFIXES:
        if name.startswith(prefix):
            return bucket
    if name.startswith(GX_PREFIX) and len(name) > 2 and name[2].isupper():
        return "gx_blitter"
    if name.startswith("q_") or name.startswith("Q") or name.startswith("_q"):
        return "q_codec"
    if name.startswith("_"):
        return "runtime_or_global_backed"
    return "engine_named"


def parse(asm_path: Path):
    functions = []
    cur = None
    xref_count = 0
    lineno = 0
    with asm_path.open("r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            lineno += 1
            line = raw.rstrip("\n")
            m = PROC_RE.match(line)
            if m:
                cur = {
                    "name": m.group(1),
                    "start_line": lineno,
                    "end_line": None,
                    "xref_lines": 0,
                }
                # count xrefs that appear on the proc line itself and any
                # immediately-following continuation lines (IDA wraps long
                # xref lists onto "..." continuation lines)
                xref_count = line.count("↓p") + line.count("↑p") + line.count("↓o") + line.count("↑o")
                cur["xref_lines"] = xref_count
                continue
            if cur is not None:
                m2 = ENDP_RE.match(line)
                if m2 and m2.group(1) == cur["name"]:
                    cur["end_line"] = lineno
                    cur["size_lines"] = cur["end_line"] - cur["start_line"]
                    functions.append(cur)
                    cur = None
                    continue
                # keep tallying xref continuation lines (lines that are
                # basically just another "name+off↓p" reference, indented)
                if line.strip().startswith(";") and ("↓p" in line or "↑p" in line or "↓o" in line or "↑o" in line):
                    cur["xref_lines"] += line.count("↓p") + line.count("↑p") + line.count("↓o") + line.count("↑o")
    return functions


def main():
    asm_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_ASM
    if not asm_path.exists():
        print(f"ASM file not found: {asm_path}", file=sys.stderr)
        sys.exit(1)

    functions = parse(asm_path)
    for fn in functions:
        fn["bucket"] = classify(fn["name"])

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out_path = OUT_DIR / "function_inventory.json"
    with out_path.open("w", encoding="utf-8") as f:
        json.dump(functions, f, indent=1)

    # summary
    from collections import Counter
    buckets = Counter(fn["bucket"] for fn in functions)
    print(f"Parsed {len(functions)} functions from {asm_path.name}")
    for bucket, count in buckets.most_common():
        print(f"  {bucket:28s} {count}")
    print(f"\nWrote {out_path}")


if __name__ == "__main__":
    main()
