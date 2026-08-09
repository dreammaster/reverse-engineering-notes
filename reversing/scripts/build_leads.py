#!/usr/bin/env python3
"""
Build a prioritized worklist of unnamed sub_XXXXXXXX functions worth
investigating next: functions whose body directly references (via
`offset aXxx`) a string literal that cross_reference.py matched into exactly
one source file.

Single pass over rob_blanc_1.asm tracking the current enclosing
proc/endp block; for each `offset aXxx` operand seen inside a sub_* function,
record the referenced label.

Output: reversing/analysis/leads.json
  [ { "func": "sub_XXXXXXXX", "candidates": [source_file, ...],
      "evidence": [ {"label", "text", "files"} ] }, ... ]
  sorted with single-candidate-file functions first (best leads), then by
  number of matched strings referenced (more evidence = more confidence).
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
ASM_PATH = ROOT / "rob_blanc_1.asm"
ANALYSIS = Path(__file__).resolve().parent.parent / "analysis"

PROC_RE = re.compile(r"^(\S+)\s+proc near\b")
ENDP_RE = re.compile(r"^(\S+)\s+endp\b")
OFFSET_RE = re.compile(r"\boffset (a[A-Za-z0-9_]+)\b")


def main():
    string_matches = {m["label"]: m for m in json.loads((ANALYSIS / "string_matches.json").read_text(encoding="utf-8"))}

    refs_by_func = {}  # func_name -> set(label)
    current_func = None

    with ASM_PATH.open(encoding="utf-8", errors="replace") as f:
        for line in f:
            pm = PROC_RE.match(line)
            if pm:
                current_func = pm.group(1)
                continue
            em = ENDP_RE.match(line)
            if em:
                current_func = None
                continue
            if current_func and current_func.startswith("sub_"):
                for m in OFFSET_RE.finditer(line):
                    refs_by_func.setdefault(current_func, set()).add(m.group(1))

    leads = []
    for func, labels in refs_by_func.items():
        evidence = []
        file_set = set()
        for label in labels:
            sm = string_matches.get(label)
            if not sm:
                continue
            files = sorted(set(m["file"] for m in sm["matches"]))
            evidence.append({"label": label, "text": sm["text"], "files": files})
            file_set.update(files)
        if evidence:
            leads.append({
                "func": func,
                "candidate_files": sorted(file_set),
                "evidence": evidence,
            })

    # best leads: exactly one candidate file, more evidence first
    leads.sort(key=lambda l: (len(l["candidate_files"]) != 1, -len(l["evidence"])))

    ANALYSIS.mkdir(parents=True, exist_ok=True)
    (ANALYSIS / "leads.json").write_text(json.dumps(leads, indent=1), encoding="utf-8")

    single_file = [l for l in leads if len(l["candidate_files"]) == 1]
    print(f"sub_* functions with >=1 matched string reference: {len(leads)}")
    print(f"  of which single-candidate-file (best leads): {len(single_file)}")


if __name__ == "__main__":
    main()
